void print(int a) {
  int b, c;
  b = 1;
  while(a >= 10 * b) {
    b = b * 10;
  }
  while(b > 0) {
    putchar((c = a/ b) + 48);
    a = a - c * b;
    b = b / 10;
  }
}

void print_string(char *a) {
  while(*a) {
    putchar(*a);
    a = a + 1;
  }
}

int fib(int a) {
  if (a <= 1) {
    return a;
  }
  return fib(a-1) + fib(a-2);
}

int main() {
  int i;
  print_string("Fibonacci seriese: ");
  for(i = 0; i < 20; i = i + 1) {
    print(fib(i));
    putchar(32);
  }
  putchar(10);
  return 0;
}

