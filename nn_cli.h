#pragma once

#include <stdbool.h>
#include <sys/time.h>

typedef enum
{
    NN_CLI__SUCCESS = 0,        // No errors have occurred.
    NN_CLI__GENERAL_ERROR,      // Errors that are difficult to categorize specifically.
    NN_CLI__INVALID_ARGS,       // Input arguments are incorrect.
    NN_CLI__EXCEED_CAPACITY,    // The size limit for arrays, etc. has been reached.
    NN_CLI__EXTERNAL_LIB_ERROR, // Errors in external libraries or tools used
    NN_CLI__PROCESS_COMPLETED,  // It means that the process has been completed, but it may have been completed successfully.
    NN_CLI__IN_PROGRESS,        // It means that the process is in progress internally. This may not always be an error.
    NN_CLI__DUPLICATE           // It means that the data is duplicated.
} NNCli_Err_t;

typedef NNCli_Err_t (*NNCli_Func_t)(int argc, char **argv);

typedef struct
{
    // `m_func` should return `NN_CLI__SUCCESS` if no error occurs.
    // If an error occurs, return another error.
    // The type of these errors does not affect the operation.
    NNCli_Func_t m_func;
    const char *m_name;
    const char *m_options;
    const char *m_help_msg;
} NNCli_Command_t;

typedef struct
{
    bool m_enabled;
    struct timeval m_timeout;
} NNCli_AsyncOption_t;

typedef struct
{
    bool m_enable_multi_line;
    bool m_show_key_codes;
    NNCli_AsyncOption_t m_async;
    const char *m_history_filename;
} NNCli_Option_t;

#ifdef __cplusplus
extern "C"
{
#endif

    NNCli_Err_t NNCli_RegisterCommand(const NNCli_Command_t *a_cmd);
    NNCli_Err_t NNCli_Init(const NNCli_Option_t *a_option);
    NNCli_Err_t NNCli_Run(void);

#ifdef __cplusplus
}
#endif
