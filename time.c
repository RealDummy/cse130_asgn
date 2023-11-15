#include <sys/time.h>
#include <stdio.h>
int main(int argc, char** argv) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    unsigned long long millisecondsSinceEpoch =
    (unsigned long long)(tv.tv_sec) * 10000 +
    (unsigned long long)(tv.tv_usec) / 100;

    printf("%llu\n", millisecondsSinceEpoch);   
}