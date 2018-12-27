#include "m99cc.h"
#include <stdlib.h>

extern Vector *string_literals;
static int str_counter;

void list_string_in_code(Vector *code);

void list_string_in_node(Node *node) {
  if (node == NULL) {
    return;
  }
  if (node->ty == ND_STR) {
    node->val = str_counter++;
    vec_push(string_literals, node->name);
    return;
  }
  list_string_in_node(node->lhs);
  list_string_in_node(node->rhs);
  list_string_in_code(node->block);
}

void list_string_in_code(Vector *code) {
  if (code == NULL) {
    return;
  }
  Node *node;
  for (int i = 0; (node = code->data[i]); i++) {
    list_string_in_node(node);
  }
}

Vector *analysis(Vector *program_code) {
  // Extract string literals.
  str_counter = 0;
  list_string_in_code(program_code);
  // TODO: add analysis of pointer addresses.
  return program_code;
}
