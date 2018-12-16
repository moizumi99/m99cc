# About

This is a compiler for a c-like-language.

The early version of the code was based on 9cc which is explained in web based tutorial
[低レイヤを知りたい人のためのCコンパイラ作成入門](https://www.sigbus.info/compilerbook\) by Rui Ueyama.

Now the large part of the code has been diverted from the original 9cc.

# What does it do?

Not much yet.

Basic codes like below can be compiled to assembly code.

'main(){a=b=1; if (a==2) {b = 3;} else if (a==1) {b = 4;} else {b = 5;} b;}'

This gives 4 as an answer.

