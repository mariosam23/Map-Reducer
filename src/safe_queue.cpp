#include "safe_queue.h"

template <typename T>
safe_queue<T>::safe_queue() {
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}

template <typename T>
safe_queue<T>::~safe_queue() {
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

template <typename T>
void safe_queue<T>::push(T elem)
{
	pthread_mutex_lock(&mutex);
	q.push(elem);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

template <typename T>
void safe_queue<T>::unsync_push(T elem)
{
	q.push(elem);
}

template <typename T>
T safe_queue<T>::pop()
{
	pthread_mutex_lock(&mutex);
	while (q.empty()) {
		pthread_cond_wait(&cond, &mutex);
	}
	T elem = q.front();
	q.pop();
	pthread_mutex_unlock(&mutex);
	return elem;
}

template <typename T>
T safe_queue<T>::unsync_pop()
{
	T elem = q.front();
	q.pop();
	return elem;
}

template <typename T>
bool safe_queue<T>::empty() {
    pthread_mutex_lock(&mutex);
    bool is_empty = q.empty();
    pthread_mutex_unlock(&mutex);
    return is_empty;
}

template <typename T>
bool safe_queue<T>::unsync_empty() {
	return q.empty();
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
