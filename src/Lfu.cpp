#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>

#include "LfuCache.h"

// 打印结果
void printResults(const std::string& testName, int capacity, int get_ops, int hits, std::chrono::duration<double> diffTime) {
    double hitRate = get_ops > 0 ? 100.0 * hits / get_ops : 0.0;
    std::cout << "=== " << testName << " ===" << std::endl;
    std::cout << "缓存类型: LFU" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    std::cout << "运行时间：" << diffTime.count() << "秒" << std::endl;
    std::cout << "命中率: " << std::fixed << std::setprecision(2)
              << hitRate << "% (" << hits << "/" << get_ops << ")\n\n";
}

// 场景1：热点数据访问
void testHotData_LFU() {
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 热点数据访问（LFU） ===" << std::endl;
    const int CAPACITY = 50;
    const int OPERATIONS = 200000;
    const int HOT_KEYS = 50;
    const int COLD_KEYS = 500;

    LfuCache<int, std::string> lfu(CAPACITY, 1000);

    std::mt19937 gen(42);
    int hits = 0, getOps = 0;

    // 预热缓存
    for (int key = 0; key < HOT_KEYS; ++key)
        lfu.put(key, "v" + std::to_string(key));

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30);
        int key = (gen() % 100 < 70) ? gen() % HOT_KEYS : HOT_KEYS + (gen() % COLD_KEYS);

        if (isPut) {
            lfu.put(key, "v" + std::to_string(key));
        } else {
            std::string res;
            getOps++;
            if (lfu.get(key, res)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printResults("热点访问", CAPACITY, getOps, hits, diffTime);
}

// 场景2：循环扫描访问
void testLoop_LFU() {
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 循环扫描（LFU） ===" << std::endl;
    const int CAPACITY = 50;
    const int LOOP_SIZE = 200;
    const int OPERATIONS = 200000;

    LfuCache<int, std::string> lfu(CAPACITY, 1000);
    std::mt19937 gen(42);
    int hits = 0, getOps = 0;
    int current = 0;

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30);
        int key = (op % 100 < 70) ? current++ % LOOP_SIZE : gen() % LOOP_SIZE;
        if (isPut)
            lfu.put(key, "v" + std::to_string(key));
        else {
            std::string res;
            getOps++;
            if (lfu.get(key, res)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printResults("循环扫描", CAPACITY, getOps, hits, diffTime);
}

// 场景3：负载剧烈变化
void testWorkloadShift_LFU() {
    auto timeStart = std::chrono::steady_clock::now(); // 计算起始时间
    std::cout << "\n=== 负载变化 ===" << std::endl;
    const int CAPACITY = 50;
    const int OPERATIONS = 200000;
    const int PHASE_LEN = OPERATIONS / 5;

    LfuCache<int, std::string> lfu(CAPACITY, 1000);
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

        if (isPut)
            lfu.put(key, "v" + std::to_string(key));
        else {
            std::string res;
            getOps++;
            if (lfu.get(key, res)) hits++;
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> diffTime = timeEnd-timeStart; // 以秒为单位

    printResults("负载变化", CAPACITY, getOps, hits, diffTime);
}

int main() {
    testHotData_LFU();
    testLoop_LFU();
    testWorkloadShift_LFU();
    return 0;
}