#include <stdio.h>
#include "9cc.h"

// Store nodes in this array. Max is 100 fornow.
Node *code[100];

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Arguments number not right.\n");
    return 1;
  }

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
  printf("  sub rsp, 208\n");

  // Generate codes from the top line to bottom
  for (int i = 0; code[i]; i++) {
    gen(code[i]);

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
  return 0;
}
