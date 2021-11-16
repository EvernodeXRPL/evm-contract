
#include "../pchheader.hpp"
#include "../util.hpp"
#include "../sqlite.hpp"
#include "evm_host.hpp"

namespace evm
{
    const evmc::address bin2addr(std::string_view bin);
    int execute(sqlite3 *db, std::string_view addr, std::string_view input, std::string_view code, std::string &output);

    int deploy(sqlite3 *db, std::string_view addr_hex, std::string_view code_hex)
    {
        // Create a new account and run deployment bytecode against it.
        // Then store the resulting bytecode in the account code.
        std::string output;
        std::string addr = util::hex2bin(addr_hex);
        std::string code = util::hex2bin(code_hex);
        evmc::uint256be balance = FULL_BALANCE;
        if (sql::insert_account(db, addr, BINSTR(balance), code) != -1 &&
            execute(db, addr, {}, code, output) == 0)
            return sql::update_account_code(db, addr, output);

        return -1;
    }

    int run(sqlite3 *db, std::string_view addr_hex, std::string_view input_hex, std::string &output)
    {
        // Retrieve the account's code and run the code.
        std::string code;
        std::string addr = util::hex2bin(addr_hex);
        std::string input = util::hex2bin(input_hex);
        if (sql::get_account_code(db, addr, code) == 1)
            return execute(db, addr, input, code, output);
        return -1;
    }

    const evmc::address bin2addr(std::string_view bin)
    {
        evmc::address addr = {};
        if (bin.size() == sizeof(addr.bytes))
            memcpy(addr.bytes, bin.data(), sizeof(addr.bytes));
        return addr;
    }

    int execute(sqlite3 *db, std::string_view addr, std::string_view input, std::string_view code, std::string &output)
    {
        evmc::address eaddr = bin2addr(addr);
        const evmc_uint256be value = {{0}};
        const int64_t gas = 200000;

        struct evmc_message msg;
        memset(&msg, 0, sizeof(msg));
        msg.kind = EVMC_CALL;
        msg.sender = {};
        msg.recipient = eaddr;
        msg.code_address = eaddr;
        msg.value = value;
        msg.input_data = input.size() ? (uint8_t *)input.data() : NULL;
        msg.input_size = input.size();
        msg.gas = gas;
        msg.depth = 0;
        msg.flags = 0;

        struct evmc_tx_context tx_context;
        memset(&tx_context, 0, sizeof(tx_context));
        tx_context.block_number = 42;
        tx_context.block_timestamp = 66;
        tx_context.block_gas_limit = gas * 2;

        // We create an EVMC-compatible vm object from this function provided by evmone library.
        struct evmc_vm *vm = evmc_create_evmone();

        const struct evmc_host_interface *host = &evmc::Host::get_interface();
        struct evmc_host_context *host_ctx = create_host_context(db, tx_context);

        struct evmc_result result = evmc_execute(vm, host, host_ctx, EVMC_LONDON, &msg,
                                                 (code.size() ? (uint8_t *)code.data() : NULL), code.size());
        printf("Execution result:\n");
        int exit_code = 0;
        if (result.status_code != EVMC_SUCCESS)
        {
            printf("  EVM execution failure: %d\n", result.status_code);
            printf("  Gas used: %" PRId64 "\n", msg.gas - result.gas_left);
            printf("  Gas left: %" PRId64 "\n", result.gas_left);
            printf("  Output size: %zd\n", result.output_size);
            exit_code = result.status_code;
        }
        else
        {
            // Success
            printf("  Gas used: %" PRId64 "\n", msg.gas - result.gas_left);
            printf("  Gas left: %" PRId64 "\n", result.gas_left);
            printf("  Output size: %zd\n", result.output_size);
            printf("  Output: ");
            size_t i = 0;
            for (i = 0; i < result.output_size; i++)
                printf("%02x", result.output_data[i]);
            printf("\n");

            output.resize(result.output_size);
            memcpy(output.data(), result.output_data, result.output_size);
        }
        evmc_release_result(&result);
        destroy_host_context(host_ctx);
        evmc_destroy(vm);
        return exit_code;
    }
}