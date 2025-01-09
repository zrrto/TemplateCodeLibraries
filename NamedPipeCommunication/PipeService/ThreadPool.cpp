#include "ThreadPool.h"

ThreadPool::ThreadPool(std::function<void(LPVOID)>& fun, int task_max, int n): handle_func_(fun),max_task_num_(task_max), thread_num_(n) {}

ThreadPool::~ThreadPool() {
    for (int i = 0; i < thread_num_; i++) {
        threads_[i].join();
    }
}

bool ThreadPool::AddTask(LPVOID handle) {
    std::lock_guard<std::mutex> guard(mut_task_);
    if (tasks_.size() >= max_task_num_) {
        return false;
    }
    tasks_.push(handle);
    cv_.notify_one();
    return true;
}

void ThreadPool::Start() {
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

void ThreadPool::Stop() {
    std::lock_guard<std::mutex> guard(mut_run_);
    stop_ = true;
    is_run_ = false;
    cv_.notify_all();
}

void ThreadPool::ThreadFun(int thread_id) {
    while (!stop_) {
        {
            std::lock_guard<std::mutex> lock(cout_mutex_);
            std::cout << "Thread" << thread_id << " run!" << std::endl;
        }

        LPVOID handle;
        {
            std::unique_lock<std::mutex> locker(mut_task_);
            cv_.wait(locker, [&]() { return stop_ || !tasks_.empty(); });
            if (stop_) {
                return;
            }
            handle = tasks_.front();
            tasks_.pop();
        }
        handle_func_(handle);
    }

    {
        std::lock_guard<std::mutex> lock(cout_mutex_);
        std::cout << "Thread" << thread_id << " quit!" << std::endl;
    }
}
