# step 1

### Need to give permission before running test.

```sh
chmod a+x test.sh
```

### Before pushing for the first time, set upstream

Instead of just ```git push```, do the following.

```sh
git remote add origin https://github.com/moizumi99/9cc.git
git push -u origin master
```

# step 2

# step 3

```Token`` should be 

```c
// Token values
enum {
  TK_NUM = 256, // integer token
  TK_EOF
};
```

```Node``` probably needs ```int op```

```c
typedef struct Node {
  int ty;           // 演算子かND_NUM
  struct Node *lhs; // 左辺
  struct Node *rhs; // 右辺
  int val;          // tyがND_NUMの場合のみ使う
  int op;
} Node;
```

```error()``` should be like this.

```c
void error(char *s, int i) {
  fprintf(stderr, s, tokens[i].input);
  exit(1);
}
```

```mul()``` needs ```tokens[pos].ty```

Inside ```new_node()``` and ```new_node_num()```, ```node->op``` should be ```node->ty```.

### At the end of ```expr()``` and ```term()```, they should return ```lhs```;

