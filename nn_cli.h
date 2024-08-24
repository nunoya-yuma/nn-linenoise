#pragma once

#ifndef NN_CLI__MAX_COMMAND_NUM
#define NN_CLI__MAX_COMMAND_NUM 200
#endif

typedef int (*NNCli_Command_t)(int argc, char **argv);

typedef struct
{
    NNCli_Command_t m_func;
    const char *m_name;
    const char *m_options;
    const char *m_help_msg;
} NNCli_Register_t;

typedef enum
{
    NN_CLI__SUCCESS = 0,        // No errors have occurred.
    NN_CLI__GENERAL_ERROR,      // Errors that are difficult to categorize specifically.
    NN_CLI__INVALID_ARGS,       // Input arguments are incorrect.
    NN_CLI__EXCEED_CAPACITY,    // The size limit for arrays, etc. has been reached.
    NN_CLI__EXTERNAL_LIB_ERROR, // Errors in external libraries or tools used
    NN_CLI__PROCESS_COMPLETED,  // This indicates that the process has been completed, but it may have been completed successfully.
} NNCli_Err_t;

NNCli_Err_t NNCli_RegisterCommand(const NNCli_Register_t *a_cmd);
NNCli_Err_t NNCli_Init(int argc, char **argv);
NNCli_Err_t NNCli_Run(void);
