#pragma once

#include <memory>
#include <mutex>

#include "CachePolicy.h"
#include "ArcLru.h"
#include "ArcLfu.h"

template<typename Key, typename Value>
class ArcCache : public cachePolicy<Key, Value>
{
public:
    explicit ArcCache(size_t capacity, size_t transformThreshold)
    : capacity_(capacity)
    , transformThreshold_(transformThreshold)
    , lru(new ArcLru<Key, Value>(capacity, transformThreshold))
    , lfu(new ArcLfu<Key, Value>(capacity, transformThreshold))
    {}

    ~ArcCache() override = default;

    void put(Key key, Value value);
    bool get(Key key, Value& value);
    Value get(Key key);

private:
    bool checkGhostCaches(Key key);

private:
    // 细粒度锁：分别保护 LRU 与 LFU
    std::mutex lruMutex_;
    std::mutex lfuMutex_;

    size_t capacity_;                        // 缓存容量
    size_t transformThreshold_;              // 定义多少次访问后从Lru迁移到Lfu阈值
    std::unique_ptr<ArcLru<Key, Value>> lru; // lru缓存
    std::unique_ptr<ArcLfu<Key, Value>> lfu; // lfu缓存
};

template<typename Key, typename Value>
void ArcCache<Key, Value>::put(Key key, Value value)
{
    // 函数内部自己加锁
    checkGhostCaches(key);

    bool inLfu = false;

    // 先查询LFU是否已经包含该key（只锁 LFU）
    {
        std::lock_guard<std::mutex> lock(lfuMutex_);
        inLfu = lfu->contain(key);
    }

    // 更新LRU（只锁LRU）
    {
        std::lock_guard<std::mutex> lock(lruMutex_);
        lru->put(key, value);
    }

    // 如果LFU中也存在该key，则同步更新LFU（只锁LFU）
    if (inLfu) {
        std::lock_guard<std::mutex> lock(lfuMutex_);
        lfu->put(key, value);
    }
}

template<typename Key, typename Value>
bool ArcCache<Key, Value>::get(Key key, Value& value)
{
    checkGhostCaches(key);

    bool shouldTransform = false;
    bool inLru = false;

    // 先查LRU（只锁LRU）
    {
        std::lock_guard<std::mutex> lock(lruMutex_);
        inLru = lru->get(key, value, shouldTransform);
    }

    if (inLru) {
        // 命中LRU
        if (shouldTransform) {
            // ARC内部的“提升”逻辑：需要放入LFU
            // 注意这里不再持有LRU的锁，只锁LFU，避免双锁死锁
            std::lock_guard<std::mutex> lock(lfuMutex_);
            lfu->put(key, value);
        }
        return true;
    }

    // LRU 未命中，再查LFU（只锁LFU）
    {
        std::lock_guard<std::mutex> lock(lfuMutex_);
        return lfu->get(key, value);
    }
}

template<typename Key, typename Value>
Value ArcCache<Key, Value>::get(Key key)
{
    Value value{};
    get(key, value);
    return value;
}

template<typename Key, typename Value>
bool ArcCache<Key, Value>::checkGhostCaches(Key key)
{
    bool inGhost = false;

    // ghost 命中会同时操作 LRU ghost / LFU ghost 和两边容量
    // 这里需要“原子性”，所以一次性锁住LRU和LFU，锁顺序固定：先LRU再LFU
    std::lock_guard<std::mutex> lockLru(lruMutex_);
    std::lock_guard<std::mutex> lockLfu(lfuMutex_);

    // 命中 LRU 的幽灵缓存：减小 LFU 容量，增加 LRU 容量
    if (lru->eraseGhost(key)) {
        if (lfu->decreaseCapacity()) {
            lru->increaseCapacity();
        }
        inGhost = true;
    }
    // 命中 LFU 的幽灵缓存：减小 LRU 容量，增加 LFU 容量
    else if (lfu->eraseGhost(key)) {
        if (lru->decreaseCapacity()) {
            lfu->increaseCapacity();
        }
        inGhost = true;
    }

    return inGhost;
}