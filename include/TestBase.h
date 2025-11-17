#pragma once

#include <string>
#include <chrono>
#include <random>
#include <iostream>
#include <iomanip>
#include <string>

template<typename Cache>
class TestBase{
public:
    TestBase(Cache& cache, std::string name)
    :cache_(cache)
    ,CacheName_(name)
    {
        std::cout<<name<<std::endl;
    }
    ~TestBase() = default;
    void testHotData(const int CAPACITY=50, const int OPERATIONS=200000, const int HOT_KEYS=50, const int COLD_KEYS=500);
    void testLoop(const int CAPACITY=50, const int LOOP_SIZE=200, const int OPERATIONS=200000);
    void testWorkloadShift(const int CAPACITY=50, const int OPERATIONS=200000);
    void printResult(const std::string& testName, int capacity, int getOps, int hits, std::chrono::duration<double> diffTime);
private:
    Cache& cache_;
    std::string CacheName_;
};

template<typename Cache>
void TestBase<Cache>::testHotData(const int CAPACITY, const int OPERATIONS, const int HOT_KEYS, const int COLD_KEYS)
{
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 热点数据访问 ===" << std::endl;

    std::mt19937 gen(42);

    int hits = 0, getOps = 0;

    // 预热缓存
    for (int k = 0; k < HOT_KEYS; ++k) {
        cache_.put(k, "v" + std::to_string(k));
    }

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30);
        int key = (gen() % 100 < 70) ? (gen() % HOT_KEYS) : (HOT_KEYS + (gen() % COLD_KEYS));

        if (isPut) {
            cache_.put(key, "value" + std::to_string(key));
        } else {
            std::string result;
            getOps++;
            if (cache_.get(key, result)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printResult("热点数据访问", CAPACITY, getOps, hits, diffTime);
}

template<typename Cache>
void TestBase<Cache>::testLoop(const int CAPACITY, const int LOOP_SIZE, const int OPERATIONS)
{
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 循环扫描 ===" << std::endl;

    std::mt19937 gen(42);

    int hits = 0, getOps = 0;
    int current = 0;

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30);
        int key = (op % 100 < 70) ? current++ % LOOP_SIZE : gen() % LOOP_SIZE;
        if (isPut)
            cache_.put(key, "loop" + std::to_string(key));
        else {
            std::string result;
            getOps++;
            if (cache_.get(key, result)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printResult("循环扫描", CAPACITY, getOps, hits, diffTime);
}

template<typename Cache>
void TestBase<Cache>::testWorkloadShift(const int CAPACITY, const int OPERATIONS)
{
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 工作负载剧烈变化 ===" << std::endl;

    const int PHASE_LEN = OPERATIONS / 5;

    std::mt19937 gen(42);

    int hits = 0, getOps = 0;

    for (int op = 0; op < OPERATIONS; ++op) {
        int phase = op / PHASE_LEN;
        bool isPut = (gen() % 100 < 30);

        int key;
        switch (phase) {
            case 0: key = gen() % 5; break;
            case 1: key = gen() % 300; break;
            case 2: key = (op - PHASE_LEN * 2) % 100; break;
            case 3: key = (op / 800 % 5) * 15 + gen() % 15; break;
            default: key = (gen() % 100 < 40) ? gen() % 5 : 5 + gen() % 45;
        }
        if (isPut){
            cache_.put(key, "val" + std::to_string(key));
        }
        else
        {
            std::string result;
            getOps++;
            if (cache_.get(key, result)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printResult("工作负载变化测试", CAPACITY, getOps, hits, diffTime);
}

template<typename Cache>
void TestBase<Cache>::printResult(const std::string& testName, int capacity, int getOps, int hits, std::chrono::duration<double> diffTime)
{
    double hitRate = 100.0 * hits / getOps;
    std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    std::cout << "运行时间：" << std::fixed << std::setprecision(6) 
            << diffTime.count() << "秒" << std::endl;
    std::cout << "命中率: " << std::fixed << std::setprecision(2) 
            << hitRate << "% (" << hits << "/" << getOps << ")\n\n";
}