#include <err.h>
#include <limits.h>

#include <sys/socket.h>
#include <sys/poll.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include "bind.h"
#include "server.h"
#include "helpers.h"
#include "strview.h"
#include "log.h"
#include "queue.h"

#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define BAD_EXIT (1)

volatile bool term = false;

void sigterm_handler(int sig) {
    (void)sig;
    term = true;
}

void setup_tv(struct timespec *tv) {
    tv->tv_sec = 1;
    tv->tv_nsec = 0;
}

void close_gracefully(int fd) {
    //shutdown(fd, SHUT_RDWR);
    // if (code < 0) {
	//     close(fd);
	//     return;
    // }
    // char buf[1000];
    // while(read(fd, buf, 1000) > 0) {
    // 	//do nothing
    // }
    close(fd);

}

typedef struct Request {
    State* s;
    int fd;
} Request;

void* request_workers(void* arg) {
    void** args = (void**) arg;
    queue_t* jobs = (queue_t*)args[0];
    queue_t* finished_reqs = (queue_t*)args[1];
    Logger l = (Logger)args[2];
    sem_t* q_size = (sem_t*)args[3];

    sigset_t block_these;
    sigaddset(&block_these, SIGTERM);
    sigaddset(&block_these, SIGINT);
    pthread_sigmask(SIG_BLOCK, &block_these, NULL);
    while(1) {
        Request* job;
        assert(sem_wait(q_size) == 0);
        if(term) {
            break;
        }
        queue_pop(jobs, (void**)&job);
        if(job->fd == -1) {
            continue;
        }
        if(handle_request(job->fd, job->s, l)) {
            close_gracefully(job->fd);
            job->fd = -1;
            queue_push(finished_reqs, job);
        } else {
            //printf("nonblock!");
            sem_post(q_size);
            queue_push(jobs, job);
        }
    }
    pthread_sigmask(SIG_UNBLOCK, &block_these, NULL);
    return NULL;
}

void init_workers(pthread_t* threads, int num_workers, void* args) {
    for(int i = 0; i < num_workers; ++i) {
        pthread_create(threads + i, NULL, request_workers, args);
    }
}

void bad_usage(const char* path) {
    dprintf(STDERR_FILENO, "usage: %s [-t threads] [-l logfile] <port>", path);
    exit(BAD_EXIT);
}

int main(int argc, char* const* argv) {
    if(argc < 2) {
        return BAD_EXIT;
    }
    const char* port_str = argv[argc - 1];
    const char* port_str_end = port_str;
    while(*port_str_end != '\0') {
        ++port_str_end;
    }
    StringView port_view = create_view(port_str, port_str_end);
    ssize_t port_long = parse_to_int(port_view);
    if(port_long > UINT16_MAX || port_long <= 0 ) {
        errx(BAD_EXIT, "Invalid port %s", argv[1]);
    }
    uint16_t port = (uint16_t)port_long;
    int socketdes = create_listen_socket(port);
    switch(socketdes) {
        case -1: 
            errx(BAD_EXIT, "Bad port %u\n", port);
        case -2:
            errx(BAD_EXIT, "Error opening port %u\n", port);
        case -3:
            errx(BAD_EXIT, "Error binding port %u\n", port);
        case -4:
            errx(BAD_EXIT, "Error listening to port %u\n", port);
    }
    int ch;
    const char* logfile = NULL;
    const char* nthreads = NULL;
    while((ch = getopt(argc, argv, "l:t:")) != -1) {
        switch (ch)
        {
        case 'l':  
            if(!optarg) {
                bad_usage(argv[0]);
            }
            logfile = optarg;
            break;
        case 't':  
            if(!optarg) {
                bad_usage(argv[0]);
            }
            nthreads = optarg;
            break;
        default:
            bad_usage(argv[0]);
        }
    }
    int num_threads = nthreads ? atoi(nthreads) : 1;
    //printf("running with %d threads\n", num_threads);
    int num_concurent_req = num_threads * 10;

    Logger l = create_log(logfile, 4096);
    State** state_buffer = create_state_buffer(num_concurent_req);
    Request* req_pool = malloc(sizeof(Request) * num_concurent_req);
    queue_t* free_req_queue = queue_new(num_concurent_req);
    for(int i = 0; i < num_concurent_req; ++i) {
        req_pool[i].s = state_buffer[i];
        queue_push(free_req_queue, req_pool + i);
    }
    queue_t* taken_req_queue = queue_new(num_concurent_req);
    sem_t* q_size = sem_open("/foo", O_CREAT, 0666, 0);
    assert(q_size != SEM_FAILED);
    sem_unlink("/foo");
    void* args[] = {taken_req_queue, free_req_queue, l, q_size};
    pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);
    init_workers(threads, num_threads, args);

    struct sigaction action;
    action.sa_handler = sigterm_handler;
    action.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);


    struct pollfd pfd;
    pfd.fd = socketdes;
    pfd.events = POLLIN;

    while (!term) {   
        sigset_t block_these;
        sigaddset(&block_these, SIGTERM);
        sigaddset(&block_these, SIGINT);
        pthread_sigmask(SIG_BLOCK, &block_these, NULL);
        if( poll(&pfd, 1, 1000) == 0) {
            pthread_sigmask(SIG_UNBLOCK, &block_these, NULL);
            continue;
        }
        pthread_sigmask(SIG_UNBLOCK, &block_these, NULL);

        int filedes = accept(socketdes, NULL, NULL);
        if(filedes < 0) {
            //warn("socket");
            continue;
        }
        fcntl(filedes, F_SETFL, fcntl(filedes, F_GETFL) | O_NONBLOCK); //we aren't ready
        
        
        Request* free_req;
        while(!term){
            if( queue_pop(free_req_queue, (void**)&free_req)) {
                break;
            }
        }
        if(term) {
            break;
        }
        init_parser_state(free_req->s);
        free_req->fd = filedes;
        sem_post(q_size);
        queue_push(taken_req_queue, free_req);
    }

    for(int i = 0; i < num_threads; ++i) {
        sem_post(q_size);
    }

    for(int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }    
    queue_delete(&taken_req_queue);
    queue_delete(&free_req_queue);
    sem_close(q_size);
    destroy_state_buffer(&state_buffer);
    destroy_log(&l);

    free(req_pool);
    free(threads);
}
