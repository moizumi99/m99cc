#ifndef M99CC_H
#define M99CC_H

#define IDENT_LEN 256


// For Vector definition. //
typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

typedef struct {
  Vector *keys;
  Vector *vals;
} Map;

// Token types
typedef struct {
  int ty;      // token type
  int val;     // value if ty is TK_NUM
  int len; // length of name if ty is IK_IDENT
  char *input;
} Token;

// Token values
enum {
  TK_NUM = 256, // 256 Integer token
  TK_IDENT,     // 257 Identifier
  TK_EQ,        // 258 Equal (==) sign
  TK_NE,        // 259 Not-equal (!=) sign
  TK_LE,        // 260 Less than or equal (<=) sign
  TK_GE,        // 261 Greater than or equal (>=) sign
  TK_IF,        // 262 if clause
  TK_ELSE,      // 263 else keyword
  TK_WHILE,     // 264 while keyword
  TK_FOR,       // 265 for keyword
  TK_RETURN,    // 266 return keyword
  TK_VOID,      // 267 for void
  TK_INT,       // 268 for integer
  TK_CHAR,      // 269 for char
  TK_EOF,       // 270 End of input
};

enum {
  ID_VAR,   // variable.
  ID_FUNC,  // function.
  ID_ARG,   // argument.
};  // Symbol table ID

typedef struct {
  int type;   // Symbol type (ID_VAR< ID_FUNC, ID_ARG)
  void *address; // address from the base.
  int num;    // 0: scalar, >=1: array
  int dtype;  // Data type (void, int, char)
} Symbol;

typedef struct Node {
  int ty;            // ND_NUM or ND_IDENT or operation
  struct Node *lhs;  // left hand size
  struct Node *rhs;  // right hand side
  int val;           // Value when ty == ND_NUM, array size when ty == ND_IDENT
  char *name;        // Used only when ty == ND_IDENT
  Vector *block;      // Used only when ty == ND_BLOCK or ND_FUNCDEF
} Node;

// Node type.
enum {
  ND_NUM = 256, // 256 Integer node.
  ND_IDENT,     // 257 Identifier node.
  ND_IDENTSEQ,  // 258 Identifier sequence.
  ND_FUNCCALL,  // 259 Function call node.
  ND_FUNCDEF,   // 260 Function definition.
  ND_BLOCK,     // 261 Block code.
  ND_ROOT,      // 262 The root of the entire program
  ND_PLUS,      // 263 Single term operator (+).
  ND_MINUS,     // 264 Single term operator (-).
  ND_EQ,        // 265 Equal operation (==).
  ND_NE,        // 266 Not-equal operation (!=).
  ND_LE,        // 267 Less than or equal (<= / =<)
  ND_GE,        // 268 Greater than or equal (>= / =>)
  ND_IF,        // 269 IF node.
  ND_WHILE,     // 270 While node.
  ND_FOR,       // 271 While node.
  ND_RETURN,    // 272 For node
  ND_DECLARE,   // 273 Declaration of variable/function
  ND_DEFINITION,// 274 Defintion of variable/function
  ND_DATATYPE,  // 275 Data type
};

// Data type.
enum {
  DT_VOID,      // 0 for void
  DT_INT,       // 1 for integer
  DT_CHAR,      // 2 for char
  DT_INVALID,   // 3 for invalid
};

Vector *tokenize(char *p);

Vector *parse(Vector *tokens_input);

void gen_program();

void gen_node(Node *node);

void error(char *s, char *message);

Vector *new_vector();

void vec_push(Vector *vec, void *elem);

void *vec_pop(Vector *vec);

Map *new_map();

void map_put(Map *map, char *key, void *val);

void *map_get(Map *map, char *key);

void *get_symbol_address(Map *symbols, char *name);

int get_symbol_size(Map *symbols, char *name);

int get_symbol_type(Map *symbols, char *name);

int data_size(int dtype);

// Unit tests.
void runtest();

void runtest_tokenize();
void dump_token();

void runtest_parse();

#endif // M99CC-H
