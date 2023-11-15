#ifndef __ENUMS_H__
#define __ENUMS_H__

typedef enum Stage {
    REQUEST_LINE,
    HEADER_FIELDS,
    READ_BODY,
    BUILD_HEADER,
    SEND_HEADER,
    READ_FILE,
    SEND_RES,
    FINISHED,
} Stage;

typedef enum Status {
    OK = 200,
    CREATED = 201,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_ERROR = 500,
    NOT_IMPLEMENTED = 501,
} Status;


#endif //__ENUMS_H__

