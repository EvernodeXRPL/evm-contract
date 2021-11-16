#include "pchheader.hpp"
#include "evm/evm.hpp"
#include "sqlite.hpp"
#include "util.hpp"

const char *dbname = "evm.db";

int main()
{
    const std::string code_buf = util::hex2bin("4360005543600052596000f3");
    const std::string input_buf = "Hello World!";

    sqlite3 *db = NULL;

    {
        std::ifstream infile(dbname);
        if (!infile.good())
        {
            sql::open_db(dbname, &db, true, false);
            sql::initialize_db(db);
            sql::close_db(&db);
        }
    }

    sql::open_db(dbname, &db, true, false);

    std::string bytecode = "608060405234801561001057600080fd5b506026600081905550602760018190555060b78061002f6000396000f3fe6080604052348015600f57600080fd5b506004361060285760003560e01c80636d4ce63c14602d575b600080fd5b60336047565b604051603e9190605e565b60405180910390f35b6000600154905090565b6058816077565b82525050565b6000602082019050607160008301846051565b92915050565b600081905091905056fea2646970667358221220acf0eaae688501b272a068d773d91abf5acc2cbf04e4b4c1077d5a14434c9b2764736f6c63430008070033";
    evm::deploy(db, "00B54E93EE2EBA3086A55F4249873E291D1AB06C", bytecode);

    std::string output;
    evm::run(db, "00B54E93EE2EBA3086A55F4249873E291D1AB06C", "6d4ce63c", output);

    sql::close_db(&db);

    return 0;
}