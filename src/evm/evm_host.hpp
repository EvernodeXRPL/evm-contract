#ifndef _EVMCONT_EVM_HOST_
#define _EVMCONT_EVM_HOST_

#include "../pchheader.hpp"

namespace evm
{
    evmc_host_context *create_host_context(sqlite3 *db, evmc_tx_context tx_context);
    void destroy_host_context(evmc_host_context *context);
}

#endif