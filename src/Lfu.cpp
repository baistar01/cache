#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>

#include "LfuCache.h"

// 打印结果
void printResults(const std::string& testName, int capacity, int getOps, int hits) {
    double hitRate = 100.0 * hits / getOps;
    std::cout << "=== " << testName << " ===\n";
    std::cout << "容量: " << capacity << "\n";
    std::cout << "LFU 命中率: " << std::fixed << std::setprecision(2) 
              << hitRate << "% (" << hits << "/" << getOps << ")\n\n";
}

// 场景1：热点数据访问
void testHotDataAccess() {
    std::cout << "\n=== 测试1：热点数据访问 ===" << std::endl;
    const int CAPACITY = 20;
    const int OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;

    LfuCache<int, std::string> lfu(CAPACITY, 1000);

    std::random_device rd;
    std::mt19937 gen(rd());
    int hits = 0, getOps = 0;

    // 预热缓存
    for (int key = 0; key < HOT_KEYS; ++key)
        lfu.put(key, "v" + std::to_string(key));

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30);
        int key = (gen() % 100 < 70) ? gen() % HOT_KEYS : HOT_KEYS + (gen() % COLD_KEYS);

        if (isPut) {
            lfu.put(key, "v" + std::to_string(key) + "_" + std::to_string(op % 100));
        } else {
            std::string res;
            getOps++;
            if (lfu.get(key, res)) hits++;
        }
    }
    printResults("热点访问", CAPACITY, getOps, hits);
}

// 场景2：循环扫描访问
void testLoopPattern() {
    std::cout << "\n=== 测试2：循环扫描 ===" << std::endl;
    const int CAPACITY = 50;
    const int LOOP_SIZE = 500;
    const int OPERATIONS = 200000;

    LfuCache<int, std::string> lfu(CAPACITY, 1000);
    std::random_device rd;
    std::mt19937 gen(rd());
    int hits = 0, getOps = 0;

    // 预热20%
    for (int key = 0; key < LOOP_SIZE / 5; ++key)
        lfu.put(key, "loop" + std::to_string(key));

    int pos = 0;
    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 20);
        int key;
        if (op % 100 < 60) key = pos = (pos + 1) % LOOP_SIZE;
        else if (op % 100 < 90) key = gen() % LOOP_SIZE;
        else key = LOOP_SIZE + (gen() % LOOP_SIZE);

        if (isPut)
            lfu.put(key, "loop" + std::to_string(key));
        else {
            std::string res;
            getOps++;
            if (lfu.get(key, res)) hits++;
        }
    }
    printResults("循环扫描", CAPACITY, getOps, hits);
}

// 场景3：负载剧烈变化
void testWorkloadShift() {
    std::cout << "\n=== 测试3：负载变化 ===" << std::endl;
    const int CAPACITY = 30;
    const int OPERATIONS = 80000;
    const int PHASE_LEN = OPERATIONS / 5;

    LfuCache<int, std::string> lfu(CAPACITY, 1000);
    std::random_device rd;
    std::mt19937 gen(rd());
    int hits = 0, getOps = 0;

    // 初始数据
    for (int i = 0; i < 30; ++i)
        lfu.put(i, "init" + std::to_string(i));

    for (int op = 0; op < OPERATIONS; ++op) {
        int phase = op / PHASE_LEN;
        int putProb = (phase == 0 ? 15 : phase == 1 ? 30 : phase == 2 ? 10 : phase == 3 ? 25 : 20);
        bool isPut = (gen() % 100 < putProb);

        int key;
        if (phase == 0) key = gen() % 5;
        else if (phase == 1) key = gen() % 400;
        else if (phase == 2) key = (op - PHASE_LEN * 2) % 100;
        else if (phase == 3) key = ((op / 800) % 5) * 15 + (gen() % 15);
        else {
            int r = gen() % 100;
            if (r < 40) key = gen() % 5;
            else if (r < 70) key = 5 + (gen() % 45);
            else key = 50 + (gen() % 350);
        }

        if (isPut)
            lfu.put(key, "v" + std::to_string(key));
        else {
            std::string res;
            getOps++;
            if (lfu.get(key, res)) hits++;
        }
    }
    printResults("负载变化", CAPACITY, getOps, hits);
}

int main() {
    testHotDataAccess();
    testLoopPattern();
    testWorkloadShift();
    return 0;
}