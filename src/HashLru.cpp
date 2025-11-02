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
                 const std::vector<int>& hits) {
    std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    
    for (size_t i = 0; i < hits.size(); ++i) {
        double hitRate = 100.0 * hits[i] / get_operations[i];
        std::cout << "HashLruCache - 命中率: " << std::fixed << std::setprecision(2) 
                  << hitRate << "% ";
        std::cout << "(" << hits[i] << "/" << get_operations[i] << ")" << std::endl;
    }
    std::cout << std::endl;
}

// 场景1：热点数据访问
void testHotDataAccess() {
    std::cout << "\n=== 测试场景1：热点数据访问 ===" << std::endl;
    
    const int CAPACITY = 20;
    const int OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;

    HashLruCache<int, std::string> hashLru(CAPACITY, 4); // 4分片示例
    std::random_device rd;
    std::mt19937 gen(rd());

    int hits = 0, get_ops = 0;

    // 预热热点数据
    for (int key = 0; key < HOT_KEYS; ++key) {
        hashLru.put(key, "value" + std::to_string(key));
    }

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30); // 30%写
        int key = (gen() % 100 < 70) ? (gen() % HOT_KEYS) : (HOT_KEYS + gen() % COLD_KEYS);

        if (isPut) {
            hashLru.put(key, "value" + std::to_string(key) + "_v" + std::to_string(op % 100));
        } else {
            std::string result;
            get_ops++;
            if (hashLru.get(key, result)) hits++;
        }
    }

    printResults("热点数据访问测试", CAPACITY, {get_ops}, {hits});
}

// 场景2：循环扫描
void testLoopPattern() {
    std::cout << "\n=== 测试场景2：循环扫描 ===" << std::endl;

    const int CAPACITY = 50;
    const int LOOP_SIZE = 500;
    const int OPERATIONS = 200000;

    HashLruCache<int, std::string> hashLru(CAPACITY, 4);
    std::random_device rd;
    std::mt19937 gen(rd());

    int hits = 0, get_ops = 0;
    int current_pos = 0;

    // 预热部分数据
    for (int key = 0; key < LOOP_SIZE/5; ++key) {
        hashLru.put(key, "loop" + std::to_string(key));
    }

    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 20); // 20%写
        int key;
        if (op % 100 < 60) { key = current_pos++; current_pos %= LOOP_SIZE; } // 顺序扫描
        else if (op % 100 < 90) key = gen() % LOOP_SIZE; // 随机跳跃
        else key = LOOP_SIZE + gen() % LOOP_SIZE;        // 范围外访问

        if (isPut) {
            hashLru.put(key, "loop" + std::to_string(key) + "_v" + std::to_string(op % 100));
        } else {
            std::string result;
            get_ops++;
            if (hashLru.get(key, result)) hits++;
        }
    }

    printResults("循环扫描测试", CAPACITY, {get_ops}, {hits});
}

// 场景3：工作负载剧烈变化
void testWorkloadShift() {
    std::cout << "\n=== 测试场景3：工作负载剧烈变化 ===" << std::endl;

    const int CAPACITY = 30;
    const int OPERATIONS = 80000;
    const int PHASE_LENGTH = OPERATIONS / 5;

    HashLruCache<int, std::string> hashLru(CAPACITY, 4);
    std::random_device rd;
    std::mt19937 gen(rd());

    int hits = 0, get_ops = 0;

    // 预热初始数据
    for (int key = 0; key < 30; ++key) {
        hashLru.put(key, "init" + std::to_string(key));
    }

    for (int op = 0; op < OPERATIONS; ++op) {
        int phase = op / PHASE_LENGTH;
        int putProb;
        switch (phase) {
            case 0: putProb = 15; break;
            case 1: putProb = 30; break;
            case 2: putProb = 10; break;
            case 3: putProb = 25; break;
            case 4: putProb = 20; break;
            default: putProb = 20;
        }

        bool isPut = (gen() % 100 < putProb);
        int key;
        if (op < PHASE_LENGTH) key = gen() % 5;
        else if (op < PHASE_LENGTH*2) key = gen() % 400;
        else if (op < PHASE_LENGTH*3) key = (op - PHASE_LENGTH*2) % 100;
        else if (op < PHASE_LENGTH*4) {
            int locality = (op / 800) % 5;
            key = locality * 15 + (gen() % 15);
        } else {
            int r = gen() % 100;
            if (r < 40) key = gen() % 5;
            else if (r < 70) key = 5 + gen() % 45;
            else key = 50 + gen() % 350;
        }

        if (isPut) hashLru.put(key, "value" + std::to_string(key) + "_p" + std::to_string(phase));
        else {
            std::string result;
            get_ops++;
            if (hashLru.get(key, result)) hits++;
        }
    }

    printResults("工作负载剧烈变化测试", CAPACITY, {get_ops}, {hits});
}

int main() {
    testHotDataAccess();
    testLoopPattern();
    testWorkloadShift();
    return 0;
}