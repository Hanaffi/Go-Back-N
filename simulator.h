#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "circular_buffer.h"
#include "linked_list.h"
#include "protocol.h"

#define NETWORK_LAYER_DELAY 1000
#define FRAME_TIMEOUT 4000
#define SOCKET_NAME "/tmp/go-back-n.socket"

typedef struct simulator_state {
  circular_buffer event_buffer;
  linked_list timers_list;
  bool network_layer_enabled;
  unsigned long long last_network_frame_time;
  int connection_socket;
  int data_socket;
  char buf[sizeof(frame)];
  size_t bytes_read_so_far;
} simulator_state;

void simulator_init(simulator_state *ss, size_t event_buffer_capacity,
                    bool is_server);

void simulator_free(simulator_state *ss, bool is_server);

void handle_signals(int s);

#endif /* SIMULATOR_H */
