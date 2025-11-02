#pragma once
using namespace std;

template<typename Key, typename Value>
class LruCache;

template<typename Key, typename Value>
class LruNode{
    private:
        Key key;
        Value value;
        size_t accessCount;
        weak_ptr<LruNode<Key, Value>> prev;
        shared_ptr<LruNode<Key, Value>> next;
    public:
        LruNode(Key key, Value value):key(key), value(value), accessCount(1), prev(), next() {}

        Key getKey() const { return key; }
        Value getValue() const { return value; }
        void setValue(const Value& value) { this->value = value;}
        size_t getAccessCount() const { return accessCount; }
        void incrementAccessCount() { ++accessCount; }

        friend class LruCache<Key, Value>;
};