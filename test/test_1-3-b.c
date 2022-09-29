#include <stdbool.h>

void doSomething()
{

}

int main(int argc, char *argv) { if (7) doSomething();

    if (0) { // This is a brace that's not on its own

    }

    // This is valid
    while (true)
    {

    }

    // This is not
    while (true)
    {

     }

    return 0;
}
