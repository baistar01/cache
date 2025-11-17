#include <thread>

#include "TestBase.h"
#include "LruCache.h"
#include "LfuCache.h"
#include "LruKCache.h"
#include "HashLruCache.h"
#include "HashLfuCache.h"
#include "ArcCache.h"
#include "ArcHashCache.h"

int main() {
    // LRU
    LruCache<int, std::string> lru(50);
    TestBase<LruCache<int, std::string>> testLru(lru);
    testLru.testHotData();
    testLru.testLoop();
    testLru.testWorkloadShift();
    // LFU
    LfuCache<int, std::string> lfu(50, 1000);
    TestBase<LfuCache<int, std::string>> testLfu(lfu);
    testLfu.testHotData();
    testLfu.testLoop();
    testLfu.testWorkloadShift();
    // LRU-K
    LruKCache<int, std::string> lruk(50, 500, 2);
    TestBase<LruKCache<int, std::string>> testLruk(lruk);
    testLruk.testHotData();
    testLruk.testLoop();
    testLruk.testWorkloadShift();
    //HashLRU
    HashLruCache<int, std::string> hashLru(50, 4);
    TestBase<HashLruCache<int, std::string>> testHashLru(hashLru);
    testHashLru.testHotData();
    testHashLru.testLoop();
    testHashLru.testWorkloadShift();
    //HashLFU
    HashLfuCache<int, std::string> hashLfu(50, 4);
    TestBase<HashLfuCache<int, std::string>> testHashLfu(hashLfu);
    testHashLfu.testHotData();
    testHashLfu.testLoop();
    testHashLfu.testWorkloadShift();
    //ARC
    ArcCache<int, std::string> arc(50, 2);
    TestBase<ArcCache<int, std::string>> testArc(arc);
    testArc.testHotData();
    testArc.testLoop();
    testArc.testWorkloadShift();
    //ARCHash
    ArcHashCache<int, std::string> hashArc(50, 4, 2);
    TestBase<ArcHashCache<int, std::string>> testHashArc(hashArc);
    testHashArc.testHotData();
    testHashArc.testLoop();
    testHashArc.testWorkloadShift();

    return 0;
}