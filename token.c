#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "m99cc.h"

#define GET_ATOKEN(T, I) (*((Token *)(T)->data[(I)]))

void add_token(Vector *ts, int i) {
  if (ts->len > i)
    return;
  Token *atoken = malloc(sizeof(Token));
  vec_push(ts, (void *)atoken);
}

int min(int a, int b) {
  return (a < b) ? a : b;
}

// split chars pointed by p into tokens
Vector *tokenize(char *p) {
  Vector *tokens = new_vector();

  int i = 0;
  while (*p) {
    // skip spaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '=' && *(p + 1) == '=') {
      add_token(tokens,i);
      GET_ATOKEN(tokens, i).ty = TK_EQ;
      GET_ATOKEN(tokens, i).input = p;
      GET_ATOKEN(tokens, i).len = 2;
      i++;
      p+=2;
      continue;
    }

    if (*p == '!' && *(p + 1) == '=') {
      add_token(tokens,i);
      GET_ATOKEN(tokens, i).ty = TK_NE;
      GET_ATOKEN(tokens, i).input = p;
      GET_ATOKEN(tokens, i).len = 2;
      i++;
      p+=2;
      continue;
    }

    if ((*p == '<' || *p == '>') && *(p + 1) == '=') {
      add_token(tokens,i);
      GET_ATOKEN(tokens, i).ty = (*p == '<') ? TK_LE : TK_GE;
      GET_ATOKEN(tokens, i).input = p;
      GET_ATOKEN(tokens, i).len = 2;
      i++;
      p+=2;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
        *p == '(' || *p == ')' || *p == '=' || *p == ';' ||
        *p == '{' || *p == '}' || *p == '<' || *p == '>' ||
        *p == '&' || *p == '[' || *p == ']' || *p == ',') {
      add_token(tokens,i);
      GET_ATOKEN(tokens, i).ty = *p;
      GET_ATOKEN(tokens, i).input = p;
      GET_ATOKEN(tokens, i).len = 1;
      i++;
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      add_token(tokens,i);
      int len = 0;
      GET_ATOKEN(tokens, i).input = p;
      while(('a' <= *p && *p <= 'z') || ('0' <= *p && *p <= '9')) {
        len++;
        p++;
      }
      GET_ATOKEN(tokens, i).len = len;
      if (strncmp(GET_ATOKEN(tokens, i).input, "if", 2) == 0) {
        GET_ATOKEN(tokens, i).len = 2;
        GET_ATOKEN(tokens, i).ty = TK_IF;
      } else if (strncmp(GET_ATOKEN(tokens, i).input, "else", 4) == 0) {
        GET_ATOKEN(tokens, i).len = 4;
        GET_ATOKEN(tokens, i).ty = TK_ELSE;
      } else if (strncmp(GET_ATOKEN(tokens, i).input, "while", 5) == 0) {
        GET_ATOKEN(tokens, i).len = 5;
        GET_ATOKEN(tokens, i).ty = TK_WHILE;
      } else if (strncmp(GET_ATOKEN(tokens, i).input, "for", 3) == 0) {
        GET_ATOKEN(tokens, i).len = 3;
        GET_ATOKEN(tokens, i).ty = TK_FOR;
      } else if (strncmp(GET_ATOKEN(tokens, i).input, "void", 4) == 0) {
        GET_ATOKEN(tokens, i).len = 4;
        GET_ATOKEN(tokens, i).ty = TK_VOID;
      } else if (strncmp(GET_ATOKEN(tokens, i).input, "int", 3) == 0) {
        GET_ATOKEN(tokens, i).len = 3;
        GET_ATOKEN(tokens, i).ty = TK_INT;
      } else if (strncmp(GET_ATOKEN(tokens, i).input, "char", 4) == 0) {
        GET_ATOKEN(tokens, i).len = 4;
        GET_ATOKEN(tokens, i).ty = TK_CHAR;
      } else {
        GET_ATOKEN(tokens, i).ty = TK_IDENT;
      }
      i++;
      continue;
    }

    if (isdigit(*p)) {
      add_token(tokens,i);
      GET_ATOKEN(tokens, i).ty = TK_NUM;
      GET_ATOKEN(tokens, i).input = p;
      char *new_p;
      GET_ATOKEN(tokens, i).val = strtol(p, &new_p, 10);
      GET_ATOKEN(tokens, i).len = (int) (new_p - p);
      p = new_p;
      i++;
      continue;
    }

    fprintf(stderr, "Can't tokenize: %s\n", p);
    exit(1);
  }

  add_token(tokens,i);
  GET_ATOKEN(tokens, i).ty = TK_EOF;
  GET_ATOKEN(tokens, i).input = p;
  GET_ATOKEN(tokens, i).len = 1;

  return tokens;
}


