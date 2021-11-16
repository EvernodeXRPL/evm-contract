#ifndef _EVMCONT_EVM_
#define _EVMCONT_EVM_

#include "../pchheader.hpp"

namespace evm
{
    int deploy(sqlite3 *db, std::string_view addr_hex, std::string_view code_hex);
    int run(sqlite3 *db, std::string_view addr_hex, std::string_view input_hex, std::string &output);
}

#endif