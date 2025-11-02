#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>

#include "LruKCache.h"

// 辅助函数：打印结果
void printResults(const std::string& testName, int capacity, int hit, int get_ops) {
    std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    double hitRate = 100.0 * hit / get_ops;
    std::cout << "LRU-K - 命中率: " << std::fixed << std::setprecision(2) 
              << hitRate << "% (" << hit << "/" << get_ops << ")" << std::endl;
    std::cout << std::endl;
}

void testHotDataAccess() {
    std::cout << "\n=== 测试场景1：热点数据访问测试 ===" << std::endl;
    
    const int CAPACITY = 20;
    const int OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;

    LruKCache<int, std::string> lruk(CAPACITY, HOT_KEYS + COLD_KEYS, 2);

    std::random_device rd;
    std::mt19937 gen(rd());

    int hits = 0;
    int get_ops = 0;

    // 先预热缓存
    for (int key = 0; key < HOT_KEYS; ++key) {
        lruk.put(key, "value" + std::to_string(key));
    }

    // 交替进行put和get操作
    for (int op = 0; op < OPERATIONS; ++op) {
        // 设置30%进行写操作，设置70%进行访问操作
        bool isPut = (gen() % 100 < 30);
        int key = (gen() % 100 < 70) ? gen() % HOT_KEYS : HOT_KEYS + (gen() % COLD_KEYS);

        if (isPut) {
            lruk.put(key, "value" + std::to_string(key) + "_v" + std::to_string(op % 100));
        } else {
            std::string result;
            get_ops++;
            if (lruk.get(key, result)) hits++;
        }
    }

    printResults("热点数据访问测试", CAPACITY, hits, get_ops);
}

void testLoopPattern() {
    std::cout << "\n=== 测试场景2：循环扫描测试 ===" << std::endl;

    const int CAPACITY = 50;
    const int LOOP_SIZE = 500;
    const int OPERATIONS = 200000;

    LruKCache<int, std::string> lruk(CAPACITY, LOOP_SIZE * 2, 2);

    std::random_device rd;
    std::mt19937 gen(rd());

    int hits = 0;
    int get_ops = 0;

    // 预热
    for (int key = 0; key < LOOP_SIZE / 5; ++key) {
        lruk.put(key, "loop" + std::to_string(key));
    }

    int current_pos = 0;
    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 20);
        int key;

        if (op % 100 < 60) {
            key = current_pos;
            current_pos = (current_pos + 1) % LOOP_SIZE;
        } else if (op % 100 < 90) {
            key = gen() % LOOP_SIZE;
        } else {
            key = LOOP_SIZE + (gen() % LOOP_SIZE);
        }

        if (isPut) {
            lruk.put(key, "loop" + std::to_string(key) + "_v" + std::to_string(op % 100));
        } else {
            std::string result;
            get_ops++;
            if (lruk.get(key, result)) hits++;
        }
    }

    printResults("循环扫描测试", CAPACITY, hits, get_ops);
}

void testWorkloadShift() {
    std::cout << "\n=== 测试场景3：工作负载剧烈变化测试 ===" << std::endl;

    const int CAPACITY = 30;
    const int OPERATIONS = 80000;
    const int PHASE_LENGTH = OPERATIONS / 5;

    LruKCache<int, std::string> lruk(CAPACITY, 500, 2);

    std::random_device rd;
    std::mt19937 gen(rd());

    int hits = 0;
    int get_ops = 0;

    // 预热
    for (int key = 0; key < 30; ++key) {
        lruk.put(key, "init" + std::to_string(key));
    }

    for (int op = 0; op < OPERATIONS; ++op) {
        int phase = op / PHASE_LENGTH;
        int putProbability;
        switch (phase) {
            case 0: putProbability = 15; break;
            case 1: putProbability = 30; break;
            case 2: putProbability = 10; break;
            case 3: putProbability = 25; break;
            case 4: putProbability = 20; break;
            default: putProbability = 20;
        }

        bool isPut = (gen() % 100 < putProbability);
        int key;

        if (op < PHASE_LENGTH) {
            key = gen() % 5;
        } else if (op < PHASE_LENGTH * 2) {
            key = gen() % 400;
        } else if (op < PHASE_LENGTH * 3) {
            key = (op - PHASE_LENGTH * 2) % 100;
        } else if (op < PHASE_LENGTH * 4) {
            int locality = (op / 800) % 5;
            key = locality * 15 + (gen() % 15);
        } else {
            int r = gen() % 100;
            if (r < 40) key = gen() % 5;
            else if (r < 70) key = 5 + (gen() % 45);
            else key = 50 + (gen() % 350);
        }

        if (isPut) {
            lruk.put(key, "value" + std::to_string(key) + "_p" + std::to_string(phase));
        } else {
            std::string result;
            get_ops++;
            if (lruk.get(key, result)) hits++;
        }
    }

    printResults("工作负载剧烈变化测试", CAPACITY, hits, get_ops);
}

int main() {
    testHotDataAccess();
    testLoopPattern();
    testWorkloadShift();
    return 0;
}