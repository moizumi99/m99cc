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
  return (p + len);
}

// split chars pointed by p into tokens
Vector *tokenize(char *p) {
  Vector *tokens = new_vector();

  int i = 0;
  while (*p) {
    int tk = -1;
    int len = 0;
    int val = 0;
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
      tk = TK_EQ;
      len = 2;
      goto ADDTOKEN;
    }

    if (*p == '!' && *(p + 1) == '=') {
      tk = TK_NE;
      len = 2;
      goto ADDTOKEN;
    }

    if (*p == '<' && *(p + 1) == '=') {
      tk = TK_GE;
      len = 2;
      goto ADDTOKEN;
    }

    if (*p == '>' && *(p + 1) == '=') {
      tk = TK_LE;
      len = 2;
      goto ADDTOKEN;
    }

    if (*p == '&' && *(p + 1) == '&') {
      tk = TK_AND;
      len = 2;
      goto ADDTOKEN;
    }

    if (*p == '|' && *(p + 1) == '|') {
      tk = TK_OR;
      len = 2;
      goto ADDTOKEN;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
        *p == '(' || *p == ')' || *p == '=' || *p == ';' ||
        *p == '{' || *p == '}' || *p == '<' || *p == '>' ||
        *p == '&' || *p == '[' || *p == ']' || *p == ',') {
      tk = *p;
      len = 1;
      goto ADDTOKEN;
    }

    if ('a' <= *p && *p <= 'z') {
      if (strncmp(p, "if", 2) == 0) {
        tk = TK_IF;
        len = 2;
      } else if (strncmp(p, "else", 4) == 0) {
        tk = TK_ELSE;
        len = 4;
      } else if (strncmp(p, "while", 5) == 0) {
        tk = TK_WHILE;
        len = 5;
      } else if (strncmp(p, "for", 3) == 0) {
        tk = TK_FOR;
        len = 3;
      } else if (strncmp(p, "void", 4) == 0) {
        tk = TK_VOID;
        len = 4;
      } else if (strncmp(p, "int", 3) == 0) {
        tk = TK_INT;
        len = 3;
      } else if (strncmp(p, "char", 4) == 0) {
        tk = TK_CHAR;
        len = 4;
      } else if (strncmp(p, "return", 6) == 0) {
        tk = TK_RETURN;
        len = 6;
      } else {
        len = 0;
        char *pn = p;
        while(('a' <= *pn && *pn <= 'z') || ('0' <= *pn && *pn <= '9') || ('_' == *pn)) {
          len++;
          pn++;
        }
        tk = TK_IDENT;
      }
      goto ADDTOKEN;
    }

    if (isdigit(*p)) {
      tk = TK_NUM;
      char *new_p;
      val = strtol(p, &new_p, 10);
      len = (int) (new_p - p);
      goto ADDTOKEN;
    }

  ADDTOKEN:
    if (len > 0) {
      add_token(tokens,i);
      GET_ATOKEN(tokens, i).ty = tk;
      GET_ATOKEN(tokens, i).input = p;
      GET_ATOKEN(tokens, i).len = len;
      GET_ATOKEN(tokens, i).val = val;
      i++;
      p += len;
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


