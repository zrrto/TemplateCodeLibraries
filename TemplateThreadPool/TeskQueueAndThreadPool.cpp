
#include "ThreadPool.h"
#include <iostream>

int main() {
	ThreadPool<int, Task<int>> pool;
	pool.Start();
	std::function<void(int)> f = [](int i) { 
		printf ("\nGet result: %d \n",i * i); 
	};
	for (int i = 0; i < 100; i++) {
		Task<int> task(i, f);
		if (!pool.AddTask(task)) {
			printf("\nAdd task %d failed\n", i);
		}
	}
	pool.Stop();
}

