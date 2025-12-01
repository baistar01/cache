#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <cmath>

#include "ArcCache.h"

template <typename Key, typename Value>
class ArcHashCache : public cachePolicy<Key, Value>
{
public:
    ArcHashCache(size_t capacity, int sliceNum, size_t transformThreshold)
    :capacity_(capacity)
    ,sliceNum_(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency())
    ,transformThreshold_(transformThreshold)
    {
        size_t sliceSize = std::ceil(capacity_ / static_cast<double>(sliceNum_));
        for (int i=0;i<sliceNum_;i++) {
            ArcSlice_.emplace_back(new ArcCache<Key, Value>(sliceSize, transformThreshold_));
        }
    }

public:
    void put(Key key, Value value);
    bool get(Key key, Value& value);
    Value get(Key key);

private:
    size_t ArcHashValue(Key key);

private:
    size_t capacity_;
    int sliceNum_;
    size_t transformThreshold_;
    std::vector<std::unique_ptr<ArcCache<Key,Value>>> ArcSlice_;
};

template<typename Key, typename Value>
void ArcHashCache<Key,Value>::put(Key key,Value value)
{
    size_t sliceIndex = ArcHashValue(key) % sliceNum_;
    ArcSlice_[sliceIndex]->put(key,value);
}

template<typename Key, typename Value>
bool ArcHashCache<Key,Value>::get(Key key, Value& value)
{
    size_t sliceIndex = ArcHashValue(key) % sliceNum_;
    return ArcSlice_[sliceIndex]->get(key,value);
}

template<typename Key,typename Value>
Value ArcHashCache<Key,Value>::get(Key key)
{
    Value value{};
    get(key, value);
    return value;
}

template<typename Key, typename Value>
size_t ArcHashCache<Key,Value>::ArcHashValue(Key key)
{
    std::hash<Key> hashFunc;
    return hashFunc(key);
}