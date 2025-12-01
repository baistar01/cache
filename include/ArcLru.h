#pragma once

#include <unordered_map>

#include "ArcCacheNode.h"

template<typename Key, typename Value>
class ArcLru
{
public:
    using NodeType = ArcNode<Key, Value>;
    using Nodeptr = std::shared_ptr<NodeType>;
    using NodeMap = std::unordered_map<Key, Nodeptr>;

    explicit ArcLru(size_t capacity, size_t transformThreshold)
    : capacity_(capacity)
    , ghostCapacity_(capacity)
    , transformThreshold_(transformThreshold)
    {
        initializeLists();
    }

    bool put(Key key, Value value);                         // 向缓存中添加数据
    bool get(Key key, Value& value, bool& shouldTransform); // 判断数据是否存在于缓存中
    Value get(Key key);                                     // 从缓存中得到数据

    bool eraseGhost(Key key);                               // 删除幽灵数据包含的某个键
    void increaseCapacity();                                // 增加缓存容量
    bool decreaseCapacity();                                // 减小缓存容量

private:
    void initializeLists();                                     // 初始化缓存链表
    bool updateExistingNode(Nodeptr node, const Value& value);  // 更新已存在节点
    bool addNewNode(const Key& key, const Value& value);        // 增加新节点
    bool updateNodeAccess(Nodeptr node);                        // 更新节点
    void moveToFront(Nodeptr node);                             // 将节点移动到链表头
    void addToFront(Nodeptr node);                              // 在链表头增加新节点
    void evictLeastRecent();                                    // 淘汰最旧未使用节点
    void removeFromMain(Nodeptr node);                          // 在主缓存中移除节点
    void removeFromGhost(Nodeptr node);                         // 在幽灵缓存中移除节点
    void addToGhost(Nodeptr node);                              // 在幽灵缓存中添加节点
    void removeOldestGhost();                                   // 在幽灵缓存中移除节点

private:
    size_t capacity_;
    size_t ghostCapacity_;
    size_t transformThreshold_; // 转换阈值

    NodeMap mainCache_;  // 主缓存
    NodeMap ghostCache_; // 幽灵缓存

    Nodeptr mainHead_;   // 主缓存头节点
    Nodeptr mainTail_;   // 主缓存尾节点

    Nodeptr ghostHead_;  // 幽灵缓存头节点
    Nodeptr ghostTail_;  // 幽灵缓存尾节点
};

template<typename Key, typename Value>
bool ArcLru<Key, Value>::put(Key key, Value value)
{
    if(capacity_ == 0) return false;
    auto it = mainCache_.find(key);
    if(it != mainCache_.end()){
        return updateExistingNode(it->second, value);
    }
    return addNewNode(key, value);
}

template<typename Key, typename Value>
bool ArcLru<Key, Value>::get(Key key, Value& value, bool& shouldTransform)
{
    auto it=mainCache_.find(key);
    if(it != mainCache_.end()){
        shouldTransform = updateNodeAccess(it->second);
        value = it->second->getValue();
        return true;
    }
    return false;
}

template<typename Key, typename Value>
Value ArcLru<Key, Value>::get(Key key)
{
    Value value{};
    get(key, value);
    return value;
}

template<typename Key, typename Value>
bool ArcLru<Key, Value>::eraseGhost(Key key)
{
    auto it = ghostCache_.find(key);
    if(it != ghostCache_.end()){
        removeFromGhost(it->second);
        ghostCache_.erase(it);
        return true;
    }
    return false;
}

template<typename Key, typename Value>
void ArcLru<Key, Value>::increaseCapacity()
{
    ++capacity_;
}

template<typename Key, typename Value>
bool ArcLru<Key, Value>::decreaseCapacity()
{
    if (capacity_ <= 0) return false;
    if (mainCache_.size() == capacity_) {
        evictLeastRecent();
    }
    --capacity_;
    return true;
}

template<typename Key, typename Value>
void ArcLru<Key, Value>::initializeLists()
{
    mainHead_ = std::make_shared<NodeType>();
    mainTail_ = std::make_shared<NodeType>();
    mainHead_->next_ = mainTail_;
    mainTail_->prev_ = mainHead_;

    ghostHead_ = std::make_shared<NodeType>();
    ghostTail_ = std::make_shared<NodeType>();
    ghostHead_->next_ = ghostTail_;
    ghostTail_->prev_ = ghostHead_;
}

template<typename Key, typename Value>
bool ArcLru<Key, Value>::updateExistingNode(Nodeptr node, const Value& value)
{
    node->setValue(value);
    moveToFront(node);
    return true;
}

template<typename Key, typename Value>
bool ArcLru<Key, Value>::addNewNode(const Key& key, const Value& value)
{
    if(mainCache_.size() >= capacity_){
        evictLeastRecent();
    }
    Nodeptr newNode = std::make_shared<NodeType>(key, value);
    mainCache_[key] = newNode;
    addToFront(newNode);
    return true;
}

template<typename Key, typename Value>
bool ArcLru<Key, Value>::updateNodeAccess(Nodeptr node)
{
    moveToFront(node);
    node->increamentAccessCount();
    return node->getAccessCount() >= transformThreshold_;
}

template<typename Key, typename Value>
void ArcLru<Key, Value>::moveToFront(Nodeptr node)
{
    if(!node->prev_.expired()&&node->next_){
        auto lastNode = node->prev_.lock();
        auto nextNode = node->next_;
        nextNode->prev_ = lastNode;
        lastNode->next_ = nextNode;
        node->next_ = nullptr;
    }
    addToFront(node);
}

template<typename Key, typename Value>
void ArcLru<Key, Value>::addToFront(Nodeptr node)
{
    auto nextNode = mainHead_->next_;
    node->next_ = nextNode;
    node->prev_ = mainHead_;
    nextNode->prev_ = node;
    mainHead_->next_ = node;
}

template<typename Key, typename Value>
void ArcLru<Key, Value>::evictLeastRecent()
{
    Nodeptr leastRecent = mainTail_->prev_.lock();
    if(!leastRecent || leastRecent == mainHead_) return;

    removeFromMain(leastRecent);

    if(ghostCache_.size() >= ghostCapacity_)
    {
        removeOldestGhost();
    }
    addToGhost(leastRecent);

    mainCache_.erase(leastRecent->getKey());
}

template<typename Key, typename Value>
void ArcLru<Key, Value>::removeFromMain(Nodeptr node)
{
    if(!node->prev_.expired() && node->next_)
    {
        auto prevNode = node->prev_.lock();
        auto nextNode = node->next_;
        prevNode->next_ = nextNode;
        nextNode->prev_ = prevNode;
        node->next_ = nullptr;
    }
}

template<typename Key, typename Value>
void ArcLru<Key, Value>::removeFromGhost(Nodeptr node)
{
    if(!node->prev_.expired() && node->next_)
    {
        auto prevNode = node->prev_.lock();
        auto nextNode = node->next_;
        prevNode->next_ = nextNode;
        nextNode->prev_ = prevNode;
        node->next_ = nullptr;
    }
}

template<typename Key, typename Value>
void ArcLru<Key, Value>::addToGhost(Nodeptr node)
{
    node->accessCount_ = 1;
    auto nextNode = ghostHead_->next_;
    node->next_ = nextNode;
    nextNode->prev_ = node;
    node->prev_ = ghostHead_;
    ghostHead_->next_ = node;

    ghostCache_[node->getKey()] = node;
}

template<typename Key, typename Value>
void ArcLru<Key, Value>::removeOldestGhost()
{
    Nodeptr oldestGhost = ghostTail_->prev_.lock();
    if(!oldestGhost || oldestGhost == ghostHead_) return;
    removeFromGhost(oldestGhost);
    ghostCache_.erase(oldestGhost->getKey());
}
