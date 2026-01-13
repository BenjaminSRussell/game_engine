#ifndef CORE_THREADING_H
#define CORE_THREADING_H

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <core/types.h>

// Simple Thread Wrapper
typedef pthread_t Thread;
typedef pthread_mutex_t Mutex;

// Function pointer for thread entry
typedef void* (*ThreadFunction)(void*);

static inline Thread thread_create(ThreadFunction func, void* arg) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, func, arg) != 0) {
        return (Thread)NULL;
    }
    return thread;
}

static inline void thread_wait(Thread thread) {
    if (thread) {
        pthread_join(thread, NULL);
    }
}

static inline void thread_destroy(Thread thread) {
    // pthread_join acts as destroy/cleanup for joinable threads
    // If detached, no action needed. Assuming joinable for now based on usage.
    (void)thread;
}

static inline void thread_sleep(u32 ms) {
    usleep(ms * 1000);
}

static inline Mutex* mutex_create(void) {
    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (mutex) {
        pthread_mutex_init(mutex, NULL);
    }
    return (Mutex*)mutex;
}

static inline void mutex_lock(Mutex* mutex) {
    if (mutex) {
        pthread_mutex_lock((pthread_mutex_t*)mutex);
    }
}

static inline void mutex_unlock(Mutex* mutex) {
    if (mutex) {
        pthread_mutex_unlock((pthread_mutex_t*)mutex);
    }
}

static inline void mutex_destroy(Mutex* mutex) {
    if (mutex) {
        pthread_mutex_destroy((pthread_mutex_t*)mutex);
        free(mutex);
    }
}

#endif // CORE_THREADING_H
