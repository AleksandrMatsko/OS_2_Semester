#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#define NUM_LINES 10

#define ERROR_MUTEX_INIT 1

int num_threads = 2;
pthread_mutex_t *mutex;
int mutex_num;

void destroyMutex() {
    for (int i = 0; i < mutex_num; i++) {
        pthread_mutex_destroy(&mutex[i]);
    }
    free(mutex);
}

void try_lock(int index) {
    int try_lock_res = 1;
    while (try_lock_res != 0) {
        try_lock_res = pthread_mutex_trylock(&mutex[index]);
    }
}

void childThread(void *arg) {
    int *int_arg = (int *)arg;
    int threadNum = *int_arg;
    pthread_mutex_lock(&mutex[threadNum + 1]);
    for (int i = threadNum + 2; i < NUM_LINES + threadNum + 2; i++) {
        try_lock(i % mutex_num);
        printf("Child %d: printing line %d\n", threadNum, i - (threadNum + 1));
        pthread_mutex_unlock(&mutex[(i + mutex_num - 1) % mutex_num]);
    }
}

int main() {
    mutex_num = num_threads + 1;
    mutex = (pthread_mutex_t *)calloc(mutex_num, sizeof(pthread_mutex_t));
    for (int i = 0; i < mutex_num; i++) {
        int err = pthread_mutex_init(&mutex[i], NULL);
        if (err != 0) {
            fprintf(stderr, "Error mutex init %s\n", strerror(err));
            exit(ERROR_MUTEX_INIT);
        }
    }
    pthread_mutex_lock(&mutex[0]);
    pthread_mutex_lock(&mutex[1]);
    pthread_t tids[num_threads - 1];
    for (int i = 0; i < num_threads - 1; i++) {
        int threadNum = i + 1;
        int err = pthread_create(&tids[i], NULL, (void *)childThread, &threadNum);
        if (err != 0) {
            fprintf(stderr, "Error pthread create %s\n", strerror(err));
        }
    }

    for (int i = 2; i < NUM_LINES + 2; i++) {
        printf("Parent printing line %d\n", i - 1);
        pthread_mutex_unlock(&mutex[(i + mutex_num - 2) % mutex_num]);
        try_lock(i % mutex_num);
    }
    destroyMutex();
    return 0;
}
