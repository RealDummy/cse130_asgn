#ifndef __FILE_READWRITE_H__
#define __FILE_READWRITE_H__

#include <fcntl.h>
#include <unistd.h>

#include "strview.h"

#define INVALID_FRWEX NULL

typedef struct FileRWEx *FileRWEx;

FileRWEx create_file_rw_ex(int max_open, int max_path);

int read_open(FileRWEx p, StringView path);
int write_open(FileRWEx p, StringView path);

void read_close(FileRWEx p, StringView path);
void write_close(FileRWEx p, StringView path);

void destroy_file_rw_ex(FileRWEx* p);


#endif // __FILE_READWRITE_H__
