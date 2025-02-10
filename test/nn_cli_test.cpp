#include <gtest/gtest.h>

#include "nn_cli.h"
#include "nn_cli_config.h"

#include "nn_cli.c"
namespace
{
    NNCli_Err_t TestCmdFunc(int argc, char **argv)
    {
        // Nothing to do
        return NN_CLI__SUCCESS;
// Additional Unit Tests for NNCli_RegisterCommand, NNCli_Init, and NNCli_Run

// Mock command function for testing
NNCli_Err_t SampleCmd(int argc, char **argv) {
    return NN_CLI__SUCCESS;
}

// Test fixture for NNCli module tests
class NNCliTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup can include reset of any static command registration if applicable
    }

    void TearDown() override {
        // Clean up if necessary
    }
};

TEST_F(NNCliTest, RegisterCommandSuccess) {
    // Assuming NNCli_RegisterCommand returns NN_CLI__SUCCESS on success
    NNCli_Err_t ret = NNCli_RegisterCommand("test", SampleCmd);
    EXPECT_EQ(ret, NN_CLI__SUCCESS);
}

TEST_F(NNCliTest, RegisterCommandDuplicate) {
    // First registration should succeed, second might fail if duplicate names are not allowed
    NNCli_RegisterCommand("dup", SampleCmd);
    NNCli_Err_t ret = NNCli_RegisterCommand("dup", SampleCmd);
    // Depending on implementation, duplicate might be an error. Here we assume it returns error code.
    EXPECT_NE(ret, NN_CLI__SUCCESS);
}

TEST_F(NNCliTest, InitSuccess) {
    // NNCli_Init should properly initialize the CLI system
    NNCli_Err_t ret = NNCli_Init();
    EXPECT_EQ(ret, NN_CLI__SUCCESS);
}

TEST_F(NNCliTest, RunValidCommand) {
    // Test NNCli_Run using a valid command
    NNCli_RegisterCommand("run", SampleCmd);
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
} // namespace

class NNCliTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        s_async = {0};
        s_command_list = {0};
    }
};

namespace testing
{
    TEST_F(NNCliTest, RegisterCommand_Success)
    {
        constexpr NNCli_Command_t cmd = {
            .m_func = TestCmdFunc,
            .m_name = "test-cmd",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };

        ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);

        constexpr NNCli_Command_t no_option_cmd = {
            .m_func = TestCmdFunc,
            .m_name = "test-cmd",
            .m_options = nullptr,
            .m_help_msg = "test help msg",
        };
        ASSERT_EQ(NNCli_RegisterCommand(&no_option_cmd), NN_CLI__SUCCESS);
    }

    TEST_F(NNCliTest, RegisterCommand_InvalidArgs)
    {
        ASSERT_EQ(NNCli_RegisterCommand(nullptr), NN_CLI__INVALID_ARGS);

        constexpr NNCli_Command_t no_func_cmd = {
            .m_func = nullptr,
            .m_name = "test-cmd",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };
        ASSERT_EQ(NNCli_RegisterCommand(&no_func_cmd), NN_CLI__INVALID_ARGS);

        constexpr NNCli_Command_t no_name_cmd = {
            .m_func = TestCmdFunc,
            .m_name = "",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };
        ASSERT_EQ(NNCli_RegisterCommand(&no_name_cmd), NN_CLI__INVALID_ARGS);
    }

    TEST_F(NNCliTest, RegisterCommand_upperLimit)
    {
        constexpr NNCli_Command_t cmd = {
            .m_func = TestCmdFunc,
            .m_name = "test-cmd",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };

        for (int i = 0; i < NN_CLI__MAX_COMMAND_NUM; i++)
        {
            ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);
        }

        ASSERT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__EXCEED_CAPACITY);
    }

    TEST_F(NNCliTest, NNCliTest_RegisterCommand_InvalidArgs)
    {
        ASSERT_EQ(NNCli_RegisterCommand(nullptr), NN_CLI__INVALID_ARGS);

        constexpr NNCli_Command_t no_func_cmd = {
            .m_func = nullptr,
            .m_name = "test-cmd",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };
        ASSERT_EQ(NNCli_RegisterCommand(&no_func_cmd), NN_CLI__INVALID_ARGS);

        constexpr NNCli_Command_t no_name_cmd = {
            .m_func = TestCmdFunc,
            .m_name = "",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };
        ASSERT_EQ(NNCli_RegisterCommand(&no_name_cmd), NN_CLI__INVALID_ARGS);
    }

    TEST_F(NNCliTest, Init_Success)
    {
        char filename[] = "/tmp/nncli_test_history_XXXXXX";
        GenerateDummyHistoryFile(filename);
        const NNCli_Option_t option = {
            .m_enable_multi_line = true,
            .m_show_key_codes = false,
            .m_async = {
                .m_enabled = false,
                .m_timeout = {
                    .tv_sec = 0,
                    .tv_usec = 0},
            },
            .m_history_filename = filename,
        };

        ASSERT_EQ(NNCli_Init(&option), NN_CLI__SUCCESS);
        free(s_history_filename);
    }

    TEST_F(NNCliTest, Init_InvalidArgs)
    {
        ASSERT_EQ(NNCli_Init(nullptr), NN_CLI__INVALID_ARGS);

        const NNCli_Option_t option = {
            .m_enable_multi_line = true,
            .m_show_key_codes = false,
            .m_async = {
                .m_enabled = false,
                .m_timeout = {
                    .tv_sec = 0,
                    .tv_usec = 0},
            },
            .m_history_filename = ""};
        ASSERT_EQ(NNCli_Init(&option), NN_CLI__INVALID_ARGS);
    }

    TEST_F(NNCliTest, Run_Success)
    {

        constexpr NNCli_Command_t cmd = {
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
            .m_async = {
                .m_enabled = false,
                .m_timeout = {
                    .tv_sec = 0,
                    .tv_usec = 0},
            },
            .m_history_filename = filename,
        };

        ASSERT_EQ(NNCli_Init(&option), NN_CLI__SUCCESS);

        DummyKeyboardInput("help\n");
        ASSERT_EQ(NNCli_Run(), NN_CLI__SUCCESS);
    }
} // namespace testing
