#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include "ArcCache.h"


// 打印结果
void printResult(const std::string& testName, int capacity, int getOps, int hits) {
    double hitRate = 100.0 * hits / getOps;
    std::cout << "=== " << testName << " (ARC 缓存策略) ===" << std::endl;
    std::cout << "缓存容量: " << capacity << std::endl;
    std::cout << "命中率: " << std::fixed << std::setprecision(2) 
              << hitRate << "% (" << hits << "/" << getOps << ")\n\n";
}

// 测试1：热点数据访问
void testHotDataAccess() {
    std::cout << "\n=== 测试场景1：热点数据访问 ===" << std::endl;

    const int CAPACITY = 20;
    const int OPERATIONS = 200000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;

    ArcCache<int, std::string> arc(CAPACITY, 2);

    std::random_device rd;
    std::mt19937 gen(rd());

    int hits = 0, getOps = 0;

    // 预热缓存
    for (int key = 0; key < HOT_KEYS; ++key) {
        arc.put(key, "value" + std::to_string(key));
    }

    // 模拟访问
    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30);
        int key = (gen() % 100 < 70) ? (gen() % HOT_KEYS)
                                     : (HOT_KEYS + (gen() % COLD_KEYS));

        if (isPut) {
            arc.put(key, "value" + std::to_string(key) + "_v" + std::to_string(op % 100));
        } else {
            std::string result;
            getOps++;
            if (arc.get(key, result)) hits++;
        }
    }

    printResult("热点数据访问测试", CAPACITY, getOps, hits);
}

// 测试2：循环扫描
void testLoopPattern() {
    std::cout << "\n=== 测试场景2：循环扫描 ===" << std::endl;

    const int CAPACITY = 50;
    const int LOOP_SIZE = 500;
    const int OPERATIONS = 200000;

    ArcCache<int, std::string> arc(CAPACITY, 2);
    std::random_device rd;
    std::mt19937 gen(rd());

    int hits = 0, getOps = 0;
    int current_pos = 0;

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 20);
        int key;
        if (op % 100 < 60)
            key = current_pos++;
        else if (op % 100 < 90)
            key = gen() % LOOP_SIZE;
        else
            key = LOOP_SIZE + (gen() % LOOP_SIZE);

        current_pos %= LOOP_SIZE;

        if (isPut)
            arc.put(key, "loop" + std::to_string(key));
        else {
            std::string result;
            getOps++;
            if (arc.get(key, result)) hits++;
        }
    }

    printResult("循环扫描测试", CAPACITY, getOps, hits);
}

// 测试3：工作负载变化
void testWorkloadShift() {
    std::cout << "\n=== 测试场景3：工作负载剧烈变化 ===" << std::endl;

    const int CAPACITY = 30;
    const int OPERATIONS = 80000;
    const int PHASE_LEN = OPERATIONS / 5;

    ArcCache<int, std::string> arc(CAPACITY, 2);
    std::random_device rd;
    std::mt19937 gen(rd());

    int hits = 0, getOps = 0;

    for (int key = 0; key < 30; ++key)
        arc.put(key, "init" + std::to_string(key));

    for (int op = 0; op < OPERATIONS; ++op) {
        int phase = op / PHASE_LEN;
        int putProb = (phase == 0) ? 15 : (phase == 1) ? 30 : (phase == 2) ? 10 : (phase == 3) ? 25 : 20;
        bool isPut = (gen() % 100 < putProb);

        int key;
        if (phase == 0) key = gen() % 5;
        else if (phase == 1) key = gen() % 400;
        else if (phase == 2) key = (op - PHASE_LEN * 2) % 100;
        else if (phase == 3) key = ((op / 800) % 5) * 15 + (gen() % 15);
        else key = (gen() % 100 < 40) ? gen() % 5 : 5 + (gen() % 45);

        if (isPut)
            arc.put(key, "value" + std::to_string(key));
        else {
            std::string result;
            getOps++;
            if (arc.get(key, result)) hits++;
        }
    }

    printResult("工作负载变化测试", CAPACITY, getOps, hits);
}

int main() {
    testHotDataAccess();
    testLoopPattern();
    testWorkloadShift();
    return 0;
}