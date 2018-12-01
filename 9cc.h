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
  TK_EOF        // End of input
};

enum {
  ID_VAR,   // variable.
  ID_FUNC,  // function.
  ID_ARG,   // argument.
};  // Symbol table ID

typedef struct {
  int type;
  void *address;
} Symbol;

// Macro for getting the next token.
#define GET_TOKEN(i) (*((Token *)tokens->data[i]))

typedef struct Node {
  int ty;            // ND_NUM or ND_IDENT or operation
  struct Node *lhs;  // left hand size
  struct Node *rhs;  // right hand side
  int val;           // Used only when ty == ND_NUM
  char *name;        // Used only when ty == ND_IDENT
} Node;

// Node type.s
enum {
  ND_NUM = 256, // Integer node.
  ND_IDENT,     // Identifier node.
  ND_FUNCCALL,  // Function call node.
  ND_FUNCDEF,   // Function definition.
  ND_ROOT,      // The root of the entire program
  ND_EQ,        // Equal operation (==).
  ND_NE,        // Not-equal operation (!=).
};

void program(Vector *program_code);

void tokenize(char *p);

void gen(Node *node);

void error(char *s, char *message);

Vector *new_vector();

void vec_push(Vector *vec, void *elem);

Map *new_map();

void map_put(Map *map, char *key, void *val);

void *map_get(Map *map, char *key);

void runtest();

void add_global_symbol(char *name, int type);

void *get_global_symbol_address(char *name);

void add_local_symbol(char *name, int type);

void *get_local_symbol_address(char *name);

#endif // M99CC-H
