#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "m99cc.h"


// Store variable look up table
// Put variables into vector, and make a list of variable for each code block.
Map *global_symbols;
Vector *local_symbols;
// Map *current_local_symbols;

// for debugging.
void dump_symbols(Map *);
void dump_tree(Vector *code);

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Arguments number not right.\n");
    return 1;
  }

  bool dump_tree_enbale = false;
  bool dump_symbols_enable = false;
  FILE *srcfile;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-test") == 0) {
      runtest();
      return 0;
    }
    if (strcmp(argv[i], "-test_token") == 0) {
      runtest_tokenize();
      return 0;
    }
    if (strcmp(argv[i], "-test_parse") == 0) {
      runtest_parse();
      return 0;
    }

    if (strcmp(argv[i], "-dump_tree") == 0) {
      dump_tree_enbale = true;
      continue;
    }
    if (strcmp(argv[i], "-dump_symbols") == 0) {
      dump_symbols_enable = true;
      continue;
    }
    srcfile = fopen(argv[i], "r");
  }

  // initialize
  // Store nodes in this vector.
  Vector *program_code;
  // program_code = new_vector();
  global_symbols = new_map();
  local_symbols = new_vector();

  // open input
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
  Vector *tokens;

  tokens = tokenize(src);
  // Parse
  program_code = parse(tokens);

  if (dump_symbols_enable) {
    dump_symbols(global_symbols);
    for(int i = 0; local_symbols->data[i]; i++) {
      dump_symbols((Map *)(local_symbols->data[i]));
    }
  }
  if (dump_tree_enbale) {
    dump_tree(program_code);
  }
  gen_program(program_code);

  return 0;
}
