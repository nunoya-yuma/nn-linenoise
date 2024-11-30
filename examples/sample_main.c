#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "nn_cli.h"

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

static NNCli_Err_t Sample_ShowStatusCmd(int argc, char **argv)
{
    int res = NN_CLI__SUCCESS;
    if (argc != 1)
    {
        printf("[NN] %s:%d Error input!\n", __FILE__, __LINE__);
        res = NN_CLI__INVALID_ARGS;
        goto done;
    }

    printf("Sample status: '%s'\n", GetStringFromSampleStatus(s_sample_status));

done:
    return res;
}

static NNCli_Err_t Sample_CtrlCmd(int argc, char **argv)
{
    int res = NN_CLI__SUCCESS;
    if (argc != 2)
    {
        printf("[NN] %s:%d Error input!\n", __FILE__, __LINE__);
        res = NN_CLI__INVALID_ARGS;
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
        res = NN_CLI__INVALID_ARGS;
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

static NNCli_Option_t parse_args_and_get_option(int argc, char **argv)
{
    NNCli_Option_t ret_option = {0};
    int opt;
    int option_index = 0;

    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"async", no_argument, NULL, 'a'},
        {"key-codes", no_argument, NULL, 'k'},
        {"multi-line", no_argument, NULL, 'm'},
        {0, 0, 0, 0},
    };

    while ((opt = getopt_long(argc, argv, "hakm", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -h, --help    Show this help message\n");
            printf("  -a, --async    Input processing is asynchronous.\n");
            printf("  -k, --key-codes    Displays the code of the character typed in.\n");
            printf("  -m, --multi-line    The string will automatically wrap when it reaches the edge of the screen.\n");
            exit(0);

        case 'a':
            ret_option.m_async.m_enabled = true;
            // By setting this timeout, input is monitored for 1 second before exiting the process.
            ret_option.m_async.m_timeout.tv_sec = 1;
            ret_option.m_async.m_timeout.tv_usec = 0;
            break;

        case 'k':
            ret_option.m_show_key_codes = true;
            break;

        case 'm':
            ret_option.m_enable_multi_line = true;
            break;

        case '?':
            fprintf(stderr, "Invalid option\n");
            exit(1);

        default:
            fprintf(stderr, "Unexpected case\n");
            exit(1);
        }
    }

    if (optind != argc)
    {
        fprintf(stderr, "Non-option argument: %s\n", argv[optind]);
        exit(1);
    }

    return ret_option;
}

int main(int argc, char **argv)
{
    NNCli_Option_t option = parse_args_and_get_option(argc, argv);
    option.m_history_filename = "/tmp/history.txt";

    static const NNCli_Command_t sample_status_cmd_config = {
        .m_func = Sample_ShowStatusCmd,
        .m_name = "sample-status",
        .m_options = NULL,
        .m_help_msg = "Show current sample status: <on/off>",
    };

    if (NNCli_RegisterCommand(&sample_status_cmd_config) != NN_CLI__SUCCESS)
    {
        return -1;
    }

    static const NNCli_Command_t sample_ctrl_cmd_config = {
        .m_func = Sample_CtrlCmd,
        .m_name = "sample-ctrl",
        .m_options = "on/off",
        .m_help_msg = "Change sample status: <on/off>",
    };

    if (NNCli_RegisterCommand(&sample_ctrl_cmd_config) != NN_CLI__SUCCESS)
    {
        return -1;
    }

    if (NNCli_Init(&option) != NN_CLI__SUCCESS)
    {
        return -1;
    }

    while (NNCli_Run() == NN_CLI__SUCCESS)
    {
        /* do nothing */
    }

    return 0;
}
