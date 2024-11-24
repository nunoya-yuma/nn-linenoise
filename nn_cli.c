#include "nn_cli.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include "linenoise.h"

#include "check_config.h"

#define MAX_NUM_OF_WORDS_PER_COMMAND 20
#define COMMAND_STRING_MAX_LEN 1024

typedef struct
{
    NNCli_Command_t m_command[NN_CLI__MAX_COMMAND_NUM];
    size_t m_num;
} CommandList_t;

static NNCli_AsyncOption_t s_async;
static CommandList_t s_command_list;
static char *s_history_filename;

static void completion(const char *buf, linenoiseCompletions *lc)
{
    NNCli_Assert(buf);
    NNCli_Assert(lc);

    bool found = false;
    for (int i = 0; i < s_command_list.m_num; i++)
    {
        if (strncmp(buf, s_command_list.m_command[i].m_name, strlen(buf)) == 0)
        {
            linenoiseAddCompletion(lc, s_command_list.m_command[i].m_name);
            found = true;
        }
    }

    // If no candidate command exists, leave it as is and do not add a space by tab.
    if (!found)
    {
        linenoiseAddCompletion(lc, buf);
    }
}

static char *hints(const char *buf, int *color, int *bold)
{
    NNCli_Assert(buf);
    NNCli_Assert(color);
    NNCli_Assert(bold);

    static char option_str[COMMAND_STRING_MAX_LEN];
    memset(option_str, 0, sizeof(option_str));

    *color = 35;
    *bold = 0;

    for (int i = 0; i < s_command_list.m_num; i++)
    {
        if (strcmp(buf, s_command_list.m_command[i].m_name) == 0 &&
            s_command_list.m_command[i].m_options != NULL)
        {
            option_str[0] = ' ';
            // 2 byte (subtracted at the end ) = 1 byte (for the first move) + 1 byte (of the last null character)
            strncpy(&option_str[1], s_command_list.m_command[i].m_options, sizeof(option_str) - 2);
            break;
        }
    }

    return option_str;
}

static int SplitStringWithSpace(const char *a_raw_command, char **out_tokens)
{
    NNCli_Assert(a_raw_command);
    NNCli_Assert(out_tokens);

    static char strCopy[COMMAND_STRING_MAX_LEN];
    strncpy(strCopy, a_raw_command, sizeof(strCopy) - 1);
    strCopy[sizeof(strCopy) - 1] = '\0';

    int tokenCount = 0;
    char *context = NULL;
    char *token = strtok_r(strCopy, " ", &context);

    while (token != NULL && tokenCount < MAX_NUM_OF_WORDS_PER_COMMAND)
    {
        out_tokens[tokenCount++] = token;
        token = strtok_r(NULL, " ", &context);
    }

    return tokenCount;
}

static void CallRegisteredCommand(const char *a_command)
{
    NNCli_Assert(a_command);

    for (int i = 0; i < s_command_list.m_num; i++)
    {
        char *args[MAX_NUM_OF_WORDS_PER_COMMAND];
        int argc = SplitStringWithSpace(a_command, args);
        const char *first_command = args[0];
        if (strcmp(first_command, s_command_list.m_command[i].m_name) == 0)
        {
            if (s_command_list.m_command[i].m_func(argc, args) != NN_CLI__SUCCESS)
            {
                NNCli_LogWarn("Command args are incorrect. %s | %s", s_command_list.m_command[i].m_name, s_command_list.m_command[i].m_help_msg);
            }
            return;
        }
    }

    // If the command is found, return above and do not come here.
    NNCli_LogError("Command not found");
}

static NNCli_Err_t GetInputAsync(char **out_string)
{
    /* Asynchronous mode using the multiplexing API: wait for
     * data on stdin, and simulate async data coming from some source
     * using the select(2) timeout. */
    NNCli_Err_t ret = NN_CLI_IN_PROGRESS;
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
        printf("%s: %s\n", s_command_list.m_command[i].m_name, s_command_list.m_command[i].m_help_msg);
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
    NNCli_Command_t help_command = {
        .m_func = HelpCommand,
        .m_help_msg = "Show registered commands",
        .m_name = "help",
        .m_options = NULL,
    };
    NNCli_Assert(NNCli_RegisterCommand(&help_command) == NN_CLI__SUCCESS);

    NNCli_Command_t history_len_command = {
        .m_func = HistoryLenCommand,
        .m_help_msg = "Set the number of histories to keep",
        .m_name = "historylen",
        .m_options = "size",
    };
    NNCli_Assert(NNCli_RegisterCommand(&history_len_command) == NN_CLI__SUCCESS);

    NNCli_Command_t mask_command = {
        .m_func = MaskCommand,
        .m_help_msg = "Turn on/off masking of input characters <on/off>",
        .m_name = "mask",
        .m_options = "on/off",
    };
    NNCli_Assert(NNCli_RegisterCommand(&mask_command) == NN_CLI__SUCCESS);
}

/**
 *
 * Public functions
 *
 */

NNCli_Err_t NNCli_RegisterCommand(const NNCli_Command_t *a_cmd)
{
    NNCli_Err_t res = NN_CLI__SUCCESS;
    // Allow m_options to be NULL.
    if (a_cmd == NULL || a_cmd->m_func == NULL || a_cmd->m_name == NULL || a_cmd->m_help_msg == NULL)
    {
        NNCli_LogError("An invalid command was attempted to be registered");
        res = NN_CLI__INVALID_ARGS;
        goto done;
    }

    if (s_command_list.m_num >= NN_CLI__MAX_COMMAND_NUM)
    {
        NNCli_LogError("The maximum number of commands that can be registered has been exceeded");
        res = NN_CLI__EXCEED_CAPACITY;
        goto done;
    }

    memcpy(&s_command_list.m_command[s_command_list.m_num], a_cmd, sizeof(NNCli_Command_t));
    s_command_list.m_num++;

done:
    return res;
}

NNCli_Err_t NNCli_Init(const NNCli_Option_t *a_option)
{
    NNCli_Assert(a_option);
    NNCli_Assert(a_option->m_history_filename);
    NNCli_Assert(strlen(a_option->m_history_filename) > 0);

    NNCli_Err_t res = NN_CLI__SUCCESS;

    /* Parse options, with --multiline we enable multi line editing. */
    if (a_option->m_enable_multi_line)
    {
        NNCli_LogInfo("Multi-line mode enabled");
        linenoiseSetMultiLine(1);
    }
    if (a_option->m_show_key_codes)
    {
        NNCli_LogInfo("Print key codes mode enabled");
        // Enter a loop within this function and stop the program after it finishes by "quit"
        linenoisePrintKeyCodes();
        exit(0);
    }
    if (a_option->m_async.m_enabled)
    {
        NNCli_LogInfo("Async mode enabled");
        s_async = a_option->m_async;
    }

    // This is not released by free() until the end, because it is used to save the command each time.
    s_history_filename = (char *)malloc(strlen(a_option->m_history_filename) + 1);
    NNCli_Assert(s_history_filename != NULL);
    strcpy(s_history_filename, a_option->m_history_filename);

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    if (linenoiseHistoryLoad(s_history_filename))
    {
        res = NN_CLI__EXTERNAL_LIB_ERROR;
        goto done;
    }

    RegisterDefaultCommand();

    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

done:
    return res;
}

/* Now this is the main loop of the typical linenoise-based application.
 * The call to linenoise() will block as long as the user types something
 * and presses enter. linenoise() is called at GetInputAsync() and GetInputSync().
 *
 * The typed string is returned as a malloc() allocated string by
 * linenoise, so the user needs to free() it. */
NNCli_Err_t NNCli_Run(void)
{
    NNCli_Err_t err = NN_CLI__SUCCESS;
    char *line;
    if (s_async.m_enabled)
    {
        NNCli_Err_t ret_async = GetInputAsync(&line);
        if (ret_async == NN_CLI_IN_PROGRESS)
        {
            goto done;
        }
    }
    else
    {
        NNCli_Err_t ret_sync = GetInputSync(&line);
        if (ret_sync != NN_CLI__SUCCESS)
        {
            err = ret_sync;
            goto done;
        }
    }

    /* Do something with the string. */
    if (line[0] != '\0')
    {
        CallRegisteredCommand(line);
        linenoiseHistoryAdd(line);                /* Add to the history. */
        linenoiseHistorySave(s_history_filename); /* Save the history on disk. */
    }
    free(line);

done:
    return err;
}
