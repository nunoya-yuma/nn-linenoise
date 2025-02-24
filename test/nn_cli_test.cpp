#include "nn_cli.h"

#include <gtest/gtest.h>

#include "nn_cli.c"
#include "nn_cli_config.h"
namespace
{
NNCli_Err_t TestCmdFunc(int argc, char **argv)
{
    // Nothing to do
    return NN_CLI__SUCCESS;
}

void DummyKeyboardInput(const char *input)
{
    char filename[] = "/tmp/nncli_testXXXXXX";
    int fd = mkstemp(filename);
    if (fd == -1)
    {
        perror("mkstemp failed");
        assert(0);
    }
    close(fd);

    FILE *fake_stdin = fopen(filename, "w+");
    if (!fake_stdin)
    {
        perror("fopen failed");
        assert(0);
    }

    fprintf(fake_stdin, "%s", input);
    fclose(fake_stdin);

    freopen(filename, "r", stdin);
}

void GenerateDummyHistoryFile(char *filename)
{
    int fd = mkstemp(filename);
    if (fd == -1)
    {
        perror("mkstemp failed");
        assert(0);
    }
    close(fd);
}
}  // namespace

class NNCliTest : public ::testing::Test
{
   protected:
    void TearDown() override
    {
        s_async = {0};
        s_command_list = {0};
        s_is_initialized = false;
        if (s_history_filename != nullptr)
        {
            free(s_history_filename);
            s_history_filename = nullptr;
        }
    }
};

namespace testing
{
TEST_F(NNCliTest, RegisterCommand_Success)
{
    const NNCli_Command_t cmd = {
        .m_func = TestCmdFunc,
        .m_name = "test-cmd",
        .m_options = "on/off",
        .m_help_msg = "test help msg",
    };

    ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);

    const NNCli_Command_t no_option_cmd = {
        .m_func = TestCmdFunc,
        .m_name = "no-option-test-cmd",
        .m_options = nullptr,
        .m_help_msg = "test help msg",
    };
    ASSERT_EQ(NNCli_RegisterCommand(&no_option_cmd), NN_CLI__SUCCESS);
}

TEST_F(NNCliTest, RegisterCommand_InvalidArgs)
{
    ASSERT_EQ(NNCli_RegisterCommand(nullptr), NN_CLI__INVALID_ARGS);

    const NNCli_Command_t no_func_cmd = {
        .m_func = nullptr,
        .m_name = "test-cmd",
        .m_options = "on/off",
        .m_help_msg = "test help msg",
    };
    ASSERT_EQ(NNCli_RegisterCommand(&no_func_cmd), NN_CLI__INVALID_ARGS);

    const NNCli_Command_t no_name_cmd = {
        .m_func = TestCmdFunc,
        .m_name = "",
        .m_options = "on/off",
        .m_help_msg = "test help msg",
    };
    ASSERT_EQ(NNCli_RegisterCommand(&no_name_cmd), NN_CLI__INVALID_ARGS);
}

TEST_F(NNCliTest, RegisterCommand_upperLimit)
{
    NNCli_Command_t cmds[NN_CLI__MAX_COMMAND_NUM];
    std::string cmd_names[NN_CLI__MAX_COMMAND_NUM];

    for (int i = 0; i < NN_CLI__MAX_COMMAND_NUM; i++)
    {
        cmd_names[i] = "test-cmd" + std::to_string(i);
        NNCli_Command_t cmd = {
            .m_func = TestCmdFunc,
            .m_name = cmd_names[i].c_str(),
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };
        cmds[i] = cmd;

        ASSERT_EQ(NNCli_RegisterCommand(&cmds[i]), NN_CLI__SUCCESS);
    }

    const NNCli_Command_t cmd = {
        .m_func = TestCmdFunc,
        .m_name = "upper-limit-test-cmd",
        .m_options = "on/off",
        .m_help_msg = "test help msg",
    };
    ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__EXCEED_CAPACITY);
}

TEST_F(NNCliTest, RegisterCommand_PreventDuplicate)
{
    const NNCli_Command_t cmd = {
        .m_func = TestCmdFunc,
        .m_name = "test-cmd",
        .m_options = "on/off",
        .m_help_msg = "test help msg",
    };

    ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);
    ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__DUPLICATE);
}

TEST_F(NNCliTest, RegisterCommand_DuringOperation)
{
    const NNCli_Command_t cmd = {
        .m_func = TestCmdFunc,
        .m_name = "test-cmd",
        .m_options = "on/off",
        .m_help_msg = "test help msg",
    };
    ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);

    char filename[] = "/tmp/nncli_test_history_XXXXXX";
    GenerateDummyHistoryFile(filename);
    const NNCli_Option_t option = {
        .m_enable_multi_line = true,
        .m_show_key_codes = false,
        .m_async =
            {
                .m_enabled = false,
                .m_timeout = {.tv_sec = 0, .tv_usec = 0},
            },
        .m_history_filename = filename,
    };

    ASSERT_EQ(NNCli_Init(&option), NN_CLI__SUCCESS);

    DummyKeyboardInput("help\n");
    ASSERT_EQ(NNCli_Run(), NN_CLI__SUCCESS);

    const NNCli_Command_t additional_cmd = {
        .m_func = TestCmdFunc,
        .m_name = "test-additional-cmd",
        .m_options = "on/off",
        .m_help_msg = "test help msg",
    };
    ASSERT_EQ(NNCli_RegisterCommand(&additional_cmd), NN_CLI__SUCCESS);

    DummyKeyboardInput("test-additional-cmd\n");
    ASSERT_EQ(NNCli_Run(), NN_CLI__SUCCESS);
}

TEST_F(NNCliTest, Init_Success)
{
    char filename[] = "/tmp/nncli_test_history_XXXXXX";
    GenerateDummyHistoryFile(filename);
    const NNCli_Option_t option = {
        .m_enable_multi_line = true,
        .m_show_key_codes = false,
        .m_async =
            {
                .m_enabled = false,
                .m_timeout = {.tv_sec = 0, .tv_usec = 0},
            },
        .m_history_filename = filename,
    };

    ASSERT_EQ(NNCli_Init(&option), NN_CLI__SUCCESS);
}

TEST_F(NNCliTest, Init_InvalidArgs)
{
    ASSERT_EQ(NNCli_Init(nullptr), NN_CLI__INVALID_ARGS);

    const NNCli_Option_t option = {
        .m_enable_multi_line = true,
        .m_show_key_codes = false,
        .m_async =
            {
                .m_enabled = false,
                .m_timeout = {.tv_sec = 0, .tv_usec = 0},
            },
        .m_history_filename = ""};
    ASSERT_EQ(NNCli_Init(&option), NN_CLI__INVALID_ARGS);
}

TEST_F(NNCliTest, Init_CallTwice)
{
    char filename[] = "/tmp/nncli_test_history_XXXXXX";
    GenerateDummyHistoryFile(filename);
    const NNCli_Option_t option = {
        .m_enable_multi_line = true,
        .m_show_key_codes = false,
        .m_async =
            {
                .m_enabled = false,
                .m_timeout = {.tv_sec = 0, .tv_usec = 0},
            },
        .m_history_filename = filename,
    };

    ASSERT_EQ(NNCli_Init(&option), NN_CLI__SUCCESS);
    free(s_history_filename);
    s_history_filename = nullptr;

    ASSERT_EQ(NNCli_Init(&option), NN_CLI__IN_PROGRESS);
}

TEST_F(NNCliTest, Run_Success)
{
    const NNCli_Command_t cmd = {
        .m_func = TestCmdFunc,
        .m_name = "test-cmd",
        .m_options = "on/off",
        .m_help_msg = "test help msg",
    };

    ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);

    char filename[] = "/tmp/nncli_test_history_XXXXXX";
    GenerateDummyHistoryFile(filename);
    const NNCli_Option_t option = {
        .m_enable_multi_line = true,
        .m_show_key_codes = false,
        .m_async =
            {
                .m_enabled = false,
                .m_timeout = {.tv_sec = 0, .tv_usec = 0},
            },
        .m_history_filename = filename,
    };

    ASSERT_EQ(NNCli_Init(&option), NN_CLI__SUCCESS);

    DummyKeyboardInput("help\n");
    ASSERT_EQ(NNCli_Run(), NN_CLI__SUCCESS);
}

TEST_F(NNCliTest, Run_BeforeInit) { ASSERT_EQ(NNCli_Run(), NN_CLI__NOT_READY); }

TEST_F(NNCliTest, Run_WordLimitPerCommand)
{
    const NNCli_Command_t cmd = {
        .m_func = TestCmdFunc,
        .m_name = "test-cmd",
        .m_options = "",
        .m_help_msg = "test help msg",
    };

    ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);

    char filename[] = "/tmp/nncli_test_history_XXXXXX";
    GenerateDummyHistoryFile(filename);
    const NNCli_Option_t option = {
        .m_enable_multi_line = true,
        .m_show_key_codes = false,
        .m_async =
            {
                .m_enabled = false,
                .m_timeout = {.tv_sec = 0, .tv_usec = 0},
            },
        .m_history_filename = filename,
    };

    ASSERT_EQ(NNCli_Init(&option), NN_CLI__SUCCESS);

    std::string cmd_option;

    // Subtract 1 as "test-cmd" is already included
    for (size_t i = 0; i < MAX_NUM_OF_WORDS_PER_COMMAND - 1; i++)
    {
        cmd_option += " arg" + std::to_string(i + 2);
    }
    std::string upper_limit_cmd = "test-cmd" + cmd_option + "\n";

    DummyKeyboardInput(upper_limit_cmd.c_str());
    ASSERT_EQ(NNCli_Run(), NN_CLI__SUCCESS);

    std::string exceeding_upper_limit_cmd = "test-cmd" + cmd_option + " arg\n";
    DummyKeyboardInput(exceeding_upper_limit_cmd.c_str());
    ASSERT_EQ(NNCli_Run(), NN_CLI__EXCEED_CAPACITY);
}

TEST_F(NNCliTest, Run_CommandLengthUpperLimit)
{
    const NNCli_Command_t cmd = {
        .m_func = TestCmdFunc,
        .m_name = "test-cmd",
        .m_options = "",
        .m_help_msg = "test help msg",
    };
    ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);

    char filename[] = "/tmp/nncli_test_history_XXXXXX";
    GenerateDummyHistoryFile(filename);
    const NNCli_Option_t option = {
        .m_enable_multi_line = true,
        .m_show_key_codes = false,
        .m_async =
            {
                .m_enabled = false,
                .m_timeout = {.tv_sec = 0, .tv_usec = 0},
            },
        .m_history_filename = filename,
    };
    ASSERT_EQ(NNCli_Init(&option), NN_CLI__SUCCESS);

    std::string cmd_base = "test-cmd ";

    std::string string_for_upper_limit;
    for (size_t i = 0; i < COMMAND_STRING_MAX_LEN - cmd_base.size() - 1; i++)
    {
        string_for_upper_limit += "x";
    }
    std::string upper_limit_cmd = cmd_base + string_for_upper_limit + "\n";
    DummyKeyboardInput(upper_limit_cmd.c_str());
    ASSERT_EQ(NNCli_Run(), NN_CLI__SUCCESS);

    std::string exceeding_upper_limit_cmd =
        cmd_base + string_for_upper_limit + "x\n";
    DummyKeyboardInput(exceeding_upper_limit_cmd.c_str());
    ASSERT_EQ(NNCli_Run(), NN_CLI__EXCEED_CAPACITY);
}

}  // namespace testing
