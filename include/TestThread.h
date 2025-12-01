#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <iostream>
#include <iomanip>
#include <future>
#include <utility>

// 定义操作类型
enum class OpType { PUT, GET };

struct Operation {
    OpType type;
    int key;
};

// 根据不同测试场景生成操作序列
class WorkloadGenerator {
public:
    // 使用static：仅使用生成一次操作序列即可
    // 热点数据访问
    static std::vector<Operation>
    generateHotData(int OPERATIONS, int HOT_KEYS, int COLD_KEYS)
    {
        std::vector<Operation> ops;
        ops.reserve(OPERATIONS);  // 一次性分配OPERATIONS大小的空间

        std::mt19937 gen(42);

        for (int i = 0; i < OPERATIONS; i++) {
            bool isPut = (gen() % 100 < 30);
            int key = (gen() % 100 < 70) ? (gen() % HOT_KEYS) : (HOT_KEYS + (gen() % COLD_KEYS));
            ops.push_back({isPut ? OpType::PUT : OpType::GET, key});
        }
        return ops;
    }

    // 循环扫描
    static std::vector<Operation>
    generateLoop(int OPERATIONS, int LOOP_SIZE)
    {
        std::vector<Operation> ops;
        ops.reserve(OPERATIONS);

        std::mt19937 gen(42);
        for (int i = 0; i < OPERATIONS; i++) {
            bool isPut = (gen() % 100 < 30);
            int key = (i % 100 < 70) ? (i % LOOP_SIZE) : (gen() % LOOP_SIZE);
            ops.push_back({isPut ? OpType::PUT : OpType::GET, key});
        }
        return ops;
    }

    // 工作负载循序变化
    static std::vector<Operation>
    generateWorkloadShift(int OPERATIONS)
    {
        std::vector<Operation> ops;
        ops.reserve(OPERATIONS);

        std::mt19937 gen(42);
        const int PHASE_LEN = OPERATIONS / 5;

        for (int i = 0; i < OPERATIONS; i++) {
            int phase = i / PHASE_LEN;
            bool isPut = (gen() % 100 < 30);

            int key = 0;
            switch (phase) {
                case 0: key = gen() % 5; break;
                case 1: key = gen() % 300; break;
                case 2: key = (i - PHASE_LEN * 2) % 100; break;
                case 3: key = ((i / 800) % 5) * 15 + gen() % 15; break;
                default: key = (gen() % 100 < 40) ? gen() % 5 : 5 + gen() % 45;
            }
            ops.push_back({isPut ? OpType::PUT : OpType::GET, key});
        }
        return ops;
    }
};

// 单线程执行器
template<typename Cache>
class TestExecutorSingle {
public:
    static std::pair<int,int> run(Cache& cache, const std::vector<Operation>& ops)
    {
        int hits = 0, getOps = 0;

        for (auto& op : ops) {
            if (op.type == OpType::PUT) {
                cache.put(op.key, "v" + std::to_string(op.key));
            } else {
                std::string result;
                getOps++;
                if (cache.get(op.key, result))
                    hits++;
            }
        }
        return {hits, getOps};
    }
};

// 多线程执行器
template<typename Cache>
class TestExecutorMulti {
public:
    static std::pair<int,int> run(Cache& cache, const std::vector<Operation>& ops, int nthreads)
    {
        int N = ops.size();      // 操作总数
        int per = N / nthreads;  // 每个线程需要操作的数量

        std::vector<std::thread> threads;                       // 存储每个线程
        std::vector<std::pair<int,int>> results(nthreads);      // 存储每个线程的结果

        for (int t = 0; t < nthreads; t++) {
            int l = t * per;                              // 左闭区间
            int r = (t == nthreads - 1 ? N : l + per);    // 右开区间，最后一个线程处理剩余所有操作

            threads.emplace_back([&, t, l, r](){
                // lambda不会显式列出捕获的内容，按需自动捕获外部变量！
                    int hits = 0, getOps = 0;
                    for (int i = l; i < r; i++) {
                        const auto& op = ops[i];
                        if (op.type == OpType::PUT) {
                            cache.put(op.key, "v" + std::to_string(op.key));
                        } else {
                            std::string result;
                            getOps++;
                            if (cache.get(op.key, result)) hits++;
                        }
                    }
                    results[t] = {hits, getOps};
                }
            );
        }

        for(auto& th : threads) {
            th.join();
        }

        int totalHits = 0, totalGetOps = 0;
        for (auto& result : results) {
            totalHits += result.first;
            totalGetOps += result.second;
        }
        return {totalHits, totalGetOps};
    }
};

// 多线程-线程池执行器
// 线程池作为参数传入
template<typename Cache, typename ThreadPool>
class TestExecutorMultiPool {
public:
    static std::pair<int,int> run(Cache& cache, ThreadPool& pool, const std::vector<Operation>& ops, int nthreads)
    {
        int N = ops.size();      // 操作总数
        int per = N / nthreads;  // 每个线程需要操作的数量

        std::vector<std::future<std::pair<int,int>>> futures;   // 存储每个线程的future结果

        for (int t = 0; t < nthreads; t++) {
            int l = t * per;                              // 左闭区间
            int r = (t == nthreads - 1 ? N : l + per);    // 右开区间，最后一个线程处理剩余所有操作

            futures.push_back(
                // lambda不会显式列出捕获的内容，按需自动捕获外部变量！
                // “->std::pair<int,int>”显式指定返回类型
                pool.add([&, l, r]() -> std::pair<int,int> {
                    int hits = 0, getOps = 0;
                    for (int i = l; i < r; i++) {
                        auto& op = ops[i];
                        if (op.type == OpType::PUT) {
                            cache.put(op.key, "v" + std::to_string(op.key));
                        } else {
                            std::string result;
                            getOps++;
                            if (cache.get(op.key, result)) hits++;
                        }
                    }
                    return {hits, getOps};
                })
            );
        }

        int totalHits = 0, totalGetOps = 0;
        for (auto& f : futures) {
            auto result = f.get();
            totalHits += result.first;
            totalGetOps += result.second;
        }
        return {totalHits, totalGetOps};
    }
};

template<typename Cache, typename ThreadPool>
class TestRunner {
public:
    // 初始化为单线程模式
    TestRunner(Cache& cache, ThreadPool* pool=nullptr, int nthreads=1, int model=0)
    : cache_(cache), pool_(pool), nthreads_(nthreads), model_(model) {}

    void testHotData(int CAP=50, int OPS=200000, int HOT=50, int COLD=500) {
        auto ops = WorkloadGenerator::generateHotData(OPS, HOT, COLD);
        runTest("热点数据访问", ops);
    }

    void testLoop(int CAP=50, int LOOP_SIZE=200, int OPS=200000) {
        auto ops = WorkloadGenerator::generateLoop(OPS, LOOP_SIZE);
        runTest("循环扫描", ops);
    }

    void testWorkloadShift(int CAP=50, int OPS=200000) {
        auto ops = WorkloadGenerator::generateWorkloadShift(OPS);
        runTest("工作负载剧烈变化", ops);
    }

private:
    void runTest(const std::string& name, const std::vector<Operation>& ops)
    {
        auto t1 = std::chrono::steady_clock::now();

        std::pair<int,int> result;

        if (model_==0 && (pool_ == nullptr || nthreads_ <= 1)) {
            // 单线程模式
            result = TestExecutorSingle<Cache>::run(cache_, ops);
        } else if(model_==1){
            // 多线程模式
            result = TestExecutorMulti<Cache>::run(cache_, ops, nthreads_);
        } else {
            // 多线程-线程池模式
            result = TestExecutorMultiPool<Cache, ThreadPool>::run(cache_, *pool_, ops, nthreads_);
        }

        auto t2 = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration<double>(t2 - t1).count();

        double hitRate = (double)result.first * 100.0 / result.second;

        std::cout << "\n=== " << name << " ===\n";
        std::cout << "线程数: " << nthreads_ << "\n";
        std::cout << "时间: " << std::fixed << std::setprecision(4) << diff << " 秒\n";
        std::cout << "命中率: " << std::fixed << std::setprecision(2)
                  << hitRate << "% (" << result.first << "/"
                  << result.second << ")\n";
    }

private:
    Cache& cache_;
    ThreadPool* pool_;
    int nthreads_;
    int model_; // 0:单线程，1:多线程，2:多线程-线程池
};