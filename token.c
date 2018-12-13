#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "m99cc.h"

extern Vector *tokens;

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
      add_token(tokens,i);
      GET_TOKEN(i).ty = TK_EQ;
      GET_TOKEN(i).input = p;
      GET_TOKEN(i).len = 2;
      i++;
      p+=2;
      continue;
    }

    if (*p == '!' && *(p + 1) == '=') {
      add_token(tokens,i);
      GET_TOKEN(i).ty = TK_NE;
      GET_TOKEN(i).input = p;
      GET_TOKEN(i).len = 2;
      i++;
      p+=2;
      continue;
    }

    if ((*p == '<' || *p == '>') && *(p + 1) == '=') {
      add_token(tokens,i);
      GET_TOKEN(i).ty = (*p == '<') ? TK_LE : TK_GE;
      GET_TOKEN(i).input = p;
      GET_TOKEN(i).len = 2;
      i++;
      p+=2;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
        *p == '(' || *p == ')' || *p == '=' || *p == ';' ||
        *p == '{' || *p == '}' || *p == '<' || *p == '>' ||
        *p == '&' || *p == '[' || *p == ']') {
      add_token(tokens,i);
      GET_TOKEN(i).ty = *p;
      GET_TOKEN(i).input = p;
      GET_TOKEN(i).len = 1;
      i++;
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      add_token(tokens,i);
      int len = 0;
      GET_TOKEN(i).input = p;
      while(('a' <= *p && *p <= 'z') || ('0' <= *p && *p <= '9')) {
        len++;
        p++;
      }
      GET_TOKEN(i).len = len;
      if (strncmp(GET_TOKEN(i).input, "if", 2) == 0) {
        GET_TOKEN(i).len = 2;
        GET_TOKEN(i).ty = TK_IF;
      } else if (strncmp(GET_TOKEN(i).input, "else", 4) == 0) {
        GET_TOKEN(i).len = 4;
        GET_TOKEN(i).ty = TK_ELSE;
      } else if (strncmp(GET_TOKEN(i).input, "while", 5) == 0) {
        GET_TOKEN(i).len = 5;
        GET_TOKEN(i).ty = TK_WHILE;
      } else if (strncmp(GET_TOKEN(i).input, "for", 3) == 0) {
        GET_TOKEN(i).len = 3;
        GET_TOKEN(i).ty = TK_FOR;
      } else if (strncmp(GET_TOKEN(i).input, "void", 4) == 0) {
        GET_TOKEN(i).len = 4;
        GET_TOKEN(i).ty = TK_VOID;
      } else if (strncmp(GET_TOKEN(i).input, "int", 3) == 0) {
        GET_TOKEN(i).len = 3;
        GET_TOKEN(i).ty = TK_INT;
      } else if (strncmp(GET_TOKEN(i).input, "char", 4) == 0) {
        GET_TOKEN(i).len = 4;
        GET_TOKEN(i).ty = TK_CHAR;
      } else {
        GET_TOKEN(i).ty = TK_IDENT;
      }
      i++;
      continue;
    }

    if (isdigit(*p)) {
      add_token(tokens,i);
      GET_TOKEN(i).ty = TK_NUM;
      GET_TOKEN(i).input = p;
      char *new_p;
      GET_TOKEN(i).val = strtol(p, &new_p, 10);
      GET_TOKEN(i).len = (int) (new_p - p);
      p = new_p;
      i++;
      continue;
    }

    fprintf(stderr, "Can't tokenize: %s\n", p);
    exit(1);
  }

  add_token(tokens,i);
  GET_TOKEN(i).ty = TK_EOF;
  GET_TOKEN(i).input = p;
  GET_TOKEN(i).len = 1;
}


