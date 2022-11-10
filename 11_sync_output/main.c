#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_LINES 10
#define MUTEX_NUM 3

#define ERROR_MUTEX_INIT 1

pthread_mutex_t mutex[MUTEX_NUM];

void acquireMutex(int index) {
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
        acquireMutex(i % MUTEX_NUM);
        printf("Child printing line %d\n", i - (threadNum + 1));
        pthread_mutex_unlock(&mutex[(i + MUTEX_NUM - 1) % MUTEX_NUM]);
    }
    pthread_exit((void *)EXIT_SUCCESS);
}

int main() {
    for (int i = 0; i < MUTEX_NUM; i++) {
        int err = pthread_mutex_init(&mutex[i], NULL);
        if (err != 0) {
            fprintf(stderr, "Error mutex init %s\n", strerror(err));
            exit(ERROR_MUTEX_INIT);
        }
    }
    pthread_mutex_lock(&mutex[0]);
    pthread_mutex_lock(&mutex[1]);
    pthread_t tid;
    bool was_created;
    int threadNum = 1;
    int err = pthread_create(&tid, NULL, (void *)childThread, &threadNum);
    if (err != 0) {
        fprintf(stderr, "Error pthread create %s\n", strerror(err));
        was_created = false;
    }
    else {
        was_created = true;
    }
    for (int i = 2; i < NUM_LINES + 2; i++) {
        printf("Parent printing line %d\n", i - 1);
        pthread_mutex_unlock(&mutex[(i + MUTEX_NUM - 2) % MUTEX_NUM]);
        acquireMutex(i % MUTEX_NUM);
    }

    if (was_created) {
        err = pthread_join(tid, NULL);
        if (err != 0) {
            fprintf(stderr, "Error pthread join %s\n", strerror(err));
        }
    }

    for (int i = 0; i < MUTEX_NUM; i++) {
        pthread_mutex_destroy(&mutex[i]);
    }
    pthread_exit((void *)EXIT_SUCCESS);
}
