int print(int a) {
  int b, c;
  b = 1;
  while(a > b - 1) {
    b = b * 10;
  }
  b = b / 10;
  if (a == 0) {
    putchar(48);
  }
  while(b > 0) {
    c = a / b;
    putchar(c + 48);
    a = a - c * b;
    b = b / 10;
  }
  putchar(32);
}

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

