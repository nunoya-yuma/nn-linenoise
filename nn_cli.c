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

static NNCli_AsyncOption_t s_async;
static NNCli_Command_t s_registered_command_list[NN_CLI__MAX_COMMAND_NUM];
static int s_current_registered_cmd_num;

static void completion(const char *buf, linenoiseCompletions *lc)
{
    NNCli_Assert(buf);
    NNCli_Assert(lc);

    bool found = false;
    for (int i = 0; i < s_current_registered_cmd_num; i++)
    {
        if (strncmp(buf, s_registered_command_list[i].m_name, strlen(buf)) == 0)
        {
            linenoiseAddCompletion(lc, s_registered_command_list[i].m_name);
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

    for (int i = 0; i < s_current_registered_cmd_num; i++)
    {
        if (s_registered_command_list[i].m_options == NULL)
        {
            continue;
        }

        if (strcmp(buf, s_registered_command_list[i].m_name) == 0)
        {
            option_str[0] = ' ';
            // 2 byte (subtracted at the end ) = 1 byte (for the first move) + 1 byte (of the last null character)
            strncpy(&option_str[1], s_registered_command_list[i].m_options, sizeof(option_str) - 2);
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

    bool is_found = false;
    for (int i = 0; i < s_current_registered_cmd_num; i++)
    {
        char *args[MAX_NUM_OF_WORDS_PER_COMMAND];
        int argc = SplitStringWithSpace(a_command, args);
        const char *first_command = args[0];
        if (strncmp(first_command, s_registered_command_list[i].m_name, strlen(first_command)) == 0)
        {
            if (s_registered_command_list[i].m_func(argc, args) != NN_CLI__SUCCESS)
            {
                NNCli_LogWarn("Command args are incorrect. %s | %s", s_registered_command_list[i].m_name, s_registered_command_list[i].m_help_msg);
            }
            is_found = true;
            break;
        }
    }

    if (!is_found)
    {
        NNCli_LogError("Invalid command: '%s'", a_command);
    }
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

    if (s_current_registered_cmd_num >= NN_CLI__MAX_COMMAND_NUM)
    {
        NNCli_LogError("The maximum number of commands that can be registered has been exceeded");
        res = NN_CLI__EXCEED_CAPACITY;
        goto done;
    }

    memcpy(&s_registered_command_list[s_current_registered_cmd_num], a_cmd, sizeof(NNCli_Command_t));
    s_current_registered_cmd_num++;

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

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    if (linenoiseHistoryLoad(a_option->m_history_filename))
    {
        res = NN_CLI__EXTERNAL_LIB_ERROR;
        goto done;
    }

    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

done:
    return res;
}

/* Now this is the main loop of the typical linenoise-based application.
 * The call to linenoise() will block as long as the user types something
 * and presses enter.
 *
 * The typed string is returned as a malloc() allocated string by
 * linenoise, so the user needs to free() it. */
NNCli_Err_t NNCli_Run(void)
{
    NNCli_Err_t err = NN_CLI__SUCCESS;
    char *line;
    if (!s_async.m_enabled)
    {
        line = linenoise("> ");
        if (line == NULL)
        {
            err = NN_CLI__PROCESS_COMPLETED;
            goto done;
        }
    }
    else
    {
        /* Asynchronous mode using the multiplexing API: wait for
         * data on stdin, and simulate async data coming from some source
         * using the select(2) timeout. */
        static bool s_is_async_started = false;
        static struct linenoiseState ls;
        static char buf[1024];
        if (!s_is_async_started)
        {
            memset(buf, 0, sizeof(buf));
            memset(&ls, 0, sizeof(ls));
            s_is_async_started = true;
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
            line = linenoiseEditFeed(&ls);
            /* A NULL return means: line editing is continuing.
             * Otherwise the user hit enter or stopped editing
             * (CTRL+C/D). */
            if (line == linenoiseEditMore)
            {
                goto done;
            }
        }
        else
        {
            // Timeout occurred
            static int counter = 0;
            linenoiseHide(&ls);
            NNCli_LogInfo("Async output %d", counter++);
            linenoiseShow(&ls);
            goto done;
        }

        linenoiseEditStop(&ls);
        s_is_async_started = false;
        if (line == NULL)
            exit(0); /* Ctrl+D/C. */
    }

    /* Do something with the string. */
    if (line[0] != '\0' && line[0] != '/')
    {
        CallRegisteredCommand(line);
        linenoiseHistoryAdd(line);                /* Add to the history. */
        linenoiseHistorySave("/tmp/history.txt"); /* Save the history on disk. */
    }
    else if (!strncmp(line, "/historylen", 11))
    {
        /* The "/historylen" command will change the history len. */
        int len = atoi(line + 11);
        linenoiseHistorySetMaxLen(len);
    }
    else if (!strncmp(line, "/mask", 5))
    {
        linenoiseMaskModeEnable();
    }
    else if (!strncmp(line, "/unmask", 7))
    {
        linenoiseMaskModeDisable();
    }
    else if (line[0] == '/')
    {
        NNCli_LogError("Unrecognized command: %s", line);
    }
    free(line);

done:
    return err;
}
