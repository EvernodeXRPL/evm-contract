#include "pchheader.hpp"
#include "evm/evm.hpp"
#include "sqlite.hpp"
#include "util.hpp"
#include "hotpocket_contract.h"

constexpr const char *DBNAME = "evm.db";
constexpr size_t HEX_ADDR_SIZE = 40;

int process_user_message(sqlite3 *db, const bool readonly, const struct hp_user *user, const char *buf, const uint32_t len);
int exec_contract();
int exec_test();

int main()
{
    return exec_contract();
    // return exec_test();
}

int exec_contract()
{
    if (hp_init_contract() == -1)
        return 1;

    const struct hp_contract_context *ctx = hp_get_context();

    sqlite3 *db = NULL;

    // Create the sqlite db if not exists.
    if (!util::file_exists(DBNAME) && !ctx->readonly)
    {
        sql::open_db(DBNAME, &db, true, false);
        sql::initialize_db(db);
        sql::close_db(&db);
    }

    // Read and process all user inputs from the mmap.
    const void *input_mmap = hp_init_user_input_mmap();

    // Iterate through all users.
    for (int u = 0; u < ctx->users.count; u++)
    {
        const struct hp_user *user = &ctx->users.list[u];

        // Iterate through all inputs from this user.
        for (int i = 0; i < user->inputs.count; i++)
        {
            const struct hp_user_input input = user->inputs.list[i];

            // Instead of mmap, we can also read the inputs from 'ctx->users.in_fd' using file I/O.
            // However, using mmap is recommended because user inputs already reside in memory.
            const char *buf = (char *)input_mmap + input.offset;

            // Lazy-open the db if user inputs area available.
            if (db == NULL)
                sql::open_db(DBNAME, &db, !ctx->readonly, false);

            process_user_message(db, ctx->readonly, user, buf, input.size);
        }
    }

    sql::close_db(&db);

    hp_deinit_user_input_mmap();
    hp_deinit_contract();
    return 0;
}

int process_user_message(sqlite3 *db, const bool readonly, const struct hp_user *user, const char *buf, const uint32_t len)
{
    if (len <= 1)
        return 0;

    // Input message formats:
    // Deploy: d<hex addr><hex bytecode>
    // Call: c<hex addr><hex input>

    // Output message format:
    // Deploy result: d<deployed hex address>
    // Call result: c<output hex>
    // Error: e<reason>

    if (strcmp(buf, "stats") == 0)
    {
        uint64_t acc_count = 0;
        uint64_t acc_storage_count = 0;
        if (evm::stats(db, acc_count, acc_storage_count) == 0)
        {
            const std::string output = "s," + std::to_string(acc_count) + "," + std::to_string(acc_storage_count);
            hp_write_user_msg(user, output.data(), output.size());
        }
        else
        {
            hp_write_user_msg(user, "estats_error", 12);
        }
    }
    else if (buf[0] == 'd' && len > (HEX_ADDR_SIZE + 1) && !readonly) // Deploy
    {
        std::string_view addr_hex(buf + 1, HEX_ADDR_SIZE);
        std::string_view code_hex(buf + 1 + HEX_ADDR_SIZE, (len - HEX_ADDR_SIZE - 1));
        const int ret = evm::deploy(db, addr_hex, code_hex);
        if (ret == 0)
        {
            std::string return_addr = "0x" + util::bin2hex(util::hex2bin(addr_hex));
            struct iovec vec[2] = {{(void *)"d", 1}, {(void *)return_addr.data(), return_addr.size()}};
            hp_writev_user_msg(user, vec, 2);
        }
        else
        {
            const std::string error = (ret == -1) ? "eaccount_exists" : "edeploy_error";
            std::cout << error << std::endl;
            hp_write_user_msg(user, error.data(), error.size());
        }
    }
    else if (buf[0] == 'c' && len > (HEX_ADDR_SIZE + 1) && !readonly) // Call
    {
        std::string_view addr_hex(buf + 1, HEX_ADDR_SIZE);
        std::string_view input_hex(buf + 1 + HEX_ADDR_SIZE, (len - HEX_ADDR_SIZE - 1));
        std::string output_hex;
        const int ret = evm::call(db, addr_hex, input_hex, output_hex);
        if (ret == 0)
        {
            struct iovec vec[2] = {{(void *)"c", 1}, {(void *)output_hex.data(), output_hex.size()}};
            hp_writev_user_msg(user, vec, 2);
        }
        else
        {
            const std::string error = (ret == -1) ? "eno_account" : "ecall_error";
            std::cout << error << std::endl;
            hp_write_user_msg(user, error.data(), error.size());
        }
    }
    else
    {

        const std::string error = "emsg_error";
        hp_write_user_msg(user, error.data(), error.size());
    }

    return 0;
}

int exec_test()
{
    sqlite3 *db = NULL;

    if (!util::file_exists(DBNAME))
    {
        sql::open_db(DBNAME, &db, true, false);
        sql::initialize_db(db);
        sql::close_db(&db);
    }

    sql::open_db(DBNAME, &db, true, false);

    // Deploy
    std::string bytecode = "608060405234801561001057600080fd5b506026600081905550602760018190555060b78061002f6000396000f3fe6080604052348015600f57600080fd5b506004361060285760003560e01c80636d4ce63c14602d575b600080fd5b60336047565b604051603e9190605e565b60405180910390f35b6000600154905090565b6058816077565b82525050565b6000602082019050607160008301846051565b92915050565b600081905091905056fea2646970667358221220acf0eaae688501b272a068d773d91abf5acc2cbf04e4b4c1077d5a14434c9b2764736f6c63430008070033";
    evm::deploy(db, "00B54E93EE2EBA3086A55F4249873E291D1AB06C", bytecode);

    // Call 'get' method of contract
    std::string output;
    evm::call(db, "00B54E93EE2EBA3086A55F4249873E291D1AB06C", "6d4ce63c", output);

    sql::close_db(&db);

    return 0;
}