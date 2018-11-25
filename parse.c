#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

extern Vector *tokens;

// Error reporting function.
void error(char *s, char *message) {
  fprintf(stderr, s, message);
  fprintf(stderr, "\n");
  exit(1);
}

extern Vector *code;

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

extern Map *variables;

void add_variable(char *name_perm) {
  static int variable_address = 0;
  map_put(variables, name_perm, (void *) (++variable_address * 8));
}

void *get_variable_address(char *name) {
  return  map_get(variables, name);
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
  if (get_variable_address(node->name) == NULL) {
    add_variable(node->name);
  }
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
      // No argument case.
      if (GET_TOKEN(pos).ty != ')') {
        arg = argument();
      }
      if (GET_TOKEN(pos).ty != ')') {
        error("No right parenthesis corresponding to left parenthesis (term, function): \"%s\"",
              GET_TOKEN(pos).input);
      }
      pos++;
      Node *node = new_node(ND_FUNCCALL, id, arg);
      return node;
    } else {
      // If not followed by (, it's a variable.
      return id;
    }
  }
  // "( expression )"
  if (GET_TOKEN(pos).ty == '(') {
    pos++;
    Node *node = expression(ASSIGN_PRIORITY + 1);
    if (GET_TOKEN(pos).ty != ')') {
      error("No right parenthesis corresponding to left parenthesis (term, parenthesis): \"%s\"",
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
  return expression(ASSIGN_PRIORITY + 1);
}

Node *assign_dash() {
  Node *lhs = expression(ASSIGN_PRIORITY + 1);
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

void add_code(int i) {
  if (code->len > i)
    return;
  Token *atoken = malloc(sizeof(Node));
  vec_push(code, (void *)atoken);
}

#define GET_CODE_P(i) (code->data[i])

void line() {
  int line_counter = 0;
  while (GET_TOKEN(pos).ty != TK_EOF && GET_TOKEN(pos).ty != '}') {
    add_code(line_counter);
    GET_CODE_P(line_counter) = assign();
    line_counter++;
  }
  add_code(line_counter);
  GET_CODE_P(line_counter) = NULL;
}

void function() {
  while (GET_TOKEN(pos).ty != TK_EOF) {
    if (GET_TOKEN(pos++).ty != TK_IDENT) {
      error("Unexpected token (function): \"%s\"", GET_TOKEN(pos).input);
    }
    if (GET_TOKEN(pos++).ty != '(') {
      error("Left parenthesis '(' missing (function): \"%s\"", GET_TOKEN(pos).input);
    }
    Node *arg = NULL;
    if (GET_TOKEN(pos).ty != ')') {
      arg = argument();
      fprintf(stderr, "Argument Val = %d", arg->val);
    }
    if (GET_TOKEN(pos++).ty != ')') {
      error("Right parenthesis ')' missing (function): \"%s\"", GET_TOKEN(pos).input);
    }
    if (GET_TOKEN(pos++).ty != '{') {
      error("Left brace '{' missing (function): \"%s\"", GET_TOKEN(pos).input);
    }
    line();
    if (GET_TOKEN(pos++).ty != '}') {
      error("Right brace '}' missing (function): \"%s\"", GET_TOKEN(pos).input);
    }
  }
}

void program() {
  function();
  //  line();
}

