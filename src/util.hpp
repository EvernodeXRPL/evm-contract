#ifndef _EVMCONT_UTIL_
#define _EVMCONT_UTIL_

#include "pchheader.hpp"

#define LARGE_BALANCE                                   \
    evmc::uint256be                                     \
    {                                                   \
        {                                               \
            255, 255, 255, 255, 255, 255, 255, 255,     \
                255, 255, 255, 255, 255, 255, 255, 255, \
                255, 255, 255, 255, 255, 255, 255, 255, \
                255, 255, 255, 255, 255, 255, 255, 255  \
        }                                               \
    }

#define BINSTR(bin) std::string_view((char *)bin.bytes, sizeof(bin.bytes))
#define BINHEX(bin) util::bin2hex(BINSTR(bin))

namespace util
{
    std::string hex2bin(std::string_view hex);
    std::string bin2hex(std::string_view s);
    const std::string read_file(std::string_view path);
}

#endif