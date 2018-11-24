#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "9cc.h"

Vector *tokens;

// Error reporting function.
void error(char *s, char *message) {
  fprintf(stderr, s, message);
  fprintf(stderr, "\n");
  exit(1);
}

extern Vector *code;

int pos;

void add_token(int i) {
  if (tokens->len > i)
    return;
  Token *atoken = malloc(sizeof(Token));
  vec_push(tokens, (void *)atoken);
}

// Macro for getting the next token.
#define GET_TOKEN(i) (*((Token *)tokens->data[i]))

// split chars pointed by p into tokens
void tokenize(char *p) {
  tokens = new_vector();
  int i = 0;
  while (*p) {
    // skip spaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '=' && *(p + 1) == '=') {
      add_token(i);
      GET_TOKEN(i).ty = TK_EQ;
      GET_TOKEN(i).input = p;
      i++;
      p+=2;
      continue;
    }

    if (*p == '!' && *(p + 1) == '=') {
      add_token(i);
      GET_TOKEN(i).ty = TK_NE;
      GET_TOKEN(i).input = p;
      i++;
      p+=2;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
        *p == '(' || *p == ')' || *p == '=' || *p == ';' ) {
      add_token(i);
      GET_TOKEN(i).ty = *p;
      GET_TOKEN(i).input = p;
      i++;
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      add_token(i);
      GET_TOKEN(i).ty = TK_IDENT;
      GET_TOKEN(i).input = p;
      GET_TOKEN(i).val = *p - 'a';
      i++;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      add_token(i);
      GET_TOKEN(i).ty = TK_NUM;
      GET_TOKEN(i).input = p;
      GET_TOKEN(i).val = strtol(p, &p, 10);
      i++;
      continue;
    }

    fprintf(stderr, "Can't tokenize: %s\n", p);
    exit(1);
  }

  add_token(i);
  GET_TOKEN(i).ty = TK_EOF;
  GET_TOKEN(i).input = p;
}

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

void add_variable(int name) {
  static int variable_address = 0;
  char *str = malloc(sizeof(char) * 2);
  str[0] = (char) name;
  map_put(variables, str, (void *) (++variable_address * 8));
}

void *variable_address(char name) {
  char str[2] = {'\0', '\0'};
  str[0] = name;
  return  map_get(variables, str);
}

Node *new_node_ident(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->val = val;
  node->name = val + 'a';
  if (variable_address(node->name) == NULL) {
    add_variable(node->name);
  }
  return node;
}

Node *new_node_function(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_FUNC;
  node->val = val;
  node->name = val + 'a';
  if (variable_address(node->name) == NULL) {
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

Node *term() {
  if (GET_TOKEN(pos).ty == TK_NUM) {
    return new_node_num(GET_TOKEN(pos++).val);
  }
  if (GET_TOKEN(pos).ty == TK_IDENT) {
    if (GET_TOKEN(pos+1).ty == '(') {
      Node *node = new_node_function(GET_TOKEN(pos++).val);
      // TODO: add arguments analysis.
      while (GET_TOKEN(pos++).ty != ')')
        ;
      return node;
    } else {
      return new_node_ident(GET_TOKEN(pos++).val);
    }
  }
  if (GET_TOKEN(pos).ty == '(') {
    pos++;
    Node *node = expression(ASSIGN_PRIORITY + 1);
    if (GET_TOKEN(pos).ty != ')') {
      error("No right parenthesis corresponding to left parenthesis (term): %s",
            GET_TOKEN(pos).input);
    }
    pos++;
    return node;
  }
  error("Unexpected token (in term): %s",
        GET_TOKEN(pos).input);
  // Code should not reach here.
  return NULL;
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

Vector *program() {
  int line = 0;
  while (GET_TOKEN(pos).ty != TK_EOF) {
    add_code(line);
    GET_CODE_P(line) = assign();
    line++;
  }
  add_code(line);
  GET_CODE_P(line) = NULL;
  return code;
}

