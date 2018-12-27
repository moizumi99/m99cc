#include "m99cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern Map *global_symbols;
extern Vector *local_symbols;
extern Vector *string_literals;
static int str_counter;
static Map *current_local_symbol;
static int local_symbol_counter;

// parse.c
Node *new_node(int op, Node *lhs, Node *rhs);
Node *new_node_num(int val);

Symbol *get_symbol(Map *global_symbol_table, Map *local_symbol_table, Node *node) {
  Symbol *s = map_get(local_symbol_table, node->name);
  if (s == NULL) {
    s = map_get(global_symbol_table, node->name);
  }
  return s;
}

bool data_type_equal(DataType *dt1, DataType *dt2) {
  if (dt1 == NULL || dt2 == NULL) {
    return false;
  }
  if (dt1->dtype == DT_PNT && dt2->dtype == DT_PNT) {
    return data_type_equal(dt1->pointer_type, dt2->pointer_type);
  }
  if (dt1->dtype != dt2->dtype) {
    return false;
  }
  return true;
}

DataType *get_node_data_type(Map *global_table, Map *local_table, Node *node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->ty == ND_IDENT) {
    Symbol *s = get_symbol(global_table, local_table, node);
    if (s == NULL) {
      fprintf(stderr, "Error: Symbol %s not found (parse, get_node_data_type())\n", node->name);
      exit(1);
    }
    return s->data_type;
  }
  if (node->ty == ND_NUM) {
    return new_data_type(DT_INT);
  }
  if (node->ty == ND_STR) {
    return new_data_pointer(new_data_type(DT_CHAR));
  }
  if (node->rhs == NULL) {
    DataType *dt = get_node_data_type(global_table, local_table, node->lhs);
    if (node->ty == ND_DEREF) {
      return dt->pointer_type;
    }
    if (node->ty == '&') {
      return new_data_pointer(dt);
    }
    return dt;
  }
  DataType *left_data_type = get_node_data_type(global_table, local_table, node->lhs);
  DataType *right_data_type = get_node_data_type(global_table, local_table, node->rhs);
  if (data_type_equal(left_data_type, right_data_type)) {
    return left_data_type;
  }
  if (left_data_type->dtype == DT_PNT && right_data_type->dtype != DT_PNT) {
    if (right_data_type->dtype == DT_INVALID || right_data_type->dtype == DT_VOID) {
      goto ERROR;
    }
    return left_data_type;
  }
  if (left_data_type->dtype != DT_PNT && right_data_type->dtype == DT_PNT) {
    if (left_data_type->dtype == DT_INVALID || left_data_type->dtype == DT_VOID) {
      goto ERROR;
    }
    return right_data_type;
  }
 ERROR:
  fprintf(stderr, "DataType error. Left: %d, right: %d\n",
          left_data_type->dtype, right_data_type->dtype);
  exit(1);
  return NULL;
}

int get_data_step_from_data_type(DataType *data_type) {
  if (data_type->dtype != DT_PNT) {
    return 1;
  }
  if (data_type->pointer_type->dtype == DT_CHAR) {
    return 1;
  }
  // TODO: Suppport other data steps, like short.
  return 8;
}

int get_data_step_from_node(Map *global_table, Map *local_table, Node *node) {
  DataType *dt = get_node_data_type(global_table, local_table, node);
  return get_data_step_from_data_type(dt);
}



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
