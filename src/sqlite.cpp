#include "sqlite.hpp"

namespace sql
{
    constexpr const char *COLUMN_DATA_TYPES[]{"INT", "TEXT", "BLOB"};
    constexpr const char *CREATE_TABLE = "CREATE TABLE IF NOT EXISTS ";
    constexpr const char *CREATE_INDEX = "CREATE INDEX ";
    constexpr const char *CREATE_UNIQUE_INDEX = "CREATE UNIQUE INDEX ";
    constexpr const char *JOURNAL_MODE_OFF = "PRAGMA journal_mode=OFF";
    constexpr const char *BEGIN_TRANSACTION = "BEGIN TRANSACTION;";
    constexpr const char *COMMIT_TRANSACTION = "COMMIT;";
    constexpr const char *ROLLBACK_TRANSACTION = "ROLLBACK;";
    constexpr const char *INSERT_INTO = "INSERT INTO ";
    constexpr const char *PRIMARY_KEY = "PRIMARY KEY";
    constexpr const char *NOT_NULL = "NOT NULL";
    constexpr const char *VALUES = "VALUES";

    constexpr const char *ACCOUNTS_TABLE = "acc";
    constexpr const char *ACCOUNTS_STORAGE_TABLE = "accstorage";

    constexpr const char *GET_BALANCE = "SELECT balance FROM acc WHERE addr=?";
    constexpr const char *GET_STORAGE = "SELECT value FROM accstorage WHERE addr=? AND key=?";
    constexpr const char *INSERT_INTO_ACCOUNTS = "INSERT INTO acc(addr, balance, code) VALUES(?,?,?)";
    constexpr const char *UPDATE_ACCOUNTS_CODE = "UPDATE acc SET code=? WHERE addr=?";
    constexpr const char *INSERT_INTO_ACCOUNTS_STORAGE = "INSERT INTO accstorage(addr, key, value) VALUES(?,?,?)";
    constexpr const char *UPDATE_ACCOUNTS_STORAGE = "UPDATE accstorage SET value=? WHERE addr=? AND key=?";
    constexpr const char *DELETE_FROM_ACCOUNTS_STORAGE = "DELETE FROM accstorage WHERE addr=? AND key=?";

#define BIND_BLOB_N(idx, field, n) ((n > 0 && field.size() == n) ? (sqlite3_bind_blob(stmt, idx, field.data(), field.size(), SQLITE_STATIC) == SQLITE_OK) : (sqlite3_bind_null(stmt, idx) == SQLITE_OK))
#define BIND_BLOB32(idx, field) BIND_BLOB_N(idx, field, 32)
#define BIND_BLOB20(idx, field) BIND_BLOB_N(idx, field, 20)

#define GET_BLOB_N(idx, n) std::string((char *)sqlite3_column_blob(stmt, idx), n)
#define GET_BLOB32(idx) GET_BLOB_N(idx, 32)
#define GET_BLOB20(idx) GET_BLOB_N(idx, 20)

    /**
     * Opens a connection to a given databse and give the db pointer.
     * @param db_name Database name to be connected.
     * @param db Pointer to the db pointer which is to be connected and pointed.
     * @param writable Whether the database must be opened in a writable mode or not.
     * @param journal Whether to enable db journaling or not.
     * @returns returns 0 on success, or -1 on error.
    */
    int open_db(std::string_view db_name, sqlite3 **db, const bool writable, const bool journal)
    {
        int ret;
        const int flags = writable ? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) : SQLITE_OPEN_READONLY;
        if ((ret = sqlite3_open_v2(db_name.data(), db, flags, 0)) != SQLITE_OK)
        {
            std::cerr << ret << ": Sqlite error when opening database " << db_name << "\n";
            *db = NULL;
            return -1;
        }

        // We can turn off journaling for the db if we don't need transaction support.
        // Journaling mode can introduce lot of extra underyling file system operations which may cause
        // lot of overhead if used on a low-performance filesystem like hpfs.
        if (writable && !journal && exec_sql(*db, JOURNAL_MODE_OFF) == -1)
            return -1;

        return 0;
    }

    /**
     * Executes given sql query.
     * @param db Pointer to the db.
     * @param sql Sql query to be executed.
     * @param callback Callback funcion which is called for each result row.
     * @param callback_first_arg First data argumat to be parced to the callback (void pointer).
     * @returns returns 0 on success, or -1 on error.
    */
    int exec_sql(sqlite3 *db, std::string_view sql, int (*callback)(void *, int, char **, char **), void *callback_first_arg)
    {
        char *err_msg;
        if (sqlite3_exec(db, sql.data(), callback, (callback != NULL ? (void *)callback_first_arg : NULL), &err_msg) != SQLITE_OK)
        {
            std::cerr << "SQL error occured: " << err_msg << "\n";
            sqlite3_free(err_msg);
            return -1;
        }
        return 0;
    }

    int begin_transaction(sqlite3 *db)
    {
        return sql::exec_sql(db, BEGIN_TRANSACTION);
    }

    int commit_transaction(sqlite3 *db)
    {
        return sql::exec_sql(db, COMMIT_TRANSACTION);
    }

    int rollback_transaction(sqlite3 *db)
    {
        return sql::exec_sql(db, ROLLBACK_TRANSACTION);
    }

    /**
     * Create a table with given table info.
     * @param db Pointer to the db.
     * @param table_name Table name to be created.
     * @param column_info Column info of the table.
     * @returns returns 0 on success, or -1 on error.
    */
    int create_table(sqlite3 *db, std::string_view table_name, const std::vector<table_column_info> &column_info)
    {
        std::string sql;
        sql.append(CREATE_TABLE).append(table_name).append(" (");

        for (auto itr = column_info.begin(); itr != column_info.end(); ++itr)
        {
            sql.append(itr->name);
            sql.append(" ");
            sql.append(COLUMN_DATA_TYPES[itr->column_type]);

            if (itr->is_key)
            {
                sql.append(" ");
                sql.append(PRIMARY_KEY);
            }

            if (!itr->is_null)
            {
                sql.append(" ");
                sql.append(NOT_NULL);
            }

            if (itr != column_info.end() - 1)
                sql.append(",");
        }
        sql.append(")");

        const int ret = exec_sql(db, sql);
        if (ret == -1)
            std::cerr << "Error when creating sqlite table " << table_name << "\n";

        return ret;
    }

    int create_index(sqlite3 *db, std::string_view table_name, std::string_view column_names, const bool is_unique)
    {
        std::string index_name = std::string("idx_").append(table_name).append("_").append(column_names);
        std::replace(index_name.begin(), index_name.end(), ',', '_');

        std::string sql;
        sql.append(is_unique ? CREATE_UNIQUE_INDEX : CREATE_INDEX)
            .append(index_name)
            .append(" ON ")
            .append(table_name)
            .append("(")
            .append(column_names)
            .append(")");

        const int ret = exec_sql(db, sql);
        if (ret == -1)
            std::cerr << "Error when creating sqlite index '" << index_name << "' in table " << table_name << "\n";

        return ret;
    }

    /**
     * Closes a connection to a given databse.
     * @param db Pointer to the db.
     * @returns returns 0 on success, or -1 on error.
    */
    int close_db(sqlite3 **db)
    {
        if (*db == NULL)
            return 0;

        if (sqlite3_close(*db) != SQLITE_OK)
        {
            std::cerr << "Can't close database: " << sqlite3_errmsg(*db) << "\n";
            return -1;
        }

        *db = NULL;
        return 0;
    }

    int initialize_db(sqlite3 *db)
    {
        const std::vector<table_column_info> accounts_columns{
            table_column_info("addr", COLUMN_DATA_TYPE::BLOB, true, false),
            table_column_info("balance", COLUMN_DATA_TYPE::BLOB, false, false),
            table_column_info("code", COLUMN_DATA_TYPE::BLOB, false, false)};

        if (create_table(db, ACCOUNTS_TABLE, accounts_columns) == -1 ||
            create_index(db, ACCOUNTS_TABLE, "addr", true) == -1)
            return -1;

        const std::vector<table_column_info> accounts_storage_columns{
            table_column_info("addr", COLUMN_DATA_TYPE::BLOB, true, false),
            table_column_info("key", COLUMN_DATA_TYPE::BLOB, true, false),
            table_column_info("value", COLUMN_DATA_TYPE::BLOB, false, false)};

        if (create_table(db, ACCOUNTS_STORAGE_TABLE, accounts_storage_columns) == -1 ||
            create_index(db, ACCOUNTS_STORAGE_TABLE, "addr,key", true) == -1)
            return -1;

        return 0;
    }

    /**
     * @return 1 if found. 0 if not found. -1 if error.
     */
    int account_exists(sqlite3 *db, std::string_view addr)
    {
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, GET_BALANCE, -1, &stmt, 0) == SQLITE_OK && stmt != NULL &&
            BIND_BLOB20(1, addr) == SQLITE_OK)
        {
            const int result = sqlite3_step(stmt);
            if (result == SQLITE_ROW)
            {
                sqlite3_finalize(stmt);
                return 1; // Account found.
            }
            else if (result == SQLITE_DONE)
            {
                sqlite3_finalize(stmt);
                return 0; // Account not found.
            }
        }

        std::cerr << "Error in sql account_exists. " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return -1;
    }

    /**
     * @return 1 if found. 0 if not found. -1 if error.
     */
    int get_account_balance(sqlite3 *db, std::string_view addr, std::string &balance)
    {
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, GET_BALANCE, -1, &stmt, 0) == SQLITE_OK && stmt != NULL &&
            BIND_BLOB20(1, addr) == SQLITE_OK)
        {
            const int result = sqlite3_step(stmt);
            if (result == SQLITE_ROW)
            {
                balance = GET_BLOB32(0);
                sqlite3_finalize(stmt);
                return 1; // Account found.
            }
            else if (result == SQLITE_DONE)
            {
                sqlite3_finalize(stmt);
                return 0; // Account not found.
            }
        }

        std::cerr << "Error in sql get_account_balance. " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return -1;
    }

    /**
     * @return 1 if found. 0 if not found. -1 if error.
     */
    int get_account_storage(sqlite3 *db, std::string_view addr, std::string_view key, std::string &value)
    {
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, GET_STORAGE, -1, &stmt, 0) == SQLITE_OK && stmt != NULL &&
            BIND_BLOB20(1, addr) == SQLITE_OK &&
            BIND_BLOB32(2, key) == SQLITE_OK)
        {
            const int result = sqlite3_step(stmt);
            if (result == SQLITE_ROW)
            {
                value = GET_BLOB32(0);
                sqlite3_finalize(stmt);
                return 1; // storage found.
            }
            else if (result == SQLITE_DONE)
            {
                sqlite3_finalize(stmt);
                return 0; // storage not found.
            }
        }

        std::cerr << "Error in sql get_account_balance. " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return -1;
    }

    int insert_account(sqlite3 *db, std::string_view addr, std::string_view balance, std::string_view code)
    {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, INSERT_INTO_ACCOUNTS, -1, &stmt, 0) == SQLITE_OK && stmt != NULL &&
            BIND_BLOB20(1, addr) &&
            BIND_BLOB32(2, balance) &&
            BIND_BLOB_N(3, code, code.size()) &&
            sqlite3_step(stmt) == SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            return 0;
        }

        std::cerr << errno << ": Error inserting account. " << sqlite3_errmsg(db) << "\n";
        return -1;
    }

    int insert_account_storage(sqlite3 *db, std::string_view addr, std::string_view key, std::string_view value)
    {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, INSERT_INTO_ACCOUNTS, -1, &stmt, 0) == SQLITE_OK && stmt != NULL &&
            BIND_BLOB20(1, addr) &&
            BIND_BLOB32(2, key) &&
            BIND_BLOB32(3, value) &&
            sqlite3_step(stmt) == SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            return 0;
        }

        std::cerr << errno << ": Error inserting account. " << sqlite3_errmsg(db) << "\n";
        return -1;
    }

    int update_account_code(sqlite3 *db, std::string_view addr, std::string_view code)
    {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, UPDATE_ACCOUNTS_CODE, -1, &stmt, 0) == SQLITE_OK && stmt != NULL &&
            BIND_BLOB_N(1, code, code.size()) &&
            BIND_BLOB20(2, addr) &&
            sqlite3_step(stmt) == SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            return 0;
        }

        std::cerr << errno << ": Error inserting account. " << sqlite3_errmsg(db) << "\n";
        return -1;
    }

    int update_account_storage(sqlite3 *db, std::string_view addr, std::string_view key, std::string_view value)
    {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, UPDATE_ACCOUNTS_STORAGE, -1, &stmt, 0) == SQLITE_OK && stmt != NULL &&
            BIND_BLOB32(1, value) &&
            BIND_BLOB20(2, addr) &&
            BIND_BLOB32(3, key) &&
            sqlite3_step(stmt) == SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            return 0;
        }

        std::cerr << errno << ": Error inserting account. " << sqlite3_errmsg(db) << "\n";
        return -1;
    }
}
