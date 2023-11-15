#ifndef __HELPERS_H__
#define __HELPERS_H__
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

//returns if c == ' '
bool is_whitespace(char c);

// is c ascii alphabetical
bool is_alpha(char c);

// is c ascii numeric
bool is_numeric(char c);

//const char* find_delim(const char* start, const char* end, const char* delim, int delim_len);

long my_utoa(uint64_t i, char* buf);

size_t buf_left(const char* start, const char* end, size_t len);

//returns a pointer to right after the end of the first occurance of delim, or NULL if not found
const char* find_delim(const char* buf_start, const char* buf_end, const char* delim);

#endif
