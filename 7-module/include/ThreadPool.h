#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

// Async overflow policy - block by default.
enum class async_overflow_policy {
    block,           // Block task can be enqueued
    overrun_oldest,  // Discard oldest task in the queue if full when trying to
                     // add new item.
    discard_new      // Discard new task if the queue is full when trying to add new item.
};

class ThreadPool {
public:
    ThreadPool(size_t threads, size_t maxQueueSize = 1000, async_overflow_policy policy = async_overflow_policy::block);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function<void()> > tasks;
    
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
    
    // overflow policy
    size_t maxQueueSize;
    async_overflow_policy overflow_policy;
};
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads, size_t maxQueueSize, async_overflow_policy policy)
    : stop(false), maxQueueSize(maxQueueSize), overflow_policy(policy)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            }
        );
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        // 应用不同的溢出策略
        if(tasks.size() >= maxQueueSize) {
            switch (overflow_policy) {
                case async_overflow_policy::block:
                    // 阻塞直到有空间
                    condition.wait(lock, [this]{ return tasks.size() < maxQueueSize; });
                    break;
                case async_overflow_policy::overrun_oldest:
                    // 溢出最旧的任务
                    tasks.pop();
                    break;
                case async_overflow_policy::discard_new:
                    // 丢弃新任务
                    return std::future<return_type>(); // 返回一个空的 future
            }
        }

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

#endif
