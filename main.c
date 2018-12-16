#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "m99cc.h"


Vector *tokens;

// Store variable look up table
// Put variables into vector, and make a list of variable for each code block.
Map *global_symbols;
Vector *local_symbols;
Map *current_local_symbols;

/* // for debugging. */
/* void dump_symbols(Map *); */


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

  if (strcmp(argv[1], "-test_parse") == 0) {
    runtest_parse();
    return 0;
  }

  // initialize
  // Store nodes in this vector.
  Vector *program_code;
  // program_code = new_vector();
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
  tokens = tokenize(src);
  // Parse
  program_code = parse();

  gen_program(program_code);

  return 0;
}
