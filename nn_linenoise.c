#include "nn_linenoise.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include "linenoise.h"

#define MAX_NUM_OF_WORDS_PER_COMMAND 20
#define COMMAND_STRING_MAX_LEN 1024

static int s_async;
static NN_LinenoiseRegisterCommand_t s_registered_command_list[NN_LINENOISE_MAX_COMMAND_NUM];
static int s_current_registered_cmd_num;

void completion(const char *buf, linenoiseCompletions *lc)
{
    if (buf[0] == 'h')
    {
        linenoiseAddCompletion(lc, "hello");
        linenoiseAddCompletion(lc, "hello there");
    }
    else if (buf[0] == 's')
    {
        linenoiseAddCompletion(lc, "sample-status");
    }
}

char *hints(const char *buf, int *color, int *bold)
{
    if (!strcasecmp(buf, "hello"))
    {
        *color = 35;
        *bold = 0;
        return " World";
    }
    return NULL;
}

static int SplitStringWithSpace(const char *a_raw_command, char **out_tokens)
{
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
    bool is_found = false;
    for (int i = 0; i < s_current_registered_cmd_num; i++)
    {
        char *args[MAX_NUM_OF_WORDS_PER_COMMAND];
        int argc = SplitStringWithSpace(a_command, args);
        const char *first_command = args[0];
        if (strncmp(first_command, s_registered_command_list[i].m_command_name, strlen(first_command)) == 0)
        {
            if (s_registered_command_list[i].m_func(argc, args))
            {
                printf("\n[Usage]\n%s | %s\n", s_registered_command_list[i].m_command_name, s_registered_command_list[i].m_help_msg);
            }
            is_found = true;
            break;
        }
    }

    if (!is_found)
    {
        printf("Invalid command: '%s'\n", a_command);
    }
}

/**
 *
 * Public functions
 *
 */

int NN_LinenoiseRegisterCommand(const NN_LinenoiseRegisterCommand_t *a_cmd)
{
    int res = 0;
    if (s_current_registered_cmd_num >= NN_LINENOISE_MAX_COMMAND_NUM)
    {
        printf("[NN_Linenoise] Error: The maximum number of commands that can be registered has been exceeded\n");
        res = 1;
        goto done;
    }

    if (a_cmd == NULL || a_cmd->m_func == NULL || a_cmd->m_command_name == NULL || a_cmd->m_help_msg == NULL)
    {
        printf("[NN_Linenoise] Error: An invalid command was attempted to be registered\n");
        res = 2;
        goto done;
    }

    memcpy(&s_registered_command_list[s_current_registered_cmd_num], a_cmd, sizeof(NN_LinenoiseRegisterCommand_t));
    s_current_registered_cmd_num++;

done:
    return res;
}

int NN_LinenoiseInit(int argc, char **argv)
{
    char *prgname = argv[0];

    /* Parse options, with --multiline we enable multi line editing. */
    while (argc > 1)
    {
        argc--;
        argv++;
        if (!strcmp(*argv, "--multiline"))
        {
            linenoiseSetMultiLine(1);
            printf("Multi-line mode enabled.\n");
        }
        else if (!strcmp(*argv, "--keycodes"))
        {
            linenoisePrintKeyCodes();
            exit(0);
        }
        else if (!strcmp(*argv, "--async"))
        {
            s_async = 1;
        }
        else
        {
            fprintf(stderr, "Usage: %s [--multiline] [--keycodes] [--async]\n", prgname);
            exit(1);
        }
    }

    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    linenoiseSetCompletionCallback(completion);
    // linenoiseSetHintsCallback(hints);

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    linenoiseHistoryLoad("/tmp/history.txt"); /* Load the history at startup */
}

/* Now this is the main loop of the typical linenoise-based application.
 * The call to linenoise() will block as long as the user types something
 * and presses enter.
 *
 * The typed string is returned as a malloc() allocated string by
 * linenoise, so the user needs to free() it. */
int NN_LinenoiseRun(void)
{
    int err = 0;
    char *line;
    if (!s_async)
    {
        line = linenoise("> ");
        if (line == NULL)
        {
            err = 1;
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
        struct timeval tv;
        int retval;

        FD_ZERO(&readfds);
        FD_SET(ls.ifd, &readfds);
        tv.tv_sec = 1; // 1 sec timeout
        tv.tv_usec = 0;

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
            printf("Async output %d.\n", counter++);
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
        printf("Unrecognized command: %s\n", line);
    }
    free(line);

done:
    return err;
}
