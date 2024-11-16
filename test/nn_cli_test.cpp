#include <gtest/gtest.h>

#include "nn_cli.h"

namespace
{
    int TestCmdFunc(int argc, char **argv)
    {
        // Nothing to do
        return 0;
    }
}

namespace testing
{
    TEST(NNCliTest, RegisterCommand)
    {
        const NNCli_Command_t cmd = {
            /* .m_func = */ TestCmdFunc,
            /* .m_name = */ "test-cmd",
            /* .m_help_msg = */ "test help msg",
            /* .m_options = */ "",
        };

        EXPECT_EQ(NNCli_RegisterCommand(&cmd), NN_CLI__SUCCESS);

        EXPECT_EQ(NNCli_RegisterCommand(nullptr), NN_CLI__INVALID_ARGS);
    }

} // testing
