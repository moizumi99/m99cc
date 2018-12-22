# About

This is a compiler for a c-like-language.

The early version of the code was based on 9cc which is explained in web based tutorial
[低レイヤを知りたい人のためのCコンパイラ作成入門](https://www.sigbus.info/compilerbook\) by Rui Ueyama.

Now the large part of the code has been diverted from the original 9cc.

# What does it do?

Not much yet.

Basic codes like below can be compiled to assembly code. (print() is defined separately. Full code is in program/fibonacci.c)

```c
int main() {
  int i;
  int d, e, f;
  d = 0;
  e = 1;
  f = 1;
  for(i = 0; i < 11; i = i + 1) {
    print(d);
    f = d + e;
    d = e;
    e = f;
  }
  putchar(10);
  0;
}
```

This code write fibonacci seriese to the console, like below.

> 0 1 1 2 3 5 8 13 21 34 55 

