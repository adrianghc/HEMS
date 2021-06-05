/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Data Storage Module.
 * This module is responsible for managing access to data storage for all other modules.
 * Whenever other modules need to read or write measurements or other data, they can issue messages
 * to the Data Storage Module.
 */

#include <mutex>

#include "hems/modules/storage/storage.h"
#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/common/exit.h"
#include "hems/messages/storage.h"

#include "sqlite/sqlite3.h"

namespace hems { namespace modules { namespace storage {

    using namespace hems::messages::storage;

    const messenger::msg_handler_map hems_storage::handler_map = {
        { messenger::special_subtype::SETTINGS_INIT, handler_wrapper_settings_init },
        { messenger::special_subtype::SETTINGS_CHECK, handler_wrapper_settings_check },
        { messenger::special_subtype::SETTINGS_COMMIT, handler_wrapper_settings_commit },
        { msg_type::MSG_SET_APPLIANCE, handler_wrapper_msg_set_appliance },
        { msg_type::MSG_SET_TASK, handler_wrapper_msg_set_task },
        { msg_type::MSG_SET_AUTO_PROFILE, handler_wrapper_msg_set_auto_profile },
        { msg_type::MSG_SET_ENERGY_CONSUMPTION, handler_wrapper_msg_set_energy_consumption },
        { msg_type::MSG_SET_ENERGY_PRODUCTION, handler_wrapper_msg_set_energy_production },
        { msg_type::MSG_SET_WEATHER, handler_wrapper_msg_set_weather },
        { msg_type::MSG_DEL_APPLIANCE, handler_wrapper_msg_del_appliance },
        { msg_type::MSG_DEL_TASK, handler_wrapper_msg_del_task },
        { msg_type::MSG_DEL_AUTO_PROFILE, handler_wrapper_msg_del_auto_profile },
        { msg_type::MSG_DEL_ENERGY_CONSUMPTION, handler_wrapper_msg_del_energy_consumption },
        { msg_type::MSG_DEL_ENERGY_PRODUCTION, handler_wrapper_msg_del_energy_production },
        { msg_type::MSG_DEL_WEATHER, handler_wrapper_msg_del_weather },
        { msg_type::MSG_GET_SETTINGS, handler_wrapper_msg_get_settings },
        { msg_type::MSG_GET_APPLIANCES, handler_wrapper_msg_get_appliances },
        { msg_type::MSG_GET_APPLIANCES_ALL, handler_wrapper_msg_get_appliances_all },
        { msg_type::MSG_GET_TASKS_BY_ID, handler_wrapper_msg_get_tasks_by_id },
        { msg_type::MSG_GET_TASKS_BY_TIME, handler_wrapper_msg_get_tasks_by_time },
        { msg_type::MSG_GET_TASKS_ALL, handler_wrapper_msg_get_tasks_all },
        { msg_type::MSG_GET_AUTO_PROFILES, handler_wrapper_msg_get_auto_profiles },
        { msg_type::MSG_GET_AUTO_PROFILES_ALL, handler_wrapper_msg_get_auto_profiles_all },
        { msg_type::MSG_GET_AUTO_PROFILES_ALL, handler_wrapper_msg_get_auto_profiles_all },
        { msg_type::MSG_GET_ENERGY_PRODUCTION, handler_wrapper_msg_get_energy_production },
        { msg_type::MSG_GET_ENERGY_CONSUMPTION, handler_wrapper_msg_get_energy_consumption },
        { msg_type::MSG_GET_ENERGY_CONSUMPTION_ALL, handler_wrapper_msg_get_energy_consumption_all },
        { msg_type::MSG_GET_WEATHER, handler_wrapper_msg_get_weather }
    };
}}}

namespace hems { namespace modules { namespace storage {

    hems_storage* hems_storage::this_instance = nullptr;

    hems_storage::hems_storage(bool test_mode, std::string db_path) : db_path(db_path) {
        /* Open messenger object. */
        messenger::this_messenger = new messenger(module_type, test_mode);

        logger::this_logger->log(
            "Starting " + modules::to_string_extended(module_type) + ".",
            logger::level::LOG
        );

        /* Create or open database. */
        if (sqlite3_open(db_path.c_str(), &db_connection) != SQLITE_OK) {
            logger::this_logger->log(
                "Could not open database at " + db_path + ", aborting: " + sqlite3_errmsg(db_connection),
                logger::level::ERR
            );
            throw EXIT_FAILURE;
        } else {
            logger::this_logger->log(
                "Successfully opened database at " + db_path + ".",
                logger::level::LOG
            );
        }

        /* Create schema if database is new. */
        if (create_schema() != SQLITE_OK) {
            logger::this_logger->log("Error creating database schema, aborting.", logger::level::ERR);
            throw EXIT_FAILURE;
        } else {
            logger::this_logger->log("Database scheme created successfully.", logger::level::DBG);
        }

        /* Begin listening for messages. */
        const std::vector<int> pre_init_whitelist = { msg_type::MSG_GET_SETTINGS };
        if (!messenger::this_messenger->listen(handler_map, pre_init_whitelist)) {
            logger::this_logger->log("Cannot listen for messages, aborting.", logger::level::ERR);
            throw EXIT_FAILURE;
        } else {
            logger::this_logger->log("Listening for messages.", logger::level::LOG);
        }

        /* Message handlers must not be called before the module's constructor has finished. */
        logger::this_logger->log("Begin handling incoming messages.", logger::level::LOG);
        messenger::this_messenger->start_handlers();
    };

    hems_storage::~hems_storage() {
        logger::this_logger->log(
            "Shutting down " + modules::to_string_extended(module_type) + ".",
            logger::level::LOG
        );

        /* Close database. */
        if (sqlite3_close(db_connection) != SQLITE_OK) {
            logger::this_logger->log(
                "Could not close database at " + db_path +
                ", proceeding anyway, but this may cause problems later: " +
                sqlite3_errmsg(db_connection),
                logger::level::ERR
            );
        } else {
            logger::this_logger->log(
                "Successfully closed database at " + db_path + ".",
                logger::level::LOG
            );
        }

        logger::this_logger->log(
            "Successfully shut down " + modules::to_string_extended(module_type) + ", stop "
                "listening for messages.",
            logger::level::LOG
        );

        /* Delete messenger object. */
        delete messenger::this_messenger;
    }

    int hems_storage::create_schema() {
        auto create_table = [this](std::string stmt, std::string name) {
            char* errmsg;
            int sqlite3_code = sqlite3_exec(db_connection, stmt.c_str(), nullptr, nullptr, &errmsg);
            if (sqlite3_code != SQLITE_OK) {
                logger::this_logger->log(
                    "Could not create table `" + name + "`: " + errmsg,
                    logger::level::ERR
                );
            }
            sqlite3_free(errmsg);
            return sqlite3_code;
        };

        int sqlite3_code;
        std::string stmt;

        /* Create table `settings`. */
        stmt = "CREATE TABLE IF NOT EXISTS settings ("
                    "id INTEGER PRIMARY KEY, "
                    "longitude REAL NOT NULL, "
                    "latitude REAL NOT NULL, "
                    "timezone REAL NOT NULL, "
                    "pv_uri TEXT, "
                    "interval_energy_production INTEGER NOT NULL, "
                    "interval_energy_consumption INTEGER NOT NULL, "
                    "interval_automation INTEGER NOT NULL, "
                    "CHECK ("
                        "id = 0 AND "
                        "interval_energy_production > 0 AND 60 % interval_energy_production = 0 AND "
                        "interval_energy_consumption > 0 AND 60 % interval_energy_consumption = 0 AND "
                        "interval_automation >= 0"
                    ")"
                ")";

        if ((sqlite3_code = create_table(stmt, "settings")) != SQLITE_OK) {
            return sqlite3_code;
        }

        /* Create table `settings_stations`. */
        stmt = "CREATE TABLE IF NOT EXISTS settings_stations ("
                    "station_id INTEGER PRIMARY KEY ASC, "
                    "settings_id INTEGER NOT NULL, "
                    "interval INTEGER NOT NULL, "
                    "uri TEXT, "
                    "FOREIGN KEY (settings_id) REFERENCES settings(id) ON DELETE CASCADE, "
                    "CHECK (interval > 0 AND 60 % interval = 0)"
                ")";

        if ((sqlite3_code = create_table(stmt, "settings_stations")) != SQLITE_OK) {
            return sqlite3_code;
        }

        /* Create table `appliances`. */
        stmt =  "CREATE TABLE IF NOT EXISTS appliances ("
                    "id INTEGER PRIMARY KEY ASC, "
                    "name TEXT NOT NULL, "
                    "uri TEXT, "
                    "rating REAL NOT NULL, "
                    "duty_cycle INTEGER, "
                    "schedules_per_week INTEGER NOT NULL, "
                    "CHECK (id > 0 AND rating >=0 AND duty_cycle >= 0 AND schedules_per_week >= 0)"
                ")";

        if ((sqlite3_code = create_table(stmt, "appliances")) != SQLITE_OK) {
            return sqlite3_code;
        }

        /* Create table `auto_profiles `. */
        stmt =  "CREATE TABLE IF NOT EXISTS auto_profiles ("
                    "id INTEGER PRIMARY KEY ASC, "
                    "name TEXT NOT NULL, "
                    "profile TEXT NOT NULL, "
                    "CHECK (id > 0)"
                ")";

        if ((sqlite3_code = create_table(stmt, "auto_profiles")) != SQLITE_OK) {
            return sqlite3_code;
        }

        /* Create table `tasks`. */
        stmt =  "CREATE TABLE IF NOT EXISTS tasks ("
                    "id INTEGER PRIMARY KEY ASC, "
                    "name TEXT NOT NULL, "
                    "start_time DATETIME NOT NULL, "
                    "end_time DATETIME NOT NULL, "
                    "auto_profile INTEGER, "
                    "is_user_declared BOOLEAN NOT NULL, "
                    "FOREIGN KEY (auto_profile) REFERENCES auto_profiles(id) ON DELETE CASCADE, "
                    "CHECK (id > 0)"
                ")";

        if ((sqlite3_code = create_table(stmt, "tasks")) != SQLITE_OK) {
            return sqlite3_code;
        }

        /* Create table `appliances_tasks`. */
        stmt =  "CREATE TABLE IF NOT EXISTS appliances_tasks ("
                    "appliance_id INTEGER NOT NULL, "
                    "task_id INTEGER NOT NULL, "
                    "PRIMARY KEY (appliance_id, task_id),"
                    "FOREIGN KEY (appliance_id) REFERENCES appliances(id) ON DELETE CASCADE, "
                    "FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE"
                ")";

        if ((sqlite3_code = create_table(stmt, "appliances_tasks")) != SQLITE_OK) {
            return sqlite3_code;
        }

        /* Create table `appliances_auto_profiles`. */
        stmt =  "CREATE TABLE IF NOT EXISTS appliances_auto_profiles ("
                    "appliance_id INTEGER NOT NULL, "
                    "auto_profile INTEGER NOT NULL, "
                    "PRIMARY KEY (appliance_id, auto_profile), "
                    "FOREIGN KEY (appliance_id) REFERENCES appliances(id) ON DELETE CASCADE, "
                    "FOREIGN KEY (auto_profile) REFERENCES auto_profiles(id) ON DELETE CASCADE"
                ")";

        if ((sqlite3_code = create_table(stmt, "appliances_auto_profiles")) != SQLITE_OK) {
            return sqlite3_code;
        }

        /* Create table `energy_consumption`. */
        stmt =  "CREATE TABLE IF NOT EXISTS energy_consumption ("
                    "time DATETIME NOT NULL, "
                    "appliance_id INTEGER, "
                    "energy REAL NOT NULL, "
                    "PRIMARY KEY (time, appliance_id), "
                    "FOREIGN KEY (appliance_id) REFERENCES appliances(id) ON DELETE CASCADE"
                ")";

        if ((sqlite3_code = create_table(stmt, "energy_consumption")) != SQLITE_OK) {
            return sqlite3_code;
        }

        /* Create table `energy_production`. */
        stmt =  "CREATE TABLE IF NOT EXISTS energy_production ("
                    "time DATETIME PRIMARY KEY, "
                    "energy REAL NOT NULL"
                ")";

        if ((sqlite3_code = create_table(stmt, "energy_production")) != SQLITE_OK) {
            return sqlite3_code;
        }

        /* Create table `weather`. */
        stmt =  "CREATE TABLE IF NOT EXISTS weather ("
                    "time DATETIME NOT NULL, "
                    "station INTEGER NOT NULL, "
                    "temperature REAL NOT NULL, "
                    "humidity INTEGER NOT NULL, "
                    "pressure REAL NOT NULL, "
                    "cloud_cover REAL NOT NULL, "
                    "radiation REAL NOT NULL, "
                    "CHECK (station >= 0), "
                    "CHECK (humidity >= 0 and humidity <= 100), "
                    "CHECK (cloud_cover >= 0 and cloud_cover <= 100), "
                    "CHECK (radiation >= 0), "
                    "PRIMARY KEY (time, station), "
                    "FOREIGN KEY (station) REFERENCES settings_stations(station_id) ON DELETE CASCADE"
                ")";

        if ((sqlite3_code = create_table(stmt, "weather")) != SQLITE_OK) {
            return sqlite3_code;
        }

        return sqlite3_code;
    }


    bool hems_storage::db_begin() {
        char* errmsg;
        db_mutex.lock();

        if (sqlite3_exec(db_connection, "BEGIN", nullptr, nullptr, &errmsg) != SQLITE_OK) {
            logger::this_logger->log(
                "Could not begin transaction. The error was: '" + std::string(errmsg) + "'",
                logger::level::ERR
            );
            db_mutex.unlock();
            return false;
        } else {
            logger::this_logger->log("Beginning transaction.", logger::level::DBG);
            return true;
        }
    }

    bool hems_storage::db_commit(bool success) {
        if (success) {
            char* errmsg;

            if (sqlite3_exec(db_connection, "COMMIT", nullptr, nullptr, &errmsg) != SQLITE_OK) {
                hems_storage::db_commit(false);
                logger::this_logger->log(
                    "Could not commit transaction. The error was: " + std::string(errmsg) + "'",
                    logger::level::ERR
                );
                db_mutex.unlock();
                return false;
            } else {
                logger::this_logger->log("Ending transaction.", logger::level::DBG);
                db_mutex.unlock();
                return true;
            }
        } else {
            sqlite3_exec(db_connection, "ROLLBACK", nullptr, nullptr, nullptr);
            db_mutex.unlock();
            return true;
        }
    }

}}}
