#pragma once

#include <unordered_map>
#include <map>
#include <list>

#include "ArcCacheNode.h"

template<typename Key, typename Value>
class ArcLfu
{
public:
    using NodeType = ArcNode<Key, Value>;
    using Nodeptr = std::shared_ptr<NodeType>;
    using NodeMap = std::unordered_map<Key, Nodeptr>;
    using FreqMap = std::map<size_t, std::list<Nodeptr>>;

    explicit ArcLfu(size_t capacity, size_t transformThreshold)
    : capacity_(capacity)
    , transformThreshold_(transformThreshold)
    , minFreq_(1)
    , ghostCapacity_(capacity)
    {
        initializeLists();
    }

    bool put(Key key, Value value);      // 向缓存中添加数据
    bool get(Key key, Value& value);     // 判断数据是否存在于缓存中
    Value get(Key key);                  // 从缓存中得到数据
    bool contain(Key key);               // 检查缓存是否包含某个键
    bool eraseGhost(Key key);            // 删除幽灵缓存包含的某个键
    void increaseCapacity();             // 增加缓存容量
    bool decreaseCapacity();             // 减小缓存容量

private:
    void initializeLists();                                     // 初始化幽灵缓存链表
    bool updateExistingNode(Nodeptr node, const Value& value);  // 更新已存在节点
    bool addNewNode(const Key& key, const Value& value);        // 增加新节点
    void updateNodeFrequency(Nodeptr node);                     // 更新节点的访问频次
    void evictLeastFrequent();                                  // 淘汰掉访问频次最低的节点
    void removeFromGhost(Nodeptr node);                         // 从幽灵缓存中移除节点
    void addToGhost(Nodeptr node);                              // 将节点添加到幽灵缓存中
    void removeOldestGhost();                                   // 移除幽灵缓存中最旧的节点

private:
    size_t capacity_;                // 主缓存容量
    size_t ghostCapacity_;           // 幽灵缓存容量
    size_t transformThreshold_;      // 转换阈值
    size_t minFreq_;                 // 最小访问频次
    
    NodeMap mainCache_;              // 主缓存
    NodeMap ghostCache_;             // 幽灵缓存
    FreqMap freqMap_;                // 频次映射

    Nodeptr ghostHead_;              // 幽灵缓存头节点
    Nodeptr ghostTail_;              // 幽灵缓存尾节点
};

template<typename Key, typename Value>
bool ArcLfu<Key, Value>::put(Key key, Value value){
    // 向缓存中添加元素，如果存在于主缓存中进行更新，否则添加新的节点
    // todo是否需要判断是否命中幽灵缓存？
    if(capacity_ == 0) return false;
    auto it = mainCache_.find(key);
    if(it != mainCache_.end()){
        return updateExistingNode(it->second, value);
    }
    return addNewNode(key, value);
}

template<typename Key, typename Value>
bool ArcLfu<Key, Value>::get(Key key, Value& value){
    // 判断是否存在于主缓存中，是的话进行更新
    auto it = mainCache_.find(key);
    if(it != mainCache_.end()){
        updateExistingNode(it->second, value);
        value = it->second->getValue();
        return true;
    }
    return false;
}

template<typename Key, typename Value>
Value ArcLfu<Key, Value>::get(Key key){
    Value value{};
    get(key, value);
    return value;
}

template<typename Key, typename Value>
bool ArcLfu<Key, Value>::contain(Key key){
    return mainCache_.find(key)!=mainCache_.end();
}

template<typename Key, typename Value>
bool ArcLfu<Key, Value>::eraseGhost(Key key){
    // 在幽灵缓存中删除某个数据
    auto it = ghostCache_.find(key);
    if(it != ghostCache_.end()){
        removeFromGhost(it->second);
        ghostCache_.erase(it);
        return true;
    }
    return false;
}

template<typename Key, typename Value>
void ArcLfu<Key, Value>::increaseCapacity(){
    // 增加主缓存容量
    ++capacity_;
}

template<typename Key, typename Value>
bool ArcLfu<Key, Value>::decreaseCapacity(){
    // 减小主缓存容量
    if(capacity_ <= 0) return false;
    if(mainCache_.size() == capacity_){
        evictLeastFrequent();
    }
    --capacity_;
    return true;
}

template<typename Key, typename Value>
void ArcLfu<Key, Value>::initializeLists(){
    // 初始化，定义幽灵缓存的头尾节点
    ghostHead_ = std::make_shared<NodeType>();
    ghostTail_ = std::make_shared<NodeType>();
    ghostHead_->next_ = ghostTail_;
    ghostTail_->prev_ = ghostHead_;
}

template<typename Key, typename Value>
bool ArcLfu<Key, Value>::updateExistingNode(Nodeptr node, const Value& value){
    // 更新主缓存中的某个节点
    node->setValue(value);
    updateNodeFrequency(node);
    return true;
}

template<typename Key, typename Value>
bool ArcLfu<Key, Value>::addNewNode(const Key& key, const Value& value){
    // 在主缓存中添加新的节点
    if(mainCache_.size() == capacity_){
        evictLeastFrequent();
    }
    Nodeptr newNode = std::make_shared<ArcNode<Key, Value>>(key, value);
    mainCache_[key] = newNode;
    freqMap_[1].push_back(newNode);
    minFreq_ = 1;
    return true;
}

template<typename Key, typename Value>
void ArcLfu<Key, Value>::updateNodeFrequency(Nodeptr node){
    // 更新节点的频次
    size_t oldFreq = node->getAccessCount();
    node->increamentAccessCount();
    size_t newFreq = node->getAccessCount();

    auto& oldList = freqMap_[oldFreq];
    oldList.remove(node);
    if(oldList.empty()){
        freqMap_.erase(oldFreq);
        if(oldFreq == minFreq_){
            minFreq_ = newFreq;
        }
    }
    if(freqMap_.find(newFreq) == freqMap_.end()){
        freqMap_[newFreq] = std::list<Nodeptr>();
    }
    freqMap_[newFreq].push_back(node);
}

template<typename Key, typename Value>
void ArcLfu<Key, Value>::evictLeastFrequent(){
    // 删掉最小频率的节点
    if(freqMap_.empty()) return;
    // revise
    auto& minFreqList = freqMap_[minFreq_];
    if(minFreqList.empty()) return;

    Nodeptr leastNode = minFreqList.front();
    minFreqList.pop_front();
    if(minFreqList.empty()){
        freqMap_.erase(minFreq_);
        if(!freqMap_.empty()){
            minFreq_ = freqMap_.begin()->first;
        }
    }
    if(ghostCache_.size() >= ghostCapacity_){
        removeOldestGhost();
    }
    addToGhost(leastNode);
    mainCache_.erase(leastNode->getKey());
}

template<typename Key, typename Value>
void ArcLfu<Key, Value>::removeFromGhost(Nodeptr node){
    // 幽灵缓存中删除节点
    if(!node->prev_.expired() && node->next_){
        auto prevNode = node->prev_.lock();
        auto nextNode = node->next_;
        prevNode->next_ = nextNode;
        nextNode->prev_ = prevNode;
        node->next_ = nullptr;
        node->prev_.reset();
    }
}

template<typename Key, typename Value>
void ArcLfu<Key, Value>::addToGhost(Nodeptr node){
    // 幽灵缓存中添加节点
    node->next_ = ghostTail_;
    auto lastNode = ghostTail_->prev_.lock();
    node->prev_ = lastNode;
    if(lastNode){
        lastNode->next_=node;
    }
    ghostTail_->prev_ = node;
    ghostCache_[node->getKey()] = node;
}

template<typename Key, typename Value>
void ArcLfu<Key, Value>::removeOldestGhost(){
    // 删除幽灵缓存中最旧的数据
    Nodeptr oldestGhost = ghostHead_->next_;
    if(oldestGhost != ghostTail_){
        removeFromGhost(oldestGhost);
        ghostCache_.erase(oldestGhost->getKey());
    }
}

