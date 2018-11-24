#include <stdio.h>
#include <stdlib.h>
#include "9cc.h"

extern Map *variables;

void add_variable(int name);

void *variable_address(char name);

void gen_lval(Node *node) {
  if (node->ty == ND_IDENT) {
    printf("  mov rax, rbp\n");
    void *address = variable_address(node->name);
    // If new variable, create a room.
    if (address == NULL) {
      fprintf(stderr, "Undefined variable used.");
      exit(1);
    }
    //    printf("  sub rax, %d\n",
    //           ('z' - node->name + 1) * 8);
    printf("  sub rax, %d\n", (int) address);
    printf("  push rax\n");
    return;
  }
  error("%s", "Left hand value isn't a variable.");
}

void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  if (node->ty == ND_IDENT) {
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->ty == '=') {
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
  case '+':
    printf("  add rax, rdi\n");
    break;
  case '-':
    printf("  sub rax, rdi\n");
    break;
  case '*':
    printf("  mul rdi\n");
    break;
  case '/':
    printf("  mov rdx, 0\n");
    printf("  div rdi\n");
    break;
  case ND_EQ:
  case ND_NE:
    printf("  cmp rdi, rax\n");
    if (node->ty == ND_EQ) {
      printf("  sete al\n");
    } else {
      printf("  setne al\n");
    }
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}
