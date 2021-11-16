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
    evm::execute(*db, code_buf, input_buf);
    sql::close_db(&db);

    return 0;
}