#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <Queue>
#include <functional>
#include <vector>
#include <condition_variable>
#include <thread>
#include <iostream>

template <class T>
class Task {
public:  
    Task() = default;
    Task(const T &data, std::function<void(T)> &f) : data_(data), f_(f){}
    void Execute() {
        f_(data_);
    };
protected:
    T data_;
    std::function<void(T)> f_;
};

template <typename T, typename Tk = Task<T>>
class ThreadPool {
public:
    ThreadPool(int task_max = 50, int n = -1);
    ~ThreadPool();
    bool AddTask(Tk t);
    void Start();
    void Stop();

private:
    void ThreadFun(int thread_id) {
        while (!stop_) {
            {
                std::lock_guard<std::mutex> lock(cout_mutex_);
                std::cout << "Thread" << thread_id << " run!" << std::endl;
            }
            
            Tk task;
            {
                std::unique_lock<std::mutex> locker(mut_task_);
                cv_.wait(locker, [&]() { return stop_ || !tasks_.empty(); });
                if(stop_) {
                    return;
                }
                task = tasks_.front();
                tasks_.pop();
                
            }
            task.Execute();
        }

        {
            std::lock_guard<std::mutex> lock(cout_mutex_);
            std::cout << "Thread" << thread_id << " quit!" << std::endl;
        }
    }


private:
    bool is_run_ = false;
    bool stop_ = false;
    std::mutex mut_run_;

    int thread_num_;
    std::mutex cout_mutex_;
    std::vector<std::thread> threads_;
    int max_task_num_;
    std::condition_variable cv_;
    std::mutex mut_task_;
    std::queue<Tk> tasks_;
};

template<typename T, typename Tk>
ThreadPool<T, Tk>::ThreadPool(int task_max, int n) {
    max_task_num_ = task_max > 10 ? task_max : 10;

    int num = std::thread::hardware_concurrency() - 1;
    thread_num_ =  num ;
}

template<typename T,  typename Tk >
inline ThreadPool<T,  Tk>::~ThreadPool() {
    for (int i = 0; i < thread_num_; i++) {
        threads_[i].join();
    }
}

template<typename T,  typename Tk>
inline bool ThreadPool<T, Tk>::AddTask(Tk t) {
    std::lock_guard<std::mutex> guard(mut_task_);
    if (tasks_.size() >= max_task_num_) {
        return false;
    }
    tasks_.push(t);
    cv_.notify_one();
    return true;
}

template<typename T,  typename Tk>
inline void ThreadPool<T, Tk>::Start() {
    {
        std::lock_guard<std::mutex> guard(mut_run_);
        if (is_run_) {
            return;
        }
        is_run_ = true;
    }
    for (int i = 0; i < thread_num_; i++) {
        threads_.emplace_back([this, i]() {return ThreadFun(i); });
    }
}

template<typename T, typename Tk>
inline void ThreadPool<T, Tk>::Stop() {
    std::lock_guard<std::mutex> guard(mut_run_);
    stop_ = true;
    is_run_ = false;
    cv_.notify_all();
}
#endif // !THREAD_POOL_H
