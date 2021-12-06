#include "linked_list.h"

#include <stdlib.h>

void ll_init(linked_list *ll) {
  ll->front = NULL;
  ll->end = NULL;
}

void ll_add_end(linked_list *ll, seq_nr frame_nr, unsigned long long end_time) {
  linked_list_node *temp;
  temp = (linked_list_node *)malloc(sizeof(linked_list_node));
  temp->frame_nr = frame_nr;
  temp->end_time = end_time;
  temp->prev = ll->end;
  temp->next = NULL;

  if (ll->end == NULL)
    ll->front = temp;
  else
    ll->end->next = temp;

  ll->end = temp;
}

void ll_delete(linked_list *ll, linked_list_node *node) {
  if (node->prev == NULL) {
    ll->front = node->next;
    if (ll->front != NULL)
      ll->front->prev = NULL;
  } else if (node->next == NULL) {
    ll->end = node->prev;
    if (ll->end != NULL)
      ll->end->next = NULL;
  } else {
    node->prev->next = node->next;
    node->next->prev = node->prev;
  }

  free(node);
}

void ll_clear(linked_list *ll) {
  linked_list_node *trav, *next_trav;
  next_trav = trav = ll->front;
  while (trav != NULL) {
    next_trav = trav->next;
    free(trav);
    trav = next_trav;
  }

  ll->front = NULL;
  ll->end = NULL;
}
