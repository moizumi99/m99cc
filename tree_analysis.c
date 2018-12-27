#include "m99cc.h"
#include <stdlib.h>

extern Map *global_symbols;
extern Vector *local_symbols;
extern Vector *string_literals;
static int str_counter;
static Map *current_local_symbol;
static int local_symbol_counter;

// parse.c
Node *new_node(int op, Node *lhs, Node *rhs);
Node *new_node_num(int val);
DataType *get_node_data_type(Map *global_table, Map *local_table, Node *node);
int get_data_step_from_data_type(DataType *data_type);


void list_string_in_node(Node *node);

void list_string_in_code(Vector *code) {
  if (code == NULL) {
    return;
  }
  Node *node;
  for (int i = 0; (node = code->data[i]); i++) {
    list_string_in_node(node);
  }
}

void list_string_in_node(Node *node) {
  if (node == NULL) {
    return;
  }
  // node analysis
  if (node->ty == ND_STR) {
    node->val = str_counter++;
    vec_push(string_literals, node->name);
    return;
  }
  if ((node->ty == '+' || node->ty == '-') && node->rhs != NULL) {
    // If adding number to pointer, adjust the size.
    DataType *ldt = get_node_data_type(global_symbols, current_local_symbol, node->lhs);
    DataType *rdt = get_node_data_type(global_symbols, current_local_symbol, node->rhs);
    if (ldt->dtype == DT_PNT && rdt->dtype != DT_PNT) {
      int step = get_data_step_from_data_type(ldt);
      node->rhs = new_node('*', new_node_num(step), node->rhs);
    } else if (ldt->dtype != DT_PNT && rdt->dtype == DT_PNT) {
      int step = get_data_step_from_data_type(rdt);
      node->lhs = new_node('*', new_node_num(step), node->lhs);
    }
  }
  if ((node->ty == ND_PE || node->ty == ND_ME)) {
    int step = 1;
    DataType *lhs_type = get_node_data_type(global_symbols, current_local_symbol, node->lhs);
    DataType *rhs_type = get_node_data_type(global_symbols, current_local_symbol, node->rhs);
    if (lhs_type->dtype == DT_PNT && rhs_type->dtype != DT_PNT) {
      step = get_data_step_from_data_type(lhs_type);
    }
    node->rhs = new_node('*', new_node_num(step), node->rhs);
  }
  // recursive analysis
  list_string_in_node(node->lhs);
  list_string_in_node(node->rhs);
  list_string_in_code(node->block);
}

Vector *analysis(Vector *program_code) {
  // Extract string literals.
  str_counter = 0;
  local_symbol_counter = 0;
  Node *node;
  for (int i = 0; (node = program_code->data[i]); i++) {
    current_local_symbol = local_symbols->data[i];
    list_string_in_node(node);
  }
  // TODO: add analysis of pointer addresses.
  return program_code;
}
