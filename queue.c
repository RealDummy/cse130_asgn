#include <stdlib.h> //malloc
#include <pthread.h>
#include <errno.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include "queue.h"

//volitile?
struct queue {
    int head; //pop from head
    int tail; //push to tail
    atomic_int len;
    pthread_mutex_t head_mutex;
    pthread_mutex_t tail_mutex;
    pthread_cond_t wait;
    int capacity;
    void** buf;
};


queue_t *queue_new(int size) {
    queue_t* ret = (queue_t*)malloc(sizeof(queue_t));
    if (!ret) {
        return NULL;
    }
    ret->buf = (void**)malloc(sizeof(void*) * size);
    if(!ret->buf) {
        free(ret);
        return NULL;
    }

    ret->head = 0;
    ret->tail = 0;

    ret->capacity = size;
    ret->len = 0;

    pthread_mutex_init(&ret->head_mutex, NULL);
    pthread_mutex_init(&ret->tail_mutex, NULL);

    pthread_cond_init(&ret->wait, NULL);
    return ret;
}
void queue_delete(queue_t **q) {
    if(!q || !*q) return;

    pthread_mutex_destroy(&(*q)->tail_mutex);
    pthread_mutex_destroy(&(*q)->head_mutex);

    pthread_cond_destroy(&(*q)->wait);

    free((*q)->buf);
    free(*q);
    *q = NULL;
}

size_t wrap(int a, int n) {
    return a % n;
}

// bool queue_push(queue_t *q, void *elem) {
    // if (!q) return false;
//    pthread_mutex_lock(&q->head_mutex);
//    while (q->len == q->capacity) {
    //    pthread_cond_wait(&q->wait, &q->head_mutex);
//    }
//    int spot = q->head;
//    q->head = wrap(spot + 1, q->capacity);
//    q->buf[spot] = elem;
//    atomic_fetch_add(&q->len, 1);
//    pthread_mutex_unlock(&q->head_mutex);
//    pthread_cond_signal(&q->wait);
//    return true;
// }
bool queue_push(queue_t *q, void *elem) {
    if(!q) return false;
    pthread_mutex_lock(&q->head_mutex);
    while (q->len == q->capacity) {
        pthread_cond_wait(&q->wait, &q->head_mutex);
    }
    int spot = q->head;
    q->head = wrap(spot + 1, q->capacity);
    q->buf[spot] = elem;
    q->len += 1;
    pthread_cond_signal(&q->wait);
    pthread_mutex_unlock(&q->head_mutex);
    return true;
}



bool queue_pop(queue_t *q, void **elem) {
    if(!q || !elem) return false;
    pthread_mutex_lock(&q->head_mutex);
    while (q->len == 0) {
        pthread_cond_wait(&q->wait, &q->head_mutex);
    }
    int spot = q->tail;
    q->tail = wrap(spot + 1, q->capacity);
    *elem = q->buf[spot];
    q->len -= 1;
    pthread_cond_signal(&q->wait);
    pthread_mutex_unlock(&q->head_mutex);
    return true;
}


// bool queue_pop(queue_t *q, void **elem) {
//    if(!q || !elem) return false;
//    pthread_mutex_lock(&q->tail_mutex);
//    while (q->len == 0) {
    //    pthread_cond_wait(&q->wait, &q->tail_mutex);
//    }
//    int spot = q->tail;
//    q->tail = wrap(spot + 1, q->capacity);
//    *elem = q->buf[spot];
//    atomic_fetch_sub(&q->len, 1);
//    pthread_mutex_unlock(&q->tail_mutex);
//    pthread_cond_signal(&q->wait);
//    return true;
// }
