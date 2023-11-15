#include "helpers.h"
bool is_whitespace(char c) {
    return c == ' ';
}

bool is_alpha(char c) {
    return ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') );
}

bool is_numeric(char c) {
    return (c >= '0' && c <= '9');
}


long my_utoa(uint64_t i, char* buf) {
    char* start = buf;
    do {
        *(buf++) = (i % 10) + '0';
        i /= 10;
    } while(i);
    long len = buf - start;
    long half = len / 2;
    for(int j = 0; j < half; ++j) {
        char c = start[j];
        start[j] = start[len - j - 1];
        start[len - j - 1] = c;
    }
    return len;
}


size_t buf_left(const char* start, const char* end, size_t len) {
    return len - (end - start);
}

const char* find_delim(const char* buf_start, const char* buf_end, const char* delim) {
    const char* delim_ptr = delim;
    while(buf_start != buf_end) {
        if(*(buf_start++) == *(delim_ptr)){
            delim_ptr += 1;
            if (*delim_ptr == '\0') {
                return buf_start;
            }
        }
        else {
            delim_ptr = delim;
        }
    }
    return NULL;
}
