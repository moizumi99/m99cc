#include <stdio.h>
#include <stdlib.h>
#include "m99cc.h"

extern Map *global_symbols;
extern Vector *local_symbols;
extern Map *current_local_symbols;

Node *get_node_p(Vector *code, int i) {
  return (Node *)code->data[i];
}

void gen_block(Vector *block_code) {
    for (int i = 0; get_node_p(block_code, i); i++)
      gen(get_node_p(block_code, i));
}

void gen_lval(Node *node) {
  if (node->ty == ND_IDENT) {
    printf("  mov rax, rbp\n");
    void *address = get_local_symbol_address(node->name);
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

static int label_counter = 0;

void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  if (node->ty == ND_FUNCDEF) {
    // code shouldn't reach here
    return;
  }

  if (node->ty == ND_IDENT) {
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->ty == ND_FUNCCALL) {
    // TODO: align rsp to 16 byte boundary.
    if (node->lhs->ty != ND_IDENT) {
      error("%s\n", "Function node doesn't have identifer.");
    }
    if (node->rhs == NULL) {
      error("%s\n", "Function node needs an argument.");
    }
    gen(node->rhs);
    printf("  pop rax\n");
    printf("  call %s\n", node->lhs->name);
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

  if (node->ty == ND_IF) {
    gen(node->lhs);
    printf("  pop rax;\n");
    printf("  cmp rax, 0\n");
    int else_label = label_counter++;
    printf("  je _else_%d\n", else_label);
    gen_block(node->block);
    int end_label = label_counter++;
    printf("  jmp _if_end_%d\n", end_label);
    printf("_else_%d:\n", else_label);
    if (node->rhs != NULL) {
      if (node->rhs->ty == ND_IF) {
        gen(node->rhs);
      } else if (node->rhs->ty == ND_BLOCK) {
        gen_block(node->rhs->block);
      } else {
        error("Unexpected node %s after if-else \n", node->name);
      }
    } else {
    }
    printf("_if_end_%d:\n", end_label);
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
  case '<':
  case '>':
    printf("  cmp rdi, rax\n");
    if (node->ty == ND_EQ) {
      printf("  sete al\n");
    } else if (node->ty == ND_NE) {
      printf("  setne al\n");
    } else if (node->ty == '>') {
      printf("  setl al\n");
    } else if (node->ty == '<') {
      printf("  setle al\n");
    } else {
      error("%s\n", "Code shouldn't reach here (codegen.c compare).");
    }
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}
