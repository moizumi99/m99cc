#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "m99cc.h"

// Store nodes in this vector.
Vector *program_code;

// Store variable look up table
// Put variables into vector, and make a list of variable for each code block.
Map *global_symbols;
Vector *local_symbols;
Map *current_local_symbols;

/* // for debugging. */
/* void dump_symbols(Map *); */

// pics the pointer for a node at i-th position fcom code.
/* #define GET_FUNCTION_P(j) ((Node *)program_code->data[j]) */
/* #define GET_NODE_P(j, i) ((Node *)(((Vector *)GET_FUNCTION_P(j)->block)->data[i])) */

Node *get_function_p(int i) {
  return (Node *) program_code->data[i];
}

void gen_block(Vector *block_code);

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Arguments number not right.\n");
    return 1;
  }

  if (strcmp(argv[1], "-test") == 0) {
    runtest();
    return 0;
  }

  if (strcmp(argv[1], "-test_token") == 0) {
    runtest_tokenize();
    return 0;
  }

  // initialize
  program_code = new_vector();
  global_symbols = new_map();
  local_symbols = new_vector();

  // open input
  FILE *srcfile = fopen(argv[1], "r");
  if (srcfile == NULL) {
    fprintf(stderr, "Cant open file %s\n", argv[1]);
    exit(1);
  }
  int size = 256;
  char *src = malloc(size);
  char *sp = src;
  int cnt = 0;
  while(!feof(srcfile)) {
    *(sp++) = fgetc(srcfile);
    if (++cnt >= size - 1) {
      size += 256;
      src = (char *)realloc(src, size);
    }
  }
  *(sp-1) = '\0';
  fclose(srcfile);

  // Tokenize
  tokenize(src);
  // Parse
  program(program_code);

  printf("  .intel_syntax noprefix\n");
  printf("  .text\n");

  // Global variables.
  for (int i = 0; i < global_symbols->keys->len; i++) {
    Symbol *s = global_symbols->vals->data[i];
    char *name = (char *) global_symbols->keys->data[i];
    if (s->type == ID_VAR) {
      printf("  .globl  %s\n", name);
      printf("  .bss\n");
      printf("  .align 8\n");
      printf("  .type   %s, @object\n", name);
      printf("  .size   %s, 4\n", name);
      printf("%s:\n", name);
      printf("  .zero   8\n");
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
    Node *identifier = get_function_p(j);
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
    printf("  sub rsp, %d\n", current_local_symbols->keys->len * 8);
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
  return 0;
}
