#include "m99cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Map *global_symbols;
extern Vector *local_symbols;
extern Vector *string_literals;

static Map *current_local_symbols;
static int label_counter = 0;
static Node *func_ident;

// from parse_test.c
char *get_type(int ty);

// from parse.c
int get_data_step_from_node(Node *node);

// from parse.c
Symbol *get_symbol(Map *global_symbol_table, Map *local_symbol_table,
                   Node *node);

int get_node_reference_type(Node *node) {
  if (node == NULL) {
    return DT_INVALID;
  }
  if (node->ty == ND_NUM) {
    return DT_INVALID;
  }
  if (node->ty == ND_IDENT) {
    Symbol *s = get_symbol(global_symbols, current_local_symbols, node);
    if (s == NULL) {
      fprintf(stderr, "Error. Symbol %s not found\n", node->name);
      exit(1);
    }
    int dtype = s->data_type->dtype;
    if (dtype != DT_PNT) {
      // Not a pointer.
      return DT_INVALID;
    }
    // Return the type of the pointer target.
    return s->data_type->pointer_type->dtype;
  }
  // String literal.
  if (node->ty == ND_STR) {
    return DT_CHAR;
  }
  int left_rtype = get_node_reference_type(node->lhs);
  int right_rtype = get_node_reference_type(node->rhs);
  if (left_rtype == right_rtype) {
    return left_rtype;
  }
  if (left_rtype != DT_INVALID && right_rtype == DT_INVALID) {
    return left_rtype;
  }
  if (left_rtype == DT_INVALID && right_rtype != DT_INVALID) {
    return right_rtype;
  }
  fprintf(stderr,
          "Reference data type of left hand value (%s) and right hand value "
          "(%s) don't match.",
          node->lhs->name, node->rhs->name);
  return DT_INVALID;
}

// Code generation from a block (a set of nodes).
void gen_block(Vector *block_code) {
  Node *next_node;
  for (int i = 0; (next_node = block_code->data[i]); i++) {
    printf("  pop rax\n");
    gen_node(next_node);
  }
}

// Create L-value (variable address and likes).
void gen_lval(Node *node) {
  if (node->ty == ND_IDENT) {
    printf("# L-value of ND_IDENT\n");
    Symbol *s = map_get(global_symbols, node->name);
    if (s != NULL) {
      // global address.
      printf("  lea rax, %s[rip]\n", node->name);
      printf("  push rax\n");
      return;
    } else {
      s = map_get(current_local_symbols, node->name);
      if (s == NULL) {
        fprintf(stderr, "Error. Undefined variable used.");
        exit(1);
      }
      // local address
      printf("  mov rax, rbp\n");
      printf("  sub rax, %d\n", (int)s->address);
      printf("  push rax\n");
      return;
    }
  }
  if (node->ty == ND_DEREF) {
    gen_node(node->lhs);
    return;
  }
  if (node->ty == ND_STR) {
    printf("  lea rax, STRLTR_%d[rip]\n", node->val);
    printf("  push rax\n");
    return;
  }
  fprintf(stderr, "The node type %d can not be a L-value", node->ty);
  exit(1);
}

int is_systemcall(Node *nd) {
  if (strcmp(nd->name, "putchar") == 0) {
    return 1;
  }
  // todo: add more system calls.
  // todo: add getchar system call.
  return 0;
}

void gen_syscall(Node *nd) {
  printf("# System Call\n");
  if (strcmp(nd->name, "putchar") == 0) {
    printf("  mov edi, eax\n");
    printf("  call putchar@PLT\n");
    printf("  mov eax, 0\n");
  } else {
    error("%s is not supported yet.\n", nd->name);
  }
}

void gen_ident(Node *node) {
  printf("# ND_IDENT\n");
  Symbol *s = map_get(current_local_symbols, node->name);
  if (s != NULL) {
    // Local variable.
    gen_lval(node);
    if (s->num == 0) {
      // regular variable. De-reference.
      printf("  pop rax\n");
      int dtype = s->data_type->dtype;
      if (dtype == DT_INT) {
        printf("  mov rax, [rax]\n");
      } else if (dtype == DT_CHAR) {
        printf("  mov al, [rax]\n");
      } else if (dtype == DT_PNT) {
        printf("  mov rax, [rax]\n");
      } else {
        fprintf(stderr, "Data type: %d\n", dtype);
        error("\"%s\" type is not INT or CHAR. (local load)", node->name);
      }
      printf("  push rax\n");
    }
    // If array address, location of the array is what you need. Do nothing to l-value.
  } else {
    // Global variable or function call.
    s = map_get(global_symbols, node->name);
    if (s == NULL) {
      fprintf(stderr, "The symbol %s was not found.", node->name);
      exit(1);
    }
    // Function calll or global variable.
    if (s->num == 0) {
      // regular variable. De-reference.
      int dtype = s->data_type->dtype;
      if (dtype == DT_INT) {
        printf("  mov rax, QWORD PTR %s[rip]\n", node->name);
      } else if (dtype == DT_CHAR) {
        printf("  mov al, BYTE PTR %s[rip]\n", node->name);
      } else if (dtype == DT_PNT) {
        printf("  mov rax, QWORD PTR %s[rip]\n", node->name);
      } else {
        error("\"%s\" type is not INT or CHAR. (global load)", node->name);
      }
    } else {
      // Array address. Don't de-reference.
      printf("  lea rax, %s[rip]\n", node->name);
    }
    printf("  push rax\n");
  }
}

void gen_funccall(Node *node) {
  printf("# ND_FUNCCALL\n");
  if (node->lhs->ty != ND_IDENT) {
    error("%s\n", "Function node doesn't have identifer.");
  }
  if (node->rhs != NULL) {
    gen_node(node->rhs);
    printf("  pop rax\n");
  }
  // Keep the current rsp to rbx (callee save register).
  printf("  mov rbx, rsp\n");
  // Align rsp to 16 byte boundary.
  printf("  and rsp, ~0x0f\n");
  if (is_systemcall(node->lhs)) {
    gen_syscall(node->lhs);
  } else {
    printf("  call %s\n", node->lhs->name);
  }
  // Retrieve rsp from rbx (calle save register).
  printf("  mov rsp, rbx\n");
  printf("  push rax\n");
}

void gen_substitute(Node *node) {
  printf("# Substitute\n");
  gen_lval(node->lhs);
  gen_node(node->rhs);
  printf("  pop rdi\n");
  printf("  pop rax\n");
  if (node->ty == '=') {
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
  } else if (node->ty == ND_PE) {
    printf("  mov rbx, [rax]\n");
    printf("  add rdi, rbx\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
  } else if (node->ty == ND_ME) {
    printf("  mov rbx, [rax]\n");
    printf("  sub rbx, rdi\n");
    printf("  mov [rax], rbx\n");
    printf("  push rbx\n");
  }
  // TODO: Add *= and /=.
}

void gen_if(Node *node) {
  printf("# ND_IF\n");
  gen_node(node->lhs);
  printf("  pop rax;\n");
  printf("  cmp rax, 0\n");
  printf("  push rax;\n");
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
      error("Unexpected non ND_BLOCK node %s after if-else \n", node->name);
    }
  }
  printf("_if_end_%d:\n", end_label);
}

void gen_while(Node *node) {
  printf("# ND_WHILE\n");
  int while_label = label_counter++;
  int while_end = label_counter++;
  printf("_while_%d:\n", while_label);
  gen_node(node->lhs);
  printf("  pop rax;\n");
  printf("  cmp rax, 0\n");
  printf("  push rax;\n");
  printf("  je _while_end_%d\n", while_end);
  gen_block(node->block);
  printf("  jmp _while_%d\n", while_label);
  printf("_while_end_%d:\n", while_end);
}

void gen_single_term_operation(Node *node) {
  printf("# Single term operation(%s)\n", get_type(node->ty));
  // Single term operation
  switch (node->ty) {
  case ND_INC:
  case ND_DEC:
    gen_lval(node->lhs);
    printf("  pop rax\n");
    printf("  mov rdi, [rax]\n");
    // Move this tep out to parse stage. Then, remove get_data_step_from_node().
    int step = get_data_step_from_node(node->lhs);
    if (node->ty == ND_INC) {
      printf("  add rdi, %d\n", step);
    } else {
      printf("  sub rdi, %d\n", step);
    }
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    break;
  case '+':
    gen_node(node->lhs);
    break;
  case '-':
    gen_node(node->lhs);
    printf("  pop rax\n");
    printf("  neg rax\n");
    printf("  push rax\n");
    break;
  case '&':
    // Reference.
    gen_lval(node->lhs);
    break;
  case ND_DEREF:
    // De-reference.
    gen_node(node->lhs);
    int dtype = get_node_reference_type(node->lhs);
    printf("  pop rax\n");
    if (dtype == DT_CHAR) {
      printf("  mov al, BYTE PTR [rax]\n");
      printf("  and rax, 0xff\n");
    } else {
      printf("  mov rax, QWORD PTR [rax]\n");
    }
    printf("  push rax\n");
    break;
  default:
    fprintf(stderr, "Error. Unsupported single term operation %d.\n", node->ty);
    exit(1);
  }
}

void gen_two_term_operation(Node *node) {
  // two term operation
  printf("# Two term operation (%s)\n", get_type(node->ty));
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
  case ND_LE:
  case ND_GE:
    printf("  cmp rdi, rax\n");
    if (node->ty == ND_EQ) {
      printf("  sete al\n");
    } else if (node->ty == ND_NE) {
      printf("  setne al\n");
    } else if (node->ty == '>') {
      printf("  setl al\n");
    } else if (node->ty == '<') {
      printf("  setg al\n");
    } else if (node->ty == ND_LE) {
      printf("  setle al\n");
    } else if (node->ty == ND_GE) {
      printf("  setge al\n");
    } else {
      error("%s\n", "Code shouldn't reach here (codegen.c compare).");
    }
    printf("  movzb rax, al\n");
    break;
  case ND_AND:
    printf("  and rax, rdi\n");
    break;
  case ND_OR:
    printf("  or rax, rdi\n");
    break;
  case ND_IDENTSEQ:
    // do nothing.
    break;
  default:
    fprintf(stderr, "Operation %s is not supported.\n", get_type(node->ty));
    exit(1);
    break;
  }
  printf("  push rax\n");
}

void gen_node(Node *node) {
  if (node->ty == ND_NUM) {
    printf("# ND_NUM\n");
    printf("  push %d\n", node->val);
    return;
  }
  if (node->ty == ND_FUNCDEF) {
    // code shouldn't reach here
    return;
  }
  if (node->ty == ND_IDENT) {
    gen_ident(node);
    return;
  }
  if (node->ty == ND_STR) {
    printf("# ND_STR\n");
    gen_lval(node);
    return;
  }
  if (node->ty == ND_FUNCCALL) {
    gen_funccall(node);
    return;
  }
  if (node->ty == '=' || node->ty == ND_PE || node->ty == ND_ME) {
    gen_substitute(node);
    return;
  }
  if (node->ty == ND_IF) {
    gen_if(node);
    return;
  }
  if (node->ty == ND_WHILE) {
    gen_while(node);
    return;
  }
  if (node->ty == ND_RETURN) {
    printf("# ND_RETURN\n");
    gen_node(node->lhs);
    printf("  jmp %s_end\n", func_ident->name);
    return;
  }
  if (node->ty == ND_DECLARE) {
    printf("# ND_DECLARE\n");
    gen_node(node->rhs);
    return;
  }
  if (node->rhs == NULL) {
    gen_single_term_operation(node);
    return;
  }
  gen_two_term_operation(node);
}

// Calculate the necessary stack size for local variable.
int accumulate_variable_size(Vector *symbols) {
  int num = 0;
  for (int i = 0; i < symbols->len; i++) {
    Symbol *s = (Symbol *)symbols->data[i];
    int variable_size = (s->num == 0) ? 1 : s->num;
    num += variable_size * data_size(s->data_type);
  }
  return num;
}

void gen_declartion(Map *local_symbol_table, Node *declaration_node) {
  current_local_symbols = local_symbol_table;
  // functions
  Node *declaration = declaration_node;
  if (declaration->ty != ND_DECLARE) {
    fprintf(stderr, "Defintion of variable or function expected.\n");
    exit(1);
  }
  Node *identifier = declaration->rhs;
  if (identifier->ty == ND_IDENT) {
    // TODO: add initialization.
    return;
  }
  if (identifier->ty == ND_IDENTSEQ) {
    // TODO: add initialization.
    return;
  }
  if (identifier->ty != ND_FUNCDEF) {
    fprintf(stderr,
            "The first line of the function isn't function definition\n");
    exit(1);
  }
  func_ident = identifier->lhs;
  printf("%s:\n", func_ident->name);
  // Prologue.
  printf("  push rbx\n");
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  // Secure room for variables
  int local_variable_size =
      accumulate_variable_size(current_local_symbols->vals);
  printf("  sub rsp, %d\n", local_variable_size);
  // store argument
  int symbol_number = current_local_symbols->vals->len;
  for (int arg_cnt = 0; arg_cnt < symbol_number; arg_cnt++) {
    Symbol *next_symbol = (Symbol *)current_local_symbols->vals->data[arg_cnt];
    if (next_symbol->type != ID_ARG) {
      continue;
    }
    if (arg_cnt == 0) {
      printf("  mov [rbp - %d], rax\n", (int)next_symbol->address);
    } else {
      // TODO: Support two or more argunents.
      fprintf(stderr, "Error: Currently, only one argument can be used.");
      exit(1);
    }
  }
  // Generate codes from the top line to bottom
  Vector *block = identifier->block;
  // dummy push to be popped by gen_block. (Then optimized later);
  printf("  push rax\n");

  gen_block(block);

  printf("%s_end:\n", func_ident->name);
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

void gen_string_literals(Vector *string_literals_table) {
  // String literals
  printf(".Ltext0:\n");
  printf("  .section .rodata\n");
  for (int i = 0; i < string_literals_table->len; i++) {
    printf("STRLTR_%d:\n", i);
    printf("  .string ");
    char *p = string_literals_table->data[i];
    while (*p != '\0') {
      putchar(*(p++));
    }
    putchar('\n');
  }
}

void gen_global_symbols(Map *global_symbols_table) {
  printf("  .text\n");
  for (int i = 0; i < global_symbols_table->keys->len; i++) {
    Symbol *s = global_symbols_table->vals->data[i];
    char *name = (char *)global_symbols_table->keys->data[i];
    if (s->type == ID_VAR) {
      int num = (s->num > 0) ? s->num : 1;
      printf("  .comm  %s, %d, %d\n", name, num * 8, num * 8);
    }
  }
}

void gen_program(Vector *program_code) {
  printf("  .intel_syntax noprefix\n");

  // Global variables.
  if (global_symbols->keys->len > 0) {
    gen_global_symbols(global_symbols);
  }
  // String literals.
  if (string_literals->len > 0) {
    gen_string_literals(string_literals);
  }
  // Main function.
  printf("  .text\n");
  printf(".global main\n");
  printf(".type main, @function\n");
  for (int j = 0; program_code->data[j]; j++) {
    gen_declartion(local_symbols->data[j], program_code->data[j]);
  }
}
