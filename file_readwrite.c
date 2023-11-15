#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "file_readwrite.h"

struct FileRWEx {
    int max_open;
    pthread_mutex_t mutex;
    char** open_arr;
    char** renamed_arr;
};

FileRWEx create_file_rw_ex(int max_open, int max_path) {
    FileRWEx res = malloc(sizeof(struct FileRWEx));
    res->max_open = max_open;
    res->mutex = PTHREAD_MUTEX_INITIALIZER;
    char* char_buf = malloc(sizeof(char) * (max_path + 1) * max_open);
    res->open_arr = malloc(sizeof(char*) * max_open);
    for(int i = 0; i < max_open; ++i){
        res->open_arr[i] = char_buf + (i * (max_path + 1));
        res->open_arr[i][0] = '\0';
    }

    char* char_buf2 = malloc(sizeof(char) * (max_path + 11) * max_open);
    res->renamed_arr = malloc(sizeof(char*) * max_open);
    for(int i = 0; i < max_open; ++i){
        res->renamed_arr[i] = char_buf2 + (i * (max_path + 11));
        res->renamed_arr[i][0] = '\0';
    }

}

void destroy_file_rw_ex(FileRWEx* pp) {
    FileRWEx p = *pp;
    free(p->reader_counts);
    free(p->open_arr[0] );
    free(p->open_arr);
    pthread_mutex_destroy(p->mutex);
    for(int i = 0; i < p->max_open; ++i) {
        pthread_mutex_destroy(p->locks + i);
    }
    free(p->locks);
    free(p);
    *pp = INVALID_FRWEX;
}

void copy_to(StringView path, char* buf) {
    while(path.start != path.end) {
        *(buf++) = *(path.start++);
    }
    *buf = '\0';
}

void copy_to_write(StringView path, char* buf, int thread_id) {
    while(path.start != path.end) {
        *(buf++) = *(path.start++);
    }
    *(buf++) = '$';
    while(thread_id != 0) {
        *(buf++) = (thread_id % 10) + '0';
        thread_id /= 10;
    }
    *buf = '\0';
}


bool exists(char* buf) {
    return *buf != '\0';
}

char* get_real_name(FileRWEx p, StringView path) {
    int first_empty = -1; 
    for(int i = 0; i < p->max_open; ++i) {
        if(p->open_arr[i][0] == '\0') {
            first_empty = i;
            continue;
        } 
        if (compare_vs(path, p->open_arr[i]) == 0) {
            if( exists(p->renamed_arr[i]) ){
                return p->renamed_arr[i];
            }
            return p->open_arr[i];
        }
    }
    if(first_empty == -1) {
        return NULL;
    }
    copy_to(path, p->open_arr[first_empty]);
    return p->renamed_arr[first_empty];
}
// $
int read_open(FileRWEx p, StringView path) {
    char cpath[PATH_MAX];
    copy_to(path, cpath);
    return open(cpath, O_RDONLY);
}

int write_open(FileRWEx p, StringView path, int thread_id, bool* created) {
    char actual_path[PATH_MAX];
    copy_to_write(path, actual_path, thread_id);
    pthread_mutex_lock(&p->mutex);
    int code = open(actual_path, O_WRONLY);
    if(code >= 0 || errno != ENOENT) {
        return code;
    }
    code = open(actual_path, O_WRONLY | O_CREAT, 0666);
    pthread_mutex_unlock(&p->mutex);
    *created = code > 0;
    return code;
}

void read_close(FileRWEx p, int fd) {
    close(fd);
}

void write_close(FileRWEx p, StringView path, int fd, int thread_id) {
    char actual_path[PATH_MAX];
    copy_to_write(path, actual_path, thread_id);

}
