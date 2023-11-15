#ifndef __HTTP_LOG_H__
#define __HTTP_LOG_H__

#define INVALID_LOGGER ((void*)0)

typedef struct _Logger *Logger;


Logger create_log(const char* log_file, int buffer_size);

void destroy_log(Logger* logger);

void log_buf(Logger l, const char* buf, int buf_size);

void flush_logger(Logger l);

#endif // __HTTP_LOG_H__
