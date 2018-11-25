#!/bin/bash

try() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" != "$expected" ]; then
        echo "for $input, $expected is expected, but got $actual"
        exit 1
    fi
}

try 0 '0;'
try 42 '42;'
try 21 '5+20-4;'
try 41 ' 12 + 34 - 5 ;'
try 47 '5+6*7;'
try 15 '5*(9-6);'
try 4 '(3+5)/2;'
try 4 'a=4;a;'
try 8 'a=b=4;a+b;'
try 15 'a=3;b=(2+3);a*b;'
try 31 'a=1;z=b=a+3;2*z*b-a;'
try 1 '1==1;'
try 0 '0==1;'
try 1 '0!=1;'
try 0 '1!=1;'
try 5 'a=(1==1)+(1!=1)*2+(0!=2)*4+(4!=4);a;'
try 20 'a=b=j=z=4;b+j*z;'
try 6 'f(3);'
try 4 'ab0=4;ab0;'
try 6 'a4=2;ab=3;a4*ab;'

echo OK
