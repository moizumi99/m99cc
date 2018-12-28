#include "m99cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define GET_TOKEN(T, I) (*((Token *)(T)->data[(I)]))

static Vector *tokens;
static int pos;

enum {
  SC_GLOBAL, // Global scope.
  SC_LOCAL   // Local scope.
};

// Error reporting function.
void error(char *s, char *message) {
  fprintf(stderr, s, message);
  fprintf(stderr, "\n");
  exit(1);
}

char *create_string_in_heap(char *str, int len) {
  char *str_mem = malloc(sizeof(char) * IDENT_LEN);
  int copy_len = (len < IDENT_LEN - 1) ? len : IDENT_LEN - 1;
  strncpy(str_mem, str, copy_len);
  str_mem[len] = '\0';
  return str_mem;
}


Node *new_node_str(char *name, int len) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_STR;
  node->lhs = NULL;
  node->rhs = NULL;
  node->val = 0;
  node->name = create_string_in_heap(name, len);
  node->block = NULL;
  return node;
}

Node *new_node(int op, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = op;
  node->lhs = lhs;
  node->rhs = rhs;
  node->val = 0;
  node->name = NULL;
  node->block = NULL;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  node->lhs = NULL;
  node->rhs = NULL;
  node->name = NULL;
  node->block = NULL;
  return node;
}

int get_array_size() {
  if (GET_TOKEN(tokens, pos).ty != '[') {
    // This is not an array.
    return 0;
  }
  // it's an array declaration.
  ++pos;
  int num = 0;
  if (GET_TOKEN(tokens, pos).ty == TK_NUM) {
    num = GET_TOKEN(tokens, pos++).val;
  }
  if (GET_TOKEN(tokens, pos++).ty != ']') {
    fprintf(stderr, "Closing bracket ']' is missing (get_array_size): \"%s\"\n",
            GET_TOKEN(tokens, pos - 1).input);
    exit(1);
  }
  return num;
}

int data_size_from_dtype(int dtype) {
  switch (dtype) {
  case DT_VOID:
    // shouldn't this be zero?
    return 8;
  case DT_INT:
    return 8;
  case DT_CHAR:
    return 1;
  case DT_PNT:
    return 8;
  default:
    return 8;
  }
}

int data_size(DataType *data_type) {
  return data_size_from_dtype(data_type->dtype);
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

int get_data_type_from_token(int token_type) {
  switch (token_type) {
  case TK_VOID:
    return DT_VOID;
  case TK_CHAR:
    return DT_CHAR;
  case TK_INT:
    return DT_INT;
  default:
    return DT_INVALID;
  }
}

// TODO: This is redundant. Just create data_type_node from tokens directly.
int get_data_type(int p, DataType **data_type) {
  int dtype = get_data_type_from_token(GET_TOKEN(tokens, p++).ty);
  DataType *dt = new_data_type(dtype);
  if (dtype != DT_INVALID) {
    while (GET_TOKEN(tokens, p).ty == '*') {
      p++;
      dt = new_data_pointer(dt);
    }
  }
  *data_type = dt;
  return p;
}

Node *get_data_type_node(int p, int *new_p) {
  int dtype = get_data_type_from_token(GET_TOKEN(tokens, p++).ty);
  Node *node = new_node(ND_DATATYPE, NULL, NULL);
  if (dtype == DT_VOID) {
    node->lhs = new_node(ND_VOID, NULL, NULL);
  } else if (dtype == DT_INT) {
    node->lhs = new_node(ND_INT, NULL, NULL);
  } else if (dtype == DT_CHAR) {
    node->lhs = new_node(ND_CHAR, NULL, NULL);
  } else {
    error("Invalud data type token %s", GET_TOKEN(tokens, p - 1).input);
    exit(1);
  }
  while (GET_TOKEN(tokens, p).ty == '*') {
    p++;
    node = new_node(ND_DATATYPE, new_node(ND_PNT, NULL, NULL), node);
  }
  *new_p = p;
  return node;
}

Node *term();
Node *argument();

Node *expression(int priority) {
  if (priority == HIGH_PRIORITY) {
    return term();
  }
  Node *lhs = expression(priority + 1);
  if (GET_TOKEN(tokens, pos).ty == TK_EOF) {
    return lhs;
  }
  int token_type = GET_TOKEN(tokens, pos).ty;
  if (operation_priority(token_type) == priority) {
    pos++;
    int node_type = get_node_type_from_token(token_type);
    Node *rhs = expression(priority);
    return new_node(node_type, lhs, rhs);
  }
  return lhs;
}

// term : number | identifier | ( expression )
Node *term() {
  // Simple number
  if (GET_TOKEN(tokens, pos).ty == TK_NUM) {
    return new_node_num(GET_TOKEN(tokens, pos++).val);
  }

  if (GET_TOKEN(tokens, pos).ty == TK_STR) {
    char *str = GET_TOKEN(tokens, pos).input;
    int len = GET_TOKEN(tokens, pos).len;
    pos++;
    return new_node_str(str, len);
  }

  // Variable or Function
  if (GET_TOKEN(tokens, pos).ty == TK_IDENT) {
    Node *id = new_node_ident(GET_TOKEN(tokens, pos).input,
                              GET_TOKEN(tokens, pos).len);
    pos++;
    // if followed by (, it's a function call.
    if (GET_TOKEN(tokens, pos).ty == '(') {
      ++pos;
      Node *arg = NULL;
      // Argument exists.
      if (GET_TOKEN(tokens, pos).ty != ')') {
        arg = expression(ASSIGN_PRIORITY);
      }
      if (GET_TOKEN(tokens, pos).ty != ')') {
        error("No right parenthesis corresponding to left parenthesis"
              " (term, function): \"%s\"",
              GET_TOKEN(tokens, pos).input);
      }
      pos++;
      Node *node = new_node(ND_FUNCCALL, id, arg);
      return node;
    }
    // If not followed by (, it's a variable.
    // is it array?
    Node *node = id;
    if (GET_TOKEN(tokens, pos).ty == '[') {
      pos++;
      Node *index = expression(ASSIGN_PRIORITY);
      if (GET_TOKEN(tokens, pos++).ty != ']') {
        fprintf(stderr, "Closing bracket ']' missing. \"%s\"\n",
                GET_TOKEN(tokens, pos - 1).input);
        exit(1);
      }
      node = new_node(ND_DEREF, new_node('+', id, index), NULL);
    }
    return node;
  }
  // "( expression )"
  if (GET_TOKEN(tokens, pos).ty == '(') {
    pos++;
    Node *node = expression(ASSIGN_PRIORITY);
    if (GET_TOKEN(tokens, pos).ty != ')') {
      error("No right parenthesis corresponding to left parenthesis "
            "(term, parenthesis): \"%s\"",
            GET_TOKEN(tokens, pos).input);
    }
    pos++;
    return node;
  }
  // Single term operators
  if (GET_TOKEN(tokens, pos).ty == '+' || GET_TOKEN(tokens, pos).ty == '-' ||
      GET_TOKEN(tokens, pos).ty == '*' || GET_TOKEN(tokens, pos).ty == '&') {
    int type = GET_TOKEN(tokens, pos).ty;
    pos++;
    Node *lhs = term();
    return new_node(get_node_type_from_token_single(type), lhs, NULL);
  }

  if (GET_TOKEN(tokens, pos).ty == TK_INC || GET_TOKEN(tokens, pos).ty == TK_DEC) {
    // convert ++x to (x += 1).
    int type = GET_TOKEN(tokens, pos).ty;
    int operation = (type == TK_INC) ? ND_PE : ND_ME;
    pos++;
    Node *lhs = term();
    int step = 1;
    Node *rhs = new_node_num(step);
    return new_node(operation, lhs, rhs);
  }

  // Code should not reach here.
  error("Unexpected token (parse.c term): \"%s\"",
        GET_TOKEN(tokens, pos).input);
  return NULL;
}

Node *data_type_node(DataType *data_type) {
  Node *node = new_node(ND_DATATYPE, NULL, NULL);
  if (data_type->dtype == DT_VOID) {
    node->lhs = new_node(ND_VOID, NULL, NULL);
  } else if (data_type->dtype == DT_INT) {
    node->lhs = new_node(ND_INT, NULL, NULL);
  } else if (data_type->dtype == DT_CHAR) {
    node->lhs = new_node(ND_CHAR, NULL, NULL);
  } else if (data_type->dtype == DT_PNT) {
    node->lhs = new_node(ND_PNT, NULL, NULL);
    node->rhs = data_type_node(data_type->pointer_type);
  } else {
    error("%s", "Invalid data type");
  }
  return node;
}

Node *argument() {
  // TODO: make argument a list.
  Node *node_dt = get_data_type_node(pos, &pos);
  if (GET_TOKEN(tokens, pos).ty != TK_IDENT) {
    error("Invalid (not IDENT) token in argument declaration position \"%s\"",
          GET_TOKEN(tokens, pos).input);
  }
  // argument name?
  char *name = create_string_in_heap(GET_TOKEN(tokens, pos).input,
                                     GET_TOKEN(tokens, pos).len);
  int array_size = get_array_size();
  if (array_size > 0) {
    // Make the node_dt pointer node
    node_dt = new_node(ND_DATATYPE, new_node(ND_PNT, NULL, NULL), node_dt);
  }
  Node *id =
      new_node_ident(GET_TOKEN(tokens, pos).input,
                     GET_TOKEN(tokens, pos).len);
  id->name = name;
  Node *arg_node = new_node(ND_DECLARE, node_dt, id);
  pos++;
  return arg_node;
}

Node *assign_dash() {
  Node *lhs = expression(ASSIGN_PRIORITY);
  int token_type = GET_TOKEN(tokens, pos).ty;
  if (token_type == '=' || token_type == TK_PE || token_type == TK_ME) {
    pos++;
    return new_node(get_node_type_from_token(token_type), lhs, assign_dash());
  }
  return lhs;
}

Node *assign() {
  Node *lhs = assign_dash();
  if (GET_TOKEN(tokens, pos).ty == '(') {
    while (GET_TOKEN(tokens, pos).ty != ')') {
      ++pos;
    }
    return lhs;
  }
  while (GET_TOKEN(tokens, pos).ty == ';') {
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
  if (GET_TOKEN(tokens, pos++).ty != '(') {
    error("Left paraenthesis '(' missing (if): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos++).ty != ')') {
    error("Right paraenthesis ')' missing (if): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *ifnd = new_node(ND_IF, cond, NULL);
  ifnd->block = new_vector();
  code_block(ifnd->block);
  if (GET_TOKEN(tokens, pos).ty == TK_ELSE) {
    pos++;
    if (GET_TOKEN(tokens, pos).ty == TK_IF) {
      ifnd->rhs = if_node();
    } else {
      ifnd->rhs = new_node(ND_BLOCK, NULL, NULL);
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
  if (GET_TOKEN(tokens, pos++).ty != '(') {
    error("Left paraenthesis '(' missing (while): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos++).ty != ')') {
    error("Right paraenthesis ')' missing (while): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *while_nd = new_node(ND_WHILE, cond, NULL);
  while_nd->block = new_vector();
  code_block(while_nd->block);
  vec_push(while_nd->block, NULL);
  return while_nd;
}

void for_node(Vector *code) {
  // TODO: allow multiple statement using ','.
  pos++;
  if (GET_TOKEN(tokens, pos++).ty != '(') {
    error("Left paraenthesis '(' missing (for): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *init = expression(ASSIGN_PRIORITY);
  vec_push(code, init);
  if (GET_TOKEN(tokens, pos++).ty != ';') {
    error("1st Semicolon ';' missing (for): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *cond = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos++).ty != ';') {
    error("2nd Semicolon ';' missing (for): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *increment = expression(ASSIGN_PRIORITY);
  if (GET_TOKEN(tokens, pos++).ty != ')') {
    error("Right paraenthesis '(' missing (for): \"%s\"\n",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *for_nd = new_node(ND_WHILE, cond, NULL);
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
  Node *return_nd = new_node(ND_RETURN, nd, NULL);
  while (GET_TOKEN(tokens, pos).ty == ';') {
    pos++;
  }
  return return_nd;
}

void declaration_node(Vector *code, int scope);

void code_block(Vector *code) {
  if (GET_TOKEN(tokens, pos++).ty != '{') {
    error("Left brace '{' missing (code_block): \"%s\"",
          GET_TOKEN(tokens, pos - 1).input);
  }
  while (GET_TOKEN(tokens, pos).ty != '}') {
    if (GET_TOKEN(tokens, pos).ty == TK_IF) {
      vec_push(code, if_node());
    } else if (GET_TOKEN(tokens, pos).ty == TK_WHILE) {
      vec_push(code, while_node());
    } else if (GET_TOKEN(tokens, pos).ty == TK_FOR) {
      for_node(code);
    } else if (GET_TOKEN(tokens, pos).ty == TK_RETURN) {
      vec_push(code, return_node());
    } else {
      DataType *data_type;
      get_data_type(pos, &data_type);
      if (data_type->dtype != DT_INVALID) {
        // TODO: declaration_node inside function can not generate function.
        // Check.
        declaration_node(code, SC_LOCAL);
      } else {
        vec_push(code, assign());
      }
    }
  }
  if (GET_TOKEN(tokens, pos++).ty != '}') {
    error("Right brace '}' missing (code_block): \"%s\"",
          GET_TOKEN(tokens, pos - 1).input);
  }
}

Node *identifier_node(Node *data_type_node, int scope) {
  if (GET_TOKEN(tokens, pos).ty != TK_IDENT) {
    error("Unexpected token (function): \"%s\"", GET_TOKEN(tokens, pos).input);
  }
  Node *id =
      new_node_ident(GET_TOKEN(tokens, pos).input,
                     GET_TOKEN(tokens, pos).len);
  pos++;
  // global or local variable.
  if (GET_TOKEN(tokens, pos).ty != '(') {
    int num = get_array_size();
    id->val = num;
    if (num > 0) {
      data_type_node = new_node(ND_DATATYPE, new_node(ND_PNT, NULL, NULL), data_type_node);
    }
    // TODO: add initialization.
    return id;
  }
  // TODO: implement function declaration.
  // function definition..
  if (scope != SC_GLOBAL) {
    error("Function declaration is allowed only"
          " in global scope. \"%s\"\n",
          id->name);
    return NULL;
  }
  Node *arg = NULL;
  pos++;
  if (GET_TOKEN(tokens, pos).ty != ')') {
    arg = argument();
  }
  if (GET_TOKEN(tokens, pos++).ty != ')') {
    // TODO: add support of nmultiple arguments.
    error("Right parenthesis ')' missing (function): \"%s\"",
          GET_TOKEN(tokens, pos - 1).input);
  }
  Node *f = new_node(ND_FUNCDEF, id, arg);
  f->block = new_vector();
  code_block(f->block);
  vec_push(f->block, NULL);
  return f;
}

Node *identifier_sequence(Node *data_type_node, int scope) {
  Node *id = identifier_node(data_type_node, scope);
  if (GET_TOKEN(tokens, pos).ty == ',') {
    pos++;
    Node *ids = identifier_sequence(data_type_node, scope);
    return new_node(ND_IDENTSEQ, id, ids);
  }
  if (GET_TOKEN(tokens, pos).ty == ';') {
    pos++;
    return id;
  }
  // GET_TOKEN(tokens, pos).input);
  return id;
}

void declaration_node(Vector *code, int scope) {
  Node *node_dt = get_data_type_node(pos, &pos);
  Node *node_ids = identifier_sequence(node_dt, scope);
  Node *declaration = new_node(ND_DECLARE, node_dt, node_ids);
  vec_push(code, declaration);
  return;
}

Vector *parse(Vector *tokens_input) {
  tokens = tokens_input;
  Vector *code = new_vector();
  while (GET_TOKEN(tokens, pos).ty != TK_EOF) {
    declaration_node(code, SC_GLOBAL);
  }
  vec_push(code, NULL);
  return code;
}
