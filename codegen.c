#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    if (get_global_symbol_address(node->name) != NULL) {
      // global address.
      printf("  lea rax, %s[rip]\n", node->name);
      printf("  push rax\n");
      return;
    }
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
  } else if (node->ty == '*') {
    gen(node->rhs);
    return;
  }
  fprintf(stderr, "The node type %d can not be left size value", node->ty);
  exit(1);
}

static int label_counter = 0;

int is_systemcall(Node *nd) {
  if (strcmp(nd->name, "putchar") == 0) {
    return 1;
  }
  return 0;
}

void gen_syscall(Node *nd) {
  if (strcmp(nd->name, "putchar") == 0) {
    printf("  mov edi, eax\n");
    printf("  call putchar@PLT\n");
    printf("  mov eax, 0\n");
  } else {
    error("%s is not supported yet.\n", nd->name);
  }
}

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
    if (get_global_symbol_address(node->name) != NULL) {
      printf("  mov rax, QWORD PTR %s[rip]\n", node->name);
    } else {
      gen_lval(node);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
    }
    printf("  push rax\n");
    return;
  }

  if (node->ty == ND_FUNCCALL) {
    // TODO: align rsp to 16 byte boundary.
    if (node->lhs->ty != ND_IDENT) {
      error("%s\n", "Function node doesn't have identifer.");
    }
    if (node->rhs != NULL) {
      gen(node->rhs);
      printf("  pop rax\n");
    }
    printf("  mov rbx, rsp\n");
    printf("  and rsp, ~0x0f\n");
    if (is_systemcall(node->lhs)) {
      gen_syscall(node->lhs);
    } else {
      printf("  call %s\n", node->lhs->name);
    }
    printf("  mov rsp, rbx\n");
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
    }
    printf("_if_end_%d:\n", end_label);
    return;
  }

  if (node->ty == ND_WHILE) {
    int while_label = label_counter++;
    int while_end = label_counter++;;
    printf("_while_%d:\n", while_label);
    gen(node->lhs);
    printf("  pop rax;\n");
    printf("  cmp rax, 0\n");
    printf("  je _while_end_%d\n", while_end);
    gen_block(node->block);
    printf("  jmp _while_%d\n", while_label);
    printf("_while_end_%d:\n", while_end);
    return;
  }

  if (node->lhs == NULL) {
    // Single term operation
    switch (node->ty) {
    case '+':
      gen(node->rhs);
      break;
    case '-':
      gen(node->rhs);
      printf("  pop rax\n");
      printf("  neg rax\n");
      printf("  push rax\n");
      break;
    case '&':
      // Reference.
      gen_lval(node->rhs);
      break;
    case '*':
      // De-reference.
      gen(node->rhs);
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      break;
    default:
      fprintf(stderr, "Error. Unsupported single term operation %d.\n", node->ty);
      exit(1);
    }
    return;
  }

  // two term operation
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
      printf("  setg al\n");
    } else {
      error("%s\n", "Code shouldn't reach here (codegen.c compare).");
    }
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}
