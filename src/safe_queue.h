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

public:
	safe_queue();

	~safe_queue();

	void unsync_push(T elem);

	bool empty();

	bool try_pop(T& value);
};
