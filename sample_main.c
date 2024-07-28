#include <stdio.h>

#include "nn_linenoise.h"

static char *s_sample_status = "OFF";

int sample_show_cmd(int argc, char **argv)
{
    int res = 0;
    if (argc != 1)
    {
        printf("[NN] %s:%d Error input!\n", __FILE__, __LINE__);
        res = 1;
        goto done;
    }
    printf("Sample status: %s\n", s_sample_status);

done:
    return res;
}

int main(int argc, char **argv)
{
    NN_LinenoiseRegisterCommand_t sample_cmd_config = {
        .m_func = sample_show_cmd,
        .m_command_name = "sample-status",
        .m_help_msg = "Show current sample status: <ON/OFF>",
    };

    if (NN_LinenoiseRegisterCommand(&sample_cmd_config))
    {
        return -1;
    }

    NN_LinenoiseInit(argc, argv);

    while (!NN_LinenoiseRun())
    {
        /* do nothing */
    }

    return 0;
}
