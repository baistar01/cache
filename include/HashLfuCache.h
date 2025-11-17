#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <cmath>

#include "CachePolicy.h"
#include "LfuCache.h"

template<typename Key, typename Value>
class HashLfuCache: public cachePolicy<Key, Value>
{
public:
    // "std::thread::hardware_concurrency(),表示硬件并发线程数（通常为CPU核心数）"
    HashLfuCache(size_t capacity, int sliceNum)
    :capacity_(capacity)
    ,sliceNum_(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency())
    {
        size_t sliceSize = std::ceil(capacity_ / static_cast<double>(sliceNum_));
        for(int i=0;i<sliceNum_; i++){
            LfuSliceCaches_.emplace_back(new LfuCache<Key, Value>(sliceSize));
        }
    }

public:
    void put(Key key, Value value);
    bool get(Key key, Value& value);
    Value get(Key key);

private:
    size_t HashValue(Key key);

private:
    size_t capacity_;
    int sliceNum_;
    std::vector<std::unique_ptr<LfuCache<Key, Value>>> LfuSliceCaches_;  // 切片Lfu缓存
};

template<typename Key, typename Value>
void HashLfuCache<Key, Value>::put(Key key, Value value){
    // 计算key对应的hash值，即slice索引
    size_t sliceIndex = HashValue(key) % sliceNum_;
    LfuSliceCaches_[sliceIndex]->put(key, value);
}

template<typename Key, typename Value>
bool HashLfuCache<Key ,Value>::get(Key key, Value& value){
    size_t sliceIndex = HashValue(key)% sliceNum_;
    return LfuSliceCaches_[sliceIndex]->get(key, value);
}

template<typename Key, typename Value>
Value HashLfuCache<Key, Value>::get(Key key){
    Value value{};
    get(key, value);
    return value;
}

template<typename Key, typename Value>
size_t HashLfuCache<Key, Value>::HashValue(Key key){
    std::hash<Key> hashFunc;
    return hashFunc(key);
}