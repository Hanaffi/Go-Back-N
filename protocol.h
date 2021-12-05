#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>

typedef struct simulator_state simulator_state;

#define MAX_SEQ 7

typedef enum {
  frame_arrival,
  cksum_err,
  timeout,
  network_layer_ready
} event_type;

#define MAX_PKT 8 /* determines packet size in bytes */

typedef unsigned int seq_nr; /* sequence or ack numbers */
typedef struct {
  char data[MAX_PKT];
} packet;                              /* packet definition */
typedef enum { data, ack } frame_kind; /* frame kind definition */

typedef struct {   /* frames are transported in this layer */
  frame_kind kind; /* what kind of frame is it? */
  seq_nr seq;      /* sequence number */
  seq_nr ack;      /* acknowledgement number */
  packet info;     /* the network layer packet */
} frame;

/* Wait for an event to happen; return its type in event. */
void wait_for_event(simulator_state *simulator, event_type *event);

/* Fetch a packet from the network layer for transmission on the channel. */
void from_network_layer(simulator_state *simulator, packet *p);

/* Deliver information from an inbound frame to the network layer. */
void to_network_layer(simulator_state *simulator, packet *p);

/*Go get an inbound frame from the physical layer and copy it to r. */
void from_physical_layer(simulator_state *simulator, frame *r);

/* Pass the frame to the physical layer for transmission. */
void to_physical_layer(simulator_state *simulator, frame *s);

/* Start the clock running and enable the timeout event. */
void start_timer(simulator_state *simulator, seq_nr k);

/* Stop the clock and disable the timeout event. */
void stop_timer(simulator_state *simulator, seq_nr k);

/* Allow the network layer to cause a network layer ready event. */
void enable_network_layer(simulator_state *simulator);

/* Forbid the network layer from causing a network layer ready event. */
void disable_network_layer(simulator_state *simulator);

/* Macro inc is expanded in - line : increment k circularly. */
#define inc(k)                                                                 \
  if (k < MAX_SEQ)                                                             \
    k = k + 1;                                                                 \
  else                                                                         \
    k = 0

#endif /* PROTOCOL_H */
