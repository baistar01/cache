#pragma once
#include <memory>

template<typename Key, typename Value>
class LfuCache;

template<typename Key, typename Value>
class FreqList{
private:
    struct Node{
        int freq;
        Key key;
        Value value;
        std::weak_ptr<Node> pre;
        std::shared_ptr<Node> next;

        Node():freq(1), next() {}
        Node(Key key, Value value):freq(1), key(key), value(value), next() {}
    };

    using Nodeptr = std::shared_ptr<Node>;
    int freq_;
    Nodeptr head_;
    Nodeptr tail_;

public:
    // 初始化列表构造函数
    explicit FreqList(int n):freq_(n){
        head_ = std::make_shared<Node>();
        tail_ = std::make_shared<Node>();
        head_->next = tail_;
        tail_->pre = head_;
    }
    bool isEmpty() const;
    void addNode(Nodeptr node);
    void removeNode(Nodeptr node);
    Nodeptr getFirstNode() const;

    friend class LfuCache<Key, Value>;
};

template<typename Key, typename Value>
bool FreqList<Key, Value>::isEmpty() const{
    return head_->next == tail_;
}

template<typename Key, typename Value>
void FreqList<Key, Value>::addNode(Nodeptr node){
    if(!node||!head_||!tail_) return;
    auto lastNode = tail_->pre;
    node->next = tail_;
    node->pre = lastNode;
    tail_->pre = node;
    lastNode.lock()->next = node;
}

template<typename Key, typename Value>
void FreqList<Key, Value>::removeNode(Nodeptr node){
    if(!node||!head_||!tail_) return;
    if(node->pre.expired()||!node->next) return;
    auto preNode = node->pre.lock();
    preNode->next = node->next;
    node->next->pre = preNode;
    node->next = nullptr;
    node->pre.reset();
};

template<typename Key, typename Value>
typename FreqList<Key,Value>::Nodeptr FreqList<Key, Value>::getFirstNode() const{
    return head_->next;
}
