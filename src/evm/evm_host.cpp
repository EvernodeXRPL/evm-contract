#include "../pchheader.hpp"
#include "evm_host.hpp"
#include "evm_account.hpp"
#include "sqlite.hpp"

using namespace evmc::literals;

#define BINSTR(addr) std::string_view((char *)addr.bytes, sizeof(addr.bytes))

namespace evm
{
    class evm_host : public evmc::Host
    {
        evmc_tx_context tx_context{};
        sqlite3 &db;

    public:
        evm_host(sqlite3 &db, evmc_tx_context &_tx_context) noexcept : tx_context{_tx_context}, db{db} {}

        bool account_exists(const evmc::address &addr) const noexcept final
        {
            return sql::account_exists(&db, BINSTR(addr)) == 1;
        }

        evmc::bytes32 get_storage(const evmc::address &addr,
                                  const evmc::bytes32 &key) const noexcept final
        {
            const auto account_iter = accounts.find(addr);
            if (account_iter == accounts.end())
                return {};

            const auto storage_iter = account_iter->second.storage.find(key);
            if (storage_iter != account_iter->second.storage.end())
            {
                return storage_iter->second;
            }
            return {};
        }

        evmc_storage_status set_storage(const evmc::address &addr,
                                        const evmc::bytes32 &key,
                                        const evmc::bytes32 &value) noexcept final
        {
            auto &account = accounts[addr];
            auto prev_value = account.storage[key];
            account.storage[key] = value;

            return (prev_value == value) ? EVMC_STORAGE_UNCHANGED : EVMC_STORAGE_MODIFIED;
        }

        evmc::uint256be get_balance(const evmc::address &addr) const noexcept final
        {
            auto it = accounts.find(addr);
            if (it != accounts.end())
                return it->second.balance;
            return {};
        }

        size_t get_code_size(const evmc::address &addr) const noexcept final
        {
            auto it = accounts.find(addr);
            if (it != accounts.end())
                return it->second.code.size();
            return 0;
        }

        evmc::bytes32 get_code_hash(const evmc::address &addr) const noexcept final
        {
            auto it = accounts.find(addr);
            if (it != accounts.end())
                return it->second.code_hash();
            return {};
        }

        size_t copy_code(const evmc::address &addr,
                         size_t code_offset,
                         uint8_t *buffer_data,
                         size_t buffer_size) const noexcept final
        {
            const auto it = accounts.find(addr);
            if (it == accounts.end())
                return 0;

            const auto &code = it->second.code;

            if (code_offset >= code.size())
                return 0;

            const auto n = std::min(buffer_size, code.size() - code_offset);

            if (n > 0)
                std::copy_n(&code[code_offset], n, buffer_data);
            return n;
        }

        void selfdestruct(const evmc::address &addr, const evmc::address &beneficiary) noexcept final
        {
            (void)addr;
            (void)beneficiary;
        }

        evmc::result call(const evmc_message &msg) noexcept final
        {
            return {EVMC_REVERT, msg.gas, msg.input_data, msg.input_size};
        }

        evmc_tx_context get_tx_context() const noexcept final { return tx_context; }

        evmc::bytes32 get_block_hash(int64_t number) const noexcept final
        {
            const int64_t current_block_number = get_tx_context().block_number;

            return (number < current_block_number && number >= current_block_number - 256) ? 0xb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5f_bytes32 : 0_bytes32;
        }

        void emit_log(const evmc::address &addr,
                      const uint8_t *data,
                      size_t data_size,
                      const evmc::bytes32 topics[],
                      size_t topics_count) noexcept final
        {
            (void)addr;
            (void)data;
            (void)data_size;
            (void)topics;
            (void)topics_count;
        }

        evmc_access_status access_account(const evmc::address &addr) noexcept final
        {
            (void)addr;
            return EVMC_ACCESS_COLD;
        }

        evmc_access_status access_storage(const evmc::address &addr,
                                          const evmc::bytes32 &key) noexcept final
        {
            (void)addr;
            (void)key;
            return EVMC_ACCESS_COLD;
        }
    };

    evmc_host_context *create_host_context(sqlite3 &db, evmc_tx_context tx_context)
    {
        auto host = new evm_host{db, tx_context};
        return host->to_context();
    }

    void destroy_host_context(evmc_host_context *context)
    {
        delete evmc::Host::from_context<evm_host>(context);
    }
}