#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "9cc.h"

// Token types
typedef struct {
  int ty;      // token type
  int val;     // value if ty is TK_NUM
  char *input;
} Token;

// Token values
enum {
  TK_NUM = 256, // Integer token
  TK_IDENT,     // Identifier
  TK_EQ,        // Equal (==) sign
  TK_NE,        // Not-equal (!=) sign
  TK_EOF        // End of input
};

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

Node *compare();

Node *term() {
  if (GET_TOKEN(pos).ty == TK_NUM) {
    return new_node_num(GET_TOKEN(pos++).val);
  }
  if (GET_TOKEN(pos).ty == TK_IDENT) {
    return new_node_ident(GET_TOKEN(pos++).val);
  }
  if (GET_TOKEN(pos).ty == '(') {
    pos++;
    Node *node = compare();
    if (GET_TOKEN(pos).ty != ')') {
      error("No right parenthesis corresponding to left parenthesis (term): %s",
            GET_TOKEN(pos).input);
    }
    pos++;
    return node;
  }
  error("Unexpected token (tem): %s",
        GET_TOKEN(pos).input);
  // Code should not reach here.
  return NULL;
}

Node *mul() {
  Node *lhs = term();
  if (GET_TOKEN(pos).ty == TK_EOF) {
    return lhs;
  }
  if (GET_TOKEN(pos).ty == '*') {
    pos++;
    return new_node('*', lhs, mul());
  }
  if (GET_TOKEN(pos).ty == '/') {
    pos++;
    return new_node('/', lhs, mul());
  }
  //error("Unexpected token (mul): %s", GET_TOKEN(pos).input);
  return lhs;
}

Node *expr() {
  Node *lhs = mul();
  if (GET_TOKEN(pos).ty == TK_EOF) {
    return lhs;
  }
  if (GET_TOKEN(pos).ty == '+') {
    pos++;
    return new_node('+', lhs, expr());
  }
  if (GET_TOKEN(pos).ty == '-') {
    pos++;
    return new_node('-', lhs, expr());
  }
  // error("Unexpected token (expr): %s", GET_TOKEN(pos).input);
  return lhs;
}

Node *compare() {
  Node *lhs = expr();
  if (GET_TOKEN(pos).ty == TK_EOF) {
    return lhs;
  }
  if (GET_TOKEN(pos).ty == TK_EQ) {
    pos++;
    return new_node(ND_EQ, lhs, expr());
  }
  if (GET_TOKEN(pos).ty == TK_NE) {
    pos++;
    return new_node(ND_NE, lhs, expr());
  }
  // error("Unexpected token (expr): %s", GET_TOKEN(pos).input);
  return lhs;
}

Node *assign_dash() {
  Node *lhs = compare();
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

