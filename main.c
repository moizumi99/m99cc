#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "m99cc.h"


// Store variable look up table
// Put variables into vector, and make a list of variable for each code block.
Map *global_symbols;
Vector *local_symbols;
Vector *string_literals;

// for debugging.
void dump_symbols(Map *);
void dump_tree(Vector *code);

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Arguments number not right.\n");
    fprintf(stderr, "Usage: %s [file] [-test] [-test_token]"
            " [-test_parse] [-dump_tree] "
            "[-dump_symbols][-dump_tokens]\n", argv[0]);
    return 1;
  }

  bool dump_tokens_enbale = false;
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
    if (strcmp(argv[i], "-test_dtype") == 0) {
      runtest_data_type();
      return 0;
    }
    if (strcmp(argv[i], "-dump_tokens") == 0) {
      dump_tokens_enbale = true;
      continue;
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
  string_literals = new_vector();

  // open input
  if (srcfile == NULL) {
    fprintf(stderr, "Can't open file %s\n", argv[1]);
    exit(1);
  }
  fseek( srcfile , 0L , SEEK_END);
  long size = ftell( srcfile );
  rewind( srcfile );
  char *src = malloc(size + 1);
  char *sp = src;

  while(!feof(srcfile)) {
    *(sp++) = fgetc(srcfile);
  }
  *(sp-1) = '\0';
  fclose(srcfile);

  // Tokenize
  Vector *tokens;

  tokens = tokenize(src);
  if (dump_tokens_enbale) {
    dump_token();
  }
  // Parse
  program_code = parse(tokens);

  program_code = analysis(program_code);

  if (dump_symbols_enable) {
    fprintf(stderr, "Global Symbols: \n");
    dump_symbols(global_symbols);
    for(int i = 0; local_symbols->data[i]; i++) {
      fprintf(stderr, "Local Symbols [%d]: \n", i);
      dump_symbols((Map *)(local_symbols->data[i]));
    }
  }
  if (dump_tree_enbale) {
    dump_tree(program_code);
  }
  gen_program(program_code);

  return 0;
}
