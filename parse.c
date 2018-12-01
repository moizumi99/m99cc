#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

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
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

void add_global_symbol(char *name_perm, int type) {
  static int global_symbol_counter = 0;
  Symbol *new_symbol = malloc(sizeof(Symbol));
  new_symbol->address = (void *) (++global_symbol_counter * 8);
  new_symbol->type = type;
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

static int local_symbol_counter = 0;
void add_local_symbol(char *name_perm, int type) {
  Symbol *new_symbol = malloc(sizeof(Symbol));
  new_symbol->address = (void *) (++local_symbol_counter * 8);
  new_symbol->type = type;
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

char *create_name_perm(char *name, int len) {
  char *str = malloc(sizeof(char) * IDENT_LEN);
  int copy_len = (len < IDENT_LEN) ? len : IDENT_LEN;
  strncpy(str, name, copy_len);
  return str;
}

Node *new_node_ident(int val, char *name, int len) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->val = val;
  node->name = create_name_perm(name, len);
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
    Node *id = new_node_ident(GET_TOKEN(pos).val, GET_TOKEN(pos).input,
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
    } else {
      // If not followed by (, it's a variable.
      if (get_local_symbol(id->name) == NULL) {
        add_local_symbol(id->name, ID_VAR);
      }
      return id;
    }
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
  add_local_symbol(name, ID_ARG);
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
  if (GET_TOKEN(pos).ty == ';') {
    pos++;
    return lhs;
  } else if (GET_TOKEN(pos).ty == '(') {
    while (GET_TOKEN(pos).ty != ')') {
      ++pos;
    }
    return lhs;
  }
  error ("Unexpected token (assign): %s", GET_TOKEN(pos).input);
  // code should not reach here.
  return NULL;
}

void add_code(Vector *code, int i) {
  if (code->len > i)
    return;
  vec_push(code, malloc(sizeof(Node)));
}

void code_block(Vector *code) {
  if (GET_TOKEN(pos++).ty != '{') {
    error("Left brace '{' missing (code_block): \"%s\"", GET_TOKEN(pos - 1).input);
  }
  while (GET_TOKEN(pos).ty != '}') {
    vec_push(code, assign());
  }
  if (GET_TOKEN(pos++).ty != '}') {
    error("Right brace '}' missing (code_block): \"%s\"", GET_TOKEN(pos - 1).input);
  }
}

void function(Vector *code) {
  if (GET_TOKEN(pos).ty != TK_IDENT) {
    error("Unexpected token (function): \"%s\"", GET_TOKEN(pos).input);
  }
  Node *id = new_node_ident(GET_TOKEN(pos).val, GET_TOKEN(pos).input,
                            GET_TOKEN(pos).len);
  if (get_global_symbol(id->name) == NULL) {
    add_global_symbol(id->name, ID_FUNC);
  } else {
    error("Function name conflict. \"%s\"", id->name);
  }
  pos++;
  if (GET_TOKEN(pos++).ty != '(') {
    error("Left parenthesis '(' missing (function): \"%s\"",
          GET_TOKEN(pos - 1).input);
  }
  Node *arg = NULL;
  if (GET_TOKEN(pos).ty != ')') {
    arg = argument();
  }
  if (GET_TOKEN(pos++).ty != ')') {
    // TODO: add support of nmultiple arguments.
    error("Right parenthesis ')' missing (function): \"%s\"",
          GET_TOKEN(pos - 1).input);
  }
  Node *f = new_node(ND_FUNCDEF, id, arg);
  vec_push(code, f);
  code_block(code);
  vec_push(code, NULL);
}

void program(Vector *program_code) {
  int function_counter = 0;
  while (GET_TOKEN(pos).ty != TK_EOF) {
    vec_push(program_code, new_vector());
    current_local_symbols = new_map();
    local_symbol_counter = 0;
    vec_push(local_symbols, current_local_symbols);
    function(program_code->data[function_counter++]);
  }
  vec_push(program_code, NULL);
  vec_push(local_symbols, NULL);
}

