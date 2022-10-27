#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ERROR_ALLOC 1
#define ERROR_READ 2

#define BUF_SIZE BUFSIZ

int start_fd;

/*
 *
 * how list_t looks
 *
 * nodes                                         end
 *   |                                            |
 *   V                                            V
 * +------+------+    +------+------+           +------+------+
 * | data | next | -> | data | next | -> ... -> | data | next | -> NULL
 * +------+------+    +------+------+           +------+------+
 *
 *
 */

typedef struct node_t {
    char *string;
    struct node_t *next;
    size_t s_len;
} node_t;

typedef struct list_t {
    struct node_t *nodes;
    struct node_t *end;
    pthread_mutex_t list_mutex;
    size_t size;
} list_t;

list_t *final_list;

list_t *initList() {
    list_t *list = (list_t *) calloc(1, sizeof(list_t));
    if (list == NULL) {
        perror("Error: calloc returned NULL");
        pthread_exit((void *) ERROR_ALLOC);
    }
    list->end = (node_t *)calloc(1, sizeof(node_t));
    if (list->end == NULL) {
        perror("Error: calloc returned NULL");
        pthread_exit((void *) ERROR_ALLOC);
    }
    list->size = 0;
    pthread_mutex_init(&list->list_mutex, NULL);
    list->nodes = NULL;
    return list;
}

node_t *createNode(char *s, size_t s_len, pthread_mutex_t *mutex) {
    node_t *new_node = (node_t *)calloc(1, sizeof(node_t));
    if (new_node == NULL) {
        perror("Error: calloc returned NULL");
        pthread_mutex_unlock(mutex);
        pthread_exit((void *) ERROR_ALLOC);
    }
    new_node->string = (char *)calloc(s_len, sizeof(char));
    if (new_node->string == NULL && s_len != 0) {
        perror("Error: calloc returned NULL");
        pthread_mutex_unlock(mutex);
        pthread_exit((void *)ERROR_ALLOC);
    }
    memcpy(new_node->string, s, s_len);
    new_node->next = NULL;
    new_node->s_len = s_len;
    return new_node;
}

void addNodeToList(list_t *list, node_t *node) {
    if (list == NULL || node == NULL) {
        return;
    }
    pthread_mutex_lock(&list->list_mutex);
    if (list->nodes == NULL) {
        list->nodes = node;
        list->nodes->next = NULL;
        list->end = list->nodes;
    }
    else {
        list->end->next = node;
        list->end->next->next = NULL;
        list->end = list->end->next;
    }
    list->size += 1;
    pthread_mutex_unlock(&list->list_mutex);
}

void addStringToList(list_t *list, char *s, size_t s_len) {
    if (list == NULL) {
        return;
    }
    pthread_mutex_lock(&list->list_mutex);
    node_t **list_nodes = &list->nodes;
    if (*list_nodes == NULL) {
        *list_nodes = createNode(s, s_len, &list->list_mutex);
        list->end = *list_nodes;
    }
    else {
        list->end->next = createNode(s, s_len, &list->list_mutex);
        list->end = list->end->next;
    }
    list->size += 1;
    pthread_mutex_unlock(&list->list_mutex);
}

void destroyList(list_t *list) {
    if (list == NULL) {
        return;
    }
    pthread_mutex_destroy(&list->list_mutex);
    while (list->nodes != NULL) {
        node_t *tmp = list->nodes->next;
        free(list->nodes->string);
        free(list->nodes);
        list->nodes = tmp;
    }
    free(list);
}

void printList(list_t *list) {
    pthread_mutex_lock(&list->list_mutex);
    node_t *list_nodes = list->nodes;
    while (list_nodes != NULL) {
        pthread_mutex_unlock(&list->list_mutex);
        write(1, list_nodes->string, list_nodes->s_len);
        pthread_mutex_lock(&list->list_mutex);
        list_nodes = list_nodes->next;
    }
    pthread_mutex_unlock(&list->list_mutex);
}

void *mythread(void *arg) {
    node_t *node = (node_t *)arg;
    char buf;
    ssize_t was_read = read(start_fd, &buf, 1);
    if (was_read < 0) {
        perror("Error read for synchronization");
    }
    usleep(node->s_len * 30000);
    addNodeToList(final_list, node);
    pthread_exit(NULL);
}

int main() {
    size_t max_buf_size = BUF_SIZE;
    char buf[BUF_SIZE];
    char *string = (char *) calloc(max_buf_size, sizeof(char));
    size_t index = 0;
    list_t *list = initList();

    while (1) {
        char *res = fgets(buf, BUF_SIZE, stdin);
        if (feof(stdin)) {
            break;
        }
        if (res == NULL) {
            perror("Error while reading data");
            exit(ERROR_READ);
        }
        size_t s_len = strlen(buf);
        if (s_len + index >= max_buf_size - 1) {
            max_buf_size *= 2;
            string = realloc(string, max_buf_size * sizeof(char));
            if (string == NULL) {
                perror("Error: realloc returned NULL");
                exit(ERROR_ALLOC);
            }
        }
        memcpy(&string[index], buf, BUF_SIZE * sizeof(char));
        index += s_len;
        if (strchr(buf, '\n') != NULL) {
            addStringToList(list, string, strlen(string) + 1);
            index = 0;
            memset(string, 0, max_buf_size * sizeof(char));
        }
    }
    free(string);

    int pipe_fds[2];
    int pipe_res = pipe(pipe_fds);
    if (pipe_res != 0) {
        perror("Error pipe():");
    }
    start_fd = pipe_fds[0];
    int write_fd = pipe_fds[1];

    size_t num_threads_to_create = list->size;
    pthread_t tids[num_threads_to_create];
    bool was_created[num_threads_to_create];

    final_list = initList();
    node_t *tmp_nodes = list->nodes; //nodes will be used by threads to create final_list
    list->nodes = NULL; // list will consist of the nodes prepared for threads which wouldn't be created
    list->end = NULL;
    list->size = 0;

    size_t i = 0;
    int really_created = 0;
    while (tmp_nodes != NULL) {
        int create_res = pthread_create(&tids[i], NULL, mythread, tmp_nodes);
        if (create_res != 0) {
            fprintf(stderr, "Error while creating thread %zu: %s\n", i, strerror(create_res));
            was_created[i] = false;
            addNodeToList(list, tmp_nodes);
        }
        else {
            really_created += 1;
            was_created[i] = true;
        }
        i += 1;
        tmp_nodes = tmp_nodes->next;
    }
    fprintf(stderr, "THREADS CREATED\n");
    printf("\n==========LINES==========\n\n");

    char start_buf[BUFSIZ];
    ssize_t bytes_written = 0;
    while (bytes_written < really_created) {
        ssize_t written = 0;
        if (really_created - bytes_written <= BUFSIZ) {
            written = write(write_fd, start_buf, really_created - bytes_written);
        }
        else {
            written = write(write_fd, start_buf, BUFSIZ);
        }
        if (written < 0) {
            perror("Error write");
            fprintf(stderr, "bytes_written: %ld / %d\n", bytes_written, really_created);
        }
        else {
            bytes_written += written;
        }
    }
    destroyList(list);

    for (size_t j = 0; j < num_threads_to_create; j++) {
        if (was_created[j]) {
            int join_res = pthread_join(tids[j], NULL);
            if (join_res != 0) {
                fprintf(stderr, "Error while joining thread %zu: %s\n", j, strerror(join_res));
            }
        }
    }

    printList(final_list);

    close(start_fd);
    close(write_fd);
    destroyList(final_list);
    return 0;
}
