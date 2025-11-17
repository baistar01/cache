#pragma once
#include <memory>
#include <unordered_map>

#include "LruCache.h"

template<typename Key, typename Value>
class LruKCache : public LruCache<Key, Value> {
public:
    LruKCache(int capacity, int historyCapacity, int k)
    :LruCache<Key, Value>(capacity)
    ,historyList_(std::unique_ptr<LruCache<Key, size_t>>(new LruCache<Key, size_t>(historyCapacity)))  // 设置为独占，当LruKCache销毁时也会被销毁
    ,k_(k) {}

    using LruCache<Key, Value>::get; // 子类重写会覆盖父类的函数实现，这里进行显式说明！
    Value get(Key key);
    void put(Key key, Value value);
private:
    int k_;
    std::unique_ptr<LruCache<Key, size_t>> historyList_;  // 访问数据的历史记录
    std::unordered_map<Key, Value> historyValueMap_;     // 存储未达到k次访问的数据值。
};

template<typename Key, typename Value>
Value LruKCache<Key, Value>::get(Key key){
    Value value{};
    bool inMainCache = LruCache<Key, Value>::get(key, value);
    
    // 数据在主缓存中
    if(inMainCache){
        return value;
    }

    size_t historyCount = historyList_->get(key);
    historyCount++;
    historyList_->put(key, historyCount);
    
    // 数据不在主缓存中，但是访问次数达到k次
    if(historyCount >= k_){
        auto it = historyValueMap_.find(key);
        if(it != historyValueMap_.end()){
            Value storedValue = it->second;
            historyList_->remove(key);
            historyValueMap_.erase(it);
            LruCache<Key, Value>::put(key, storedValue);
            return storedValue;
        }
    }
    return value;
}

template<typename Key, typename Value>
void LruKCache<Key, Value>::put(Key key, Value value){
    Value existingValue{};
    bool inMainCache = LruCache<Key, Value>::get(key, existingValue);

    // 在主缓存中，更新
    if(inMainCache){
        LruCache<Key, Value>::put(key, value);
        return;
    }

    // 不在主缓存中，更新访问历史
    size_t historyCount = historyList_->get(key);
    historyCount++;
    historyList_->put(key, historyCount);
    historyValueMap_[key] = value;

    // 达到k次访问阈值
    if(historyCount >= k_){
        historyList_->remove(key);
        historyValueMap_.erase(key);
        LruCache<Key, Value>::put(key, value);
    }
}

