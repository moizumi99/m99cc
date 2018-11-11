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

// Store tokens in this array. Max is 100 for now.
Token tokens[100];

// Error reporting function.
void error(char *s, char *message) {
  fprintf(stderr, s, message);
  fprintf(stderr, "\n");
  exit(1);
}
extern Node *code[];

int pos;

// split chars pointed by p into tokens
void tokenize(char *p) {
  int i = 0;
  while (*p) {
    // skip spaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '=' && *(p + 1) == '=') {
      tokens[i].ty = TK_EQ;
      tokens[i].input = p;
      i++;
      p+=2;
      continue;
    }

    if (*p == '!' && *(p + 1) == '=') {
      tokens[i].ty = TK_NE;
      tokens[i].input = p;
      i++;
      p+=2;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
        *p == '(' || *p == ')' || *p == '=' || *p == ';' ) {
      tokens[i].ty = *p;
      tokens[i].input = p;
      i++;
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      tokens[i].ty = TK_IDENT;
      tokens[i].input = p;
      tokens[i].val = *p - 'a';
      i++;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      tokens[i].ty = TK_NUM;
      tokens[i].input = p;
      tokens[i].val = strtol(p, &p, 10);
      i++;
      continue;
    }

    fprintf(stderr, "Can't tokenize: %s\n", p);
    exit(1);
  }

  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
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

Node *new_node_ident(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->name = val;
  return node;
}

Node *compare();

Node *term() {
  if (tokens[pos].ty == TK_NUM) {
    return new_node_num(tokens[pos++].val);
  }
  if (tokens[pos].ty == TK_IDENT) {
    return new_node_ident(tokens[pos++].val);
  }
  if (tokens[pos].ty == '(') {
    pos++;
    Node *node = compare();
    if (tokens[pos].ty != ')') {
      error("No right parenthesis corresponding to left parenthesis (term): %s",
            tokens[pos].input);
    }
    pos++;
    return node;
  }
  error("Unexpected token (tem): %s",
        tokens[pos].input);
  // Code should not reach here.
  return NULL;
}

Node *mul() {
  Node *lhs = term();
  if (tokens[pos].ty == TK_EOF) {
    return lhs;
  }
  if (tokens[pos].ty == '*') {
    pos++;
    return new_node('*', lhs, mul());
  }
  if (tokens[pos].ty == '/') {
    pos++;
    return new_node('/', lhs, mul());
  }
  //error("Unexpected token (mul): %s", tokens[pos].input);
  return lhs;
}

Node *expr() {
  Node *lhs = mul();
  if (tokens[pos].ty == TK_EOF) {
    return lhs;
  }
  if (tokens[pos].ty == '+') {
    pos++;
    return new_node('+', lhs, expr());
  }
  if (tokens[pos].ty == '-') {
    pos++;
    return new_node('-', lhs, expr());
  }
  // error("Unexpected token (expr): %s", tokens[pos].input);
  return lhs;
}

Node *compare() {
  Node *lhs = expr();
  if (tokens[pos].ty == TK_EOF) {
    return lhs;
  }
  if (tokens[pos].ty == TK_EQ) {
    pos++;
    return new_node(ND_EQ, lhs, expr());
  }
  if (tokens[pos].ty == TK_NE) {
    pos++;
    return new_node(ND_NE, lhs, expr());
  }
  // error("Unexpected token (expr): %s", tokens[pos].input);
  return lhs;
}

Node *assign_dash() {
  Node *lhs = compare();
  if (tokens[pos].ty == '=') {
    pos++;
    return new_node('=', lhs, assign_dash());
  }
  return lhs;
}

Node *assign() {
  Node *lhs = assign_dash();
  if (tokens[pos].ty == ';') {
    pos++;
    return lhs;
  }
  error ("Unexpected token (assign): %s", tokens[pos].input);
  // code should not reach here.
  return NULL;
}

Node **program() {
  int line = 0;
  while (tokens[pos].ty != TK_EOF) {
    code[line] = assign();
    line++;
  }
  code[line] = NULL;
  return code;
}
