#include <stdio.h>

#include "nn_linenoise.h"

int main(int argc, char **argv)
{
    NN_LinenoiseInit(argc, argv);

    while (!NN_LinenoiseRun())
    {
        /* do nothing */
    }

    return 0;
}
