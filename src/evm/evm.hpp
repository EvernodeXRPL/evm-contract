#ifndef _EVMCONT_EVM_
#define _EVMCONT_EVM_

#include "../pchheader.hpp"

namespace evm
{
    int execute(sqlite3 &db, const std::string &codebuf, const std::string &inputbuf);
}

#endif