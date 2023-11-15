#ifndef __SERVER_HEADER_H__
#define __SERVER_HEADER_H__

#include <stdbool.h>
#include "log.h"

typedef struct State State;

bool handle_request(int fd, State* s, Logger l);
void init_parser_state(State* s);
State** create_state_buffer(int num_states);
void destroy_state_buffer(State*** state_buffer);

#endif
