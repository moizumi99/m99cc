#include "m99cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//TODO: shouldn't this be GET_TOKEN(T, I) ((Token *)(T)->data[(I)]) ?
#define GET_TOKEN(T, I) ((Token *)(T)->data[(I)])

static Vector *tokens;
static int pos;

enum {
  SC_GLOBAL, // Global scope.
  SC_LOCAL   // Local scope.
};

// Error reporting function.
void error(char *s, char *message, char *file, int line) {
  fprintf(stderr, s, message);
  fprintf(stderr, " in %s, at %d.", file, line);
  fprintf(stderr, "\n");
  exit(1);
}

char *create_string_in_heap(char *str, int len) {
  len = (len < IDENT_LEN - 1) ? len : IDENT_LEN - 1;
  char *str_mem = malloc(sizeof(char) * (len + 1));
  strncpy(str_mem, str, len);
  str_mem[len] = '\0';
  return str_mem;
}

Node *new_node(int op) {
  Node *node = malloc(sizeof(Node));
  node->ty = op;
  node->lhs = NULL;
  node->rhs = NULL;
  node->val = 0;
  node->name = NULL;
  node->block = NULL;
  return node;
}

Node *new_node_str(char *str, int len) {
  Node *node = new_node(ND_STR);
  node->name = create_string_in_heap(str, len);
  return node;
}

Node *new_2term_node(int op, Node *lhs, Node *rhs) {
  Node *node = new_node(op);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

int get_array_size() {
  if (GET_TOKEN(tokens, pos)->ty != '[') {
    return 0;
  }
  ++pos;
  int num = 0;
  if (GET_TOKEN(tokens, pos)->ty == TK_NUM) {
    num = GET_TOKEN(tokens, pos++)->val;
  }
  if (GET_TOKEN(tokens, pos++)->ty != ']') {
    fprintf(stderr, "Closing bracket ']' is missing (get_array_size): \"%s\"\n",
            GET_TOKEN(tokens, pos - 1)->input);
    exit(1);
  }
  return num;
}

Node *new_node_ident(char *name, int len) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->lhs = NULL;
  node->rhs = NULL;
  node->val = 0;
  node->name = create_string_in_heap(name, len);
  node->block = NULL;
  return node;
}

#define HIGH_PRIORITY (8)
#define MULTIPLE_PRIORITY (HIGH_PRIORITY - 1)
#define ADD_PRIORITY (HIGH_PRIORITY - 2)
#define COMPARE_PRIORITY (HIGH_PRIORITY - 3)
#define EQUAL_PRIORITY (HIGH_PRIORITY - 4)
#define AND_PRIORITY (HIGH_PRIORITY - 5)
#define OR_PRIORITY (HIGH_PRIORITY - 6)
#define ASSIGN_PRIORITY (HIGH_PRIORITY - 7)
#define LOW_PRIORITY (0)

int operation_priority(int token_type) {
  switch (token_type) {
  case '*':
  case '/':
  case '%':
    return MULTIPLE_PRIORITY;
  case '+':
  case '-':
    return ADD_PRIORITY;
  case '<':
  case '>':
  case TK_GE:
  case TK_LE:
    return COMPARE_PRIORITY;
  case TK_EQ:
  case TK_NE:
    return EQUAL_PRIORITY;
  case TK_AND:
    return AND_PRIORITY;
  case TK_OR:
    return OR_PRIORITY;
  case '=':
    return ASSIGN_PRIORITY;
  default:
    return LOW_PRIORITY;
  }
}

int get_node_type_from_token(int token_type) {
  switch (token_type) {
  case TK_NUM:
    return ND_NUM;
  case TK_IDENT:
    return ND_IDENT;
  case TK_EQ:
    return ND_EQ;
  case TK_NE:
    return ND_NE;
  case TK_LE:
    return ND_LE;
  case TK_GE:
    return ND_GE;
  case TK_AND:
    return ND_AND;
  case TK_OR:
    return ND_OR;
  case TK_STR:
    return ND_STR;
  case TK_PE:
    return ND_PE;
  case TK_ME:
    return ND_ME;
  default:
    // For other operations (*/+- others, token_type -> node_type)
    return token_type;
  }
}

int get_node_type_from_token_single(int token_type) {
  if (token_type == '*') {
    return ND_DEREF;
  }
  return get_node_type_from_token(token_type);
}

int get_dtype_from_token(int token_type) {
  switch (token_type) {
  case TK_VOID:
    return DT_VOID;
  case TK_CHAR:
    return DT_CHAR;
  case TK_INT:
    return DT_INT;
  case TK_STRUCT:
    return DT_STRUCT;
  default:
    return DT_INVALID;
  }
}

// TODO: Re-write this with global pos.
Node *get_data_type_node_at_pos() {
  Token *tk = GET_TOKEN(tokens, pos++);
  int dtype = get_dtype_from_token(tk->ty);
  Node *node = new_node(ND_DATATYPE);
  if (dtype == DT_VOID) {
    node->lhs = new_node(ND_VOID);
  } else if (dtype == DT_INT) {
    node->lhs = new_node(ND_INT);
  } else if (dtype == DT_CHAR) {
    node->lhs = new_node(ND_CHAR);
  } else {
    error("Invalid data type token %s", tk->input, __FILE__, __LINE__);
  }
  while (GET_TOKEN(tokens, pos)->ty == '*') {
    pos++;
    node = new_2term_node(ND_DATATYPE, new_node(ND_PNT), node);
  }
  return node;
}

Node *term();
Node *argument();

Node *expression(int priority) {
  if (priority == HIGH_PRIORITY) {
    return term();
  }
  Node *lhs = expression(priority + 1);
  if (GET_TOKEN(tokens, pos)->ty == ';') {
    return lhs;
  }
  int token_type = GET_TOKEN(tokens, pos)->ty;
  if (operation_priority(token_type) == priority) {
    pos++;
    int node_type = get_node_type_from_token(token_type);
    Node *rhs = expression(priority);
    return new_2term_node(node_type, lhs, rhs);
  }
  return lhs;
}

// term : number | identifier | ( expression )
Node *term() {
  // Simple number
  if (GET_TOKEN(tokens, pos)->ty == TK_NUM) {
    return new_node_num(GET_TOKEN(tokens, pos++)->val);
  }
  Token *tk = GET_TOKEN(tokens, pos);
  if (tk->ty == TK_STR) {
    char *str = tk->input;
    int len = tk->len;
    pos++;
    return new_node_str(str, len);
  }

  // Variable or Function
  tk = GET_TOKEN(tokens, pos);
  if (tk->ty == TK_IDENT) {
    Node *id = new_node_ident(tk->input,
                              tk->len);
    pos++;
    // if followed by (, it's a function call.
    if (GET_TOKEN(tokens, pos)->ty == '(') {
      ++pos;
      Node *arg = NULL;
      // Argument exists.
      if (GET_TOKEN(tokens, pos)->ty != ')') {
        arg = expression(ASSIGN_PRIORITY);
      }
      if (GET_TOKEN(tokens, pos)->ty != ')') {
        error("No right parenthesis corresponding to left parenthesis"
              " (term, function): \"%s\"",
              GET_TOKEN(tokens, pos)->input, __FILE__, __LINE__);
      }
      pos++;
      Node *node = new_2term_node(ND_FUNCCALL, id, arg);
      return node;
    }
    // If not followed by (, it's a variable.
    // is it array?
    Node *node = id;
    if (GET_TOKEN(tokens, pos)->ty == '[') {
      pos++;
      Node *index = expression(ASSIGN_PRIORITY);
      if (GET_TOKEN(tokens, pos++)->ty != ']') {
        fprintf(stderr, "Closing bracket ']' missing. \"%s\"\n",
                GET_TOKEN(tokens, pos - 1)->input);
        exit(1);
      }
      node = new_2term_node(ND_DEREF, new_2term_node('+', id, index), NULL);
    }
    return node;
  }
  // "( expression )"
  if (GET_TOKEN(tokens, pos)->ty == '(') {
    pos++;
    Node *node = expression(ASSIGN_PRIORITY);
    if (GET_TOKEN(tokens, pos)->ty != ')') {
      error("No right parenthesis corresponding to left parenthesis "
            "(term, parenthesis): \"%s\"",
            GET_TOKEN(tokens, pos)->input, __FILE__, __LINE__);
    }
    pos++;
    return node;
  }
  // Single term operators
  tk = GET_TOKEN(tokens, pos);
  if (tk->ty == '+' || tk->ty == '-' ||
      tk->ty == '*' || tk->ty == '&') {
    int type = tk->ty;
    pos++;
    Node *lhs = term();
    return new_2term_node(get_node_type_from_token_single(type), lhs, NULL);
  }

  if (tk->ty == TK_INC || tk->ty == TK_DEC) {
    // convert ++x to (x += 1).
    int operation = (tk->ty == TK_INC) ? ND_PE : ND_ME;
    pos++;
    Node *lhs = term();
    int step = 1;
    Node *rhs = new_node_num(step);
    return new_2term_node(operation, lhs, rhs);
  }

  // Code should not reach here.
  error("Unexpected token (parse.c term): \"%s\"",
        GET_TOKEN(tokens, pos)->input, __FILE__, __LINE__);
  return NULL;
}

Node *argument() {
  // TODO: make argument a list.
  Node *node_dt = get_data_type_node_at_pos();
  Token *tk = GET_TOKEN(tokens, pos);
  if (tk->ty != TK_IDENT) {
    error("Invalid (not IDENT) token in argument declaration position \"%s\"",
          tk->input, __FILE__, __LINE__);
  }
  // argument name?
  tk = GET_TOKEN(tokens, pos);
  char *name = create_string_in_heap(tk->input,
                                     tk->len);
  int array_size = get_array_size();
  if (array_size > 0) {
    // Make the node_dt pointer node
    node_dt = new_2term_node(ND_DATATYPE, new_node(ND_PNT), node_dt);
  }
  tk = GET_TOKEN(tokens, pos);
  Node *id =
      new_node_ident(tk->input,
                     tk->len);
  id->name = name;
  Node *arg_node = new_2term_node(ND_DECLARE, node_dt, id);
  pos++;
  return arg_node;
}

Node *assign_dash() {
  Node *lhs = expression(ASSIGN_PRIORITY);
  int token_type = GET_TOKEN(tokens, pos)->ty;
  if (token_type == '=' || token_type == TK_PE || token_type == TK_ME) {
    pos++;
    return new_2term_node(get_node_type_from_token(token_type), lhs, assign_dash());
  }
  return lhs;
}

Node *assign() {
  Node *lhs = assign_dash();
  if (GET_TOKEN(tokens, pos)->ty == '(') {
    while (GET_TOKEN(tokens, pos)->ty != ')') {
      ++pos;
    }
    return lhs;
  }
  while (GET_TOKEN(tokens, pos)->ty == ';') {
    pos++;
  }
  return lhs;
}

void add_code(Vector *code, int i) {
  if (code->len > i)
    return;
  vec_push(code, malloc(sizeof(Node)));
}

void code_block(Vector *code);

Node *if_node() {
  pos++;
  if (GET_TOKEN(tokens, pos++)->ty != '(') {
    error("Left paraenthesis '(' missing (if): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1)->input, __FILE__, __LINE__);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos++)->ty != ')') {
    error("Right paraenthesis ')' missing (if): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1)->input, __FILE__, __LINE__);
  }
  Node *ifnd = new_2term_node(ND_IF, cond, NULL);
  ifnd->block = new_vector();
  code_block(ifnd->block);
  if (GET_TOKEN(tokens, pos)->ty == TK_ELSE) {
    pos++;
    if (GET_TOKEN(tokens, pos)->ty == TK_IF) {
      ifnd->rhs = if_node();
    } else {
      ifnd->rhs = new_node(ND_BLOCK);
      ifnd->rhs->block = new_vector();
      code_block(ifnd->rhs->block);
      vec_push(ifnd->rhs->block, NULL);
    }
  }
  vec_push(ifnd->block, NULL);
  // TODO: Add else
  return ifnd;
}

Node *while_node() {
  pos++;
  Token *tk = GET_TOKEN(tokens, pos++);
  if (tk->ty != '(') {
    error("Left paraenthesis '(' missing (while): \"%s\"\n",
          tk->input, __FILE__, __LINE__);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  tk = GET_TOKEN(tokens, pos++);
  if (tk->ty != ')') {
    error("Right paraenthesis ')' missing (while): \"%s\"\n",
          tk->input, __FILE__, __LINE__);
  }
  Node *while_nd = new_2term_node(ND_WHILE, cond, NULL);
  while_nd->block = new_vector();
  code_block(while_nd->block);
  vec_push(while_nd->block, NULL);
  return while_nd;
}

void for_node(Vector *code) {
  // TODO: allow multiple statement using ','.
  pos++;
  Token *tk = GET_TOKEN(tokens, pos++);
  if (tk->ty != '(') {
    error("Left paraenthesis '(' missing (for): \"%s\"\n",
          tk->input, __FILE__, __LINE__);
  }
  Node *init = expression(ASSIGN_PRIORITY);
  vec_push(code, init);
  tk = GET_TOKEN(tokens, pos++);
  if (tk->ty != ';') {
    error("1st Semicolon ';' missing (for): \"%s\"\n",
          tk->input, __FILE__, __LINE__);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  tk = GET_TOKEN(tokens, pos++);
  if (tk->ty != ';') {
    error("2nd Semicolon ';' missing (for): \"%s\"\n",
          tk->input, __FILE__, __LINE__);
  }
  Node *increment = expression(ASSIGN_PRIORITY);
  tk = GET_TOKEN(tokens, pos++);
  if (tk->ty != ')') {
    error("Right paraenthesis '(' missing (for): \"%s\"\n",
          tk->input, __FILE__, __LINE__);
  }
  Node *for_nd = new_2term_node(ND_WHILE, cond, NULL);
  vec_push(code, for_nd);
  // Add the increment code and {} block.
  for_nd->block = new_vector();
  code_block(for_nd->block);
  vec_push(for_nd->block, increment);
  vec_push(for_nd->block, NULL);
}

Node *return_node() {
  pos++;
  Node *nd = expression(ASSIGN_PRIORITY);
  Node *return_nd = new_2term_node(ND_RETURN, nd, NULL);
  while (GET_TOKEN(tokens, pos)->ty == ';') {
    pos++;
  }
  return return_nd;
}

void declaration_node(Vector *code);

void code_block(Vector *code) {
  Token *tk = GET_TOKEN(tokens, pos++);
  if (tk->ty != '{') {
    error("Left brace '{' missing (code_block): \"%s\"",
          tk->input, __FILE__, __LINE__);
  }
  int ty;
  while ((ty = GET_TOKEN(tokens, pos)->ty) != '}') {
    if (ty == TK_IF) {
      vec_push(code, if_node());
    } else if (ty == TK_WHILE) {
      vec_push(code, while_node());
    } else if (ty == TK_FOR) {
      for_node(code);
    } else if (ty == TK_RETURN) {
      vec_push(code, return_node());
    } else {
      int dtype = get_dtype_from_token(ty);
      if (dtype != DT_INVALID) {
        // TODO: declaration_node inside function can not generate function.
        // Check.
        declaration_node(code);
      } else {
        vec_push(code, assign());
      }
    }
  }
  if (GET_TOKEN(tokens, pos++)->ty != '}') {
    error("Right brace '}' missing (code_block): \"%s\"",
          GET_TOKEN(tokens, pos - 1)->input, __FILE__, __LINE__);
  }
}

Node *identifier_node() {
  Token *tk = GET_TOKEN(tokens, pos);
  if (tk->ty != TK_IDENT) {
    error("Unexpected token: \"%s\"", tk->input, __FILE__, __LINE__);
  }
  Node *id =
      new_node_ident(tk->input,
                     tk->len);
  pos++;
  // global or local variable.
  if (GET_TOKEN(tokens, pos)->ty != '(') {
    id->val = get_array_size();
    // TODO: add initialization.
    return id;
  }
  // TODO: implement function declaration.
  // function definition..
  Node *arg = NULL;
  pos++;
  if (GET_TOKEN(tokens, pos)->ty != ')') {
    arg = argument();
  }
  if (GET_TOKEN(tokens, pos++)->ty != ')') {
    // TODO: add support of nmultiple arguments.
    error("Right parenthesis ')' missing (function): \"%s\"",
          GET_TOKEN(tokens, pos - 1)->input, __FILE__, __LINE__);
  }
  Node *f = new_2term_node(ND_FUNCDEF, id, arg);
  f->block = new_vector();
  code_block(f->block);
  vec_push(f->block, NULL);
  return f;
}

Node *identifier_sequence() {
  Node *id = identifier_node();
  if (GET_TOKEN(tokens, pos)->ty == ',') {
    pos++;
    Node *ids = identifier_sequence();
    return new_2term_node(ND_IDENTSEQ, id, ids);
  }
  if (GET_TOKEN(tokens, pos)->ty == ';') {
    pos++;
    return id;
  }
  return id;
}

Node *struct_identifier_node() {
  Node *data_type_node = get_data_type_node_at_pos();
  Node *id = identifier_node();
  return new_2term_node(ND_DECLARE, data_type_node, id);
}

Node *struct_identifier_sequence() {
  Node *node = struct_identifier_node();
  if (GET_TOKEN(tokens, pos++)->ty != ';') {
    error("Struct token not ending with ';' (initialization not supported yet) %s",
          GET_TOKEN(tokens, pos - 1)->input, __FILE__, __LINE__);
  }
  Node *next_node = NULL;
  if (GET_TOKEN(tokens, pos)->ty != '}') {
    next_node = struct_identifier_sequence();
  }
  return new_2term_node(ND_IDENTSEQ, node, next_node);
}

Node *get_data_type_from_struct_node(Node *struct_node) {
  Node *node = new_2term_node(ND_DATATYPE, struct_node, NULL);
  while (GET_TOKEN(tokens, pos)->ty == '*') {
    pos++;
    node = new_2term_node(ND_DATATYPE, new_node(ND_PNT), node);
  }
  return node;
}

Node *struct_variable_declaration(Node *struct_node) {
  Node *data_node = get_data_type_from_struct_node(struct_node);
  Node *ids = identifier_sequence();
  Node *node = new_2term_node(ND_DECLARE, data_node, ids);
  return node;
}

Node *struct_declaration() {
  Node *struct_node = new_node(ND_STRUCT);
  ++pos;
  Token *tk = GET_TOKEN(tokens, pos);
  struct_node->name = create_string_in_heap(tk->input,
                                            tk->len);
  pos++;
  tk = GET_TOKEN(tokens, pos);
  if (tk->ty != '{') {
    // if '{' doesn't follow, it's a variable;
    return struct_variable_declaration(struct_node);
  }
  pos++;
  struct_node->lhs = struct_identifier_sequence();
  tk = GET_TOKEN(tokens, pos++);
  if (tk->ty != '}') {
    error("Struct declaration missing '}'. (%s)", tk->input, __FILE__, __LINE__);
  }
  tk = GET_TOKEN(tokens, pos++);
  if (tk->ty != ';') {
    error("Struct declaration missing ';'. (%s)", tk->input, __FILE__, __LINE__);
  }
  return struct_node;
}

void declaration_node(Vector *code) {
  // Struct declaration.
  if (GET_TOKEN(tokens, pos)->ty == TK_STRUCT) {
    vec_push(code, struct_declaration());
    return;
  }
  // Regular declaration of variable and function.
  Node *data_node = get_data_type_node_at_pos();
  Node *node_ids = identifier_sequence();
  Node *declaration = new_2term_node(ND_DECLARE, data_node, node_ids);
  vec_push(code, declaration);
  return;
}

Vector *parse(Vector *tokens_input) {
  tokens = tokens_input;
  pos = 0;
  Vector *code = new_vector();
  while (GET_TOKEN(tokens, pos)->ty != TK_EOF) {
    declaration_node(code);
  }
  vec_push(code, NULL);
  return code;
}
