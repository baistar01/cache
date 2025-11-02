#pragma once

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
    size_t capacity_;                        // 缓存容量
    size_t transformThreshold_;              // 定义多少次访问后从Lru迁移到Lfu阈值
    std::unique_ptr<ArcLru<Key, Value>> lru; // lru缓存
    std::unique_ptr<ArcLfu<Key, Value>> lfu; // lfu缓存
};

template<typename Key, typename Value>
void ArcCache<Key, Value>::put(Key key, Value value)
{
    checkGhostCaches(key);

    bool inLfu = lfu->contain(key);
    lru->put(key, value);
    if(inLfu){
        lfu->put(key,value);
    }
}

template<typename Key, typename Value>
bool ArcCache<Key, Value>::get(Key key, Value& value)
{
    checkGhostCaches(key);

    bool shouldTransform = false;
    if(lru->get(key, value, shouldTransform)){
        if(shouldTransform){
            lfu->put(key, value);
        }
        return true;
    }
    return lfu->get(key ,value);
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
    // 命中lru的幽灵缓存：减小lfu容量，增加lru容量
    if(lru->eraseGhost(key)){
        if(lfu->decreaseCapacity())
        {
            lru->increaseCapacity();
        }
        inGhost = true;
    }
    // 命中lfu幽灵缓存：减小lru容量，增加lfu容量
    else if(lfu->eraseGhost(key))
    {
        if(lru->decreaseCapacity())
        {
            lfu->increaseCapacity();
        }
        inGhost = true;
    }
    return inGhost;
}