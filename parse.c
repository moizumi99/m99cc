#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "m99cc.h"

extern Vector *tokens;
// TODO: separate local and global symbol table.
extern Map *global_symbols;
extern Vector *local_symbols;
extern Map *current_local_symbols;

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
  if (GET_TOKEN(pos).ty != '[') {
    // This is not an array.
    return 0;
  }
  // it's an array declaration.
  ++pos;
  int num = 0;
  if (GET_TOKEN(pos).ty == TK_NUM) {
    num = GET_TOKEN(pos++).val;
  }
  if (GET_TOKEN(pos++).ty != ']') {
    fprintf(stderr, "Closing bracket ']' is missing (get_array_size): \"%s\"\n",
            GET_TOKEN(pos-1).input );
    exit(1);
  }
  return num;
}

void add_global_symbol(char *name_perm, int type, int num) {
  static int global_symbol_counter = 0;
  Symbol *new_symbol = malloc(sizeof(Symbol));
  global_symbol_counter += (num == 0) ? 1 : num;
  new_symbol->address = (void *) (global_symbol_counter * 8);
  new_symbol->type = type;
  new_symbol->num = num;
  map_put(global_symbols, name_perm, (void *) new_symbol);
}

void *get_global_symbol(char *name) {
  return map_get(global_symbols, name);
}

void *get_global_symbol_address(char *name) {
  Symbol *tmp_symbol = get_global_symbol(name);
  if (tmp_symbol == NULL) {
    return NULL;
  }
  return tmp_symbol->address;
}

int get_global_symbol_size(char *name) {
  Symbol *tmp_symbol = get_global_symbol(name);
  if (tmp_symbol == NULL) {
    return -1;
  }
  return tmp_symbol->num;
}

static int local_symbol_counter = 0;
void add_local_symbol(char *name_perm, int type, int num) {
  Symbol *new_symbol = malloc(sizeof(Symbol));
  local_symbol_counter += (num == 0) ? 1 : num;
  new_symbol->address = (void *) (local_symbol_counter * 8);
  new_symbol->type = type;
  new_symbol->num = num;
  map_put(current_local_symbols, name_perm, (void *) new_symbol);
}

void *get_local_symbol(char *name) {
  return map_get(current_local_symbols, name);
}

void *get_local_symbol_address(char *name) {
  Symbol *tmp_symbol = get_local_symbol(name);
  if (tmp_symbol == NULL) {
    return NULL;
  }
  return tmp_symbol->address;
}

int get_local_symbol_size(char *name) {
  Symbol *tmp_symbol = get_local_symbol(name);
  if (tmp_symbol == NULL) {
    return -1;
  }
  return tmp_symbol->num;
}

char *create_name_perm(char *name, int len) {
  char *str = malloc(sizeof(char) * IDENT_LEN);
  int copy_len = (len < IDENT_LEN) ? len : IDENT_LEN;
  strncpy(str, name, copy_len);
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
  /* case TK_LE: */
  /* case TK_GE: */
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
  default:
    // For other operations (*/+- others, token_type -> node_type)
    return token_type;
  }
}

Node *term();
Node *argument();

Node *expression(int priority) {
  if (priority == HIGH_PRIORITY) {
    return term();
  }
  Node *lhs = expression(priority + 1);
  if (GET_TOKEN(pos).ty == TK_EOF) {
    return lhs;
  }
  int token_type = GET_TOKEN(pos).ty;
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
  if (GET_TOKEN(pos).ty == TK_NUM) {
    return new_node_num(GET_TOKEN(pos++).val);
  }

  // Variable or Function
  if (GET_TOKEN(pos).ty == TK_IDENT) {
    Node *id = new_node_ident(0, GET_TOKEN(pos).input,
                              GET_TOKEN(pos).len);
    pos++;
    // if followed by (, it's a function call.
    if (GET_TOKEN(pos).ty == '(') {
      ++pos;
      Node *arg = NULL;
      // Argument exists.
      if (GET_TOKEN(pos).ty != ')') {
        arg = expression(ASSIGN_PRIORITY);
      }
      if (GET_TOKEN(pos).ty != ')') {
        error("No right parenthesis corresponding to left parenthesis"
              " (term, function): \"%s\"",
              GET_TOKEN(pos).input);
      }
      pos++;
      Node *node = new_node(ND_FUNCCALL, id, arg);
      return node;
    }
    // If not followed by (, it's a variable.
    // is it array?
    Node *node = id;
    int array_size = 0;
    if (GET_TOKEN(pos).ty == '[') {
      pos++;
      Node *index = expression(ASSIGN_PRIORITY);
      if (index->ty == ND_NUM) {
        array_size = index->val;
      } else {
        // put unreasonable number;
        array_size = -1;
      }
      if (GET_TOKEN(pos++).ty != ']') {
        fprintf(stderr, "Closing bracket ']' missing. \"%s\"\n", GET_TOKEN(pos - 1).input);
        exit(1);
      }
      Node *offset = new_node('*', index, new_node_num(8));
      node = new_node('*', NULL, new_node('+', id, offset));
    }
    if (get_global_symbol(id->name) == NULL && get_local_symbol(id->name) == NULL) {
      // This must be a declaration. Size must be specific;
      if (array_size < 0) {
        fprintf(stderr, "Array size must be defined statically.\n");
        exit(1);
      }
      add_local_symbol(id->name, ID_VAR, array_size);
    }
    return node;
  }
  // "( expression )"
  if (GET_TOKEN(pos).ty == '(') {
    pos++;
    Node *node = expression(ASSIGN_PRIORITY);
    if (GET_TOKEN(pos).ty != ')') {
      error("No right parenthesis corresponding to left parenthesis "
            "(term, parenthesis): \"%s\"",
            GET_TOKEN(pos).input);
    }
    pos++;
    return node;
  }
  // Single term operators
  if (GET_TOKEN(pos).ty == '+' || GET_TOKEN(pos).ty == '-' || GET_TOKEN(pos).ty == '*' || GET_TOKEN(pos).ty == '&') {
    int type = GET_TOKEN(pos).ty;
    pos++;
    Node *rhs = term();
    return new_node(type, NULL, rhs);
  }
  // Code should not reach here.
  error("Unexpected token (parse.c term): \"%s\"",
        GET_TOKEN(pos).input);
  return NULL;
}

Node *argument() {
  // TODO: make argument a list.
  if (GET_TOKEN(pos).ty != TK_IDENT) {
    error("Invalid (not IDENT) token in argument declaration position \"%s\"",
          GET_TOKEN(pos).input);
  }
  // argument name?
  char *name = create_name_perm(GET_TOKEN(pos).input, GET_TOKEN(pos).len);
  if (get_local_symbol(name) != NULL) {
    error("Argument name conflict: %s\n", name);
  }
  add_local_symbol(name, ID_ARG, get_array_size());
  Node *id = new_node_ident(GET_TOKEN(pos).val, GET_TOKEN(pos).input,
                              GET_TOKEN(pos).len);
  pos++;
  return id;
}

Node *assign_dash() {
  Node *lhs = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(pos).ty == '=') {
    pos++;
    return new_node('=', lhs, assign_dash());
  }
  return lhs;
}

Node *assign() {
  Node *lhs = assign_dash();
  if (GET_TOKEN(pos).ty == '(') {
    while (GET_TOKEN(pos).ty != ')') {
      ++pos;
    }
    return lhs;
  }
  while (GET_TOKEN(pos).ty == ';') {
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
  if (GET_TOKEN(pos++).ty != '(') {
    error("Left paraenthesis '(' missing (if): \"%s\"\n",
          GET_TOKEN(pos - 1).input);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(pos++).ty != ')') {
    error("Right paraenthesis ')' missing (if): \"%s\"\n",
          GET_TOKEN(pos - 1).input);
  }
  Node *ifnd = new_node(ND_IF, cond, NULL);
  ifnd->block = new_vector();
  code_block(ifnd->block);
  if (GET_TOKEN(pos).ty == TK_ELSE) {
    pos++;
    if (GET_TOKEN(pos).ty == TK_IF) {
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
  if (GET_TOKEN(pos++).ty != '(') {
    error("Left paraenthesis '(' missing (while): \"%s\"\n",
          GET_TOKEN(pos - 1).input);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(pos++).ty != ')') {
    error("Right paraenthesis ')' missing (while): \"%s\"\n",
          GET_TOKEN(pos - 1).input);
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
  if (GET_TOKEN(pos++).ty != '(') {
    error("Left paraenthesis '(' missing (for): \"%s\"\n",
          GET_TOKEN(pos - 1).input);
  }
  Node *init = expression(ASSIGN_PRIORITY);
  vec_push(code, init);
  if (GET_TOKEN(pos++).ty != ';') {
    error("1st Semicolon ';' missing (for): \"%s\"\n",
          GET_TOKEN(pos - 1).input);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(pos++).ty != ';') {
    error("2nd Semicolon ';' missing (for): \"%s\"\n",
          GET_TOKEN(pos - 1).input);
  }
  Node *increment = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(pos++).ty != ')') {
    error("Right paraenthesis '(' missing (for): \"%s\"\n",
          GET_TOKEN(pos - 1).input);
  }
  Node *for_nd = new_node(ND_WHILE, cond, NULL);
  vec_push(code, for_nd);
  // Add the increment code and {} block.
  for_nd->block = new_vector();
  code_block(for_nd->block);
  vec_push(for_nd->block, increment);
  vec_push(for_nd->block, NULL);
}

void code_block(Vector *code) {
  if (GET_TOKEN(pos++).ty != '{') {
    error("Left brace '{' missing (code_block): \"%s\"", GET_TOKEN(pos - 1).input);
  }
  while (GET_TOKEN(pos).ty != '}') {
    if (GET_TOKEN(pos).ty == TK_IF) {
      vec_push(code, if_node());
    } else if (GET_TOKEN(pos).ty == TK_WHILE) {
      vec_push(code, while_node());
    } else if (GET_TOKEN(pos).ty == TK_FOR) {
      for_node(code);
    } else {
      vec_push(code, assign());
    }
  }
  if (GET_TOKEN(pos++).ty != '}') {
    error("Right brace '}' missing (code_block): \"%s\"", GET_TOKEN(pos - 1).input);
  }
}

void identifier_node(Vector *code) {
  Node *id = new_node_ident(GET_TOKEN(pos).val, GET_TOKEN(pos).input,
                            GET_TOKEN(pos).len);
  if (get_global_symbol(id->name) != NULL) {
    error("Global name conflict. \"%s\"", id->name);
  }
  pos++;
  if (GET_TOKEN(pos).ty != '(') {
    // global variable.
    vec_push(code, id);
    int num = get_array_size();
    add_global_symbol(id->name, ID_VAR, num);
    // TODO: add initialization.
    while (GET_TOKEN(pos).ty == ';') {
      pos++;
    }
    return;
  }
  // TODO: implement function declaration.
  // function definition..
  add_global_symbol(id->name, ID_FUNC, 0);
  Node *arg = NULL;
  pos++;
  if (GET_TOKEN(pos).ty != ')') {
    arg = argument();
  }
  if (GET_TOKEN(pos++).ty != ')') {
    // TODO: add support of nmultiple arguments.
    error("Right parenthesis ')' missing (function): \"%s\"",
          GET_TOKEN(pos - 1).input);
  }
  Node *f = new_node(ND_FUNCDEF, id, arg);
  f->block = new_vector();
  code_block(f->block);
  vec_push(code, f);
  vec_push(f->block, NULL);
  return;
}

void function(Vector *code) {
  if (GET_TOKEN(pos).ty == TK_IDENT) {
    // TODO: Move this out to a new function.
    identifier_node(code);
    return;
  }
  error("Unexpected token (function): \"%s\"", GET_TOKEN(pos).input);
}

void program(Vector *code) {
  while (GET_TOKEN(pos).ty != TK_EOF) {
    current_local_symbols = new_map();
    local_symbol_counter = 0;
    vec_push(local_symbols, current_local_symbols);
    function(code);
  }
  vec_push(code, NULL);
  vec_push(local_symbols, NULL);
}

