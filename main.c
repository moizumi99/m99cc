#include <stdio.h>
#include <string.h>
#include "9cc.h"

// Store nodes in this vector.
Vector *code;
// pics the pointer for a node at i-th position fcom code.
#define GET_CODE_P(i) ((Node *)code->data[i])

// Store variable look up table
Map *variables;

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
  code = new_vector();
  variables = new_map();

  // Tokenize and parse.
  tokenize(argv[1]);
  program();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // Prologue.
  // Secure room for 26 variables (26 * 8 = 208 bytes).
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", variables->keys->len * 8);

  // Generate codes from the top line to bottom
  for (int i = 0; GET_CODE_P(i); i++) {
    gen(GET_CODE_P(i));

    // The evaluated value is at the top of stack.
    // Need to pop this value so the stack is not overflown.
    printf("  pop rax\n");
  }

  // Epilogue
  // The value on the top of the stack is the final value.
  // The last value is already in rax, which is return value.
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  // Function call (just return 3)
  printf("_func:\n");
  printf("  mov rax, 3\n");
  printf("  ret\n");
  return 0;
}
