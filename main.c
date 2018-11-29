#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "9cc.h"

// Store nodes in this vector.
Vector *program_code;

// Store variable look up table
// Put variables into vector, and make a list of variable for each code block.
Map *global_symbols;
Vector *local_symbols;
Map *current_local_symbols;

// pics the pointer for a node at i-th position fcom code.
#define GET_NODE_P(j, i) ((Node *)((Vector *)program_code->data[j])->data[i])

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Arguments number not right.\n");
    return 1;
  }

  if (strcmp(argv[1], "-test") == 0) {
    runtest();
    return 0;
  }

  // initialize
  program_code = new_vector();
  global_symbols = new_map();
  local_symbols = new_vector();

  // Tokenize
  tokenize(argv[1]);
  // Parse
  program(program_code);

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  printf("  call func_main\n");
  printf("  ret\n");

  for (int j = 0; program_code->data[j]; j++) {
    current_local_symbols = (Map *)local_symbols->data[j];
    // functions
    if (GET_NODE_P(j, 0)->ty != ND_FUNCDEF) {
      fprintf(stderr, "The first line of the function isn't function definition");
      exit(1);
    }
    Node *func_ident = GET_NODE_P(j, 0)->lhs;
    if (strcmp(func_ident->name, "main") == 0) {
      printf("func_main:\n");
    } else {
      printf("%s:\n", func_ident->name);
    }
    // Prologue.
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    // Secure room for variables
    printf("  sub rsp, %d\n", current_local_symbols->keys->len * 8);
    // argument
    printf("  mov [rbp - 8], rax\n");
    // Generate codes from the top line to bottom
    for (int i = 1; GET_NODE_P(j, i); i++)
      gen(GET_NODE_P(j, i));

    // The evaluated value is at the top of stack.
    // Need to pop this value so the stack is not overflown.
    printf("  pop rax\n");
    // Epilogue
    // The value on the top of the stack is the final value.
    // The last value is already in rax, which is return value.
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
  return 0;
}
