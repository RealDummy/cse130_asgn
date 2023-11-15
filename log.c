#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include "log.h"



struct _Logger {
    char* buf;
    char* buf_head;
    int buffer_size;
    int logfd;
    pthread_mutex_t write_mutex;
    pthread_rwlock_t flush_lock;
};

Logger create_log(const char* log_file, int buffer_size) {
    int fd;
    if(log_file){
        fd = open(log_file, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR );
    } else {
        fd = STDOUT_FILENO;
    }
    if (fd < 0) {
        return INVALID_LOGGER;
    }
    Logger res = malloc(sizeof(struct _Logger));
    res->logfd = fd;
    res->buf = malloc(sizeof(char) * buffer_size);
    res->buf_head = res->buf;
    res->buffer_size = buffer_size;
    pthread_mutex_init(&res->write_mutex, NULL);
    pthread_rwlock_init(&res->flush_lock, NULL);
    return res;
}

void destroy_log(Logger* logger_ptr) {
    if(!logger_ptr || *logger_ptr == INVALID_LOGGER) return;
    flush_logger(*logger_ptr);
    free((*logger_ptr)->buf);
    close( (*logger_ptr)->logfd );
    free(*logger_ptr);
    *logger_ptr = INVALID_LOGGER;
}

void log_buf(Logger l, const char* string_ish, int str_len) {
    pthread_mutex_lock(&l->write_mutex);
    pthread_rwlock_rdlock(&l->flush_lock);
    if((l->buf_head - l->buf) + str_len < l->buffer_size) {
        l->buf_head += str_len;
        pthread_mutex_unlock(&l->write_mutex);
        for(int i = str_len; i > 0; --i) {
            *(l->buf_head - i) = *(string_ish++);
        }
        pthread_rwlock_unlock(&l->flush_lock);
        return;
\
    }
    int flushed_partial = false;
    const char* str_end = string_ish + str_len;
    const char* log_end = l->buf + l->buffer_size;
    while(string_ish != str_end) {
        while(l->buf_head != log_end && string_ish != str_end) {
            *(l->buf_head++) = *(string_ish++);
        }
        if(l->buf_head == log_end) {
            pthread_rwlock_unlock(&l->flush_lock);
            flush_logger(l);
            pthread_rwlock_rdlock(&l->flush_lock);
            flushed_partial = true;
        }
    }
    if(flushed_partial && l->buf_head != l->buf) {
        pthread_rwlock_unlock(&l->flush_lock);
        flush_logger(l);
    } else {
        pthread_rwlock_unlock(&l->flush_lock);
    }
    pthread_mutex_unlock(&l->write_mutex);
}

void flush_logger(Logger l) {
    pthread_rwlock_wrlock(&l->flush_lock);
    while(l->buf_head != l->buf) {
        int code = write(l->logfd, l->buf, l->buf_head - l->buf);
        if(code > 0) {
            l->buf_head -= code;
        } else {
            pthread_rwlock_unlock(&l->flush_lock);
            return;
        }
    }
    pthread_rwlock_unlock(&l->flush_lock);
}
