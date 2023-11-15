#ifndef __STRVIEW_H__
#define __STRVIEW_H__

#include <sys/types.h>
#include "helpers.h"
//[start, end)
typedef struct {
    const char* start;
    const char* end;
} StringView;

#define INVALID_STRVIEW ((StringView){(void*)0, (void*)0}) 

int is_empty_view(StringView s);

int compare_vv(StringView a, StringView b);

int compare_vs(StringView a, const char* b);

char* fill_str(StringView a, char* buf, size_t max_len);

StringView create_view(const char* start, const char* end);

ssize_t parse_to_int(StringView number);

StringView trim_whitespace(StringView a);

#endif //__STRVIEW_H__

