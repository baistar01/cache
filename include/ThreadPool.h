
#include <vector>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <type_traits>

// ThreadGuard类用于线程析构时自动join所有线程
class ThreadGuard {
public:
    ThreadGuard(std::vector<std::thread>& threads): threads_(threads) {}
    ~ThreadGuard(){
        for(auto& thread : threads_){
            if(thread.joinable()){
                thread.join();
            }
        }
    }

private:
    ThreadGuard(const ThreadGuard&) = delete;                // 禁止拷贝构造
    ThreadGuard& operator = (const ThreadGuard&) = delete;   // 禁止拷贝赋值
    ThreadGuard(ThreadGuard&&) = delete;                     // 禁止移动构造
    ThreadGuard& operator = (ThreadGuard&&) = delete;        // 禁止移动赋值

private:
    std::vector<std::thread>& threads_;
};

// 线程池类
class ThreadPool{
public:
    using Task = std::function<void()>; // 任务类型，无参无返回值，通过包装器传递参数和返回值，即“std::packaged_task”
public:
    explicit ThreadPool(int n=0);
    // 析构时将stop_设置为true，并唤醒所有线程退出。
    ~ThreadPool(){
        stop();
        cond_.notify_all();
    }

    void stop(){
        stop_.store(true, std::memory_order_release);  // “std::memory_order_release”:生产者：“我准备好了数据”
    }

    template<typename F, typename... Args>
    std::future<typename std::result_of<F(Args...)>::type> add(F&& f, Args&&... args); // 添加任务

private:
    ThreadPool(const ThreadPool&) = delete;                   // 禁止拷贝构造
    ThreadPool& operator = (const ThreadPool&) = delete;      // 禁止拷贝赋值
    ThreadPool(ThreadPool&&) = delete;                        // 禁止移动构造
    ThreadPool& operator = (ThreadPool&&) = delete;           // 禁止移动赋值
private:
    std::atomic<bool> stop_;  // 原子变量，多个线程同时访问保持数据一致
    std::vector<std::thread> threads_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<Task> task_;
    ThreadGuard threadguard_;
};

inline ThreadPool::ThreadPool(int n)
    :stop_(false)
    ,threadguard_(threads_){
    int nthreads = 0;
    if(n <= 0){
        nthreads = std::thread::hardware_concurrency();
    }else{
        nthreads = n;
    }
    for(int i=0; i<nthreads; i++){
        threads_.emplace_back(
            // 创建线程，并进行任务循环
            // lambda表达式，this指针捕获，可访问类成员
            [this](){
                // "std::memory_order_acquire":消费者：“我看到了生产者准备好的数据”
                while(!stop_.load(std::memory_order_acquire)){
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(this->mutex_);
                        // 线程阻塞，“cond.wait(lock,predicate)”，当predicate返回true时解除阻塞，否则阻塞，等待notify唤醒
                        // 条件：stop_为false或任务队列为空，进行阻塞，等待唤醒执行任务
                        this->cond_.wait(lock, [this](){
                            return stop_.load(std::memory_order_acquire) || !this->task_.empty();
                        });
                        // 如果被唤醒后stop_为true且任务队列为空，直接返回，结束线程
                        if(stop_.load(std::memory_order_acquire) && this->task_.empty()){
                            return;
                        }
                        // 移动赋值
                        task = std::move(this->task_.front());
                        this->task_.pop();
                    }
                    task();  // 调用任务
                }
            }
        );
    }
}


// 添加任务到任务队列
template<typename F, typename... Args>  // 可变参数模板
std::future<typename std::result_of<F(Args...)>::type>
ThreadPool::add(F&& f, Args&&... args)
{
    using returnType = typename std::result_of<F(Args...)>::type;    // 获取函数的返回值类型
    using TaskType = std::packaged_task<returnType()>;               // 可以将函数的返回值放进future中

    auto task = std::make_shared<TaskType>(
        // 绑定函数和参数，返回无参函数对象
        // std::bind(function, arg1, arg2, ...)：将函数和参数绑定，返回一个可调用对象
        // std::forward<T>(value)：完美转发，保持参数的左值/右值属性
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    std::future<returnType> fte = task->get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(stop_.load(std::memory_order_acquire)){
            throw std::runtime_error("ThreadPool has been stopped");
        }
        task_.emplace([task](){ (*task)();});
    }
    this->cond_.notify_one();
    return fte;
}
