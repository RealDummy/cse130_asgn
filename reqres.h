#ifndef __REQRES_H__
#define __REQRES_H__

#include <stdint.h>

#include "strview.h"


typedef enum Method {
    GET = 0,
    PUT = 1,
    HEAD = 2,
    NONE_YET = 3,
    UNKNOWN = 4,
} Method;

typedef struct {
    char* buf;
    char* buf_end;
    const char* parsed_end;
    size_t buf_len;
    size_t body_len;
    StringView id;
    StringView uri;
    StringView method_view;
    Method method;

    int put_fd;
    int get_fd;
    //union {
    //    int put_fd;
    //    int get_fd;
    //};
} ReqState;

typedef struct {
    char* buf;
    char* buf_end;
    const char* flushed;
    size_t buf_len;
} ResState;

#endif //REQRES
