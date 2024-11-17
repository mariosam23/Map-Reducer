#pragma once

#include <pthread.h>
#include <string>
#include <vector>

#include "safe_queue.h"

struct mapper_t {
	safe_queue<std::pair<std::string, int>>& q;
	std::vector<std::pair<std::string, int>> words;
	pthread_barrier_t& barrier;
};
