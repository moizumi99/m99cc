#include "m99cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GET_TKN(T, P) ((Token *)(T)->data[(P)])

int expect_token(Vector *tokens, int pos, int ty, int val, char *name) {
  if (GET_TKN(tokens, pos)->ty != ty) {
    goto ERROR;
  }
  if (GET_TKN(tokens, pos)->val != val) {
    goto ERROR;
  }
  if (GET_TKN(tokens, pos)->len != strlen(name)) {
    goto ERROR;
  }
  if (strncmp(GET_TKN(tokens, pos)->input, name, GET_TKN(tokens, pos)->len) != 0) {
    goto ERROR;
  }
  return 0;
 ERROR:
  fprintf(stderr, "%d: (%d, %d, \"%s\") expected, got (%d, %d, \"%s\")\n", pos, ty, val, name, GET_TKN(tokens, pos)->ty, GET_TKN(tokens, pos)->val, GET_TKN(tokens, pos)->input);
  exit(1);
}

void test_tokenize(Vector *tokens) {
  char *p = "a[4]; f(x) {2 <= 30 >= 2 == 4;} main() { f(0 != 1 * 2 / 3 - 5)"
    " if else while for;} void int char -3 [ & ] , return 4;"
    "'a' '\\t' '\\r' '\\n' '\\\"' '\\\'' '\\\?' '\\\\' '\\0' "
    "&& || f_2()"
    "(\"Hello, world\\n\")";
  tokens = tokenize(p);
  int cnt = 0;
  expect_token(tokens, cnt++, TK_IDENT, 0, "a");
  expect_token(tokens, cnt++, '[', 0, "[");
  expect_token(tokens, cnt++, TK_NUM, 4, "4");
  expect_token(tokens, cnt++, ']', 0, "]");
  expect_token(tokens, cnt++, ';', 0, ";");
  expect_token(tokens, cnt++, TK_IDENT, 0, "f");
  expect_token(tokens, cnt++, '(', 0, "(");
  expect_token(tokens, cnt++, TK_IDENT, 0, "x");
  expect_token(tokens, cnt++, ')', 0, ")");
  expect_token(tokens, cnt++, '{', 0, "{");
  expect_token(tokens, cnt++, TK_NUM, 2, "2");
  expect_token(tokens, cnt++, TK_GE, 0, "<=");
  expect_token(tokens, cnt++, TK_NUM, 30, "30");
  expect_token(tokens, cnt++, TK_LE, 0, ">=");
  expect_token(tokens, cnt++, TK_NUM, 2, "2");
  expect_token(tokens, cnt++, TK_EQ, 0, "==");
  expect_token(tokens, cnt++, TK_NUM, 4, "4");
  expect_token(tokens, cnt++, ';', 0, ";");
  expect_token(tokens, cnt++, '}', 0, "}");
  expect_token(tokens, cnt++, TK_IDENT, 0, "main");
  expect_token(tokens, cnt++, '(', 0, "(");
  expect_token(tokens, cnt++, ')', 0, ")");
  expect_token(tokens, cnt++, '{', 0, "{");
  expect_token(tokens, cnt++, TK_IDENT, 0, "f");
  expect_token(tokens, cnt++, '(', 0, "(");
  expect_token(tokens, cnt++, TK_NUM, 0, "0");
  expect_token(tokens, cnt++, TK_NE, 0, "!=");
  expect_token(tokens, cnt++, TK_NUM, 1, "1");
  expect_token(tokens, cnt++, '*', 0, "*");
  expect_token(tokens, cnt++, TK_NUM, 2, "2");
  expect_token(tokens, cnt++, '/', 0, "/");
  expect_token(tokens, cnt++, TK_NUM, 3, "3");
  expect_token(tokens, cnt++, '-', 0, "-");
  expect_token(tokens, cnt++, TK_NUM, 5, "5");
  expect_token(tokens, cnt++, ')', 0, ")");
  expect_token(tokens, cnt++, TK_IF, 0, "if");
  expect_token(tokens, cnt++, TK_ELSE, 0, "else");
  expect_token(tokens, cnt++, TK_WHILE, 0, "while");
  expect_token(tokens, cnt++, TK_FOR, 0, "for");
  expect_token(tokens, cnt++, ';', 0, ";");
  expect_token(tokens, cnt++, '}', 0, "}");
  expect_token(tokens, cnt++, TK_VOID, 0, "void");
  expect_token(tokens, cnt++, TK_INT, 0, "int");
  expect_token(tokens, cnt++, TK_CHAR, 0, "char");
  expect_token(tokens, cnt++, '-', 0, "-");
  expect_token(tokens, cnt++, TK_NUM, 3, "3");
  expect_token(tokens, cnt++, '[', 0, "[");
  expect_token(tokens, cnt++, '&', 0, "&");
  expect_token(tokens, cnt++, ']', 0, "]");
  expect_token(tokens, cnt++, ',', 0, ",");
  expect_token(tokens, cnt++, TK_RETURN, 0, "return");
  expect_token(tokens, cnt++, TK_NUM, 4, "4");
  expect_token(tokens, cnt++, ';', 0, ";");
  expect_token(tokens, cnt++, TK_NUM, 'a', "a");
  expect_token(tokens, cnt++, TK_NUM, '\t', "\\t");
  expect_token(tokens, cnt++, TK_NUM, '\r', "\\r");
  expect_token(tokens, cnt++, TK_NUM, '\n', "\\n");
  expect_token(tokens, cnt++, TK_NUM, '\"', "\\\"");
  expect_token(tokens, cnt++, TK_NUM, '\'', "\\\'");
  expect_token(tokens, cnt++, TK_NUM, '\?', "\\\?");
  expect_token(tokens, cnt++, TK_NUM, '\\', "\\\\");
  expect_token(tokens, cnt++, TK_NUM, '\0', "\\0");
  expect_token(tokens, cnt++, TK_AND, 0, "&&");
  expect_token(tokens, cnt++, TK_OR, 0, "||");
  expect_token(tokens, cnt++, TK_IDENT, 0, "f_2");
  expect_token(tokens, cnt++, '(', 0, "(");
  expect_token(tokens, cnt++, ')', 0, ")");
  expect_token(tokens, cnt++, '(', 0, "(");
  expect_token(tokens, cnt++, TK_STR, 0, "\"Hello, world\\n\"");
  expect_token(tokens, cnt++, ')', 0, ")");
}

void dump_token(Vector *tokens) {
  int p = 0;
  Token *t;
  char *name = malloc(256);
  while((t = (Token *)tokens->data[p++]) != NULL) {
    int len = (t->len < 255) ? t->len : 255;
    len = (len > 0) ? len : 1;
    strncpy(name, t->input, len);
    name[len] = '\0';
    fprintf(stderr, "ty = %d, val = %d, len = %d, input = \"%s\"\n", t->ty, t->val, t->len, name);
  }
}

void runtest_tokenize() {
  Vector *tokens;
  tokens = new_vector();
  test_tokenize(tokens);
  // dump_token();
  printf("OK\n");
}

