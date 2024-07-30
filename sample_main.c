#include <stdio.h>
#include <string.h>

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
        status_str = "on";
        break;

    case SAMPLE_STATUS_OFF:
        status_str = "off";
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

    printf("Sample status: '%s'\n", GetStringFromSampleStatus(s_sample_status));

done:
    return res;
}

static int Sample_CtrlCmd(int argc, char **argv)
{
    int res = 0;
    if (argc != 2)
    {
        printf("[NN] %s:%d Error input!\n", __FILE__, __LINE__);
        res = 1;
        goto done;
    }

    SampleStatus_t pre_status = s_sample_status;
    if (strcmp(argv[1], "on") == 0)
    {
        s_sample_status = SAMPLE_STATUS_ON;
    }
    else if (strcmp(argv[1], "off") == 0)
    {
        s_sample_status = SAMPLE_STATUS_OFF;
    }
    else
    {
        printf("[NN] %s:%d Error input!\n", __FILE__, __LINE__);
        res = 1;
        goto done;
    }

    if (s_sample_status == pre_status)
    {
        printf("Sample status does not change: '%s'\n", GetStringFromSampleStatus(s_sample_status));
    }
    else
    {
        printf("Sample status changed to '%s'\n", GetStringFromSampleStatus(s_sample_status));
    }

done:
    return res;
}

int main(int argc, char **argv)
{
    NN_LinenoiseRegisterCommand_t sample_status_cmd_config = {
        .m_func = Sample_ShowStatusCmd,
        .m_command_name = "sample-status",
        .m_help_msg = "Show current sample status: <ON/OFF>",
    };

    if (NN_LinenoiseRegisterCommand(&sample_status_cmd_config))
    {
        return -1;
    }

    NN_LinenoiseRegisterCommand_t sample_ctrl_cmd_config = {
        .m_func = Sample_CtrlCmd,
        .m_command_name = "sample-ctrl",
        .m_help_msg = "Change sample status: <on/off>",
    };

    if (NN_LinenoiseRegisterCommand(&sample_ctrl_cmd_config))
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
