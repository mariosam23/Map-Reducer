#pragma once

#include <pthread.h>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include "safe_queue.h"
#include "utils.h"

struct mapper_t {
    safe_queue<pair<string, int>>* q;
    unordered_map<string, unordered_set<int>> word_to_file_ids;
    pthread_barrier_t* barrier;
};
