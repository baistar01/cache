#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>

#include "HashLruCache.h"

// 打印命中率结果
void printResults(const std::string& testName, int capacity, 
                 const std::vector<int>& get_operations, 
                 const std::vector<int>& hits, 
                 std::chrono::duration<double> diffTime) {
    std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    std::cout << "运行时间：" << diffTime.count() << "秒" << std::endl;
    
    for (size_t i = 0; i < hits.size(); ++i) {
        double hitRate = 100.0 * hits[i] / get_operations[i];
        std::cout << "命中率: " << std::fixed << std::setprecision(2) 
                  << hitRate << "% (" << hits[i] << "/" << get_operations[i] << ")" << std::endl;
    }
}

// 场景1：热点数据访问
void testHotData_HashLRU() {
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 热点数据访问(HashLRU) ===" << std::endl;
    
    const int CAPACITY = 50;
    const int OPERATIONS = 200000;
    const int HOT_KEYS = 50;
    const int COLD_KEYS = 500;

    HashLruCache<int, std::string> hashLru(CAPACITY, 4); // 4分片示例
    std::mt19937 gen(42);

    int hits = 0, get_ops = 0;

    // 预热热点数据
    for (int k = 0; k < HOT_KEYS; ++k) {
        hashLru.put(k, "v" + std::to_string(k));
    }

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30);
        int key = (gen() % 100 < 70) ? gen() % HOT_KEYS : HOT_KEYS + gen() % COLD_KEYS;

        if (isPut) {
            hashLru.put(key, "val" + std::to_string(key));
        } else {
            std::string result;
            get_ops++;
            if (hashLru.get(key, result)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printResults("热点数据访问", CAPACITY, {get_ops}, {hits}, diffTime);
}

// 场景2：循环扫描
void testLoop_HashLRU() {
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 循环扫描(HashLRU) ===" << std::endl;

    const int CAPACITY = 50;
    const int LOOP_SIZE = 200;
    const int OPERATIONS = 200000;

    HashLruCache<int, std::string> hashLru(CAPACITY, 4);
    std::mt19937 gen(42);

    int hits = 0, get_ops = 0;
    int current = 0;

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30);
        int key = (op % 100 < 70) ? current++ % LOOP_SIZE : gen() % LOOP_SIZE;
        if (isPut) {
            hashLru.put(key, "loop" + std::to_string(key));
        } else {
            std::string result;
            get_ops++;
            if (hashLru.get(key, result)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printResults("循环扫描", CAPACITY, {get_ops}, {hits}, diffTime);
}

// 场景3：工作负载剧烈变化
void testWorkloadShift_HashLRU() {
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 工作负载剧烈变化(HashLRU) ===" << std::endl;

    const int CAPACITY = 50;
    const int OPERATIONS = 200000;
    const int PHASE_LEN = OPERATIONS / 5;

    HashLruCache<int, std::string> hashLru(CAPACITY, 4);
    std::mt19937 gen(42);

    int hits = 0, get_ops = 0;

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
            hashLru.put(key, "val" + std::to_string(key));
        }
        else
        {
            std::string result;
            get_ops++;
            if (hashLru.get(key, result)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printResults("工作负载剧烈变化测试", CAPACITY, {get_ops}, {hits}, diffTime);
}

int main() {
    testHotData_HashLRU();
    testLoop_HashLRU();
    testWorkloadShift_HashLRU();
    return 0;
}