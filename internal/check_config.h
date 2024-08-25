#pragma once

#include "nn_cli_config.h"

#ifndef NN_CLI__MAX_COMMAND_NUM
#define NN_CLI__MAX_COMMAND_NUM 200
#endif

#ifndef NNCli_LogInfo
#define NNCli_LogInfo(fmt, ...) printf("[NNCli][INFO]" fmt "\n", ##__VA_ARGS__)
#endif

#ifndef NNCli_LogWarn
#define NNCli_LogWarn(fmt, ...) printf("[NNCli][WARN]" fmt "\n", ##__VA_ARGS__)
#endif

#ifndef NNCli_LogError
#define NNCli_LogError(fmt, ...) fprintf(stderr, "[NNCli][ERROR]" fmt "\n", ##__VA_ARGS__)
#endif

#ifndef NNCli_Assert
#include <assert.h>
#define NNCli_Assert(condition) assert(condition)
#endif
