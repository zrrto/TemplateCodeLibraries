#pragma once

#include<mutex>
#include<queue>
#include<iostream>
#include<windows.h>
#include<functional>
  
class ThreadPool {
public:
	ThreadPool(std::function<void(LPVOID)> &fun, int task_max = 50, int n = 6);
	~ThreadPool();
	bool AddTask(LPVOID handle);
	void Start();
	void Stop();

private:
	void ThreadFun(int thread_id);

private:
	bool is_run_ = false;
	bool stop_ = false;
	std::mutex mut_run_;

	int max_task_num_;
	int thread_num_;
	std::function<void(LPVOID)> handle_func_;

	std::mutex cout_mutex_;
	std::vector<std::thread> threads_;
	
	std::condition_variable cv_;
	std::mutex mut_task_;
	std::queue<LPVOID> tasks_;
};

