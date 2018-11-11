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
  ND_EQ,        // Equal operation (==)
  ND_NE,        // Not-equal operation (!=)
};

Node **program();

void tokenize(char *p);

void gen(Node *node);

void error(char *s, char *message);

