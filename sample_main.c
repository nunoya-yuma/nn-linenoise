#include <stdio.h>

#include "nn_linenoise.h"

typedef enum
{
    SAMPLE_STATUS_INVALID,
    SAMPLE_STATUS_ON,
    SAMPLE_STATUS_OFF,
} SampleStatus_t;

SampleStatus_t s_sample_status = SAMPLE_STATUS_INVALID;

static const char *GetStringFromSampleStatus(SampleStatus_t a_status)
{
    const char *status_str = "Error";

    switch (a_status)
    {
    case SAMPLE_STATUS_INVALID:
        status_str = "Invalid";
        break;

    case SAMPLE_STATUS_ON:
        status_str = "ON";
        break;

    case SAMPLE_STATUS_OFF:
        status_str = "OFF";
        break;

    default:
        printf("[NN] Error: Unexpected status\n");
        break;
    }

    return status_str;
}

static int Sample_ShowStatusCmd(int argc, char **argv)
{
    int res = 0;
    if (argc != 1)
    {
        printf("[NN] %s:%d Error input!\n", __FILE__, __LINE__);
        res = 1;
        goto done;
    }

    printf("Sample status: %s\n", GetStringFromSampleStatus(s_sample_status));

done:
    return res;
}

int main(int argc, char **argv)
{
    NN_LinenoiseRegisterCommand_t sample_cmd_config = {
        .m_func = Sample_ShowStatusCmd,
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
