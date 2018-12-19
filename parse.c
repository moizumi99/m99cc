#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "m99cc.h"

#define GET_TOKEN(T, I) (*((Token *)(T)->data[(I)]))

static Vector *tokens;

extern Map *global_symbols;
extern Vector *local_symbols;
static Map *current_local_symbols;

// Error reporting function.
void error(char *s, char *message) {
  fprintf(stderr, s, message);
  fprintf(stderr, "\n");
  exit(1);
}

int pos;

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
            GET_TOKEN(tokens, pos-1).input );
    exit(1);
  }
  return num;
}

void add_global_symbol(char *name_perm, int type, int num, int dtype) {
  static int global_symbol_counter = 0;
  Symbol *new_symbol = malloc(sizeof(Symbol));
  int dsize = data_size(dtype);
  global_symbol_counter += (num == 0) ? dsize : num * dsize;
  new_symbol->address = (void *) global_symbol_counter;
  new_symbol->type = type;
  new_symbol->num = num;
  new_symbol->dtype = dtype;
  map_put(global_symbols, name_perm, (void *) new_symbol);
}

void *get_symbol_address(Map *symbols, char *name) {
  Symbol *tmp_symbol = map_get(symbols, name);
  if (tmp_symbol == NULL) {
    return NULL;
  }
  return tmp_symbol->address;
}

int get_symbol_size(Map *symbols, char *name) {
  Symbol *tmp_symbol = map_get(symbols, name);
  if (tmp_symbol == NULL) {
    return -1;
  }
  return tmp_symbol->num;
}

int get_symbol_type(Map *symbols, char *name) {
  Symbol *tmp_symbol = map_get(symbols, name);
  if (tmp_symbol == NULL) {
    return DT_INVALID;
  }
  return tmp_symbol->dtype;
}

int data_size(int dtype) {
  switch(dtype) {
  case DT_VOID: return 8;
  case DT_INT: return 8;
  case DT_CHAR: return 1;
  default: return 8;
  }
}

int get_symbol_datasize(Map *symbols, char *name) {
  Symbol *tmp_symbol = map_get(symbols, name);
  return data_size(tmp_symbol->dtype);
}

static int local_symbol_counter = 0;
void add_local_symbol(char *name_perm, int type, int num, int dtype) {
  Symbol *new_symbol = malloc(sizeof(Symbol));
  int dsize = data_size(dtype);
  local_symbol_counter += (num == 0) ? dsize : num * dsize;
  new_symbol->address = (void *) local_symbol_counter;
  new_symbol->type = type;
  new_symbol->num = num;
  new_symbol->dtype = dtype;
  map_put(current_local_symbols, name_perm, (void *) new_symbol);
}

char *create_name_perm(char *name, int len) {
  char *str = malloc(sizeof(char) * IDENT_LEN);
  int copy_len = (len < IDENT_LEN - 1) ? len : IDENT_LEN - 1;
  strncpy(str, name, copy_len);
  str[len] = '\0';
  return str;
}

Node *new_node_ident(int val, char *name, int len) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->lhs = NULL;
  node->rhs = NULL;
  node->val = val;
  node->name = create_name_perm(name, len);
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
  switch(token_type) {
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
  /* case TK_AND: */
    /* return AND_PRIORITY; */
  /* case TK_OR: */
  /*   return OR_PRIORITY */
  case '=':
    return ASSIGN_PRIORITY;
  default:
    return LOW_PRIORITY;
  }
}

int get_node_type(int token_type) {
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
  default:
    // For other operations (*/+- others, token_type -> node_type)
    return token_type;
  }
}

int get_data_type(int token_type) {
  switch(token_type) {
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
    int node_type = get_node_type(token_type);
    return new_node(node_type, lhs, expression(priority));
  }
  return lhs;
}

// term : number | identifier | ( expression )
Node *term() {
  // Simple number
  if (GET_TOKEN(tokens, pos).ty == TK_NUM) {
    return new_node_num(GET_TOKEN(tokens, pos++).val);
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
      pos++;
      Node *index = expression(ASSIGN_PRIORITY);
      if (GET_TOKEN(tokens, pos++).ty != ']') {
        fprintf(stderr, "Closing bracket ']' missing. \"%s\"\n", GET_TOKEN(tokens, pos - 1).input);
        exit(1);
      }
      Node *offset = new_node('*', index, new_node_num(8));
      node = new_node('*', NULL, new_node('+', id, offset));
    }
    if (map_get(global_symbols, id->name) == NULL &&
        map_get(current_local_symbols, id->name) == NULL) {
      error("A symbold (\"%s\" is used without declaration.", id->name);
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
  if (GET_TOKEN(tokens, pos).ty == '+' || GET_TOKEN(tokens, pos).ty == '-' || GET_TOKEN(tokens, pos).ty == '*' || GET_TOKEN(tokens, pos).ty == '&') {
    int type = GET_TOKEN(tokens, pos).ty;
    pos++;
    Node *rhs = term();
    return new_node(type, NULL, rhs);
  }
  // Code should not reach here.
  error("Unexpected token (parse.c term): \"%s\"",
        GET_TOKEN(tokens, pos).input);
  return NULL;
}

Node *argument() {
  // TODO: make argument a list.
  int dtype = get_data_type(GET_TOKEN(tokens, pos++).ty);
  if (dtype == DT_INVALID) {
    error("Invalid (not data type) token in argument declaration position \"%s\"",
          GET_TOKEN(tokens, pos - 1).input);
  }
  if (GET_TOKEN(tokens, pos).ty != TK_IDENT) {
    error("Invalid (not IDENT) token in argument declaration position \"%s\"",
          GET_TOKEN(tokens, pos).input);
  }
  // argument name?
  char *name = create_name_perm(GET_TOKEN(tokens, pos).input, GET_TOKEN(tokens, pos).len);
  if (map_get(current_local_symbols, name) != NULL) {
    error("Argument name conflict: %s\n", name);
  }
  add_local_symbol(name, ID_ARG, get_array_size(), dtype);
  Node *id = new_node_ident(GET_TOKEN(tokens, pos).val, GET_TOKEN(tokens, pos).input,
                              GET_TOKEN(tokens, pos).len);
  pos++;
  return id;
}

Node *assign_dash() {
  Node *lhs = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos).ty == '=') {
    pos++;
    return new_node('=', lhs, assign_dash());
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

void declaration_node(Vector *code);

void code_block(Vector *code) {
  if (GET_TOKEN(tokens, pos++).ty != '{') {
    error("Left brace '{' missing (code_block): \"%s\"", GET_TOKEN(tokens, pos - 1).input);
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
    } else if ( get_data_type((GET_TOKEN(tokens, pos).ty)) != DT_INVALID) {
      // TODO: declaration_node inside function can not generate function. Check.
      declaration_node(code);
    } else {
      vec_push(code, assign());
    }
  }
  if (GET_TOKEN(tokens, pos++).ty != '}') {
    error("Right brace '}' missing (code_block): \"%s\"", GET_TOKEN(tokens, pos - 1).input);
  }
}

Node *identifier_node(int dtype) {
  if (GET_TOKEN(tokens, pos).ty != TK_IDENT) {
    error("Unexpected token (function): \"%s\"", GET_TOKEN(tokens, pos).input);
  }
  Node *id = new_node_ident(GET_TOKEN(tokens, pos).val, GET_TOKEN(tokens, pos).input,
                            GET_TOKEN(tokens, pos).len);
  if (map_get(global_symbols, id->name) != NULL) {
    error("Global name conflict. \"%s\"", id->name);
    return NULL;
  }
  pos++;
  // global variable.
  if (GET_TOKEN(tokens, pos).ty != '(') {
    int num = get_array_size();
    add_global_symbol(id->name, ID_VAR, num, dtype);
    // TODO: add initialization.
    return id;
  }
  // TODO: implement function declaration.
  // function definition..
  add_global_symbol(id->name, ID_FUNC, 0, dtype);
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

Node *identifier_sequence(int dtype) {
  Node *id = identifier_node(dtype);
  if (GET_TOKEN(tokens, pos).ty == ',') {
    pos++;
    Node *ids = identifier_sequence(dtype);
    return new_node(ND_IDENTSEQ, id, ids);
  }
  if (GET_TOKEN(tokens, pos).ty == ';') {
    pos++;
    return id;
  }
  // error("Invalid token (parse.c, identifier_sequence())): \"%s\"", GET_TOKEN(tokens, pos).input);
  return id;
}

Node *new_node_datatype(int data_type) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_DATATYPE;
  node->lhs = NULL;
  node->rhs = NULL;
  node->val = data_type;
  node->name = NULL;
  node->block = NULL;
  return node;
}

void declaration_node(Vector *code) {
  int data_type = get_data_type(GET_TOKEN(tokens, pos++).ty);
  if (data_type == DT_INVALID) {
    error("Data type needed before declaration of function or variable. (\"%s\")",
          GET_TOKEN(tokens, pos-1).input);
  }
  Node *node_dt = new_node_datatype(data_type);
  Node *node_ids = identifier_sequence(data_type);
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
    declaration_node(code);
  }
  vec_push(code, NULL);
  vec_push(local_symbols, NULL);
  return code;
}

