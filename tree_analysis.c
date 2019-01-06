#include "m99cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern Map *global_symbols;
extern Vector *local_symbols;
extern Vector *string_literals;
extern Map *global_struct_table;
static int str_counter;
static Map *current_local_symbols;
static int local_symbol_counter;

// parse.c
Node *new_2term_node(int op, Node *lhs, Node *rhs);
Node *new_node_num(int val);

int struct_size(Map *struct_table, char *struct_name) {
  Vector *struct_members = map_get(struct_table, struct_name);
  int total_size = 0;
  for (int j = 0; j < struct_members->len; j++) {
    StructMember *st = (StructMember *)struct_members->data[j];
    total_size += data_size(st->data_type);
  }
  return total_size;
}

// helper function to calculate data size from dtype.
int data_size_from_dtype(int dtype) {
  switch (dtype) {
  case DT_VOID:
    return 0;
  case DT_INT:
    return 8;
  case DT_CHAR:
    return 1;
  case DT_PNT:
    return 8;
  case DT_STRUCT:
    error("%s", "DT_STRUCT size not defined", __FILE__, __LINE__);
    return 8;
  default:
    return 8;
  }
}

// helper function to calculate data size from DataType struct.
int data_size(DataType *data_type) {
  if (data_type->dtype == DT_STRUCT) {
    return struct_size(global_struct_table, data_type->struct_name);
  }
  return data_size_from_dtype(data_type->dtype);
}

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
      error("Error: Symbol %s not found", node->name, __FILE__, __LINE__);
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
  error("%s", "", __FILE__, __LINE__);
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
    DataType *ldt = get_node_data_type(global_symbols, current_local_symbols, node->lhs);
    DataType *rdt = get_node_data_type(global_symbols, current_local_symbols, node->rhs);
    if (ldt->dtype == DT_PNT && rdt->dtype != DT_PNT) {
      int step = get_data_step_from_data_type(ldt);
      node->rhs = new_2term_node('*', new_node_num(step), node->rhs);
    } else if (ldt->dtype != DT_PNT && rdt->dtype == DT_PNT) {
      int step = get_data_step_from_data_type(rdt);
      node->lhs = new_2term_node('*', new_node_num(step), node->lhs);
    }
  }
  if ((node->ty == ND_PE || node->ty == ND_ME)) {
    int step = 1;
    DataType *lhs_type = get_node_data_type(global_symbols, current_local_symbols, node->lhs);
    DataType *rhs_type = get_node_data_type(global_symbols, current_local_symbols, node->rhs);
    if (lhs_type->dtype == DT_PNT && rhs_type->dtype != DT_PNT) {
      step = get_data_step_from_data_type(lhs_type);
    }
    node->rhs = new_2term_node('*', new_node_num(step), node->rhs);
  }
  // recursive analysis
  list_string_in_node(node->lhs);
  list_string_in_node(node->rhs);
  list_string_in_code(node->block);
}

void add_global_symbol(char *name_perm, int type, int num,
                       struct DataType *data_type) {
  static int global_symbol_counter = 0;
  Symbol *new_symbol = malloc(sizeof(Symbol));
  int dsize;
  dsize = data_size(data_type);
  global_symbol_counter += (num == 0) ? dsize : num * dsize;
  new_symbol->address = (void *)global_symbol_counter;
  new_symbol->type = type;
  new_symbol->num = num;
  new_symbol->data_type = data_type;
  map_put(global_symbols, name_perm, (void *)new_symbol);
}

void add_local_symbol(char *name_perm, int type, int num, struct DataType *data_type) {
  Symbol *new_symbol = malloc(sizeof(Symbol));
  int dsize;
  dsize = data_size(data_type);
  local_symbol_counter += (num == 0) ? dsize : num * dsize;
  new_symbol->address = (void *)local_symbol_counter;
  new_symbol->type = type;
  new_symbol->num = num;
  new_symbol->data_type = data_type;
  map_put(current_local_symbols, name_perm, (void *)new_symbol);
}

DataType *conv_data_type_node_to_data_type(Node *node) {
  if (node->ty != ND_DATATYPE) {
    error("%s", "Not a data type node", __FILE__, __LINE__);
  }
  DataType *data_type;
  if (node->lhs->ty == ND_VOID) {
    data_type = new_data_type(DT_VOID);
  } else if (node->lhs->ty == ND_INT) {
    data_type = new_data_type(DT_INT);
  } else if (node->lhs->ty == ND_CHAR) {
    data_type = new_data_type(DT_CHAR);
  } else if (node->lhs->ty == ND_STRUCT) {
    data_type = new_data_type(DT_STRUCT);
    data_type->struct_name = node->lhs->name;
  } else if (node->lhs->ty == ND_PNT) {
    data_type = new_data_type(DT_PNT);
    data_type->pointer_type = conv_data_type_node_to_data_type(node->rhs);
  } else {
    error("%s", "Invalid data type pointer", __FILE__, __LINE__);
  }
  return data_type;
}

void add_local_node_to_table(DataType *data_type, Node *node) {
  if (node->ty == ND_IDENTSEQ) {
    add_local_node_to_table(data_type, node->lhs);
    add_local_node_to_table(data_type, node->rhs);
    return;
  }
  if (node->ty == ND_IDENT) {
    if (node->val > 0) {
      data_type = new_data_pointer(data_type);
    }
    add_local_symbol(node->name, ID_VAR, node->val, data_type);
    return;
  }
}

void process_local_block(Vector *block_code);

void process_local_node(Node *node) {
  if (node == NULL) {
    return;
  }
  if (node->ty == ND_DECLARE) {
    DataType *data_type = conv_data_type_node_to_data_type(node->lhs);
    add_local_node_to_table(data_type, node->rhs);
  }
  process_local_node(node->rhs);
  process_local_node(node->lhs);
  process_local_block(node->block);
}

void process_local_block(Vector *block_code) {
  if (block_code == NULL) {
    return;
  }
  Node *node;
  for (int i = 0; (node = block_code->data[i]); i++) {
    process_local_node(node);
  }
}


StructMember *new_struct_member() {
  StructMember *sm = malloc(sizeof(StructMember));
  sm->address = 0;
  sm->data_type = NULL;
  sm->name = NULL;
  return sm;
}

// parse.c.
Node *get_data_type_from_struct_node(Node *struct_node);

int add_struct_member(Vector *member_list, Node *node, int address) {
  if (node->ty == ND_IDENTSEQ) {
    address = add_struct_member(member_list, node->lhs, address);
    if (node->rhs) {
      address = add_struct_member(member_list, node->rhs, address);
    }
    return address;
  }
  if (node->ty == ND_DECLARE) {
    StructMember *st = new_struct_member();
    st->data_type = conv_data_type_node_to_data_type(node->lhs);
    st->address = address;
    st->name = node->rhs->name;
    vec_push(member_list, st);
    return address + data_size(st->data_type);
  }
  fprintf(stderr, "Node type %d is not expected. ", node->ty);
  error("%s", "Error", __FILE__, __LINE__);
  return -1;
}

void process_top_level_node(DataType *data_type, Node *node) {
  if (node->ty == ND_IDENTSEQ) {
    process_top_level_node(data_type, node->lhs);
    process_top_level_node(data_type, node->rhs);
    return;
  }
  if (node->ty == ND_IDENT) {
    if (node->val > 0) {
      data_type = new_data_pointer(data_type);
    }
    add_global_symbol(node->name, ID_VAR, node->val, data_type);
    return;
  }
  if (node->ty == ND_FUNCDEF) {
    add_global_symbol(node->lhs->name, ID_FUNC, node->lhs->val, data_type);
    Node *args = node->rhs;
    if (args != NULL) {
      // This function has an argument.
      if (args->ty != ND_DECLARE) {
        error("%s", "Argument not starting with declaration", __FILE__, __LINE__);
      }
      DataType *arg_data_type = conv_data_type_node_to_data_type(args->lhs);
      add_local_symbol(args->rhs->name, ID_ARG, args->rhs->val, arg_data_type);
    }
    if (node->block != NULL) {
      process_local_block(node->block);
    }
    return;
  }
  error("%s", "Identifier not found.", __FILE__, __LINE__);
}

Vector *analysis(Vector *program_code) {
  Node *node;
  local_symbols = new_vector();
  // Generate global table and local tables
  for (int i = 0; (node = program_code->data[i]); i++) {
    if (node->ty == ND_DECLARE) {
      current_local_symbols = new_map();
      vec_push(local_symbols, current_local_symbols);
      local_symbol_counter = 0;
      DataType *data_type = conv_data_type_node_to_data_type(node->lhs);
      process_top_level_node(data_type, node->rhs);
    } else if (node->ty == ND_STRUCT) {
      // Just to avoid error.
      vec_push(local_symbols, NULL);
      Vector *struct_member_list = new_vector();
      map_put(global_struct_table, node->name, struct_member_list);
      add_struct_member(struct_member_list, node->lhs, 0);
    } else {
      vec_push(local_symbols, NULL);
    }
  }
  vec_push(local_symbols, NULL);

  // Extract string literals.
  str_counter = 0;
  local_symbol_counter = 0;
  for (int i = 0; (node = program_code->data[i]); i++) {
    current_local_symbols = local_symbols->data[i];
    list_string_in_node(node);
  }
  // TODO: add analysis of pointer addresses.
  return program_code;
}
