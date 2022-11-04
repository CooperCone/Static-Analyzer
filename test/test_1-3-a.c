#include <stdbool.h>

void tmp()
{

}

int main(int argc, char *argv)
{
    if (7) argc = 0;

    if (0)
    {
    }

    if (7)
    {
        // this is some stuff
    }
    else
        tmp();

    // This is valid
    while (true)
    {

    }

    do tmp();
    while (true);

    do
    {
        // nothing
    }
    while (true);

    for (int i = 0; i < 7; i++)
        i++;

    for (int i = 0; i < 8; i++)
    {
        i--;
    }

    return 0;
}
