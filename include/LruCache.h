#pragma once

#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "CachePolicy.h"
#include "LruNode.h"

template<typename Key,typename Value>
class LruCache : public cachePolicy<Key,Value>{
public:
    using LruNodeType = LruNode<Key, Value>;
    using Nodeptr = shared_ptr<LruNodeType>;
    using Nodemap = unordered_map<Key, Nodeptr>;

    LruCache(int capacity):capacity_(capacity){ initializeList(); }
    ~LruCache() override = default;

    void put(Key key, Value value) override;
    bool get(Key key, Value& value) override;
    Value get(Key key) override;
    void remove(Key key);
private:
    void initializeList();
    void addNewNode(Key key, Value value);
    void updateExistingNode(Nodeptr node, Value value);
    void moveToMostRecent(Nodeptr node);
    void removeNode(Nodeptr node);
    void evictLeastRecent();
    void insertNode(Nodeptr node);
private:
    int capacity_;
    Nodemap nodeMap_;
    mutex mutex_;
    Nodeptr dummyHead_;
    Nodeptr dummyTail_;
};

template<typename Key, typename Value>
void LruCache<Key, Value>::put(Key key, Value value){
    if(capacity_<=0) return;
    lock_guard<mutex> lock(mutex_);
    auto it = nodeMap_.find(key);
    if(it != nodeMap_.end()){
        updateExistingNode(it->second, value);
        return;
    }
    addNewNode(key, value);
}

template<typename Key, typename Value>
bool LruCache<Key, Value>::get(Key key, Value& value){
    lock_guard<mutex> lock(mutex_);
    auto it = nodeMap_.find(key);
    if(it != nodeMap_.end()){
        moveToMostRecent(it->second);
        value = it->second->getValue();
        return true;
    }
    return false;
}

template<typename Key, typename Value>
Value LruCache<Key, Value>::get(Key key){
    Value value{};
    get(key, value);
    return value;
}

template<typename Key, typename Value>
void LruCache<Key, Value>::remove(Key key){
    lock_guard<mutex> lock(mutex_);
    auto it = nodeMap_.find(key);
    if(it != nodeMap_.end()){
        removeNode(it->second);
        nodeMap_.erase(it);
    }
}

template<typename Key, typename Value>
void LruCache<Key, Value>::initializeList(){
    dummyHead_ = make_shared<LruNodeType>(Key{}, Value{});
    dummyTail_ = make_shared<LruNodeType>(Key{}, Value{});
    dummyHead_->next = dummyTail_;
    dummyTail_->prev = dummyHead_;
}

template<typename Key, typename Value>
void LruCache<Key, Value>::addNewNode(Key key, Value value){
    if(nodeMap_.size() >= capacity_){
        evictLeastRecent();
    }
    Nodeptr newNode = make_shared<LruNodeType>(key, value);
    insertNode(newNode);
    nodeMap_[key]= newNode;
}

template<typename Key, typename Value>
void LruCache<Key, Value>::updateExistingNode(Nodeptr node, Value value){
    node->setValue(value);
    moveToMostRecent(node);
}

template<typename Key, typename Value>
void LruCache<Key, Value>::moveToMostRecent(Nodeptr node){
    removeNode(node);
    insertNode(node);
}

template<typename Key, typename Value>
void LruCache<Key, Value>::removeNode(Nodeptr node){
    if(!node->prev.expired() && node->next){
        auto prevNode = node->prev.lock();
        auto nextNode = node->next;
        prevNode->next = nextNode;
        nextNode->prev = prevNode;
        node->next = nullptr;
        node->prev.reset();
    }
}

template<typename Key, typename Value>
void LruCache<Key, Value>::evictLeastRecent(){
    Nodeptr leastNode = dummyHead_->next;
    removeNode(leastNode);
    nodeMap_.erase(leastNode->getKey());
}

template<typename Key, typename Value>
void LruCache<Key, Value>::insertNode(Nodeptr node){
    auto lastNode = dummyTail_->prev.lock();
    lastNode->next = node;
    node->prev = lastNode;
    node->next = dummyTail_;
    dummyTail_->prev = node;
}
