#include "m99cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define GET_TOKEN(T, I) (*((Token *)(T)->data[(I)]))

static Vector *tokens;
extern Vector *string_literals;

extern Map *global_symbols;
extern Vector *local_symbols;
static Map *current_local_symbols;

enum {
  SC_GLOBAL, // Global scope.
  SC_LOCAL   // Local scope.
};

// Error reporting function.
void error(char *s, char *message) {
  fprintf(stderr, s, message);
  fprintf(stderr, "\n");
  exit(1);
}

char *create_string_in_heap(char *str, int len) {
  char *str_mem = malloc(sizeof(char) * IDENT_LEN);
  int copy_len = (len < IDENT_LEN - 1) ? len : IDENT_LEN - 1;
  strncpy(str_mem, str, copy_len);
  str_mem[len] = '\0';
  return str_mem;
}

int pos;

static int str_counter = 0;

Node *new_node_str(char *name, int len) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_STR;
  node->lhs = NULL;
  node->rhs = NULL;
  node->val = str_counter;
  node->name = NULL;
  node->block = NULL;
  vec_push(string_literals, create_string_in_heap(name, len));
  return node;
}

Node *new_node(int op, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = op;
  node->lhs = lhs;
  node->rhs = rhs;
  node->val = 0;
  node->name = NULL;
  node->block = NULL;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  node->lhs = NULL;
  node->rhs = NULL;
  node->name = NULL;
  node->block = NULL;
  return node;
}

int get_array_size() {
  if (GET_TOKEN(tokens, pos).ty != '[') {
    // This is not an array.
    return 0;
  }
  // it's an array declaration.
  ++pos;
  int num = 0;
  if (GET_TOKEN(tokens, pos).ty == TK_NUM) {
    num = GET_TOKEN(tokens, pos++).val;
  }
  if (GET_TOKEN(tokens, pos++).ty != ']') {
    fprintf(stderr, "Closing bracket ']' is missing (get_array_size): \"%s\"\n",
            GET_TOKEN(tokens, pos - 1).input);
    exit(1);
  }
  return num;
}

void add_global_symbol(char *name_perm, int type, int num,
                       struct DataType *data_type) {
  static int global_symbol_counter = 0;
  Symbol *new_symbol = malloc(sizeof(Symbol));
  int dsize = data_size(data_type);
  global_symbol_counter += (num == 0) ? dsize : num * dsize;
  new_symbol->address = (void *)global_symbol_counter;
  new_symbol->type = type;
  new_symbol->num = num;
  new_symbol->data_type = data_type;
  map_put(global_symbols, name_perm, (void *)new_symbol);
}

/* int get_symbol_type(char *name) { */
/*   Symbol *tmp_symbol = map_get(current_local_symbols, name); */
/*   if (tmp_symbol == NULL) { */
/*     tmp_symbol = map_get(global_symbols, name); */
/*   } */
/*   if (tmp_symbol->data_type == NULL) { */
/*     return DT_INVALID; */
/*   } */
/*   return tmp_symbol->data_type->dtype; */
/* } */

Symbol *get_symbol(Map *global_symbol_table, Map *local_symbol_table, Node *node) {
  Symbol *s = map_get(local_symbol_table, node->name);
  if (s == NULL) {
    s = map_get(global_symbol_table, node->name);
  }
  return s;
}

int data_size_from_dtype(int dtype) {
  switch (dtype) {
  case DT_VOID:
    // shouldn't this be zero?
    return 8;
  case DT_INT:
    return 8;
  case DT_CHAR:
    return 1;
  case DT_PNT:
    return 8;
  default:
    return 8;
  }
}

int data_size(DataType *data_type) {
  return data_size_from_dtype(data_type->dtype);
}

static int local_symbol_counter = 0;
void add_local_symbol(char *name_perm, int type, int num, DataType *data_type) {
  Symbol *new_symbol = malloc(sizeof(Symbol));
  int dsize = data_size(data_type);
  local_symbol_counter += (num == 0) ? dsize : num * dsize;
  new_symbol->address = (void *)local_symbol_counter;
  new_symbol->type = type;
  new_symbol->num = num;
  new_symbol->data_type = data_type;
  map_put(current_local_symbols, name_perm, (void *)new_symbol);
}

Node *new_node_ident(int val, char *name, int len) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->lhs = NULL;
  node->rhs = NULL;
  node->val = val;
  node->name = create_string_in_heap(name, len);
  node->block = NULL;
  return node;
}

#define HIGH_PRIORITY (8)
#define MULTIPLE_PRIORITY (HIGH_PRIORITY - 1)
#define ADD_PRIORITY (HIGH_PRIORITY - 2)
#define COMPARE_PRIORITY (HIGH_PRIORITY - 3)
#define EQUAL_PRIORITY (HIGH_PRIORITY - 4)
#define AND_PRIORITY (HIGH_PRIORITY - 5)
#define OR_PRIORITY (HIGH_PRIORITY - 6)
#define ASSIGN_PRIORITY (HIGH_PRIORITY - 7)
#define LOW_PRIORITY (0)

int operation_priority(int token_type) {
  switch (token_type) {
  case '*':
  case '/':
  case '%':
    return MULTIPLE_PRIORITY;
  case '+':
  case '-':
    return ADD_PRIORITY;
  case '<':
  case '>':
  case TK_GE:
  case TK_LE:
    return COMPARE_PRIORITY;
  case TK_EQ:
  case TK_NE:
    return EQUAL_PRIORITY;
  case TK_AND:
    return AND_PRIORITY;
  case TK_OR:
    return OR_PRIORITY;
  case '=':
    return ASSIGN_PRIORITY;
  default:
    return LOW_PRIORITY;
  }
}

int get_node_type_from_token(int token_type) {
  switch (token_type) {
  case TK_NUM:
    return ND_NUM;
  case TK_IDENT:
    return ND_IDENT;
  case TK_EQ:
    return ND_EQ;
  case TK_NE:
    return ND_NE;
  case TK_LE:
    return ND_LE;
  case TK_GE:
    return ND_GE;
  case TK_AND:
    return ND_AND;
  case TK_OR:
    return ND_OR;
  case TK_STR:
    return ND_STR;
  case TK_INC:
    return ND_INC;
  case TK_DEC:
    return ND_DEC;
  case TK_PE:
    return ND_PE;
  case TK_ME:
    return ND_ME;
  default:
    // For other operations (*/+- others, token_type -> node_type)
    return token_type;
  }
}

int get_node_type_from_token_single(int token_type) {
  if (token_type == '*') {
    return ND_DEREF;
  }
  return get_node_type_from_token(token_type);
}

int get_data_type_from_token(int token_type) {
  switch (token_type) {
  case TK_VOID:
    return DT_VOID;
  case TK_CHAR:
    return DT_CHAR;
  case TK_INT:
    return DT_INT;
  default:
    return DT_INVALID;
  }
}

int get_data_type(int p, DataType **data_type) {
  int dtype = get_data_type_from_token(GET_TOKEN(tokens, p++).ty);
  DataType *dt = new_data_type(dtype);
  if (dtype != DT_INVALID) {
    while (GET_TOKEN(tokens, p).ty == '*') {
      p++;
      dt = new_data_pointer(dt);
    }
  }
  *data_type = dt;
  return p;
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

DataType *get_node_data_type(Node *node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->ty == ND_IDENT) {
    Symbol *s = get_symbol(global_symbols, current_local_symbols, node);
    if (s == NULL) {
      fprintf(stderr, "Symbol %s not found\n", node->name);
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
    DataType *dt = get_node_data_type(node->lhs);
    if (node->ty == '*') {
      return dt->pointer_type;
    }
    if (node->ty == '&') {
      return new_data_pointer(dt);
    }
    return dt;
  }
  DataType *left_data_type = get_node_data_type(node->lhs);
  DataType *right_data_type = get_node_data_type(node->rhs);
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
  // Suppport other data steps, like short.
  return 8;
}

int get_data_step_from_node(Node *node) {
  DataType *dt = get_node_data_type(node);
  return get_data_step_from_data_type(dt);
}

Node *term();
Node *argument();

Node *expression(int priority) {
  if (priority == HIGH_PRIORITY) {
    return term();
  }
  Node *lhs = expression(priority + 1);
  if (GET_TOKEN(tokens, pos).ty == TK_EOF) {
    return lhs;
  }
  int token_type = GET_TOKEN(tokens, pos).ty;
  if (operation_priority(token_type) == priority) {
    pos++;
    int node_type = get_node_type_from_token(token_type);
    Node *rhs = expression(priority);
    if (node_type == '+' || node_type == '-') {
      DataType *ldt = get_node_data_type(lhs);
      DataType *rdt = get_node_data_type(rhs);
      if (ldt->dtype == DT_PNT && rdt->dtype != DT_PNT) {
        int step = get_data_step_from_data_type(ldt);
        rhs = new_node('*', new_node_num(step), rhs);
      } else if (ldt->dtype != DT_PNT && rdt->dtype == DT_PNT) {
        int step = get_data_step_from_data_type(rdt);
        lhs = new_node('*', new_node_num(step), lhs);
      }
    }
    return new_node(node_type, lhs, rhs);
  }
  return lhs;
}

// term : number | identifier | ( expression )
Node *term() {
  // Simple number
  if (GET_TOKEN(tokens, pos).ty == TK_NUM) {
    return new_node_num(GET_TOKEN(tokens, pos++).val);
  }

  if (GET_TOKEN(tokens, pos).ty == TK_STR) {
    char *str = GET_TOKEN(tokens, pos).input;
    int len = GET_TOKEN(tokens, pos).len;
    pos++;
    return new_node_str(str, len);
  }

  // Variable or Function
  if (GET_TOKEN(tokens, pos).ty == TK_IDENT) {
    Node *id = new_node_ident(0, GET_TOKEN(tokens, pos).input,
                              GET_TOKEN(tokens, pos).len);
    pos++;
    // if followed by (, it's a function call.
    if (GET_TOKEN(tokens, pos).ty == '(') {
      ++pos;
      Node *arg = NULL;
      // Argument exists.
      if (GET_TOKEN(tokens, pos).ty != ')') {
        arg = expression(ASSIGN_PRIORITY);
      }
      if (GET_TOKEN(tokens, pos).ty != ')') {
        error("No right parenthesis corresponding to left parenthesis"
              " (term, function): \"%s\"",
              GET_TOKEN(tokens, pos).input);
      }
      pos++;
      Node *node = new_node(ND_FUNCCALL, id, arg);
      return node;
    }
    // If not followed by (, it's a variable.
    // is it array?
    Node *node = id;
    if (GET_TOKEN(tokens, pos).ty == '[') {
      Symbol *s = map_get(current_local_symbols, id->name);
      if (s == NULL) {
        s = map_get(global_symbols, id->name);
      }
      if (s == NULL) {
        error("A symbold (\"%s\" is used without declaration.", id->name);
      }
      DataType *data_type = s->data_type;
      pos++;
      Node *index = expression(ASSIGN_PRIORITY);
      if (GET_TOKEN(tokens, pos++).ty != ']') {
        fprintf(stderr, "Closing bracket ']' missing. \"%s\"\n",
                GET_TOKEN(tokens, pos - 1).input);
        exit(1);
      }
      if (data_type->dtype != DT_PNT) {
        fprintf(stderr,
                "Array type is declared as regular type not pointer. %s\n",
                id->name);
        exit(1);
      }
      int step = data_size(data_type->pointer_type);
      Node *offset = new_node('*', index, new_node_num(step));
      node = new_node(ND_DEREF, new_node('+', id, offset), NULL);
    }
    return node;
  }
  // "( expression )"
  if (GET_TOKEN(tokens, pos).ty == '(') {
    pos++;
    Node *node = expression(ASSIGN_PRIORITY);
    if (GET_TOKEN(tokens, pos).ty != ')') {
      error("No right parenthesis corresponding to left parenthesis "
            "(term, parenthesis): \"%s\"",
            GET_TOKEN(tokens, pos).input);
    }
    pos++;
    return node;
  }
  // Single term operators
  if (GET_TOKEN(tokens, pos).ty == '+' || GET_TOKEN(tokens, pos).ty == '-' ||
      GET_TOKEN(tokens, pos).ty == '*' || GET_TOKEN(tokens, pos).ty == '&' ||
      GET_TOKEN(tokens, pos).ty == TK_INC ||
      GET_TOKEN(tokens, pos).ty == TK_DEC) {
    int type = GET_TOKEN(tokens, pos).ty;
    pos++;
    Node *lhs = term();
    return new_node(get_node_type_from_token_single(type), lhs, NULL);
  }
  // Code should not reach here.
  error("Unexpected token (parse.c term): \"%s\"",
        GET_TOKEN(tokens, pos).input);
  return NULL;
}

Node *argument() {
  // TODO: make argument a list.
  DataType *data_type;
  pos = get_data_type(pos, &data_type);
  if (data_type->dtype == DT_INVALID) {
    error(
        "Invalid (not data type) token in argument declaration position \"%s\"",
        GET_TOKEN(tokens, pos - 1).input);
  }
  if (GET_TOKEN(tokens, pos).ty != TK_IDENT) {
    error("Invalid (not IDENT) token in argument declaration position \"%s\"",
          GET_TOKEN(tokens, pos).input);
  }
  // argument name?
  char *name = create_string_in_heap(GET_TOKEN(tokens, pos).input,
                                     GET_TOKEN(tokens, pos).len);
  if (map_get(current_local_symbols, name) != NULL) {
    error("Argument name conflict: %s\n", name);
  }
  int array_size = get_array_size();
  if (array_size > 0) {
    data_type = new_data_pointer(data_type);
  }
  add_local_symbol(name, ID_ARG, array_size, data_type);
  Node *id =
      new_node_ident(GET_TOKEN(tokens, pos).val, GET_TOKEN(tokens, pos).input,
                     GET_TOKEN(tokens, pos).len);
  pos++;
  return id;
}

Node *assign_dash() {
  Node *lhs = expression(ASSIGN_PRIORITY);
  int token_type = GET_TOKEN(tokens, pos).ty;
  if (token_type == '=' || token_type == TK_PE || token_type == TK_ME) {
    pos++;
    return new_node(get_node_type_from_token(token_type), lhs, assign_dash());
  }
  return lhs;
}

Node *assign() {
  Node *lhs = assign_dash();
  if (GET_TOKEN(tokens, pos).ty == '(') {
    while (GET_TOKEN(tokens, pos).ty != ')') {
      ++pos;
    }
    return lhs;
  }
  while (GET_TOKEN(tokens, pos).ty == ';') {
    pos++;
  }
  return lhs;
}

void add_code(Vector *code, int i) {
  if (code->len > i)
    return;
  vec_push(code, malloc(sizeof(Node)));
}

void code_block(Vector *code);

Node *if_node() {
  pos++;
  if (GET_TOKEN(tokens, pos++).ty != '(') {
    error("Left paraenthesis '(' missing (if): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos++).ty != ')') {
    error("Right paraenthesis ')' missing (if): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *ifnd = new_node(ND_IF, cond, NULL);
  ifnd->block = new_vector();
  code_block(ifnd->block);
  if (GET_TOKEN(tokens, pos).ty == TK_ELSE) {
    pos++;
    if (GET_TOKEN(tokens, pos).ty == TK_IF) {
      ifnd->rhs = if_node();
    } else {
      ifnd->rhs = new_node(ND_BLOCK, NULL, NULL);
      ifnd->rhs->block = new_vector();
      code_block(ifnd->rhs->block);
      vec_push(ifnd->rhs->block, NULL);
    }
  }
  vec_push(ifnd->block, NULL);
  // TODO: Add else
  return ifnd;
}

Node *while_node() {
  pos++;
  if (GET_TOKEN(tokens, pos++).ty != '(') {
    error("Left paraenthesis '(' missing (while): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos++).ty != ')') {
    error("Right paraenthesis ')' missing (while): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *while_nd = new_node(ND_WHILE, cond, NULL);
  while_nd->block = new_vector();
  code_block(while_nd->block);
  vec_push(while_nd->block, NULL);
  return while_nd;
}

void for_node(Vector *code) {
  // TODO: allow multiple statement using ','.
  pos++;
  if (GET_TOKEN(tokens, pos++).ty != '(') {
    error("Left paraenthesis '(' missing (for): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *init = expression(ASSIGN_PRIORITY);
  vec_push(code, init);
  if (GET_TOKEN(tokens, pos++).ty != ';') {
    error("1st Semicolon ';' missing (for): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos++).ty != ';') {
    error("2nd Semicolon ';' missing (for): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *increment = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos++).ty != ')') {
    error("Right paraenthesis '(' missing (for): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *for_nd = new_node(ND_WHILE, cond, NULL);
  vec_push(code, for_nd);
  // Add the increment code and {} block.
  for_nd->block = new_vector();
  code_block(for_nd->block);
  vec_push(for_nd->block, increment);
  vec_push(for_nd->block, NULL);
}

Node *return_node() {
  pos++;
  Node *nd = expression(ASSIGN_PRIORITY);
  Node *return_nd = new_node(ND_RETURN, nd, NULL);
  while (GET_TOKEN(tokens, pos).ty == ';') {
    pos++;
  }
  return return_nd;
}

void declaration_node(Vector *code, int scope);

void code_block(Vector *code) {
  if (GET_TOKEN(tokens, pos++).ty != '{') {
    error("Left brace '{' missing (code_block): \"%s\"",
          GET_TOKEN(tokens, pos - 1).input);
  }
  while (GET_TOKEN(tokens, pos).ty != '}') {
    if (GET_TOKEN(tokens, pos).ty == TK_IF) {
      vec_push(code, if_node());
    } else if (GET_TOKEN(tokens, pos).ty == TK_WHILE) {
      vec_push(code, while_node());
    } else if (GET_TOKEN(tokens, pos).ty == TK_FOR) {
      for_node(code);
    } else if (GET_TOKEN(tokens, pos).ty == TK_RETURN) {
      vec_push(code, return_node());
    } else {
      DataType *data_type;
      get_data_type(pos, &data_type);
      if (data_type->dtype != DT_INVALID) {
        // TODO: declaration_node inside function can not generate function.
        // Check.
        declaration_node(code, SC_LOCAL);
      } else {
        vec_push(code, assign());
      }
    }
  }
  if (GET_TOKEN(tokens, pos++).ty != '}') {
    error("Right brace '}' missing (code_block): \"%s\"",
          GET_TOKEN(tokens, pos - 1).input);
  }
}

int check_conflict(char *name, int scope) {
  if (map_get(global_symbols, name) != NULL) {
    error("Global name conflict. \"%s\"", name);
    return -1;
  }
  if (scope == SC_LOCAL && map_get(current_local_symbols, name) != NULL) {
    error("Local name conflict. \"%s\"", name);
    return -1;
  }
  return 0;
}

Node *identifier_node(DataType *data_type, int scope) {
  if (GET_TOKEN(tokens, pos).ty != TK_IDENT) {
    error("Unexpected token (function): \"%s\"", GET_TOKEN(tokens, pos).input);
  }
  Node *id =
      new_node_ident(GET_TOKEN(tokens, pos).val, GET_TOKEN(tokens, pos).input,
                     GET_TOKEN(tokens, pos).len);
  pos++;
  check_conflict(id->name, scope);
  // global or local variable.
  if (GET_TOKEN(tokens, pos).ty != '(') {
    int num = get_array_size();
    if (num > 0) {
      data_type = new_data_pointer(data_type);
    }
    if (scope == SC_GLOBAL) {
      add_global_symbol(id->name, ID_VAR, num, data_type);
    } else {
      add_local_symbol(id->name, ID_VAR, num, data_type);
    }
    // TODO: add initialization.
    return id;
  }
  // TODO: implement function declaration.
  // function definition..
  if (scope != SC_GLOBAL) {
    error("Function declaration is allowed only"
          " in global scope. \"%s\"\n",
          id->name);
    return NULL;
  }
  add_global_symbol(id->name, ID_FUNC, 0, data_type);
  Node *arg = NULL;
  pos++;
  if (GET_TOKEN(tokens, pos).ty != ')') {
    arg = argument();
  }
  if (GET_TOKEN(tokens, pos++).ty != ')') {
    // TODO: add support of nmultiple arguments.
    error("Right parenthesis ')' missing (function): \"%s\"",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *f = new_node(ND_FUNCDEF, id, arg);
  f->block = new_vector();
  code_block(f->block);
  vec_push(f->block, NULL);
  return f;
}

Node *identifier_sequence(DataType *data_type, int scope) {
  Node *id = identifier_node(data_type, scope);
  if (GET_TOKEN(tokens, pos).ty == ',') {
    pos++;
    Node *ids = identifier_sequence(data_type, scope);
    return new_node(ND_IDENTSEQ, id, ids);
  }
  if (GET_TOKEN(tokens, pos).ty == ';') {
    pos++;
    return id;
  }
  // error("Invalid token (parse.c, identifier_sequence())): \"%s\"",
  // GET_TOKEN(tokens, pos).input);
  return id;
}

Node *new_node_datatype() {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_DATATYPE;
  node->lhs = NULL;
  node->rhs = NULL;
  node->val = 0;
  node->name = NULL;
  node->block = NULL;
  return node;
}

void declaration_node(Vector *code, int scope) {
  DataType *data_type;
  pos = get_data_type(pos, &data_type);
  if (data_type->dtype == DT_INVALID) {
    error(
        "Data type needed before declaration of function or variable. (\"%s\")",
        GET_TOKEN(tokens, pos - 1).input);
  }
  Node *node_dt = new_node_datatype();
  Node *node_ids = identifier_sequence(data_type, scope);
  Node *declaration = new_node(ND_DECLARE, node_dt, node_ids);
  vec_push(code, declaration);
  return;
}

Vector *parse(Vector *tokens_input) {
  tokens = tokens_input;
  Vector *code = new_vector();
  while (GET_TOKEN(tokens, pos).ty != TK_EOF) {
    current_local_symbols = new_map();
    local_symbol_counter = 0;
    vec_push(local_symbols, current_local_symbols);
    declaration_node(code, SC_GLOBAL);
  }
  vec_push(code, NULL);
  vec_push(local_symbols, NULL);
  return code;
}
