#pragma once

#include <pthread.h>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include "safe_queue.h"
#include "utils.h"

typedef string filename;
typedef int file_id;

struct mapper_t {
    safe_queue<pair<filename, file_id>>* q;
    unordered_map<string, unordered_set<int>> word_to_file_ids;
    pthread_barrier_t* barrier;
};
