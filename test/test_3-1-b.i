# 1 "test/test_3-1-b.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "test/test_3-1-b.c"
int main(int argc, char **argv)
{
    int x = 5;
    x += 1;
    x -= 1;
    x *= 2;
    x /= 2;
    x %= 1;
    x &= 0xFF;
    x |= 0x00;
    x ^= ~x;

    x +=1;
    x -=1;
    x *=2;
    x /=2;
    x %=1;
    x &=0xFF;
    x |=0x00;
    x ^=~x;

    x+= 1;
    x-= 1;
    x*= 2;
    x/= 2;
    x%= 1;
    x&= 0xFF;
    x|= 0x00;
    x^= ~x;

    return 0;
}
