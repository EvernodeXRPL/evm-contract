#ifndef _EVMCONT_UTIL_
#define _EVMCONT_UTIL_

#include "pchheader.hpp"

namespace util
{
    std::string hex2bin(const std::string &s);
    std::string bin2hex(const std::string &s);
    const std::string read_file(const std::string &path);
}

#endif