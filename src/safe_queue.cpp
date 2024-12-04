#include "safe_queue.h"

template <typename T>
safe_queue<T>::safe_queue() {
	pthread_mutex_init(&mutex, NULL);
}

template <typename T>
safe_queue<T>::~safe_queue() {
	pthread_mutex_destroy(&mutex);
}


template <typename T>
void safe_queue<T>::unsync_push(T elem)
{
	q.push(elem);
}


template <typename T>
bool safe_queue<T>::empty() {
    pthread_mutex_lock(&mutex);
    bool is_empty = q.empty();
    pthread_mutex_unlock(&mutex);
    return is_empty;
}

template <typename T>
bool safe_queue<T>::try_pop(T& value) {
    pthread_mutex_lock(&mutex);
    if (q.empty()) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    value = q.front();
    q.pop();
    pthread_mutex_unlock(&mutex);
    return true;
}

template class safe_queue<std::pair<std::string, int>>;
