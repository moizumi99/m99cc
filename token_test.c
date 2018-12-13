#include "m99cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Vector *tokens;

#define GET(pos) ((Token *)tokens->data[pos])

int expect_token(int pos, int ty, int val, char *name) {
  if (GET(pos)->ty != ty) {
    goto ERROR;
  }
  if (GET(pos)->val != val) {
    goto ERROR;
  }
  if (GET(pos)->len != strlen(name)) {
    goto ERROR;
  }
  if (strncmp(GET(pos)->input, name, GET(pos)->len) != 0) {
    goto ERROR;
  }
  return 0;
 ERROR:
  fprintf(stderr, "%d: (%d, %d, \"%s\") expected, got (%d, %d, \"%s\")\n", pos, ty, val, name, GET(pos)->ty, GET(pos)->val, GET(pos)->input);
  exit(1);
}

void test_tokenize() {
  char *p = "a[4]; f(x) {2 <= 30 >= 2 == 4;} main() { f(0 != 1 * 2 / 3 - 5) if else while for;} void int char -3 [ & ]";
  tokenize(p);
  int cnt = 0;
  expect_token(cnt++, TK_IDENT, 0, "a");
  expect_token(cnt++, '[', 0, "[");
  expect_token(cnt++, TK_NUM, 4, "4");
  expect_token(cnt++, ']', 0, "]");
  expect_token(cnt++, ';', 0, ";");
  expect_token(cnt++, TK_IDENT, 0, "f");
  expect_token(cnt++, '(', 0, "(");
  expect_token(cnt++, TK_IDENT, 0, "x");
  expect_token(cnt++, ')', 0, ")");
  expect_token(cnt++, '{', 0, "{");
  expect_token(cnt++, TK_NUM, 2, "2");
  expect_token(cnt++, TK_LE, 0, "<=");
  expect_token(cnt++, TK_NUM, 30, "30");
  expect_token(cnt++, TK_GE, 0, ">=");
  expect_token(cnt++, TK_NUM, 2, "2");
  expect_token(cnt++, TK_EQ, 0, "==");
  expect_token(cnt++, TK_NUM, 4, "4");
  expect_token(cnt++, ';', 0, ";");
  expect_token(cnt++, '}', 0, "}");
  expect_token(cnt++, TK_IDENT, 0, "main");
  expect_token(cnt++, '(', 0, "(");
  expect_token(cnt++, ')', 0, ")");
  expect_token(cnt++, '{', 0, "{");
  expect_token(cnt++, TK_IDENT, 0, "f");
  expect_token(cnt++, '(', 0, "(");
  expect_token(cnt++, TK_NUM, 0, "0");
  expect_token(cnt++, TK_NE, 0, "!=");
  expect_token(cnt++, TK_NUM, 1, "1");
  expect_token(cnt++, '*', 0, "*");
  expect_token(cnt++, TK_NUM, 2, "2");
  expect_token(cnt++, '/', 0, "/");
  expect_token(cnt++, TK_NUM, 3, "3");
  expect_token(cnt++, '-', 0, "-");
  expect_token(cnt++, TK_NUM, 5, "5");
  expect_token(cnt++, ')', 0, ")");
  expect_token(cnt++, TK_IF, 0, "if");
  expect_token(cnt++, TK_ELSE, 0, "else");
  expect_token(cnt++, TK_WHILE, 0, "while");
  expect_token(cnt++, TK_FOR, 0, "for");
  expect_token(cnt++, ';', 0, ";");
  expect_token(cnt++, '}', 0, "}");
  expect_token(cnt++, TK_VOID, 0, "void");
  expect_token(cnt++, TK_INT, 0, "int");
  expect_token(cnt++, TK_CHAR, 0, "char");
  expect_token(cnt++, '-', 0, "-");
  expect_token(cnt++, TK_NUM, 3, "3");
  expect_token(cnt++, '[', 0, "[");
  expect_token(cnt++, '&', 0, "&");
  expect_token(cnt++, ']', 0, "]");
}

void dump_token() {
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
  tokens = new_vector();
  test_tokenize();
  // dump_token();
  printf("OK\n");
}

