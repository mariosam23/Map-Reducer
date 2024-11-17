#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <pthread.h>
#include <unordered_set>

#include "safe_queue.h"
#include "mapper.h"

using namespace std;

void get_file_names(const string& input_file, safe_queue<pair<string, int>>& q);

void *map_func(void *arg) {
    mapper_t *mapper_data = (mapper_t *)arg;
    safe_queue<pair<string, int>>& q = mapper_data->q;
    vector<pair<string, int>>& words_map = mapper_data->words;
    pthread_barrier_t& barrier = mapper_data->barrier;

    words_map.reserve(100);
    unordered_set<string> unique_words;

    pair<string, int> file_data;
    while (q.try_pop(file_data)) {
        string file_name = file_data.first;
        int file_id = file_data.second;

        ifstream file(file_name);
        assert(file.is_open() && "Could not open file");

        string line;
        while (getline(file, line)) {
            string word;
            for (char c : line) {
                if (isalpha(c)) {
                    word.push_back(tolower(c));
                } else if (c == '\'') {
                    continue;
                } else {
                    if (!word.empty()) {
                        words_map.push_back({word, file_id});
                        word.clear();
                    }
                }
            }
            if (!word.empty()) {
                words_map.push_back({word, file_id});
            }
        }
        file.close();
    }

    pthread_barrier_wait(&barrier);
    return NULL;
}

void *reduce_func(void *arg) {
    pthread_barrier_t *barrier = (pthread_barrier_t *)arg;
    pthread_barrier_wait(barrier);

    // reduce operations

    return NULL;
}


int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);
    assert(argc == 4 && "Usage: ./tema1 <numar_mapperi> <numar_reduceri> <fisier_intrare>");
    int num_mappers = atoi(argv[1]), num_reducers = atoi(argv[2]);
    string input_file = argv[3];
    
    safe_queue<pair<string, int>> q;
    get_file_names(input_file, q);

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, num_mappers + num_reducers);

    pthread_t mappers[num_mappers];
    pthread_t reducers[num_reducers];
    vector<mapper_t> mappers_data;
    mappers_data.reserve(num_mappers);
    ofstream out("output.txt");

    for (int i = 0; i < num_mappers + num_reducers; ++i) {
        if (i < num_mappers) {
            mappers_data.push_back({q, vector<pair<string, int>>(), barrier});
            pthread_create(&mappers[i], NULL, &map_func, &mappers_data[i]);
        } else {
            pthread_create(&reducers[i - num_mappers], NULL, &reduce_func, &barrier);
        }
    }

    for (int i = 0; i < num_mappers; ++i) {
        pthread_join(mappers[i], NULL);
    }

    for (int i = 0; i < num_reducers; ++i) {
        pthread_join(reducers[i], NULL);
    }


    pthread_barrier_destroy(&barrier);

    return 0;
}

void get_file_names(const string& input_file, safe_queue<pair<string, int>>& q) {
    ifstream file(input_file);
    assert(file.is_open() && "Could not open file");

    int num_files;
    file >> num_files;
    file.ignore();

    string line;
    const string dir = "../checker/";
    int file_id = 1;
    while (getline(file, line)) {
        q.unsync_push({dir + line, file_id++});
    }

    file.close();
}
