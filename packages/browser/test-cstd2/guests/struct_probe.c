/* Structs passed by value: every size class of the calling
 * convention — one or two register words for up to 8 bytes
 * (including the ragged 5/6/7-byte tails), a pointer to the
 * caller's value above 8 bytes — directly, through function
 * pointers, and fed straight from another call's struct
 * return. */
#include <stdio.h>
#include "guest_check.h"

struct S1 { char a; };                    /* 1 byte */
struct S2 { short a, b; };                /* 4 bytes */
struct S3 { int a; char b; };             /* 8 bytes, padded */
struct S5 { char a[5]; };                 /* 5 bytes */
struct S6 { char a[6]; };                 /* 6 bytes */
struct S7 { char a[7]; };                 /* 7 bytes */
struct S8 { long long a; };               /* 8 bytes */
struct SD { double d; int i; };           /* 16 bytes */
struct SBig { long long a, b, c; };       /* 24 bytes */
struct Outer { struct S3 in; int tail; }; /* nested, 16 bytes */

static int get1(struct S1 s) { return s.a; }
static int sum2(struct S2 s) { return s.a + s.b; }
static int sum3(struct S3 s) { return s.a + s.b; }
static int sum5(struct S5 s) { int t = 0; for (int i = 0; i < 5; i++) t += s.a[i]; return t; }
static int sum6(struct S6 s) { int t = 0; for (int i = 0; i < 6; i++) t += s.a[i]; return t; }
static int sum7(struct S7 s) { int t = 0; for (int i = 0; i < 7; i++) t += s.a[i]; return t; }
static long long get8(struct S8 s) { return s.a; }
static int sdsum(struct SD s) { return (int)s.d + s.i; }
static int outsum(struct Outer o) { return o.in.a + o.in.b + o.tail; }
static struct S3 mk3(int a, char b) { struct S3 s; s.a = a; s.b = b; return s; }
static struct SBig addbig(struct SBig x, struct SBig y) {
  struct SBig r;
  r.a = x.a + y.a; r.b = x.b + y.b; r.c = x.c + y.c;
  return r;
}

int main(void) {
  struct S1 one = { 42 };
  CK("size1", get1(one) == 42);
  struct S2 two = { 20, 22 };
  CK("size4", sum2(two) == 42);
  struct S3 a = { 40, 2 };
  CK("direct8", sum3(a) == 42);
  int (*fp)(struct S3) = sum3;
  CK("indirect8", fp(a) == 42);
  CK("ret_to_arg", sum3(mk3(19, 23)) == 42);
  struct S5 five = { { 1, 2, 3, 4, 5 } };
  CK("size5", sum5(five) == 15);
  struct S6 six = { { 1, 1, 1, 1, 1, 1 } };
  CK("size6", sum6(six) == 6);
  struct S7 seven = { { 1, 1, 1, 1, 1, 1, 1 } };
  CK("size7", sum7(seven) == 7);
  struct S8 e = { -123456789012345LL };
  CK("size8_ll", get8(e) == -123456789012345LL);
  struct SD sd = { 2.5, 40 };
  CK("mixed16", sdsum(sd) == 42);
  struct SBig b1 = { 1, 2, 3 }, b2 = { 10, 20, 30 };
  struct SBig br = addbig(b1, b2);
  CK("big24", br.a == 11 && br.b == 22 && br.c == 33);
  struct Outer o = { { 30, 5 }, 7 };
  CK("nested", outsum(o) == 42);
  DONE();
}
