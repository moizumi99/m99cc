#include "m99cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Vector *tokens;
extern Vector *local_symbols;
extern Map *global_symbols;
extern Vector *program_code;
extern Vector *string_literals;

Node *get_function_p(int i);

void dump_tree(Vector *code);

char *get_type(int ty) {
  static char num[2] = {'\0', '\0'};
  switch(ty) {
  case ND_NUM: return "ND_NUM";
  case ND_IDENT: return "ND_IDENT";
  case ND_IDENTSEQ: return "ND_IDENTSEQ";
  case ND_FUNCCALL: return "ND_FUNCCALL";
  case ND_FUNCDEF: return "ND_FUNCDEF";
  case ND_BLOCK: return "ND_BLOCK";
  case ND_ROOT: return "ND_ROOT";
  case ND_EQ: return "ND_EQ";
  case ND_NE: return "ND_NE";
  case ND_LE: return "ND_LE";
  case ND_GE: return "ND_GE";
  case ND_PE: return "ND_PE";
  case ND_ME: return "ND_ME";
  case ND_AND: return "ND_AND";
  case ND_OR: return "ND_OR";
  case ND_IF: return "ND_IF";
  case ND_WHILE: return "ND_WHILE";
  case ND_FOR: return "ND_FOR";
  case ND_RETURN: return "ND_RETURN";
  case ND_DECLARE: return "ND_DECLARE";
  case ND_DEFINITION: return "ND_DEFINITION";
  case ND_DATATYPE: return "ND_DATATYPE";
  case ND_STR: return "ND_STR";
  case ND_DEREF: return "ND_DEREF";
  case ND_VOID: return "ND_VOID";
  case ND_INT: return "ND_INT";
  case ND_CHAR: return "ND_CHAR";
  case ND_PNT: return "ND_PNT";
  case ND_STRUCT: return "ND_STRUCT";
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
  fprintf(stderr, "(");
  if (nd->rhs != NULL) {
    dump_node(nd->lhs);
  }
  fprintf(stderr, "%s", get_type(nd->ty));
  if (nd->ty >= 256) {
    if (nd->name) {
      fprintf(stderr, ":\"%s\"", nd->name);
    } else {
      fprintf(stderr, ":%d", nd->val);
    }
  }
  if (nd->rhs == NULL) {
    dump_node(nd->lhs);
  } else {
    dump_node(nd->rhs);
  }
  fprintf(stderr, ")");
  if (nd->block) {
    fprintf(stderr, "{\n");
    dump_tree(nd->block);
    fprintf(stderr, "}\n");
  }
}

void dump_tree(Vector *code) {
  if (code==NULL) {
    return;
  }
  Node *nd;
  for (int j = 0; (nd = code->data[j]) != NULL; j++) {
    dump_node(nd);
    fprintf(stderr, "\n");
  }
}

void test_parse_1() {
  char *input = "int main(){int a; a = 0; retutrn a;}";
  Vector *v;
  v = tokenize(input);
  Vector *p;
  p = parse(v);
  dump_tree(p);
}

void test_parse_2() {
  char *input = "struct V {int a;} int main(){int a; a = 0; retutrn a;}";
  Vector *v;
  v = tokenize(input);
  Vector *p;
  p = parse(v);
  dump_tree(p);
}

void test_parse() {
  test_parse_2();
}

void runtest_parse() {
  tokens = new_vector();
  test_parse();
  fprintf(stderr, "OK\n");
}
