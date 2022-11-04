#include <stdbool.h>

int main(int argc, char *argv)
{
    bool a = (1 + 2) || (3 - 6);

    bool b = 1 + 2 || (3 - 6);

    bool c = (1 + 2) || 3 - 6;

    bool d = (1 + 2) && (3 - 6) || (7 + 9);

    bool a = (1 + 2) && (3 - 6);

    bool b = 1 + 2 && (3 - 6);

    bool c = (1 + 2) && 3 - 6;
}