#ifndef _EVMCONT_EVM_ACCOUNT_
#define _EVMCONT_EVM_ACCOUNT_

#include "../pchheader.hpp"

namespace evm
{
    struct evm_account
    {
        evmc::uint256be balance = {};
        std::vector<uint8_t> code;
        std::map<evmc::bytes32, evmc::bytes32> storage;

        evmc::bytes32 code_hash() const
        {
            // Extremely dumb "hash" function.
            evmc::bytes32 ret{};
            for (std::vector<uint8_t>::size_type i = 0; i != code.size(); i++)
            {
                auto v = code[i];
                ret.bytes[v % sizeof(ret.bytes)] ^= v;
            }
            return ret;
        }
    };
}

#endif