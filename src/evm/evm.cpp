
#include "../pchheader.hpp"
#include "evm_host.h"

namespace evm
{
    int execute(const std::string &code_buf, const std::string &input_buf)
    {
        const uint8_t *code = (uint8_t *)code_buf.data();
        const size_t code_size = code_buf.size();
        const uint8_t *input = (uint8_t *)input_buf.data();
        const size_t input_size = input_buf.size();

        const evmc_uint256be value = {{0}};
        const evmc_address addr = {{0, 1, 2}};
        const int64_t gas = 200000;

        struct evmc_message msg;
        memset(&msg, 0, sizeof(msg));
        msg.kind = EVMC_CALL;
        msg.sender = addr;
        msg.recipient = addr;
        msg.value = value;
        msg.input_data = input;
        msg.input_size = input_size;
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
        struct evmc_host_context *host_ctx = create_host_context(tx_context);

        struct evmc_result result = evmc_execute(vm, host, host_ctx, EVMC_LONDON, &msg, code, code_size);
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
            printf("  Gas used: %" PRId64 "\n", msg.gas - result.gas_left);
            printf("  Gas left: %" PRId64 "\n", result.gas_left);
            printf("  Output size: %zd\n", result.output_size);
            printf("  Output: ");
            size_t i = 0;
            for (i = 0; i < result.output_size; i++)
                printf("%02x", result.output_data[i]);
            printf("\n");
            const evmc_bytes32 storage_key = {{0}};
            evmc_bytes32 storage_value = host->get_storage(host_ctx, &msg.recipient, &storage_key);
            printf("  Storage at 0x00..00: ");
            for (i = 0; i < sizeof(storage_value.bytes) / sizeof(storage_value.bytes[0]); i++)
                printf("%02x", storage_value.bytes[i]);
            printf("\n");
        }
        evmc_release_result(&result);
        destroy_host_context(host_ctx);
        evmc_destroy(vm);
        return exit_code;
    }
}