// Token types
typedef struct {
  int ty;      // token type
  int val;     // value if ty is TK_NUM
  char *input;
} Token;

typedef struct Node {
  int ty;            // ND_NUM or ND_IDENT
  struct Node *lhs;  // left hand size
  struct Node *rhs;  // right hand side
  int val;           // Used only when ty == ND_NUM
  char name;         // Used only when ty == ND_IDENT 
} Node;

// Token values
enum {
  TK_NUM = 256, // Integer token
  TK_IDENT,     // Identifier
  TK_EOF        // End of input
};

enum {
  ND_NUM = 256, // Integer node
  ND_IDENT,     // Identifier node
};

Node **program();

void tokenize(char *p);

void gen(Node *node);

void error(char *s, char *message);

extern Token tokens[];

