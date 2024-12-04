#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <pthread.h>
#include <unordered_set>
#include <sstream>

#include "safe_queue.h"
#include "mapper.h"
#include "reducer.h"

using namespace std;

// get all the file names and IDs and push them to the queue
void push_filenames_and_ids(const string& input_file, safe_queue<pair<filename, file_id>>& q);


void *map_func(void *arg) {
    mapper_t *mapper_data = (mapper_t *)arg;
    safe_queue<pair<filename, file_id>>* q = mapper_data->q;
    unordered_map<string, unordered_set<int>>& word_to_file_ids = mapper_data->word_to_file_ids;

    // shared barrier with reducers
    pthread_barrier_t* barrier = mapper_data->barrier;

    pair<string, int> file_data;
    // each mapper thread takes a file from queu after finishing current file
    while (q->try_pop(file_data)) {
        string file_name = file_data.first;
        int file_id = file_data.second;

        ifstream in(file_name);
        assert(in.is_open() && "Could not open file");

        string line;
        while (getline(in, line)) {
            string raw_word;
            stringstream ss(line);
            // Go through each word in line
            while (ss >> raw_word) {
                string word = "";
                for (auto ch : raw_word) {
                    if (isalpha(ch)) word += tolower(ch);
                }

                if (!word.empty()) {
                    word_to_file_ids[word].insert(file_id);
                }
            }
        }

        in.close();
    }

    pthread_barrier_wait(barrier);
    return NULL;
}


void* reduce_func(void* arg) {
    reducer_t* reducer_data = (reducer_t*)arg;
    // shared barrier with mappers
    pthread_barrier_t* barrier = reducer_data->barrier;
    pthread_barrier_wait(barrier);

    int reducer_id = reducer_data->reducer_id;
    int num_reducers = reducer_data->num_reducers;
    vector<mapper_t>& mappers_data = *(reducer_data->mappers_data);

    // each reducer will have a set of letters assigned to work on
    unordered_set<char> assigned_letters;
    for (char c = 'a' + reducer_id; c <= 'z'; c += num_reducers) {
        assigned_letters.insert(c);
    }

    unordered_map<string, unordered_set<int>> word_file_map_merged;

    for (const auto& mapper_data : mappers_data) {
        const unordered_map<string, unordered_set<int>>& word_map = mapper_data.word_to_file_ids;
        for (const auto& file_ids : word_map) {
            const auto file_name = file_ids.first;
            const auto file_id_set = file_ids.second;

            if (assigned_letters.count(file_name.at(0))) {
                word_file_map_merged[file_name].insert(file_id_set.begin(), file_id_set.end());
            }
        }
    }

    vector<pair<string, vector<int>>> word_file_list;
    word_file_list.reserve(word_file_map_merged.size());

    // adding to the list the words and their sorted file ids
    for (const auto& file_ids : word_file_map_merged) {
        const auto file_name = file_ids.first;
        const auto file_id_set = file_ids.second;

        if (assigned_letters.count(file_name.at(0)) == 0) continue;
        
        vector<int> sorted_file_ids(file_id_set.begin(), file_id_set.end());
        std::sort(sorted_file_ids.begin(), sorted_file_ids.end());
        word_file_list.push_back({file_name, sorted_file_ids});
    }

    std::sort(word_file_list.begin(), word_file_list.end(), [](const auto& w1, const auto& w2) {
            return (w1.second.size() == w2.second.size())
                ? w1.first < w2.first
                : w1.second.size() > w2.second.size();
        });

    
    // write results to assigned letter files
    for (const char letter : assigned_letters) {
        ofstream out(string(1, letter) + ".txt");
        assert(out.is_open() && "Could not open file");

        for (const auto& word_file : word_file_list) {
            if (word_file.first.front() != letter) continue;
            out << word_file.first << ":[";
            for (size_t i = 0; i < word_file.second.size(); ++i) {
                out << word_file.second.at(i);
                (i + 1 < word_file.second.size()) ? out << " " : out << "";
            }
            out << "]\n";
        }

        out.close();
    }

    return NULL;
}


int main(int argc, char **argv)
{
    // speed up I/O operations with cin/cout
    std::ios::sync_with_stdio(false);
    std::cout.tie(nullptr);

    assert(argc == 4 && "Usage: ./tema1 <numar_mapperi> <numar_reduceri> <fisier_intrare>");
    int num_mappers = atoi(argv[1]), num_reducers = atoi(argv[2]);
    string input_file = argv[3];
    pthread_t mappers[num_mappers];
    pthread_t reducers[num_reducers];

    // {filename: string, file_id: int}
    safe_queue<pair<filename, file_id>> q;
    push_filenames_and_ids(input_file, q);

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, num_mappers + num_reducers);

    vector<mapper_t> mappers_data;
    mappers_data.reserve(num_mappers);
    vector<reducer_t> reducers_data;
    reducers_data.reserve(num_reducers);

    for (int i = 0; i < num_mappers + num_reducers; ++i) {
        if (i < num_mappers) {
            mappers_data.push_back({&q, unordered_map<string, unordered_set<int>>(), &barrier});
            pthread_create(&mappers[i], NULL, &map_func, &mappers_data[i]);
        } else {
            reducers_data.push_back({i - num_mappers, num_reducers, &mappers_data, &barrier});
            pthread_create(&reducers[i - num_mappers], NULL, &reduce_func, &reducers_data[i - num_mappers]);
        }
    }

    for (int i = 0; i < num_mappers + num_reducers; ++i) {
        if (i < num_mappers) pthread_join(mappers[i], NULL);
        else pthread_join(reducers[i - num_mappers], NULL);
    }

    pthread_barrier_destroy(&barrier);
    return 0;
}


void push_filenames_and_ids(const string& input_file, safe_queue<pair<string, int>>& q) {
    ifstream file(input_file);
    assert(file.is_open() && "Could not open file");

    int num_files;
    file >> num_files;
    file.ignore();

    string line;
    const string dir = "../checker/";
    int file_id = 1;

    // no synchronization needed, because this is done before the threads are created
    while (getline(file, line)) {
        q.unsync_push({dir + line, file_id++});
    }

    file.close();
}
