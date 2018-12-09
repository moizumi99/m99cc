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
  TK_NUM = 256, // Integer token
  TK_IDENT,     // Identifier
  TK_EQ,        // Equal (==) sign
  TK_NE,        // Not-equal (!=) sign
  TK_LE,        // Less than or equal (<=) sign
  TK_GE,        // Greater than or equal (>=) sign
  TK_IF,        // if clause
  TK_ELSE,      // else keyword
  TK_WHILE,     // while keyword
  TK_FOR,       // for keyword
  TK_VOID,      // for void
  TK_INT,       // for integer
  TK_CHAR,      // for char
  TK_EOF,       // End of input
};

enum {
  ID_VAR,   // variable.
  ID_FUNC,  // function.
  ID_ARG,   // argument.
};  // Symbol table ID

typedef struct {
  int type;
  void *address;
  int num;
} Symbol;

// Macro for getting the next token.
#define GET_TOKEN(i) (*((Token *)tokens->data[i]))

typedef struct Node {
  int ty;            // ND_NUM or ND_IDENT or operation
  struct Node *lhs;  // left hand size
  struct Node *rhs;  // right hand side
  int val;           // Value when ty == ND_NUM, array size when ty == ND_IDENT
  char *name;        // Used only when ty == ND_IDENT
  Vector *block;      // Used only when ty == ND_BLOCK or ND_FUNCDEF
} Node;

// Node type.s
enum {
  ND_NUM = 256, // Integer node.
  ND_IDENT,     // Identifier node.
  ND_FUNCCALL,  // Function call node.
  ND_FUNCDEF,   // Function definition.
  ND_BLOCK,     // Block code.
  ND_ROOT,      // The root of the entire program
  ND_PLUS,      // Single term operator (+).
  ND_MINUS,     // Single term operator (-).
  ND_EQ,        // Equal operation (==).
  ND_NE,        // Not-equal operation (!=).
  ND_IF,        // IF node.
  ND_WHILE,     // While node.
  // TODO implement For loop without using while.
  ND_FOR,       // For node
  ND_VOID,      // for void
  ND_INT,       // for integer
  ND_CHAR,      // for char
};

void program(Vector *code);

void tokenize(char *p);

void gen(Node *node);

void error(char *s, char *message);

Vector *new_vector();

void vec_push(Vector *vec, void *elem);

void *vec_pop(Vector *vec);

Map *new_map();

void map_put(Map *map, char *key, void *val);

void *map_get(Map *map, char *key);

void add_global_symbol(char *name, int type, int num);

void *get_global_symbol_address(char *name);

int get_global_symbol_size(char *name);

void add_local_symbol(char *name, int type, int num);

void *get_local_symbol_address(char *name);

int get_local_symbol_size(char *name);

// Unit tests.
void runtest();

void runtest_tokenize();
void dump_token();

void runtest_parse();

#endif // M99CC-H
