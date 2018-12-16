#!/bin/bash

try() {
    expected="$1"
    input="$2"

    echo "$input" > tmp.c
    ./m99cc tmp.c > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" != "$expected" ]; then
        echo "for \"$input\", \"$expected\" is expected, but got \"$actual\""
        exit 1
    fi
}

run() {
    expected="$1"
    input="$2"

    ./m99cc $input > tmp.s
    gcc -o tmp tmp.s
    actual=`./tmp`
    # actual="$?"

    if [ "$actual" != "$expected" ]; then
        echo "for \"$input\", \"$expected\" is expected, but got \"$actual\""
        exit 1
    fi
}

try 2 'int main(){int a; a = 2; a;}'
try 3 'int a; int main(){int b; a = 3; b = a; a;}'
try 3 'int a; int b; int main(){a = 3; b = a; b;}'

try 0 'int main(){0;}'
try 42 'int main(){42;}'
try 21 'int main(){5+20-4;}'
try 41 'int main(){ 12 + 34 - 5 ;}'
try 47 'int main(){5+6*7;}'
try 15 'int main(){5*(9-6);}'
try 4 'int main(){(3+5)/2;}'
try 4 'int main(){int a; a=4;a;}'
try 8 'int main(){int a, b; a=b=4;a+b;}'
try 15 'int main(){int a, b; a=3;b=(2+3);a*b;}'
try 31 'int main(){int a, b, z; a=1;z=b=a+3;2*z*b-a;}'
try 1 'int main(){1==1;}'
try 0 'int main(){0==1;}'
try 1 'int main(){0!=1;}'
try 0 'int main(){1!=1;}'
try 1 'int main(){0<1;}'
try 0 'int main(){1<1;}'
try 0 'int main(){0>1;}'
try 1 'int main(){1>0;}'
try 1 'int main(){int a; a=1; a<11;}'
try 0 'int main(){int a; a=2; 2>a;}'

try 7 'int main(){+7;}'
try 250 'int main(){-6;}'
try 1 'int main(){int a; a = -4; -a - 3;}'

try 5 'int main(){int a; a=(1==1)+(1!=1)*2+(0!=2)*4+(4!=4);a;}'
try 20 'int main(){int a, b, j, z; a=b=j=z=4;b+j*z;}'
try 4 'int main(){int ab0; ab0=4;ab0;}'
try 6 'int main(){int a4; a4=2; int ab; ab=3;a4*ab;}'

try 2 'int f(){2;} int main(){f();}'
try 2 'int f(){2;} int main(){int b; f(b=1);}'
try 2 'int f(){2;} int main(){f(f());}'
try 5 'int f(int a){5;} int main(){f(3);}'
try 5 'int f(int a){a + 2;} int main(){f(3);}'
try 4 'int f(int a){2 * a;} int main(){f(2);}'
try 4 'int f(int a){2 * a;} int main(){int b; f(f(b=1));}'
try 4 'int f(int a){2 * a;} int main(){int c; c=f(f(1));c;}'
try 4 'int f(int a){4;} int main(){int c, b; c=f(b=1);c;}'
try 4 'int f(int a){4;} int main(){int c, b; c=f(f(b=1));c;}'
try 4 'int f(int a){2 * a;} int main(){int b, c; c=f(f(b=1));c;}'

try 2 'int f(int a){int b; b=1;if(a>2){b=f(a-1)+f(a-2);} b;} int main(){f(3);}'
try 13 'int f(int a){int b; b=1;if(a>2){b=f(a-1)+f(a-2);} b;} int main(){f(7);}'

try 2 'int main(){int a, b; a=b=1; if (a == 1) {b=2;} b;}'
try 4 'int main(){int a, b; a=b=1; if (a==0) {b = 3;} else {b = 4;} b;}'
# two if-else lines.
try 20 'int main(){int a, b; a=b=1; if (a==1) {b = 2;} else {b = 3;} if (b != 2) {a = 10;} else {a = 20;} a;}'
# if-else-if-else.
try 4 'int main(){int a, b; a=b=1; if (a==2) {b = 3;} else if (a==1) {b = 4;} else {b = 5;} b;}'

try 11 'int main(){int a; a=1; while (a<11) {a=a+1;} a;}'
try 55 'int main(){int a, b; b=0; a=1; while (a<11) {b = b + a; a = a + 1;} b;}'
try 11 'int main(){int a, b; for(a=1; a<11; a=a+1) {b=0;} a;}'
try 55 'int main(){int a, b; b=0; for(a=1; a<11; a = a+1) {b = b + a;} b;}'

# putchar test
try 2 'int main(){putchar(97); putchar(13); putchar(10); 2;}'
run 'hello, world!' program/hello.c

# global function.
try 10 'int a; int main(){a=10;}'
try 10 'int a; int f(){a;} int main(){a=10;f();}'

# pointer
try 4 'int main(){int a, b; a=2;b=&a;*b=4;a;}'
try 4 'int a; int main(){int b; a=2;b=&a;*b=4;a;}'

# array
try 2 'int main(){int a[4];a[0]=2;a[0];}'
try 4 'int main(){int a[3];a[0]=2;a[1]=3;a[2]=4;a[a[0]];}'
try 3 'int main(){int a[2];a[0]=1;a[1]=2;a[0]+a[1];}'
try 3 'int a[2]; int main(){a[0]=1;a[1]=2;a[0]+a[1];}'

# char type
try 2 'int main(){char a; a = 2; a;}'
# char overflow
try 1 'int main(){char a; a = 257; a;}'
# local char array
try 3 'int main(){char a[1]; a[0] = 3; a[0];}'
try 7 'int main(){char a[2]; a[0] = 5; a[1] = 2; a[0] + a[1];}'
# global char
try 2 'int a; int main(){a = 2; a;}'
# global char array
try 3 'int a[1]; int main(){a[0] = 3; a[0];}'
try 7 'char a[2]; int main(){a[0] = 5; a[1] = 2; a[0] + a[1];}'
# replace char value with int
try 7 'int main(){int a; char b; a = 7; b = a; b;}'
# replace int value with char
try 3 'int main(){int a; char b; b = 3; a = b; a;}'
# replace char value with int and overflow
try 7 'int main(){int a; char b; a = 7 + 256; b = a; b;}'


echo OK
