#include "strview.h"

int is_empty_view(StringView s) {
    return (s.start == (void*)0 || s.end == (void*)0 || s.start >= s.end);
}

int compare_vv(StringView a, StringView b) {
    char ac,bc;
    while(a.start != a.end && b.start != b.end) {
        if ((ac = *(a.start++)) == (bc = *(b.start++))) {
            continue;
        }
        return (ac > bc) ? 1 : -1;
    }
    return a.start == a.end ? (b.start == b.end ? 0 : -1) : 1;
}

int compare_vs(StringView a, const char* b) {
    char ac, bc;
    while(a.start != a.end && *b != '\0') {
        if ((ac = *(a.start++)) == (bc = *(b++))) {
            continue;
        }
        return (ac > bc) ? 1 : -1;
    }
    return a.start == a.end ? (*b == '\0' ? 0 : -1) : 1;
}

StringView create_view(const char* start, const char* end) {
    return (StringView) {
        start,
        end
    };
}

StringView trim_whitespace(StringView a) {
    if(is_empty_view(a)) {
        return a;
    }
    while(*(a.start) == ' ') {
        ++a.start;
    }
    while(*(a.end - 1) == ' ') {
        --a.end;
    }
    if (a.end < a.start) {
        a.end = a.start;
    }
    return a;
}

ssize_t parse_to_int(StringView number) {
    number = trim_whitespace(number);
    if(is_empty_view(number)) {
        return -1;
    }
    ssize_t pow = 1;
    ssize_t res = 0;
    const char* end = number.end - 1;
    while (end >= number.start) {
        if (!is_numeric(*end)) {
            return -1;
        }
        ssize_t val = *end - '0';
        res += pow * val;
        pow *= 10;
        --end;
    }
    return res;
}

char* fill_str(StringView a, char* buf, size_t max_len) {
    char* res = buf;
    if((a.end - a.start) >= (ssize_t)max_len) {
        return (void*)0;
    }
    while (a.start < a.end) {
        *(buf++) = *(a.start++);
    }
    *buf = '\0';
    return res;
}
