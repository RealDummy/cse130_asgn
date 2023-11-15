#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <err.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "server.h"
#include "strview.h"
#include "helpers.h"
#include "server_enums.h"
#include "reqres.h"
#include "log.h"

#include <stdio.h>

#define MAX_LOG (500)
#define MAX_PATH (300)
#define READ_LEN (10000) //must be greater than 2050
#define MAX_HEADER (2050)

#define NUM_SUPPORTED_METHODS (3)

static const char* const SUPPORTED_METHODS[] = {"GET", "PUT", "HEAD"};

struct State {
    ReqState req;
    ResState res;
    Status status;
    Stage stage;
};




const char* burn_whitespace(const char* bufs, const char* bufe) {
    while(bufs != bufe && is_whitespace(*(bufs))) {
        bufs += 1;
    }
    return bufs;
}

bool validate_path_char(char c) {
    return is_alpha(c) || is_numeric(c) || c == '.' || c == '-' || c == '_';
}

bool validate_method_char(char c) {
    return is_alpha(c);
}

bool validate_proto(StringView proto) {
    if(proto.end - proto.start < 8) {
        return false;
    }
    //can check better if more protocols are added
    if(compare_vs(proto, "HTTP/1.1") != 0) {
        return false;
    }
    return true;
}

bool validate_path(StringView path) {
    if(path.end - path.start < 2) {
        return false;
    }
    if(*(path.start++) != '/') {
        return false;
    }
    while(path.start != path.end) {
        if(!validate_path_char(*(path.start++))) {
            return false;
        }
    }
    return true;
}

bool validate_method(StringView method) {
    if(method.end - method.start > 8) {
        return false;
    }
    while(method.start != method.end) {
        if(!validate_method_char(*(method.start++))) {
            return false;
        }
    }
    return true;
}

Method get_method(StringView method_str, Status* status) {
    for(int i=0; i < NUM_SUPPORTED_METHODS; ++i) {
        if(compare_vs(method_str, SUPPORTED_METHODS[i]) == 0) {
            return (Method)i;
        }
    }
    if( validate_method(method_str) ) {
        *status = NOT_IMPLEMENTED;
    }
    else {
        *status = BAD_REQUEST;
    }
    return UNKNOWN;
}


bool open_get_file(StringView uri, Status* status, int* fd) {
    char buf[MAX_PATH];
    const char* str = fill_str(uri, buf, MAX_PATH);
    // uri was too long
    if (str == NULL) {
        *status = BAD_REQUEST;
        return false;
    }
    str += 1;
    //check if uri is a directory

    struct stat file_stat;

    int code = lstat(str, &file_stat);
    if(code < 0) {
        if(errno == ENOENT) {
            *status = NOT_FOUND;
        }
        else{
            *status = FORBIDDEN;
        }
        return false;
    }
    if(!S_ISREG(file_stat.st_mode)) {
        *fd = -1;
        *status = FORBIDDEN;
        return false;
    }
    *fd = open(str, O_RDONLY);
    if(*fd < 0) {
        (void)errno;
        *status = FORBIDDEN;
        return false;
    }
    return true;
}


void parse_rl(ReqState* req, StringView rl, Status* status, Stage* stage) {

    const char* method_end = find_delim(rl.start, rl.end, " ");
    if(!method_end) {
        *status = BAD_REQUEST;
        *stage  = BUILD_HEADER;
        return;
    }
    req->method_view = trim_whitespace(create_view(rl.start, method_end));
    req->method = get_method( 
                    req->method_view,
                    status
                );
    if( req->method == UNKNOWN) {
        *stage = BUILD_HEADER;
        return;
    }
    const char* uri_end = find_delim(method_end, rl.end, " ");
    if(!uri_end) {
        *status = BAD_REQUEST;
        *stage  = BUILD_HEADER;
        return;
    }
    req->uri = trim_whitespace(create_view(method_end, uri_end));
    if(!validate_path(req->uri)) {
        *status = BAD_REQUEST;
        *stage = BUILD_HEADER;
        return;
    }
    if( !validate_proto( trim_whitespace(create_view(uri_end, rl.end)) ) ) {
        *status = BAD_REQUEST;
        *stage = BUILD_HEADER;
        return;
    }
    *stage = HEADER_FIELDS;
}

typedef enum ReadRet {
    READ_OK,
    READ_ERROR,
    READ_AGAIN,
    READ_END,
} ReadRet;

ReadRet my_read(int fd, char* buf, size_t buf_len, char** buf_end) {
    ssize_t bytes_read = read(fd, buf, buf_len);
    if(bytes_read == 0 ) {
        return READ_END;
    }
    if(bytes_read < 0){
        if(errno == EAGAIN){
            return READ_AGAIN;
        }
        return READ_ERROR;
    }
    *buf_end = buf + bytes_read;
    return READ_OK;
}

bool read_rl(int fd, ReqState* req, Status* status, Stage* stage) {    
    const char* line_end = find_delim(req->parsed_end, req->buf_end, "\r\n"); 
    while(line_end == NULL) {
        size_t space_in_buffer = buf_left(req->buf, req->buf_end, req->buf_len);
        if(space_in_buffer < 2) {
            *status = BAD_REQUEST;
            *stage = BUILD_HEADER;
            return true;
        }
        ////printf("space in buffer = %lu\n", space_in_buffer);
        switch(my_read(fd, req->buf_end, space_in_buffer, &req->buf_end )) {
            case READ_OK:
                line_end = find_delim(req->parsed_end, req->buf_end, "\r\n");
                break;
            case READ_AGAIN:
                ////printf("read_rl again\n");
                return false;
            case READ_END:
                *stage = FINISHED;
                return true;
            case READ_ERROR:
                *stage = BUILD_HEADER;
                *status = INTERNAL_ERROR;
                return true;
        }
    }
    StringView rl = create_view(req->parsed_end, line_end - 2);
    parse_rl(req, rl, status, stage);
    req->parsed_end = line_end;
    return true;
}

void parse_hf(ReqState* req, StringView hf, Status* status, Stage* stage) {
    const char* colon = find_delim(hf.start, hf.end, ":");
    if(colon == NULL || colon < hf.start+1) {
        *status = BAD_REQUEST;
        *stage = BUILD_HEADER;
    }
    StringView key = create_view(hf.start, colon - 1);
    StringView value = create_view(colon, hf.end);

    if(compare_vs(key, "Content-Length") == 0) {
        ssize_t len = parse_to_int(value);
        if(len < 0) {
            *status = BAD_REQUEST;
            *stage = BUILD_HEADER;
            return;
        }
        req->body_len = len;
    }
    else if (compare_vs(key, "RequestID") == 0) {
        req->id = trim_whitespace(value);
    }
}


bool read_hf(int fd, ReqState* req, Status* status, Stage* stage) {
    ssize_t space_in_buffer = buf_left(req->buf, req->buf_end, req->buf_len);
    if(space_in_buffer < 2) {
        *status = BAD_REQUEST;
        *stage = BUILD_HEADER;
    }
    const char* line_end = find_delim(req->parsed_end, req->buf_end, "\r\n"); 
    while(line_end == NULL) {
        switch(my_read(fd, req->buf_end, space_in_buffer, &req->buf_end )) {
            case READ_OK:
                line_end = find_delim(req->parsed_end, req->buf_end, "\r\n");
                break;
            case READ_AGAIN:
                ////printf("read_hf again\n");
                return false;
            case READ_END:
                *stage = FINISHED;
                return true;
            case READ_ERROR:
                *stage = BUILD_HEADER;
                *status = INTERNAL_ERROR;
                return true;
        }
    }
    StringView hf = create_view(req->parsed_end, line_end - 2);
    if( is_empty_view(hf) ) {
        if(req->method == HEAD || req->method == GET) {
            if(!open_get_file(req->uri, status, &req->get_fd) ) {
                *stage = BUILD_HEADER;
            }
        }
        *stage = req->method == PUT ? READ_BODY : BUILD_HEADER;
        req->parsed_end = line_end;
        return true;
    }
    parse_hf(req, hf, status, stage);
    req->parsed_end = line_end;
    return true;
}

bool create_filename(StringView filename, int* fd, Status* status) {

    char buf[MAX_PATH];
    const char* c_str = fill_str(filename, buf, MAX_PATH);
    if (c_str == NULL) {
        *status = INTERNAL_ERROR;
        return false;
    }
    *fd = open(c_str + 1, O_WRONLY | O_TRUNC);
    if( *fd < 0 && errno == ENOENT ) {
        *fd = open(c_str + 1, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
        *status = CREATED;
    }
    if (*fd < 0) {
        *status = FORBIDDEN;
        return false;
    }
    return true;
}



bool read_request_body(int fd, ReqState* req, Status* status, Stage* stage) {
    char buf[READ_LEN];
    char* scratch_buf = buf;
    char* scratch_end = scratch_buf;
    const char* tpe = req->parsed_end;
    while(tpe != req->buf_end) {
        *(scratch_end++) = *(tpe++);
    }
    size_t space_in_buffer = buf_left(scratch_buf, scratch_end, READ_LEN);
    if(space_in_buffer == 0) {
        *status = BAD_REQUEST;
        *stage = BUILD_HEADER;
        return true;
    }
    size_t read_len = space_in_buffer > req->body_len ? req->body_len : space_in_buffer;
    read_len -= (scratch_end - scratch_buf);
    if(req->put_fd == -1){
        if( !create_filename(req->uri, &req->put_fd, status) ){
            *stage = BUILD_HEADER;
            return true;
        }
    }
    if(read_len > 0){
        //printf("%p %p\n", (void*)scratch_end, (void*)scratch_buf);
        switch(my_read(fd, scratch_end, read_len, &scratch_end )) {
            case READ_OK:
                break;
            case READ_AGAIN:
                ////printf("foo\n");
                return false;
            case READ_END:
                *stage = FINISHED;
                return true;
            case READ_ERROR:
                *stage = BUILD_HEADER;
                *status = INTERNAL_ERROR;
                return true;
        }
    }
    if( (size_t)(scratch_end - scratch_buf) > req->body_len ) {
        *stage = BUILD_HEADER;
        *status = BAD_REQUEST;
        return true;
    }
    while(scratch_buf != scratch_end) {
        ssize_t bytes_flushed = write(req->put_fd, scratch_buf, (scratch_end - scratch_buf));
        if(bytes_flushed < 0) {
            *status = INTERNAL_ERROR;
            *stage = BUILD_HEADER;
            return true;
        }
        scratch_buf += bytes_flushed;
        req->body_len -= bytes_flushed;
    }
    req->parsed_end = req->buf_end;
    if(req->body_len <= 0) {
        assert(req->body_len == 0);
        *stage = BUILD_HEADER;
    }
    return true;
}


size_t file_size(int fd) {
    ssize_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    assert(size >= 0);
    return size;
}

char* copy_to(char* buf, const char* str, size_t len) {
    while(*str != '\0' && (len--) > 0 ) {
        *(buf++) = *(str++);
    }
    return buf;
}
char* copy_protocol(char* buf, size_t len) {
    return copy_to(buf, "HTTP/1.1 ", len);
}

// NOT NULL TERMINATED!!!! (because lots of numbers dont need to be)
// MAKE SURE len of buf > LOG10 i
char* copy_code(Status s, char* buf, size_t len) {
    assert(len > 4);
    assert((int)s >= 100 && (int)s <= 999);
    uint64_t istatus = (int)s;
    ssize_t res = my_utoa(istatus, buf);
    *(buf + 3) = ' ';
    return res + buf + 1;
}

const char* get_status_name(Status status) {
    switch(status) {
        case OK:
            return "OK";
        case CREATED:
            return "Created";
        case BAD_REQUEST:
            return "Bad Request";
        case FORBIDDEN:
            return "Forbidden";
        case NOT_FOUND:
            return "Not Found";
        case INTERNAL_ERROR:
            return "Internal Server Error";
        case NOT_IMPLEMENTED:
            return "Not Implemented";
        default:
            return "Unknown"; //empty, shouldnt happen as switch case covers all valid enums
    }
}

char* copy_msg(Status status, char* buf, size_t len) {
    const char* msg = get_status_name(status);
    assert(msg);
    return copy_to(buf, msg, len);
}

void build_response_request_line(ResState* res, Status status) {
    res->buf_end = copy_protocol(res->buf_end, buf_left(res->buf, res->buf_end, res->buf_len) );
    res->buf_end = copy_code(status, res->buf_end, buf_left(res->buf, res->buf_end, res->buf_len) );
    res->buf_end = copy_msg(status, res->buf_end, buf_left(res->buf, res->buf_end, res->buf_len) );
    res->buf_end = copy_to(res->buf_end, "\r\n", buf_left(res->buf, res->buf_end, res->buf_len) );
}

void build_response_header_fields(const ReqState* req, ResState* res, Status status) {
    ssize_t body_size = strlen(get_status_name(status)) + 1; // +1 for the \n we add to the end of the name
    if(status == OK && (req->method == GET || req->method == HEAD)) {
        body_size = file_size(req->get_fd);
    }
    res->buf_end = copy_to(res->buf_end, "Content-Length: ", res->buf_len);
    size_t code = my_utoa(body_size, res->buf_end);
    assert(code >= 0);
    res->buf_end += code;
    res->buf_end = copy_to(res->buf_end, "\r\n\r\n", req->buf_len - (res->buf_end - res->buf));
}

void add_static_body(ResState* res, Status status) {
    const char* msg = get_status_name(status); 
    res->buf_end = copy_to(res->buf_end, msg, buf_left(res->buf, res->buf_end, res->buf_len));
    res->buf_end = copy_to(res->buf_end, "\n", buf_left(res->buf, res->buf_end, res->buf_len));
}

void build_header(ReqState* req, ResState* res, Status* status, Stage* stage) {
    build_response_request_line(res, *status);
    build_response_header_fields(req, res, *status);
    if(*status != OK || req->method == PUT){
        add_static_body(res, *status);
    }
    *stage = SEND_RES;
}


void read_get_body(const ReqState* req, ResState* res, Status status, Stage* stage) {
    if(req->method != GET || status != OK) {
        *stage = SEND_RES;
        return;
    }
    assert(req->get_fd != -1);
    ReadRet ret = my_read(
        req->get_fd, 
        res->buf_end, 
        buf_left(res->buf, res->buf_end, res->buf_len), 
        &res->buf_end
    );
    if( ret == READ_ERROR ){
        *stage = FINISHED; //too late to send error :(
    } else if(ret == READ_END) {
        *stage = FINISHED;
    } else if(ret == READ_OK) {
        *stage = SEND_RES;
    }
}

bool send_res(int fd, ReqState* req, ResState* res, Status status, Stage* stage) {
    while(res->flushed != res->buf_end) {
        ssize_t bytes_flushed = write(fd, res->flushed, res->buf_end - res->flushed);
        if(bytes_flushed <= 0) {
            if(errno == EAGAIN) {
                ////printf("send_res again\n");
                return false;
            }
            *stage = FINISHED;
            return true;
        }
        res->flushed += bytes_flushed;
    }
    res->buf_end = res->buf;
    res->flushed = res->buf;
    if(req->method != GET || status != OK) {
        *stage = FINISHED;
    } else if (req->method == GET) {
        *stage = READ_FILE;
    }
    return true;
}

char* add_seperator(char* buf, char* end) {
    return copy_to(buf, ",", end - buf);
}

char* copy_view_to(char* buf, const char* bufend, StringView sv) {
    if(is_empty_view(sv)){
        return buf;
    }
    while(sv.start != sv.end && buf != bufend) {
        *(buf++) = *(sv.start++);
    }
    return buf;
}

void send_log(Logger l, State* s) {
    char res[MAX_LOG];
    char* end = res + MAX_LOG;
    char* out = res;
    out = copy_view_to(out, end, s->req.method_view); // method
    out = add_seperator(out, end);
    out = copy_view_to(out, end, s->req.uri); // uri
    out = add_seperator(out, end);
    out += my_utoa((uint64_t)s->status, out); //status
    out = add_seperator(out, end);
    if(is_empty_view(s->req.id)) {
        out = copy_to(out, "0", end - out);
    }
    else{
        out = copy_view_to(out, end, s->req.id);
    }
    out = copy_to(out, "\n", end - out);
    log_buf(l, res, out - res);
}

void cleanup(State* state) {
    if(state->req.put_fd != -1) {
        close(state->req.put_fd);
    }
    if(state->req.get_fd != -1) {
        close(state->req.get_fd);
    }
}

bool handle_request(int fd, State* state, Logger l) {
    while (true){
        switch(state->stage){
            case REQUEST_LINE:
                //printf("RL\n");
                if( !read_rl(fd, &state->req, &state->status, &state->stage) ){
                    return false;
                }
                break;
            case HEADER_FIELDS:
                //printf("HF\n");
                if( !read_hf(fd, &state->req, &state->status, &state->stage) ){
                    return false;
                }
                break;
            case READ_BODY:
                //printf("RB\n");
                if( !read_request_body(fd, &state->req, &state->status, &state->stage) ){
                    return false;
                }
                break;
            case BUILD_HEADER:
                //printf("BH\n");
                build_header(&state->req, &state->res, &state->status, &state->stage);
                break;
            case SEND_HEADER:
                //printf("SH");
                break;
            case READ_FILE:
                read_get_body(&state->req, &state->res, state->status, &state->stage);
                break;
            case SEND_RES:
                //printf("SR\n");
                if( !send_res(fd, &state->req, &state->res, state->status, &state->stage) ){
                    return false;
                }
                break;
            case FINISHED:
                //printf("DONE\n");
                //(void)l;
                send_log(l, state);
                cleanup(state);
                return true;
        }
        
    }
    //cant happen, but if it does i want the request to end!
    return true;
}

void init_parser_state(State* s) {
    s->stage = REQUEST_LINE;
    s->status = OK;
    s->req = (ReqState){
        s->req.buf,
        s->req.buf,
        s->req.buf,
        s->req.buf_len,
        0,
        INVALID_STRVIEW,
        INVALID_STRVIEW,
        INVALID_STRVIEW,
        NONE_YET,
        -1,
        -1
    };
    s->res = (ResState){
        s->res.buf,
        s->res.buf,
        s->res.buf,
        s->res.buf_len,
    };
}

State** create_state_buffer(int num_states) {
    char* req_bufs = malloc(sizeof(char) * MAX_HEADER * num_states);
    char* res_bufs = malloc(sizeof(char) * MAX_HEADER * num_states);
    State** state_ptrs = malloc(sizeof(State*) * num_states);
    State* state_buf = malloc(sizeof(State) * num_states);
    for(int i = 0; i < num_states; ++i) {
        state_ptrs[i] = state_buf + i;
        state_ptrs[i]->req.buf = req_bufs + (MAX_HEADER * i);
        state_ptrs[i]->req.buf_len = MAX_HEADER;
        state_ptrs[i]->res.buf = res_bufs + (MAX_HEADER * i);
        state_ptrs[i]->res.buf_len = MAX_HEADER;
        
    }
    return state_ptrs;
}
void destroy_state_buffer(State*** state_buffer) {
    free((*state_buffer[0])->req.buf);
    free((*state_buffer[0])->res.buf);
    free((*state_buffer[0]));
    free(*state_buffer);
    *state_buffer = NULL;
}
