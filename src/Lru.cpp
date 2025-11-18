#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <chrono>

#include "LruCache.h"


// 打印结果函数
void printLruResult(const std::string& testName, int capacity, int get_ops, int hits, std::chrono::duration<double> diffTime) {
    double hitRate = get_ops > 0 ? 100.0 * hits / get_ops : 0.0;
    std::cout << "=== " << testName << " ===" << std::endl;
    std::cout << "缓存类型: LRU" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    std::cout << "运行时间：" << diffTime.count() << "秒" << std::endl;
    std::cout << "命中率: " << std::fixed << std::setprecision(2)
              << hitRate << "% (" << hits << "/" << get_ops << ")\n\n";
}

/* 场景1：热点数据访问 */
void testHotData_LRU() {
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 热点数据访问测试（LRU） ===" << std::endl;
    const int CAPACITY = 50;
    const int OPERATIONS = 200000;
    const int HOT_KEYS = 50;
    const int COLD_KEYS = 500;

    LruCache<int, std::string> lru(CAPACITY);
    std::mt19937 gen(42);

    int hits = 0, get_ops = 0;  // hits表示缓存命中，get_ops表示get操作
    // 预热
    for (int k = 0; k < HOT_KEYS; ++k)
        lru.put(k, "v" + std::to_string(k));

    for (int op = 0; op < OPERATIONS; ++op) {
        // isPut，30%概率put操作，70%概率get操作
        bool isPut = (gen() % 100 < 30);
        // key，70%概率选择热点，30%概率选择冷点
        int key = (gen() % 100 < 70) ? gen() % HOT_KEYS : HOT_KEYS + (gen() % COLD_KEYS);
        if (isPut)
            lru.put(key, "val" + std::to_string(key));
        else {
            std::string result;
            get_ops++;
            if (lru.get(key, result)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printLruResult("热点访问", CAPACITY, get_ops, hits, diffTime);
}

/* 场景2：循环扫描 */
void testLoop_LRU() {
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 循环扫描（LRU） ===" << std::endl;
    const int CAPACITY = 50;
    const int LOOP_SIZE = 200;
    const int OPERATIONS = 200000;

    LruCache<int, std::string> lru(CAPACITY);

    std::mt19937 gen(42);
    int hits = 0, get_ops = 0;
    int current = 0;

    for (int op = 0; op < OPERATIONS; ++op) {
        // isPut，30%概率put操作，70%概率get操作
        bool isPut = (gen() % 100 < 30);
        // key，70%循环扫描，30%随机扫描
        int key = (op % 100 < 70) ? current++ % LOOP_SIZE : gen() % LOOP_SIZE;
        if (isPut)
            lru.put(key, "v" + std::to_string(key));
        else {
            std::string val;
            get_ops++;
            if (lru.get(key, val)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printLruResult("循环扫描", CAPACITY, get_ops, hits, diffTime);
}

/* 场景3：工作负载变化 */
void testWorkloadShift_LRU() {
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 工作负载剧烈变化测试（LRU） ===" << std::endl;
    const int CAPACITY = 50;
    const int OPERATIONS = 200000;
    const int PHASE_LEN = OPERATIONS / 5; // 用于定义访问case

    LruCache<int, std::string> lru(CAPACITY);
    std::mt19937 gen(42);
    int hits = 0, get_ops = 0;

    for (int op = 0; op < OPERATIONS; ++op) {
        int phase = op / PHASE_LEN;
        // isPut，30%概率put操作，70%概率get操作
        bool isPut = (gen() % 100 < 30);
        int key;
        switch (phase) {
            case 0: key = gen() % 5; break;
            case 1: key = gen() % 300; break;
            case 2: key = (op - PHASE_LEN * 2) % 100; break;
            case 3: key = (op / 800 % 5) * 15 + gen() % 15; break;
            default: key = (gen() % 100 < 40) ? gen() % 5 : 5 + gen() % 45;
        }

        if (isPut)
            lru.put(key, "val" + std::to_string(key));
        else {
            std::string val;
            get_ops++;
            if (lru.get(key, val)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printLruResult("工作负载变化", CAPACITY, get_ops, hits, diffTime);
}

int main() {
    testHotData_LRU();
    testLoop_LRU();
    testWorkloadShift_LRU();
    return 0;
}