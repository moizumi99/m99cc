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

### ```error()``` should be like this.

```c
void error(char *s, int i) {
  fprintf(stderr, s, tokens[i].input);
  exit(1);
}
```

###```mul()``` needs ```tokens[pos].ty```

### Inside ```new_node()``` and ```new_node_num()```, ```node->op``` should be ```node->ty```.

### At the end of ```expr()``` and ```term()```, they should return ```lhs```;

# step 4

### Tokenizer also needs to include ```=```

### Add extraction of identifer in ```term()```

### Add ```new_node_ident()``` to create a node for an identifier.

### Add ```assign()``` and ```program()``` function calls.

### Add ";" at the end of each test case.

### Add more tests with identifiers and value assignment.

## Step 7

### Map struct is probably wrong

```c
typedef struct {
  Vector *keys;
  Vector *vals;
} Map;
```

### vec_push is probably wrong. Need ```sizeof(void *) * vec->capacity``` instead of just ```vec->capacity```.

```c
void vec_push(Vector *vec, void *elem) {
  if (vec->capacity == vec->len) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
  }
  vec->data[vec->len++] = elem;
}
```

## After Step 7

I may follow the order here.
https://note.mu/ruiu/n/n00ebc977fd60

1. Add/Subtract
2. Multiply/Divid
3. Local variable
4. Function call without arguments
5. Function call with arguments
6. Function declaration without arguments
7. Function declaration with arguments
8. Global variable
9. Array
10. Pointer
11. char type
12. String literal
13. Struct
14. #include

Nov 21.
So far, p to the middle of 3 is done.
Let's implement variable using map and vector.

Nov 23.
4. Function call without arguments is added.


## Structure Tree

program: assign program'
program': ε | assign program'

assign: expr assign' ";"
assign': ε | "=" expr assign'

compare: expr
compare: expr "==" expr
compare: expr "!=" expr
expr: mul
expr: mul "+" expr
expr: mul "-" expr
mul:  term
mul:  term "*" mul
mul:  term "/" mul
term: number | ident
term: ident "(" arguments ")"
term: "(" expr ")"

arguments: expr arguments'
arguments': ε | ", " expr arguments'
