#pragma once

#ifndef NN_LINENOISE_MAX_COMMAND_NUM
#define NN_LINENOISE_MAX_COMMAND_NUM 200
#endif

typedef int (*NN_LinenoiseCommand_t)(int argc, char **argv);

typedef struct
{
    NN_LinenoiseCommand_t m_func;
    const char *m_name;
    const char *m_options;
    const char *m_help_msg;
} NN_LinenoiseRegisterCommand_t;

int NN_LinenoiseRegisterCommand(const NN_LinenoiseRegisterCommand_t *a_cmd);
int NN_LinenoiseInit(int argc, char **argv);
int NN_LinenoiseRun(void);
