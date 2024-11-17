#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <pthread.h>
#include <unordered_set>
#include <syncstream>

#include "safe_queue.h"
#include "mapper.h"
#include "reducer.h"
#include "utils.h"
#include <atomic>

using namespace std;

void get_file_names(const string& input_file, safe_queue<pair<string, int>>& q);

void *map_func(void *arg) {
    mapper_t *mapper_data = (mapper_t *)arg;
    safe_queue<pair<string, int>>* q = mapper_data->q;
    unordered_map<string, unordered_set<int>>& word_to_file_ids = mapper_data->word_to_file_ids;
    pthread_barrier_t* barrier = mapper_data->barrier;

    pair<string, int> file_data;
    while (q->try_pop(file_data)) {
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
                        word_to_file_ids[word].insert(file_id);
                        word.clear();
                    }
                }
            }
            if (!word.empty()) {
                word_to_file_ids[word].insert(file_id);
            }
        }

        file.close();
    }

    pthread_barrier_wait(barrier);
    return NULL;
}


void* reduce_func(void* arg) {
    // Wait for all mappers to finish
    reducer_t* reducer_data = (reducer_t*)arg;
    pthread_barrier_t* barrier = reducer_data->barrier;
    pthread_barrier_wait(barrier);

    int reducer_id = reducer_data->reducer_id;
    int num_reducers = reducer_data->num_reducers;
    vector<mapper_t>& mappers_data = *(reducer_data->mappers_data);


    // Determine the letters this reducer is responsible for
    vector<char> assigned_letters;
    for (char c = 'a'; c <= 'z'; ++c) {
        int letter_index = c - 'a';
        int assigned_reducer = letter_index % num_reducers;
        if (assigned_reducer == reducer_id) {
            assigned_letters.push_back(c);
        }
    }

    // Map from word to vector of file IDs
    unordered_map<string, unordered_set<int>> word_file_map_merged;

    // Merge words assigned to this reducer
    for (const auto& mapper_data : mappers_data) {
        const unordered_map<string, unordered_set<int>>& word_map = mapper_data.word_to_file_ids;
        for (const auto& entry : word_map) {
            const string& word = entry.first;
            const unordered_set<int>& file_ids = entry.second;

            // Convert first character to lowercase
            char first_char = word[0];

            // Check if this reducer is responsible for this letter
            if (std::find(assigned_letters.begin(), assigned_letters.end(), first_char) != assigned_letters.end()) {
                // Merge file IDs
                word_file_map_merged[word].insert(file_ids.begin(), file_ids.end());
            }
        }
    }

    // Convert map to vector for sorting
    vector<pair<string, vector<int>>> word_file_list;
    word_file_list.reserve(word_file_map_merged.size());
    for (const auto& entry : word_file_map_merged) {
        vector<int> sorted_file_ids(entry.second.begin(), entry.second.end());
        sort(sorted_file_ids.begin(), sorted_file_ids.end());
        word_file_list.push_back({entry.first, sorted_file_ids});
    }

    // Sort the words as per the requirements
    std::sort(word_file_list.begin(), word_file_list.end(),
        [](const pair<string, vector<int>>& a, const pair<string, vector<int>>& b) {
            if (a.second.size() == b.second.size()) {
                return a.first < b.first;
            }
            return a.second.size() > b.second.size();
        });

    pthread_barrier_t* reducer_threads_barrier = reducer_data->reducer_threads_barrier;
    pthread_barrier_wait(reducer_threads_barrier);

    // Write to output files
    for (char c : assigned_letters) {
        string file_name = string(1, c) + ".txt";
        ofstream file(file_name);
        if (!file.is_open()) {
            cerr << "Could not open file: " << file_name << endl;
            continue;
        }

        for (const auto& entry : word_file_list) {
            if (entry.first[0] == c) {
                file << entry.first << ":[";
                for (size_t i = 0; i < entry.second.size(); ++i) {
                    file << entry.second[i];
                    if (i + 1 < entry.second.size()) {
                        file << " ";
                    }
                }
                file << "]\n";
            }
        }

        file.close();
    }

    return NULL;
}


int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);
    assert(argc == 4 && "Usage: ./tema1 <numar_mapperi> <numar_reduceri> <fisier_intrare>");
    int num_mappers = atoi(argv[1]), num_reducers = atoi(argv[2]);
    string input_file = argv[3];

    // Declare arrays of pthread_t to hold thread identifiers
    pthread_t mappers[num_mappers];
    pthread_t reducers[num_reducers];

    safe_queue<pair<string, int>> q;
    get_file_names(input_file, q);

    pthread_barrier_t barrier;
    pthread_barrier_t reducer_threads_barrier;
    pthread_barrier_init(&barrier, NULL, num_mappers + num_reducers);
    pthread_barrier_init(&reducer_threads_barrier, NULL, num_reducers);

    // Initialize vectors
    vector<mapper_t> mappers_data;
    mappers_data.reserve(num_mappers);
    vector<reducer_t> reducers_data;
    reducers_data.reserve(num_reducers);

    // Create mappers and reducers
    for (int i = 0; i < num_mappers + num_reducers; ++i) {
        if (i < num_mappers) {
            mappers_data.push_back({&q, unordered_map<string, unordered_set<int>>(), &barrier});
            pthread_create(&mappers[i], NULL, &map_func, &mappers_data[i]);
        } else {
            reducers_data.push_back({i - num_mappers, num_reducers, &mappers_data, &barrier, &reducer_threads_barrier});
            pthread_create(&reducers[i - num_mappers], NULL, &reduce_func, &reducers_data[i - num_mappers]);
        }
    }

    // Join mapper threads
    for (int i = 0; i < num_mappers; ++i) {
        pthread_join(mappers[i], NULL);
    }

    // Join reducer threads
    for (int i = 0; i < num_reducers; ++i) {
        pthread_join(reducers[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    pthread_barrier_destroy(&reducer_threads_barrier);

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
