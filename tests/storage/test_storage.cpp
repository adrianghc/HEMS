/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * These are unit tests for the Data Storage Module.
 */

#include <iostream>
#include <functional>
#include <map>
#include <string>

#include <boost/filesystem.hpp>

#include "../test.hpp"
#include "../extras/dummy_logger.hpp"
#include "../extras/random_file_name.hpp"

#include "hems/modules/storage/storage.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

#include "sqlite/sqlite3.h"

namespace hems { namespace modules { namespace storage {

    std::map<std::string, bool> table_exists = {
        { "settings", false },
        { "settings_stations", false },
        { "appliances", false },
        { "auto_profiles", false },
        { "tasks", false },
        { "appliances_tasks", false },
        { "appliances_auto_profiles", false },
        { "energy_consumption", false },
        { "energy_production", false },
        { "weather", false }
    };

    int query_callback(void *unused, int num_columns, char **data, char **column_names) {
        try {
            std::string value(data[0]);
            if (!table_exists.at(value)) {
                table_exists[value] = true;
            }
        } catch (const std::out_of_range& e) { }
        return 0;
    }


    bool test_create_db() {
        logger::this_logger = new dummy_logger();

        char db_path[11];
        do {
            generate_random_file_name(db_path);
        } while (boost::filesystem::exists(db_path));

        /* Create a message queue for the Data Storage Module so that its constructor does not fail. */
        struct mq_attr attr = { 
            mq_flags    : 0,
            mq_maxmsg   : 10,
            mq_msgsize  : sizeof(messenger::msg_t),
            mq_curmsgs  : 0
        };
        mq_close(mq_open(
            messenger::mq_names.at(modules::type::STORAGE).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));
        mq_close(mq_open(
            messenger::mq_res_names.at(modules::type::STORAGE).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));

        try {
            hems_storage::this_instance = new hems_storage(true, db_path);
        } catch (int err) {
            std::cout <<
                "Constructing Data Storage Module threw an exception " + std::to_string(err) + ", test failed.\n";
            return false;
        }

        delete hems_storage::this_instance;
        delete logger::this_logger;


        /* BEGIN Check if the database tables were created successfully. */

        sqlite3* db_connection;

        if (sqlite3_open(db_path, &db_connection) != SQLITE_OK) {
            std::cout << "Could not open database, test failed.\n";
            return false;
        }

        std::string stmt = "SELECT name FROM sqlite_master WHERE type='table'";

        if (sqlite3_exec(db_connection, stmt.c_str(), &query_callback, nullptr, nullptr) == SQLITE_ABORT) {
            std::cout << "Callback function returned an error.\n";
            return false;
        }

        int count_fail = 0;
        for (const auto& pair : table_exists) {
            if (!pair.second) {
                ++count_fail;
                std::cout << "Table '" + pair.first + "' was not created.\n";
            }
        }

        /* END Check if the database tables were created successfully. */

        remove(db_path);
        mq_unlink(messenger::mq_names.at(modules::type::STORAGE).c_str());

        return (count_fail == 0);
    }

}}}


int main() {
    return run_tests({
        { "01 Storage: Database creation test", &hems::modules::storage::test_create_db }
    });
}
