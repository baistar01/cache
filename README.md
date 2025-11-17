# cache
实现了Lru, Lru-k, Lfu, Lru 分片, Lfu 分片, Arc 缓存策略

c++11

g++ -std:c++11 -O2 src/…… -Iinclude -o bin/……

# 实现
./include/CachePolicy.h：缓存策略的基类函数，声明接口

# LRU缓存
./include/LruNode.h：定义LRU缓存的节点类

./include/LruCache.h：实现了LRU缓存策略

./include/LruKCache.h：基于LruCache实现LRU-K缓存策略

./include/HashLruCache.h：实现了切片LRU缓存策略

# LFU缓存
./include/LfuList.h：定义了LFU缓存的链表结构

./include/LfuCache.h：实现了LFU缓存策略

./include/HashLfuCache.h：实现了切片LFU缓存策略

# ARC缓存
./include/ArcCacheNode.h：定义了ARC缓存的节点

./include/ArcCache.h：定义自适应缓存策略

./include/ArcLfu.h：定义Arc的LFU缓存机制

./include/ArcLru.h：定义Arc的LRU缓存机制

./include/ArcHashCache.h：定义ArcHash的缓存机制


！制作ArcHash缓存时需要对Arc进行分片而不是对其中的Lru和Lfu分片

# 测试代码
./src/Lru.cpp

./src/Lfu.cpp

./src/LruK.cpp

./src/HashLru.cpp

./src/HashLfu.cpp

./src/Arc.cpp

./src/ArcHash.cpp


./include/TestBase.h 定义的测试基准

./src/TestAll.cpp  基于TestBase的所有类型测试

场景1:热点数据访问：

参数名称：capacity缓存容量、ops操作次数、put概率、get概率、hotKeys热点数据、coldKeys冷点数据、热点概率、冷点概率

参数设置：50，200 000，30%，70%，50，500，70%，30%


场景2:循环扫描：

参数名称：capacity缓存容量、ops操作次数、put概率、get概率、循环扫描范围、顺序扫描、随机扫描

参数设置：50，200 000，30%，70%，200，70%，30%


场景3:工作负载剧烈变化：

参数名称：capacity缓存容量、ops操作次数、put概率、get概率、case种类

参数设置：50，200 000，30%，70%，5

case0:只访问5个缓存

case1:访问300个缓存

case2:线性扫描前100个缓存

case3:分组访问

case4:热冷混合访问



result.txt：记录测试结果

# 个人收获
基于c++实现了支持线程安全的高并发缓存系统，实现了多种缓存策略（LRU、LFU、ARC）及其变种（LRU-K、HashLRU、HashLFU、HashArc）

理解多线程编程和线程同步机制，包括互斥锁的使用、线程的使用、降低锁的粒度等。

掌握了多种缓存策略的设计与实现。

制定多种测试环境，热冷点数据访问、循环扫描访问、工作负载剧烈变化等。
