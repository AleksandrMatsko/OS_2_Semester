#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define NUM_LINES 10
#define MUTEX_NUM 3

#define ERROR_MUTEXATTR_INIT 1
#define ERROR_MUTEX_INIT 2

pthread_mutex_t mutex[MUTEX_NUM];
int mutex_owner[MUTEX_NUM];

void acquireMutex(int index, int threadNum) {
    int err = pthread_mutex_lock(&mutex[index]);
    if (err != 0) {
        fprintf(stderr, "Error mutex lock %s\n", strerror(err));
    }
    else {
        mutex_owner[index] = threadNum;
    }
}

void releaseMutex(int index) {
    int prev_owner = mutex_owner[index];
    mutex_owner[index] = -1;
    int err = pthread_mutex_unlock(&mutex[index]);
    if (err != 0) {
        fprintf(stderr, "Error mutex unlock %s\n", strerror(err));
        mutex_owner[index] = prev_owner;
    }
}

void childThread(void *arg) {
    int *int_arg = (int *)arg;
    int threadNum = *int_arg;
    acquireMutex(threadNum + 1, threadNum);
    for (int i = threadNum + 2; i < NUM_LINES + threadNum + 2; i++) {
        acquireMutex(i % MUTEX_NUM, threadNum);
        printf("Child printing line %d\n", i - (threadNum + 1));
        releaseMutex((i + MUTEX_NUM - 1) % MUTEX_NUM);
    }
    for (int i = 0; i < MUTEX_NUM; i++) {
        if (mutex_owner[i] == threadNum) {
            releaseMutex(i);
        }
    }
    pthread_exit((void *)EXIT_SUCCESS);
}

int main() {
    pthread_mutexattr_t mutexattr;
    int err = pthread_mutexattr_init(&mutexattr);
    if (err != 0) {
        fprintf(stderr, "Error mutex attr init %s\n", strerror(err));
        exit(ERROR_MUTEXATTR_INIT);
    }
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);

    int parentThreadNum = 0;
    for (int i = 0; i < MUTEX_NUM; i++) {
        mutex_owner[i] = -1;
        err = pthread_mutex_init(&mutex[i], &mutexattr);
        if (err != 0) {
            fprintf(stderr, "Error mutex init %s\n", strerror(err));
            exit(ERROR_MUTEX_INIT);
        }
    }

    pthread_mutexattr_destroy(&mutexattr);
    acquireMutex(0, parentThreadNum);
    pthread_t tid;
    bool was_created;
    int childThreadNum = 1;
    err = pthread_create(&tid, NULL, (void *)childThread, &childThreadNum);
    if (err != 0) {
        fprintf(stderr, "Error pthread create %s\n", strerror(err));
        was_created = false;
    }
    else {
        was_created = true;
    }
    usleep(10);
    for (int i = 1; i < NUM_LINES + 1; i++) {
        acquireMutex(i % MUTEX_NUM, parentThreadNum);
        printf("Parent printing line %d\n", i);
        releaseMutex((i + MUTEX_NUM - 1) % MUTEX_NUM);
    }
    for (int i = 0; i < MUTEX_NUM; i++) {
        if (mutex_owner[i] == parentThreadNum) {
            releaseMutex(i);
        }
    }

    if (was_created) {
        err = pthread_join(tid, NULL);
        if (err != 0) {
            fprintf(stderr, "Error pthread join %s\n", strerror(err));
        }
    }

    for (int i = 0; i < MUTEX_NUM; i++) {
        err = pthread_mutex_destroy(&mutex[i]);
        if (err != 0) {
            fprintf(stderr, "Error mutex destroy %s\n", strerror(err));
        }
    }
    pthread_exit((void *)EXIT_SUCCESS);
}
