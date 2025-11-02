#pragma once

#include <memory>

// 前向声明
template<typename Key, typename Value> class ArcLru;
template<typename Key, typename Value> class ArcLfu;


template<typename Key, typename Value>
class ArcNode
{
public:
    ArcNode() : accessCount_(1), next_(nullptr) {}
    ArcNode(Key key, Value value)
    : key_(key)
    , value_(value)
    , accessCount_(1)
    , next_(nullptr)
    {}

    Key getKey() const { return key_; }
    Value getValue() const { return value_; }
    size_t getAccessCount() const { return accessCount_; }
    
    void setValue(const Value& value) { value_ = value; }
    void increamentAccessCount() { ++accessCount_; }

    friend class ArcLru<Key, Value>;
    friend class ArcLfu<Key, Value>;

private:
    Key key_;
    Value value_;
    size_t accessCount_;
    std::weak_ptr<ArcNode<Key, Value>> prev_;
    std::shared_ptr<ArcNode<Key, Value>> next_;
};