#include <stdbool.h>

int main(int argc, char **argv)
{
    int x = 5;
    bool y = false;

    x = x + 1;
    x = x - 1;
    x = x * 1;
    x = x / 1;
    x = x % 1;
    x = x << 1;
    x = x >> 1;
    x = x & 1;
    x = x ^ 1;
    y = x < 7;
    y = x <= 7;
    y = x > 7;
    y = x >= 7;
    y = x == 5;
    y = x != 7;
    y = y && false;
    y = y || false;

    x = x+ 1;
    x = x- 1;
    x = x* 1;
    x = x/ 1;
    x = x% 1;
    x = x<< 1;
    x = x>> 1;
    x = x& 1;
    x = x^ 1;
    y = x< 7;
    y = x<= 7;
    y = x> 7;
    y = x>= 7;
    y = x== 5;
    y = x!= 7;
    y = y&& false;
    y = y|| false;

    x = x +1;
    x = x -1;
    x = x *1;
    x = x /1;
    x = x %1;
    x = x <<1;
    x = x >>1;
    x = x &1;
    x = x ^1;
    y = x <7;
    y = x <=7;
    y = x >7;
    y = x >=7;
    y = x ==5;
    y = x !=7;
    y = y &&false;
    y = y ||false;
}