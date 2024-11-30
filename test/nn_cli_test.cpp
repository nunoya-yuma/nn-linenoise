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

        EXPECT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);

        constexpr NNCli_Command_t no_option_cmd = {
            .m_func = TestCmdFunc,
            .m_name = "test-cmd",
            .m_options = nullptr,
            .m_help_msg = "test help msg",
        };
        EXPECT_EQ(NNCli_RegisterCommand(&no_option_cmd), NN_CLI__SUCCESS);
    }

    TEST_F(NNCliTest, RegisterCommand_InvalidArgs)
    {
        EXPECT_EQ(NNCli_RegisterCommand(nullptr), NN_CLI__INVALID_ARGS);

        constexpr NNCli_Command_t no_func_cmd = {
            .m_func = nullptr,
            .m_name = "test-cmd",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };
        EXPECT_EQ(NNCli_RegisterCommand(&no_func_cmd), NN_CLI__INVALID_ARGS);

        constexpr NNCli_Command_t no_name_cmd = {
            .m_func = TestCmdFunc,
            .m_name = "",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };
        EXPECT_EQ(NNCli_RegisterCommand(&no_name_cmd), NN_CLI__INVALID_ARGS);
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
            EXPECT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);
        }

        EXPECT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__EXCEED_CAPACITY);
    }

    TEST_F(NNCliTest, NNCliTest_RegisterCommand_InvalidArgs)
    {
        EXPECT_EQ(NNCli_RegisterCommand(nullptr), NN_CLI__INVALID_ARGS);

        constexpr NNCli_Command_t no_func_cmd = {
            .m_func = nullptr,
            .m_name = "test-cmd",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };
        EXPECT_EQ(NNCli_RegisterCommand(&no_func_cmd), NN_CLI__INVALID_ARGS);

        constexpr NNCli_Command_t no_name_cmd = {
            .m_func = TestCmdFunc,
            .m_name = "",
            .m_options = "on/off",
            .m_help_msg = "test help msg",
        };
        EXPECT_EQ(NNCli_RegisterCommand(&no_name_cmd), NN_CLI__INVALID_ARGS);
    }

    TEST_F(NNCliTest, Init_Success)
    {
        const NNCli_Option_t option = {
            .m_enable_multi_line = true,
            .m_show_key_codes = false,
            .m_async = {
                .m_enabled = false,
                .m_timeout = {
                    .tv_sec = 0,
                    .tv_usec = 0},
            },
            .m_history_filename = "test_history.txt"};

        EXPECT_EQ(NNCli_Init(&option), NN_CLI__SUCCESS);
        free(s_history_filename);
    }

    TEST_F(NNCliTest, Init_InvalidArgs)
    {
        EXPECT_EQ(NNCli_Init(nullptr), NN_CLI__INVALID_ARGS);

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
        EXPECT_EQ(NNCli_Init(&option), NN_CLI__INVALID_ARGS);
    }
} // namespace testing
