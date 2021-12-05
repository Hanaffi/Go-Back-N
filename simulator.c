#include "simulator.h"
#include "linked_list.h"
#include "protocol.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

void simulator_init(simulator_state *ss, size_t event_buffer_capacity,
                    bool is_server) {
  cb_init(&ss->event_buffer, event_buffer_capacity, sizeof(event_type));
  ll_init(&ss->timers_list);
  ss->network_layer_enabled = false;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long epoch = (unsigned long long)(tv.tv_sec) * 1000 +
                             (unsigned long long)(tv.tv_usec) / 1000;
  ss->last_network_frame_time = epoch;

  int ret;
  if (is_server) {
    struct sockaddr_un name;

    ss->connection_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (ss->connection_socket == -1) {
      fprintf(stderr, "Error creating connection socket\n");
      exit(EXIT_FAILURE);
    }

    memset(&name, 0, sizeof(name));

    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

    ret = bind(ss->connection_socket, (const struct sockaddr *)&name,
               sizeof(name));
    if (ret == -1) {
      fprintf(stderr, "Error binding connection socket\n");
      exit(EXIT_FAILURE);
    }

    ret = listen(ss->connection_socket, 1);
    if (ret == -1) {
      fprintf(stderr, "Error listening on connection socket\n");
      exit(EXIT_FAILURE);
    }

    ss->data_socket = accept(ss->connection_socket, NULL, NULL);
    if (ss->data_socket == -1) {
      fprintf(stderr, "Error accepting connection on connection socket\n");
      exit(EXIT_FAILURE);
    }
  } else {
    struct sockaddr_un addr;

    ss->data_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (ss->data_socket == -1) {
      fprintf(stderr, "Error creating data socket\n");
      exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    ret =
        connect(ss->data_socket, (const struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
      fprintf(stderr, "Error connecting to server\n");
      exit(EXIT_FAILURE);
    }
  }

  int flags = fcntl(ss->data_socket, F_GETFL, 0);
  fcntl(ss->data_socket, F_SETFL, flags | O_NONBLOCK);

  ss->bytes_read_so_far = 0;

  srand(time(NULL));
}

void simulator_free(simulator_state *ss, bool is_server) {
  cb_free(&ss->event_buffer);

  close(ss->data_socket);
  if (is_server) {
    close(ss->connection_socket);
    unlink(SOCKET_NAME);
  }
}

void handle_signals(int s) {
  unlink(SOCKET_NAME);
  exit(EXIT_SUCCESS);
}

void check_timers(simulator_state *simulator) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long epoch = (unsigned long long)(tv.tv_sec) * 1000 +
                             (unsigned long long)(tv.tv_usec) / 1000;

  if (simulator->timers_list.front != NULL &&
      simulator->timers_list.front->end_time < epoch) {
    event_type event = timeout;
    cb_push_back(&simulator->event_buffer, &event);
    ll_clear(&simulator->timers_list);
  }
}

void check_network_layer(simulator_state *simulator) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long epoch = (unsigned long long)(tv.tv_sec) * 1000 +
                             (unsigned long long)(tv.tv_usec) / 1000;

  if (simulator->network_layer_enabled &&
      (epoch - simulator->last_network_frame_time) > NETWORK_LAYER_DELAY) {
    event_type event = network_layer_ready;
    cb_push_back(&simulator->event_buffer, &event);
    simulator->last_network_frame_time = epoch;
  }
}

void check_physical_layer(simulator_state *simulator) {
  size_t bytes_to_read = sizeof(frame);
  int ret;

  while ((ret = read(simulator->data_socket,
                     simulator->buf + simulator->bytes_read_so_far,
                     bytes_to_read - simulator->bytes_read_so_far)) > 0)
    simulator->bytes_read_so_far += ret;

  if (ret == -1 && errno != EWOULDBLOCK) {
    fprintf(stderr, "Error reading from data socket\n");
    exit(EXIT_FAILURE);
  }

  if (simulator->bytes_read_so_far == bytes_to_read) {
    event_type event = frame_arrival;
    cb_push_back(&simulator->event_buffer, &event);
    simulator->bytes_read_so_far = 0;
  }
}

void wait_for_event(simulator_state *simulator, event_type *event) {
  while (true) {
    check_timers(simulator);
    check_network_layer(simulator);
    check_physical_layer(simulator);

    if (cb_pop_front(&simulator->event_buffer, event))
      return;
  }
}

void from_network_layer(simulator_state *simulator, packet *p) {
  for (int i = 0; i < MAX_PKT - 1; i++) {
    p->data[i] = 32 + (rand() % 95); // visible ASCII chars
  }
  p->data[MAX_PKT - 1] = '\0';
}

void to_network_layer(simulator_state *simulator, packet *p) { return; }

void from_physical_layer(simulator_state *simulator, frame *r) {
  memcpy(r, simulator->buf, sizeof(frame));
}

void to_physical_layer(simulator_state *simulator, frame *s) {
  size_t bytes_to_write = sizeof(frame);
  size_t bytes_written_so_far = 0;
  int ret;

  while ((ret = write(simulator->data_socket, s + bytes_written_so_far,
                      bytes_to_write - bytes_written_so_far)) > 0)
    bytes_written_so_far += ret;

  if (ret == -1) {
    fprintf(stderr, "Error writing to data socket\n");
    exit(EXIT_FAILURE);
  }

  if (bytes_written_so_far != bytes_to_write) {
    fprintf(stderr,
            "Error writing in physical layer: Expected to read %lu "
            "bytes, got %lu bytes\n",
            bytes_to_write, bytes_written_so_far);
    exit(EXIT_FAILURE);
  }
}

void start_timer(simulator_state *simulator, seq_nr k) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long epoch = (unsigned long long)(tv.tv_sec) * 1000 +
                             (unsigned long long)(tv.tv_usec) / 1000;

  ll_add_end(&simulator->timers_list, k, epoch + FRAME_TIMEOUT);
}

void stop_timer(simulator_state *simulator, seq_nr k) {
  if (simulator->timers_list.front->frame_nr == k) {
    ll_delete(&simulator->timers_list, simulator->timers_list.front);
  } else {
    fprintf(stderr, "Error stopping timer: Expected seq_nr (%d), found (%d)\n",
            simulator->timers_list.front->frame_nr, k);
    exit(EXIT_FAILURE);
  }
}

void enable_network_layer(simulator_state *simulator) {
  simulator->network_layer_enabled = true;
}

void disable_network_layer(simulator_state *simulator) {
  simulator->network_layer_enabled = false;
}
