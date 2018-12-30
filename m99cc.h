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
  TK_INC,       // 258 Increment
  TK_DEC,       // 259 Decrement
  TK_EQ,        // 260 Equal (==) sign
  TK_NE,        // 261 Not-equal (!=) sign
  TK_LE,        // 262 Less than or equal (<=) sign
  TK_GE,        // 263 Greater than or equal (>=) sign
  TK_PE,        // 264 Plus then substitute (+=).
  TK_ME,        // 265 Minus then substitute (-=).
  TK_AND,       // 266 Logical and (&&) sign.
  TK_OR,        // 267 Logical and (||) sign.
  TK_IF,        // 268 if clause
  TK_ELSE,      // 269 else keyword
  TK_WHILE,     // 270 while keyword
  TK_FOR,       // 271 for keyword
  TK_RETURN,    // 272 return keyword
  TK_STRUCT,    // 273 struct
  TK_VOID,      // 274 for void
  TK_INT,       // 275 for integer
  TK_CHAR,      // 276 for char
  TK_STR,       // 277 char string literal
  TK_EOF,       // 278 End of input
};

enum {
  ID_VAR,   // variable.
  ID_FUNC,  // function.
  ID_ARG,   // argument.
};  // Symbol table ID

typedef struct {
  int type;   // Symbol type (ID_VAR, ID_FUNC, ID_ARG)
  void *address; // address from the base.
  int num;    // 0: scalar, >=1: array
  struct DataType *data_type;  // Data type (void, int, char)
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
  ND_EQ,        // 263 Equal operation (==).
  ND_NE,        // 264 Not-equal operation (!=).
  ND_LE,        // 265 Less than or equal (<= / =<)
  ND_GE,        // 266 Greater than or equal (>= / =>)
  ND_PE,        // 267 Add and substitute (+=).
  ND_ME,        // 278 Subtract and substitute (-=).
  ND_AND,       // 279 Logical AND (&&).
  ND_OR,        // 270 Logical OR (&&).
  ND_IF,        // 271 IF node.
  ND_WHILE,     // 272 While node.
  ND_FOR,       // 273 While node.
  ND_RETURN,    // 274 For node
  ND_DECLARE,   // 275 Declaration of variable/function
  ND_DEFINITION,// 276 Defintion of variable/function
  ND_DATATYPE,  // 277 Data type
  ND_STR,       // 278 String literal.
  ND_DEREF,     // 279 De-reference (*) operator.
  ND_VOID,      // 280 Data Type of VOID.
  ND_INT,       // 281 Data Type of VOID.
  ND_CHAR,      // 282 Data Type of VOID.
  ND_PNT,       // 283 Data Type of De-reference.
  ND_STRUCT,    // 284 ND Struct
};

// Data type.
enum {
  DT_VOID,      // 0 for void
  DT_INT,       // 1 for integer
  DT_CHAR,      // 2 for char
  DT_PNT,       // 3 for pointer
  DT_INVALID,   // 4 for invalid
};

typedef struct DataType{
  int dtype;
  struct DataType *pointer_type;
} DataType;

typedef struct StructMember {
  char *name;
  DataType *data_type;
  int address;
} StructMember;

DataType *new_data_type(int dt);

DataType *new_data_pointer(DataType *dt);

// token.c
Vector *tokenize(char *p);

// parse.c
Vector *parse(Vector *tokens_input);
void gen_node(Node *node);

// tree_analysis.c.
Vector *analysis(Vector *program_code);

// codegen.c
void gen_program();

void error(char *s, char *message, char *file, int line);

Vector *new_vector();

// utils.c
void vec_push(Vector *vec, void *elem);
void *vec_pop(Vector *vec);
Map *new_map();
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);

int data_size(DataType *dtype);


// Unit tests.
void runtest();
void runtest_tokenize();
void dump_token();
void runtest_parse();
int runtest_data_type();

#endif // M99CC-H
