#include "../pchheader.hpp"
#include "../sqlite.hpp"
#include "../util.hpp"
#include "evm_host.hpp"

using namespace evmc::literals;

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
            evmc::bytes32 value = {};
            if (sql::get_account_storage(&db, BINSTR(addr), BINSTR(key), value.bytes) == 1)
                return value;

            return {};
        }

        evmc_storage_status set_storage(const evmc::address &addr,
                                        const evmc::bytes32 &key,
                                        const evmc::bytes32 &value) noexcept final
        {
            const int acc = sql::account_exists(&db, BINSTR(addr));
            if (acc == -1)
                return evmc_storage_status::EVMC_STORAGE_UNCHANGED;

            // Create account if not exists.
            if (acc == 0)
            {
                evmc::uint256be balance = FULL_BALANCE;
                std::cout << "INSERT " << BINHEX(addr) << "\n";
                if (sql::insert_account(&db, BINSTR(addr), BINSTR(balance), {}) == -1)
                    return evmc_storage_status::EVMC_STORAGE_UNCHANGED;
            }

            if (sql::account_storage_exists(&db, BINSTR(addr), BINSTR(key)) != 1)
            {
                if (sql::insert_account_storage(&db, BINSTR(addr), BINSTR(key), BINSTR(value)) != -1)
                    return evmc_storage_status::EVMC_STORAGE_MODIFIED;
            }
            else
            {
                if (sql::update_account_storage(&db, BINSTR(addr), BINSTR(key), BINSTR(value)) != -1)
                    return evmc_storage_status::EVMC_STORAGE_MODIFIED;
            }

            return evmc_storage_status::EVMC_STORAGE_UNCHANGED;
        }

        evmc::uint256be get_balance(const evmc::address &addr) const noexcept final
        {
            evmc::uint256be balance = {};
            if (sql::get_account_balance(&db, BINSTR(addr), balance.bytes) == 1)
                return balance;
            return {};
        }

        size_t get_code_size(const evmc::address &addr) const noexcept final
        {
            size_t size = 0;
            sql::get_account_code_size(&db, BINSTR(addr), size);
            return size;
        }

        evmc::bytes32 get_code_hash(const evmc::address &addr) const noexcept final
        {
            std::string code;
            if (sql::get_account_code(&db, BINSTR(addr), code) == 1)
            {
                // Extremely dumb "hash" function.
                evmc::bytes32 ret{};
                for (char v : code)
                    ret.bytes[v % sizeof(ret.bytes)] ^= v;
                return ret;
            }

            return {};
        }

        size_t copy_code(const evmc::address &addr,
                         size_t code_offset,
                         uint8_t *buffer_data,
                         size_t buffer_size) const noexcept final
        {
            std::string code;
            if (sql::get_account_code(&db, BINSTR(addr), code) == 1)
            {
                if (code_offset >= code.size())
                    return 0;

                const size_t n = std::min(buffer_size, code.size() - code_offset);

                if (n > 0)
                    std::copy_n(&code[code_offset], n, buffer_data);
                return n;
            }

            return 0;
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