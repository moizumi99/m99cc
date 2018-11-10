#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Token values
enum {
  TK_NUM = 256, // integer token
  TK_EOF,       // end of input token
};

// Token types
typedef struct {
  int ty;   // token type
  int val;  // value if ty is TK_NUM
  char *input; // token string for error message
} Token;

// Store tokens in this array. Max is 100 for now.
Token tokens[100];

// split chars pointed by p into tokens
void tokenize(char *p) {
  int i = 0;
  while (*p) {
    // skip spaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '+' || *p == '-') {
      tokens[i].ty = *p;
      tokens[i].input = p;
      i++;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      tokens[i].ty = TK_NUM;
      tokens[i].input = p;
      tokens[i].val = strtol(p, &p, 10);
      i++;
      continue;
    }

    fprintf(stderr, "Can't tokenize: %s\n", p);
    exit(1);
  }

  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
}

// Error reporting function.
void error(int i) {
  fprintf(stderr, "Unexpected token: %s\n",
          tokens[i].input);
  exit(1);
}
             
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Arguments number not right.\n");
    return 1;
  }

  // Tokenize.
  tokenize(argv[1]);
  
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // The head of the equation must be a number.
  // Check it and output the first mov.
  if (tokens[0].ty != TK_NUM) {
    error(0);
  }
  printf("  mov rax, %d\n", tokens[0].val);

  // Find '+ <number>' or '- <number>' tokens,
  // and output corresponding Assembly codes.
  int i = 1;
  while (tokens[i].ty != TK_EOF) {
    if (tokens[i].ty == '+') {
      i++;
      if (tokens[i].ty != TK_NUM) {
        error(i);
      }
      printf("  add rax, %d\n", tokens[i].val);
      i++;
      continue;
    }
    
    if (tokens[i].ty == '-') {
      i++;
      if (tokens[i].ty != TK_NUM) {
        error(1);
      }
      printf("  sub rax, %d\n", tokens[i].val);
      i++;
      continue;
    }

    error(i);
  }
  
  printf("  ret\n");
  return 0;
}
