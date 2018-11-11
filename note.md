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

