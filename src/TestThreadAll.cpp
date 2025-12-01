#include<iostream>
#include<thread>
#include<string>

#include "LruCache.h"
#include "LfuCache.h"
#include "LruKCache.h"
#include "HashLruCache.h"
#include "HashLfuCache.h"
#include "ArcCache.h"
#include "ArcHashCache.h"

#include "ThreadPool.h"
#include "TestThread.h"

template<typename Cache>
void runAllTests(const std::string& title, Cache& cache, ThreadPool *pool, int nthreads, int mode)
{
    std::cout<< title<<std::endl;
    TestRunner<ArcHashCache<int, std::string>, ThreadPool> runner(cache, pool, nthreads, mode);
    runner.testHotData(50,200000,50,500);
    runner.testLoop(50,200,200000);
    runner.testWorkloadShift(50,200000);
    std::cout<<std::endl;
}

int main()
{
    using CacheType = ArcHashCache<int, std::string>;

    // 单线程测试
    CacheType cache1(50,32,2);
    runAllTests("单线程测试", cache1, nullptr, 1, 0);

    // 多线程测试
    CacheType cache2(50,32,2);
    runAllTests("多线程测试", cache2, nullptr, 4, 1);

    // 线程池测试
    ThreadPool tp(4);
    CacheType cache3(50,32,2);
    runAllTests("线程池测试", cache3, &tp , 4, 2);

    return 0;
}