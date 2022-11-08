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
