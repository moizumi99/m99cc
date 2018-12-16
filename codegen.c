#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m99cc.h"

extern Map *global_symbols;
extern Vector *local_symbols;
Map *current_local_symbols;
static int label_counter = 0;

Node *get_node_p(Vector *code, int i) {
  return (Node *)code->data[i];
}

void gen_block(Vector *block_code) {
    for (int i = 0; get_node_p(block_code, i); i++)
      gen_node(get_node_p(block_code, i));
}

void gen_lval(Node *node) {
  if (node->ty == ND_IDENT) {
    if (get_symbol_address(global_symbols, node->name) != NULL) {
      // global address.
      printf("  lea rax, %s[rip]\n", node->name);
      printf("  push rax\n");
      return;
    }
    printf("  mov rax, rbp\n");
    void *address = get_symbol_address(current_local_symbols, node->name);
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
    gen_node(node->rhs);
    return;
  }
  fprintf(stderr, "The node type %d can not be left size value", node->ty);
  exit(1);
}

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

void gen_node(Node *node) {
  if (node->ty == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  if (node->ty == ND_FUNCDEF) {
    // code shouldn't reach here
    return;
  }

  if (node->ty == ND_IDENT) {
    if (get_symbol_address(global_symbols, node->name) != NULL) {
      if (get_symbol_size(global_symbols, node->name) == 0) {
        // regular variable. De-reference.
        printf("  mov rax, QWORD PTR %s[rip]\n", node->name);
      } else {
        // Array address. Don't de-reference.
        printf("  lea rax, %s[rip]\n", node->name);
      }
      printf("  push rax\n");
    }else {
      gen_lval(node);
      if (get_symbol_size(current_local_symbols, node->name) == 0) {
        // regular variable. De-reference.
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
      }
      // Don't de-reference if array address.
    }
    return;
  }

  if (node->ty == ND_FUNCCALL) {
    // TODO: align rsp to 16 byte boundary.
    if (node->lhs->ty != ND_IDENT) {
      error("%s\n", "Function node doesn't have identifer.");
    }
    if (node->rhs != NULL) {
      gen_node(node->rhs);
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
    gen_node(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }

  if (node->ty == ND_IF) {
    gen_node(node->lhs);
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
        gen_node(node->rhs);
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
    gen_node(node->lhs);
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
      gen_node(node->rhs);
      break;
    case '-':
      gen_node(node->rhs);
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
      gen_node(node->rhs);
      printf("  pop rax\n");
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
  gen_node(node->lhs);
  gen_node(node->rhs);

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

int accumulate_variable_size(Vector *symbols) {
  int num = 0;
  for (int i = 0; i < symbols->len; i++) {
    Symbol *s = (Symbol *)symbols->data[i];
    num += (s->num == 0) ? 1 : s->num;
  }
  return num;
}

// pics the pointer for a node at i-th position fcom code.
/* #define GET_FUNCTION_P(j) ((Node *)program_code->data[j]) */
/* #define GET_NODE_P(j, i) ((Node *)(((Vector *)GET_FUNCTION_P(j)->block)->data[i])) */

Node *get_function_p(Vector *program_code, int i) {
  return (Node *) program_code->data[i];
}

void gen_program(Vector *program_code) {
  printf("  .intel_syntax noprefix\n");

  // Global variables.
  if (global_symbols->keys->len > 0) {
    printf("  .text\n");
    for (int i = 0; i < global_symbols->keys->len; i++) {
      Symbol *s = global_symbols->vals->data[i];
      char *name = (char *) global_symbols->keys->data[i];
      if (s->type == ID_VAR) {
        int num = (s->num > 0) ? s->num : 1;
        printf("  .comm  %s, %d, %d\n", name, num * 8, num * 8);
      }
    }
  }

  // Main function.
  printf("  .text\n");
  printf(".global main\n");
  printf(".type main, @function\n");
  printf("main:\n");
  printf("  call func_main\n");
  printf("  ret\n");

  for (int j = 0; program_code->data[j]; j++) {
    current_local_symbols = (Map *)local_symbols->data[j];
    //dump_symbols(current_local_symbols);
    // functions
    Node *identifier = get_function_p(program_code, j);
    if (identifier->ty == ND_IDENT) {
      // TODO: add initialization.
      continue;
    }
    if (identifier->ty != ND_FUNCDEF) {
      fprintf(stderr, "The first line of the function isn't function definition");
      exit(1);
    }
    Node *func_ident = identifier->lhs;
    if (strcmp(func_ident->name, "main") == 0) {
      printf("func_main:\n");
    } else {
      printf("%s:\n", func_ident->name);
    }
    // Prologue.
    printf("  push rbx\n");
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    // Secure room for variables
    int local_variable_size = accumulate_variable_size(current_local_symbols->vals);
    printf("  sub rsp, %d\n", local_variable_size * 8);
    // store argument
    int symbol_number = current_local_symbols->vals->len;
    for(int arg_cnt = 0; arg_cnt < symbol_number; arg_cnt++) {
      Symbol *next_symbol = (Symbol *)current_local_symbols->vals->data[arg_cnt];
      if (next_symbol->type != ID_ARG) {
        continue;
      }
      if (arg_cnt == 0) {
        printf("  mov [rbp - %d], rax\n", (int) next_symbol->address);
      } else {
        // TODO: Support two or more argunents.
        fprintf(stderr, "Error: Currently, only one argument can be used.");
        exit(1);
      }
    }
    // Generate codes from the top line to bottom
    Vector *block = identifier->block;
    gen_block(block);

    // The evaluated value is at the top of stack.
    // Need to pop this value so the stack is not overflown.
    printf("  pop rax\n");
    // Epilogue
    // The value on the top of the stack is the final value.
    // The last value is already in rax, which is return value.
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  pop rbx\n");
    printf("  ret\n");
  }
}
