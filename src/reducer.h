#pragma once

#include <pthread.h>
#include <string>
#include <vector>
#include <unordered_set>

#include "utils.h"
#include "mapper.h" 

struct reducer_t {
    int reducer_id;
    int num_reducers;
    vector<mapper_t>* mappers_data;
    pthread_barrier_t* barrier;
    pthread_barrier_t* reducer_threads_barrier;
};

