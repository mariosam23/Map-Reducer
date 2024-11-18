#pragma once

#include <queue>
#include <pthread.h>
#include <string>
#include <utility>

// thread safe queue to allow dynamic allocation of files to mappers
template <typename T>
class safe_queue {
private:
	std::queue<T> q;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

public:
	safe_queue();

	~safe_queue();

	void push(T elem);

	void unsync_push(T elem);

	T pop();

	T unsync_pop();

	bool empty();

	bool unsync_empty();

	bool try_pop(T& value);
};
