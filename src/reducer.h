#pragma once

#include <pthread.h>
#include <string>
#include <vector>

struct reducer_t {
	std::vector<std::pair<std::string, int>> words;
	pthread_barrier_t& barrier;
};
