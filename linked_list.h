#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef unsigned int seq_nr; /* sequence or ack numbers */

typedef struct linked_list_node {
  seq_nr frame_nr;
  unsigned long long end_time;
  struct linked_list_node *prev;
  struct linked_list_node *next;
} linked_list_node;

typedef struct linked_list {
  linked_list_node *front;
  linked_list_node *end;
} linked_list;

void ll_init(linked_list *ll);

void ll_add_end(linked_list *ll, seq_nr data, unsigned long long end_time);

void ll_delete(linked_list *ll, linked_list_node *node);

void ll_clear(linked_list *ll);

#endif /* LINKED_LIST_H */
