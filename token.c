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

char get_one_char(char *p, int *ret_len) {
  int  len = 1;
  char val = *p++;
  if (val == '\\') {
    switch(*p++) {
    case 't':
      val = '\t'; break;
    case 'r':
      val = '\r'; break;
    case 'n':
      val = '\n'; break;
    case '\"':
      val = '\"'; break;
    case '\'':
      val = '\''; break;
    case '\?':
      val = '\?'; break;
    case '\\':
      val = '\\'; break;
    case '0':
      val = '\0'; break;
    default:
      fprintf(stderr, "Unsupported escape sequence. %s", p);
      exit(1);
    }
    len++;
  }
  *ret_len = len;
  return val;
}

char *char_literal(Vector *tokens, char *p, int i) {
  p++;
  add_token(tokens,i);
  GET_ATOKEN(tokens, i).ty = TK_NUM;
  GET_ATOKEN(tokens, i).input = p;
  int len;
  GET_ATOKEN(tokens, i).val = get_one_char(p, &len);
  GET_ATOKEN(tokens, i).len = len;
  p += len;
  i++;
  if (*p++ != '\'') {
    fprintf(stderr, "Char literal not closed with a single quote. %s", p);
    exit(1);
  }
  return p;
}

// Return literal length including double quatations.
int get_length(char *p) {
  int len = 1;
  p++;
  while(*p != '\"' && *p != EOF) {
    p++;
    len++;
  }
  return len + 1;
}

char *str_literal(Vector *tokens, char *p, int i) {
  add_token(tokens,i);
  GET_ATOKEN(tokens, i).ty = TK_STR;
  GET_ATOKEN(tokens, i).input = p;
  GET_ATOKEN(tokens, i).val = 0;
  int len = get_length(p);
  GET_ATOKEN(tokens, i).len = len;
  i++;
  if (*(p + len - 1) != '\"') {
    fprintf(stderr, "String literal not closed with a double quote. %s\n", p);
    exit(1);
  }
  return (p + len + 1);
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

    if (*p == '\'') {
      p = char_literal(tokens, p, i);
      i++;
      continue;
    }

    if (*p == '\"') {
      p = str_literal(tokens, p, i);
      i++;
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

    if (*p == '<' && *(p + 1) == '=') {
      add_token(tokens,i);
      GET_ATOKEN(tokens, i).ty = TK_GE;
      GET_ATOKEN(tokens, i).input = p;
      GET_ATOKEN(tokens, i).len = 2;
      i++;
      p+=2;
      continue;
    }

    if (*p == '>' && *(p + 1) == '=') {
      add_token(tokens,i);
      GET_ATOKEN(tokens, i).ty = TK_LE;
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
      } else if (strncmp(GET_ATOKEN(tokens, i).input, "return", 6) == 0) {
        GET_ATOKEN(tokens, i).len = 6;
        GET_ATOKEN(tokens, i).ty = TK_RETURN;
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


