#pragma once
#include <memory>
#include <mutex>
#include <unordered_map>

#include "CachePolicy.h"
#include "LfuList.h"

template<typename Key, typename Value>
class LfuCache : public cachePolicy<Key, Value> {
public:
    using Node = typename FreqList<Key, Value>::Node;
    using Nodeptr = std::shared_ptr<Node>;
    using NodeMap = std::unordered_map<Key, Nodeptr>;

    LfuCache(int Capacity, int maxAverageNum=1000)
    : capacity_(Capacity)
    , minFreq_(INT8_MAX)
    , maxAverageNum_(maxAverageNum)
    , curAverageNum_(0)
    , curTotalNum_(0)
    {}

    ~LfuCache() override = default;

    void put(Key key, Value value) override;
    bool get(Key key, Value& value) override;
    Value get(Key key) override;
    void purge(); // 清空缓存

private:
    void putInternal(Key key, Value value);        // 添加缓存
    void getInternal(Nodeptr node, Value& value);  // 获取缓存
    void kickOut();                                // 移除缓存中的过期数据
    void removeFromFreqList(Nodeptr node);         // 从频率列表中移除节点
    void addToFreqList(Nodeptr node);              // 将节点添加到频率列表
    void addFreqNum();                             // 增加平均访问等频率
    void decreaseFreqNum(int num);                 // 减少平均访问等频率
    void handleOverMaxAverageNum();                // 处理当前平均访问频率超过上限的情况，“自我调节机制”
    void updateMinFreq();                          // 更新最小访问频率

private:
    int capacity_;                                 // 容量
    int minFreq_;                                  // 最小访问频率
    int maxAverageNum_;                            // 最大平均访问频率
    int curAverageNum_;                            // 当前平均访问频率
    int curTotalNum_;                              // 当前访问总频率
    std::mutex mutex_;
    NodeMap nodeMap_;

    std::unordered_map<int, FreqList<Key, Value>*> freqToFreqList_;  // value为指向FreqList的指针，访问频次到频次链表的映射
};

template<typename Key, typename Value>
void LfuCache<Key, Value>::put(Key key, Value value){
    if(capacity_<=0) return;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodeMap_.find(key);
    // 在缓存中找到key，更新value值，调用getInternal更新访问频次
    if(it != nodeMap_.end()){
        it->second->value = value;
        getInternal(it->second, value);
        return;
    }
    // 未找到缓存key，创建新节点
    putInternal(key, value);
}

template<typename Key, typename Value>
bool LfuCache<Key, Value>::get(Key key, Value& value){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodeMap_.find(key);
    // 在缓存中找到key，调用getInternal更新访问频次
    if(it != nodeMap_.end()){
        getInternal(it->second, value);
        return true;
    }
    return false;
}

template<typename Key, typename Value>
Value LfuCache<Key, Value>::get(Key key){
    Value value;
    get(key, value);
    return value;
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::purge(){
    nodeMap_.clear();
    freqToFreqList_.clear();
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::getInternal(Nodeptr node, Value& value){
    value = node->value;
    // 将当前节点在频次链表中删除，频次加1再添加到新的链表中
    removeFromFreqList(node);
    node->freq++;
    addToFreqList(node);
    
    // 节点需要从旧链表中移动到新链表中
    // 如果移动后该链表为空，则增加最小访问频率
    if(node->freq-1 == minFreq_ && freqToFreqList_[node->freq-1]->isEmpty()){
        minFreq_++;
    }
    // 增加总访问频次和当前平均访问频次
    addFreqNum();
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::putInternal(Key key, Value value){
    // 容量有限，淘汰最不常用的节点
    if(nodeMap_.size() >= capacity_){
        kickOut();
    }
    // 创建新节点，添加到频次链表中，更新最小访问频次
    Nodeptr node = std::make_shared<Node>(key, value);
    nodeMap_[key] = node;
    addToFreqList(node);
    addFreqNum();
    //因为新添加了节点，最小访问频次设置为1
    minFreq_ = std::min(minFreq_, 1);
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::kickOut(){
    // 在最小频次链表中删除第一个节点
    Nodeptr node = freqToFreqList_[minFreq_]->getFirstNode();
    removeFromFreqList(node);
    nodeMap_.erase(node->key);
    decreaseFreqNum(node->freq);
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::removeFromFreqList(Nodeptr node){
    // 根据freq的值定位到对应的频次链表，然后将节点从链表中进行删除
    if(!node) return;
    auto freq = node->freq;
    freqToFreqList_[freq]->removeNode(node);
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::addToFreqList(Nodeptr node){
    // 根据freq的值定位到对应的频次链表，然后将节点添加到链表中
    if(!node) return;
    auto freq = node->freq;
    // 如果频次链表不存在，创建一个新的频次链表
    if(freqToFreqList_.find(freq) == freqToFreqList_.end()){
        freqToFreqList_[node->freq] = new FreqList<Key, Value>(node->freq);
    }
    freqToFreqList_[freq]->addNode(node);
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::addFreqNum(){
    curTotalNum_++;
    if(nodeMap_.empty()) curAverageNum_=0;
    else curAverageNum_ = curTotalNum_ / nodeMap_.size();
    if(curAverageNum_ > maxAverageNum_){
        handleOverMaxAverageNum();
    }
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::decreaseFreqNum(int num){
    // 减少平均访问频次和总访问频次
    curTotalNum_ -= num;
    if(nodeMap_.empty()) curAverageNum_ =0;
    else curAverageNum_ = curTotalNum_ / nodeMap_.size();
}

// debug版本
template<typename Key, typename Value>
void LfuCache<Key, Value>::handleOverMaxAverageNum(){
    // 自我调节机制，防止频次过高
    // add：需要将总频次进行重新计算
    curTotalNum_ = 0;

    if(nodeMap_.empty()) return;
    for(auto it = nodeMap_.begin(); it != nodeMap_.end(); it++){
        if(!it->second) continue;
        Nodeptr node = it->second;
        removeFromFreqList(node);
        node->freq -= maxAverageNum_/2;
        if(node->freq < 1) node->freq =1;
        // add
        curTotalNum_ += node->freq;

        addToFreqList(node);
    }
    // add
    if(nodeMap_.empty()) curAverageNum_ =0;
    else curAverageNum_ = curTotalNum_ / nodeMap_.size();

    updateMinFreq();
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::updateMinFreq(){
    // 更新最小访问频率
    minFreq_ = INT8_MAX;
    for(const auto& pair : freqToFreqList_){
        if(pair.second && !pair.second->isEmpty()){
            minFreq_ = std::min(minFreq_, pair.first);
        }
    }
    if(minFreq_ == INT8_MAX) minFreq_=1;
}