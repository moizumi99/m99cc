#include "m99cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Vector *tokens;
extern Vector *local_symbols;
extern Map *global_symbols;
extern Vector *program_code;

Node *get_function_p(int i);

void dump_tree(Vector *code);

char *get_type(int ty) {
  static char num[2] = {'\0', '\0'};
  switch(ty) {
  case ND_NUM: return "ND_NUM";
  case ND_IDENT: return "ND_IDENT";
  case ND_FUNCCALL: return "ND_FUNCCALL";
  case ND_FUNCDEF: return "ND_FUNCDEF";
  case ND_BLOCK: return "ND_BLOCK";
  case ND_ROOT: return "ND_ROOT";
  case ND_PLUS: return "ND_PLUS";
  case ND_MINUS: return "ND_MINUS";
  case ND_EQ: return "ND_EQ";
  case ND_NE: return "ND_NE";
  case ND_IF: return "ND_IF";
  case ND_WHILE: return "ND_WHILE";
  case ND_FOR: return "ND_FOR";
  case ND_VOID: return "ND_VOID";
  case ND_INT: return "ND_INT";
  case ND_CHAR: return "ND_CHAR";
  default:
    if (ty < 256) {
      num[0] = (char) ty;
    } else {
      num[0] = '\0';
    }
    return num;
  }
}

void dump_node(Node *nd) {
  if (nd == NULL) {
    return;
  }
  printf("(");
  dump_node(nd->lhs);
  printf("%s", get_type(nd->ty));
  if (nd->ty >= 256) {
    if (nd->name) {
      printf(":\"%s\"", nd->name);
    } else {
      printf(":%d", nd->val);
    }
  }
  dump_node(nd->rhs);
  printf(")");
  if (nd->block) {
    printf("{\n");
    dump_tree(nd->block);
    printf("}\n");
  }
}

void dump_tree(Vector *code) {
  if (code==NULL) {
    return;
  }
  Node *nd;
  for (int j = 0; (nd = code->data[j]) != NULL; j++) {
    dump_node(nd);
    printf("\n");
  }
}

void test_parse() {
  char *p = "main(){a[2];a[0]=1;a[1]=2;a[0]+a[1];}";
  tokenize(p);
  // dump_token();

  local_symbols = new_vector();
  global_symbols = new_map();
  Vector *program_code = new_vector();
  gen_program(program_code);
  dump_tree(program_code);
}

void runtest_parse() {
  tokens = new_vector();
  test_parse();
  printf("OK\n");
}
