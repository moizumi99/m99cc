typedef struct Node {
  int ty;            // ND_NUM or ND_IDENT or operation
  struct Node *lhs;  // left hand size
  struct Node *rhs;  // right hand side
  int val;           // Used only when ty == ND_NUM
  char name;         // Used only when ty == ND_IDENT 
} Node;

enum {
  ND_NUM = 256, // Integer node
  ND_IDENT,     // Identifier node
  ND_FUNC,      // Function node
  ND_EQ,        // Equal operation (==)
  ND_NE,        // Not-equal operation (!=)
};

// Token types
typedef struct {
  int ty;      // token type
  int val;     // value if ty is TK_NUM
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

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

typedef struct {
  Vector *keys;
  Vector *vals;
} Map;

Vector *program();

void tokenize(char *p);

void gen(Node *node);

void error(char *s, char *message);

Vector *new_vector();

void vec_push(Vector *vec, void *elem);

Map *new_map();

void map_put(Map *map, char *key, void *val);

void *map_get(Map *map, char *key);

void runtest();

