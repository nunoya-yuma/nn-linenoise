#include "nn_cli.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "check_config.h"
#include "linenoise.h"

#define MAX_NUM_OF_WORDS_PER_COMMAND 20
#define COMMAND_STRING_MAX_LEN 1024

#define NNCli_AssertWithMsg(cond, ...) \
    if (!(cond))                       \
    {                                  \
        NNCli_LogError(__VA_ARGS__);   \
        NNCli_Assert(cond);            \
    }

#define NNCli_AssertOrReturn(cond, res, ...)    \
    if (!(cond))                                \
    {                                           \
        NNCli_AssertWithMsg(cond, __VA_ARGS__); \
        return res;                             \
    }

#define NNCli_AssertOrReturnVoid(cond, ...)     \
    if (!(cond))                                \
    {                                           \
        NNCli_AssertWithMsg(cond, __VA_ARGS__); \
        return;                                 \
    }

typedef struct
{
    const NNCli_Command_t *m_command[NN_CLI__MAX_COMMAND_NUM];
    size_t m_num;
} CommandList_t;

static NNCli_AsyncOption_t s_async;
static CommandList_t s_command_list;
static char *s_history_filename;
static bool s_is_initialized = false;

static void completion(const char *buf, linenoiseCompletions *lc)
{
    NNCli_AssertOrReturnVoid(buf, "buf is NULL");
    NNCli_AssertOrReturnVoid(lc, "lc is NULL");

    bool found = false;
    for (int i = 0; i < s_command_list.m_num; i++)
    {
        if (strncmp(buf, s_command_list.m_command[i]->m_name, strlen(buf)) == 0)
        {
            linenoiseAddCompletion(lc, s_command_list.m_command[i]->m_name);
            found = true;
        }
    }

    // If no candidate command exists, leave it as is and do not add a space by
    // tab.
    if (!found)
    {
        linenoiseAddCompletion(lc, buf);
    }
}

static char *hints(const char *buf, int *color, int *bold)
{
    NNCli_AssertOrReturn(buf, NULL, "buf is NULL");
    NNCli_AssertOrReturn(color, NULL, "color is NULL");
    NNCli_AssertOrReturn(bold, NULL, "bold is NULL");

    static char option_str[COMMAND_STRING_MAX_LEN];
    memset(option_str, 0, sizeof(option_str));

    *color = 35;
    *bold = 0;

    for (int i = 0; i < s_command_list.m_num; i++)
    {
        if (strcmp(buf, s_command_list.m_command[i]->m_name) == 0 &&
            s_command_list.m_command[i]->m_options != NULL)
        {
            option_str[0] = ' ';
            // 2 byte (subtracted at the end ) = 1 byte (for the first move) + 1
            // byte (of the last null character)
            strncpy(&option_str[1], s_command_list.m_command[i]->m_options,
                    sizeof(option_str) - 2);
            break;
        }
    }

    return option_str;
}

static NNCli_Err_t SplitStringWithSpace(const char *a_raw_command,
                                        char **out_tokens, int *out_token_count)
{
    NNCli_Err_t res = NN_CLI__INVALID_ARGS;
    NNCli_AssertOrReturn(a_raw_command, NN_CLI__INVALID_ARGS,
                         "a_raw_command is NULL");
    NNCli_AssertOrReturn(out_tokens, NN_CLI__INVALID_ARGS,
                         "out_tokens is NULL");

    static char strCopy[COMMAND_STRING_MAX_LEN];
    strncpy(strCopy, a_raw_command, sizeof(strCopy) - 1);
    strCopy[sizeof(strCopy) - 1] = '\0';

    int token_count = 0;
    char *context = NULL;
    char *token = strtok_r(strCopy, " ", &context);

    while (token != NULL)
    {
        if (token_count == MAX_NUM_OF_WORDS_PER_COMMAND)
        {
            NNCli_LogError(
                "The number of words in the command exceeds the "
                "maximum limit: %d",
                MAX_NUM_OF_WORDS_PER_COMMAND);
            res = NN_CLI__EXCEED_CAPACITY;
            goto done;
        }
        out_tokens[token_count++] = token;
        token = strtok_r(NULL, " ", &context);
    }

    *out_token_count = token_count;
    res = NN_CLI__SUCCESS;

done:
    return res;
}

static NNCli_Err_t CallRegisteredCommand(const char *a_command)
{
    NNCli_Err_t res = NN_CLI__INVALID_ARGS;
    NNCli_AssertOrReturn(a_command, res, "a_command is NULL");

    for (int i = 0; i < s_command_list.m_num; i++)
    {
        char *args[MAX_NUM_OF_WORDS_PER_COMMAND];
        int argc;
        res = SplitStringWithSpace(a_command, args, &argc);
        if (res != NN_CLI__SUCCESS)
        {
            goto done;
        }

        const char *first_command = args[0];
        if (strcmp(first_command, s_command_list.m_command[i]->m_name) == 0)
        {
            if (s_command_list.m_command[i]->m_func(argc, args) !=
                NN_CLI__SUCCESS)
            {
                NNCli_LogWarn("Command args are incorrect. %s | %s",
                              s_command_list.m_command[i]->m_name,
                              s_command_list.m_command[i]->m_help_msg);
            }
            res = NN_CLI__SUCCESS;
            goto done;
        }
    }

    // If the command is found, this function will be exited before this line.
    NNCli_LogError("Command not found");
    res = NN_CLI__SUCCESS;  // This is not an error.

done:
    return res;
}

static NNCli_Err_t GetInputAsync(char **out_string)
{
    /* Asynchronous mode using the multiplexing API: wait for
     * data on stdin, and simulate async data coming from some source
     * using the select(2) timeout. */
    NNCli_Err_t ret = NN_CLI__IN_PROGRESS;
    static bool s_requires_init = true;
    static struct linenoiseState ls;
    static char buf[1024];
    if (s_requires_init)
    {
        memset(buf, 0, sizeof(buf));
        memset(&ls, 0, sizeof(ls));
        s_requires_init = false;
        linenoiseEditStart(&ls, -1, -1, buf, sizeof(buf), "> ");
    }

    fd_set readfds;
    int retval;
    struct timeval tv = s_async.m_timeout;

    FD_ZERO(&readfds);
    FD_SET(ls.ifd, &readfds);

    retval = select(ls.ifd + 1, &readfds, NULL, NULL, &tv);
    if (retval == -1)
    {
        perror("select()");
        exit(1);
    }
    else if (retval)
    {
        *out_string = linenoiseEditFeed(&ls);
        /* A NULL return means: line editing is continuing.
         * Otherwise the user hit enter or stopped editing
         * (CTRL+C/D). */
        if (*out_string == linenoiseEditMore)
        {
            goto done;
        }
    }
    else
    {
        // Timeout occurred

        // static int s_debug_print_counter = 0;
        // if (s_debug_print_counter < 3 || s_debug_print_counter % 10 == 0)
        // {
        //     linenoiseHide(&ls);
        //     printf("...\r\n");
        //     linenoiseShow(&ls);
        // }
        // ++s_debug_print_counter;

        goto done;
    }

    linenoiseEditStop(&ls);
    s_requires_init = true;
    if (*out_string == NULL)
    {
        exit(0); /* Ctrl+D/C. */
    }

    ret = NN_CLI__SUCCESS;

done:
    return ret;
}

static NNCli_Err_t GetInputSync(char **out_string)
{
    char *line = linenoise("> ");
    if (line == NULL)
    {
        return NN_CLI__PROCESS_COMPLETED;
    }

    *out_string = line;
    return NN_CLI__SUCCESS;
}

/**
 * Default CLI commands
 */

static void ShowAllCommands(void)
{
    for (int i = 0; i < s_command_list.m_num; i++)
    {
        printf("%s: %s\n", s_command_list.m_command[i]->m_name,
               s_command_list.m_command[i]->m_help_msg);
    }
}

static NNCli_Err_t HelpCommand(int argc, char **argv)
{
    if (argc != 1)
    {
        return NN_CLI__INVALID_ARGS;
    }

    ShowAllCommands();
    return NN_CLI__SUCCESS;
}

static NNCli_Err_t HistoryLenCommand(int argc, char **argv)
{
    if (argc != 2)
    {
        return NN_CLI__INVALID_ARGS;
    }

    /* The "/historylen" command will change the history len. */
    linenoiseHistorySetMaxLen(atoi(argv[1]));

    return NN_CLI__SUCCESS;
}

static NNCli_Err_t MaskCommand(int argc, char **argv)
{
    NNCli_Err_t res = NN_CLI__INVALID_ARGS;
    if (argc != 2)
    {
        goto done;
    }

    if (strcmp(argv[1], "on") == 0)
    {
        linenoiseMaskModeEnable();
    }
    else if (strcmp(argv[1], "off") == 0)
    {
        linenoiseMaskModeDisable();
    }
    else
    {
        goto done;
    }

    res = NN_CLI__SUCCESS;

done:
    return res;
}

static void RegisterDefaultCommand(void)
{
    static const NNCli_Command_t help_command = {
        .m_func = HelpCommand,
        .m_name = "help",
        .m_options = NULL,
        .m_help_msg = "Show registered commands",
    };
    NNCli_Err_t help_res = NNCli_RegisterCommand(&help_command);
    NNCli_AssertWithMsg(help_res == NN_CLI__SUCCESS,
                        "Failed to register help command: %d", help_res);

    static const NNCli_Command_t history_len_command = {
        .m_func = HistoryLenCommand,
        .m_name = "historylen",
        .m_options = "size",
        .m_help_msg = "Set the number of histories to keep",
    };
    NNCli_Err_t history_len_res = NNCli_RegisterCommand(&history_len_command);
    NNCli_AssertWithMsg(history_len_res == NN_CLI__SUCCESS,
                        "Failed to register historylen command: %d",
                        history_len_res);

    static const NNCli_Command_t mask_command = {
        .m_func = MaskCommand,
        .m_name = "mask",
        .m_options = "on/off",
        .m_help_msg = "Turn on/off masking of input characters <on/off>",
    };
    NNCli_Err_t mask_res = NNCli_RegisterCommand(&mask_command);
    NNCli_AssertWithMsg(mask_res == NN_CLI__SUCCESS,
                        "Failed to register mask command: %d", mask_res);
}

static bool CheckOrCreateFile(const char *filename)
{
    struct stat buffer;
    if (stat(filename, &buffer) != 0)
    {
        NNCli_LogWarn("History file does not exist. Creating a new file");
        FILE *fp = fopen(filename, "w");
        if (fp == NULL)
        {
            NNCli_LogError("Failed to create history file");
            return false;
        }
        fclose(fp);
    }

    return true;
}

static bool IsInitialized(void) { return s_is_initialized; }

/**
 * Public functions
 */

NNCli_Err_t NNCli_RegisterCommand(const NNCli_Command_t *a_cmd)
{
    NNCli_Err_t res = NN_CLI__SUCCESS;
    // Allow m_options to be NULL.
    if (a_cmd == NULL || a_cmd->m_func == NULL || a_cmd->m_name == NULL ||
        a_cmd->m_help_msg == NULL || strlen(a_cmd->m_name) == 0)
    {
        NNCli_LogError("An invalid command was attempted to be registered");
        res = NN_CLI__INVALID_ARGS;
        goto done;
    }

    if (s_command_list.m_num >= NN_CLI__MAX_COMMAND_NUM)
    {
        NNCli_LogError(
            "The maximum number of commands that can be registered has been "
            "exceeded");
        res = NN_CLI__EXCEED_CAPACITY;
        goto done;
    }

    for (size_t i = 0; i < s_command_list.m_num; i++)
    {
        if (strcmp(s_command_list.m_command[i]->m_name, a_cmd->m_name) == 0)
        {
            NNCli_LogError("%s command is already registered", a_cmd->m_name);
            res = NN_CLI__DUPLICATE;
            goto done;
        }
    }

    s_command_list.m_command[s_command_list.m_num] = a_cmd;
    s_command_list.m_num++;

done:
    return res;
}

NNCli_Err_t NNCli_Init(const NNCli_Option_t *a_option)
{
    NNCli_Err_t res = NN_CLI__IN_PROGRESS;
    if (IsInitialized())
    {
        goto done;
    }

    res = NN_CLI__INVALID_ARGS;
    if (a_option == NULL)
    {
        NNCli_LogError("a_option is NULL");
        goto done;
    }
    if (a_option->m_history_filename == NULL)
    {
        NNCli_LogError("a_option->m_history_filename is NULL");
        goto done;
    }
    if (strlen(a_option->m_history_filename) == 0)
    {
        NNCli_LogError("history filename is empty");
        goto done;
    }

    if (!CheckOrCreateFile(a_option->m_history_filename))
    {
        res = NN_CLI__GENERAL_ERROR;
        goto done;
    }

    /* Parse options, with --multiline we enable multi line editing. */
    if (a_option->m_enable_multi_line)
    {
        NNCli_LogInfo("Multi-line mode enabled");
        linenoiseSetMultiLine(1);
    }
    if (a_option->m_show_key_codes)
    {
        NNCli_LogInfo("Print key codes mode enabled");
        // Enter a loop within this function and stop the program after it
        // finishes by "quit"
        linenoisePrintKeyCodes();
        exit(0);
    }
    if (a_option->m_async.m_enabled)
    {
        NNCli_LogInfo("Async mode enabled");
        s_async = a_option->m_async;
    }

    NNCli_AssertOrReturn(s_history_filename == NULL, NN_CLI__GENERAL_ERROR,
                         "s_history_filename is not NULL");
    // This is not released by free() until the end, because it is used to save
    // the command each time.
    s_history_filename =
        (char *)malloc(strlen(a_option->m_history_filename) + 1);
    if (s_history_filename == NULL)
    {
        res = NN_CLI__GENERAL_ERROR;
        NNCli_LogError("Failed to allocate memory for history filename");
        goto done;
    }
    strcpy(s_history_filename, a_option->m_history_filename);

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    if (linenoiseHistoryLoad(s_history_filename))
    {
        res = NN_CLI__EXTERNAL_LIB_ERROR;
        goto done;
    }

    // Register basic commands such as help.
    RegisterDefaultCommand();

    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

    s_is_initialized = true;
    res = NN_CLI__SUCCESS;

done:
    return res;
}

/* Now this is the main loop of the typical linenoise-based application.
 * The call to linenoise() will block as long as the user types something
 * and presses enter. linenoise() is called at GetInputAsync() and
 * GetInputSync().
 *
 * The typed string is returned as a malloc() allocated string by
 * linenoise, so the user needs to free() it. */
NNCli_Err_t NNCli_Run(void)
{
    NNCli_Err_t err = NN_CLI__NOT_READY;
    char *line;
    if (!IsInitialized())
    {
        NNCli_LogError("NNCli is not initialized");
        goto done;
    }

    if (s_async.m_enabled)
    {
        err = GetInputAsync(&line);
        if (err != NN_CLI__SUCCESS)
        {
            goto done;
        }
    }
    else
    {
        err = GetInputSync(&line);
        if (err != NN_CLI__SUCCESS)
        {
            goto done;
        }
    }

    /* Do something with the string. */
    if (line[0] != '\0')
    {
        err = CallRegisteredCommand(line);
        if (err == NN_CLI__SUCCESS)
        {
            linenoiseHistoryAdd(line); /* Add to the history. */
            linenoiseHistorySave(
                s_history_filename); /* Save the history on disk. */
        }
    }
    free(line);

done:
    return err;
}
