/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * These are unit tests for the Data Storage Module.
 */

#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "../test.hpp"
#include "../extras/dummy_logger.hpp"
#include "../extras/random_file_name.hpp"

#include "hems/modules/storage/storage.h"
#include "hems/messages/storage.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

#include "sqlite/sqlite3.h"

namespace hems { namespace modules { namespace storage {

    using namespace hems::messages::storage;
    using namespace hems::types;

    using boost::posix_time::ptime;
    using boost::posix_time::time_from_string;

    /**
     * @brief   This class exists to provide helper functions that need access to private members of
     *          `hems_storage`.
     */
    class hems_storage_test : public hems_storage {
        public:
            hems_storage_test(std::string db_path) : hems_storage(true, db_path) {};

            bool prepare_and_evaluate(const std::string& stmt, sqlite3_stmt*& prepared_stmt) {
                int errcode = sqlite3_prepare_v2(
                    db_connection, stmt.c_str(), -1, &prepared_stmt, nullptr
                );
                if (errcode != SQLITE_OK) {
                    std::cout <<
                        "Error preparing a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else if ((errcode = sqlite3_step(prepared_stmt)) != SQLITE_ROW) {
                    std::cout <<
                        "Error evaluating a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    return true;
                }
            }

            bool prepare_and_evaluate_no_row_expected(std::string& stmt, sqlite3_stmt*& prepared_stmt) {
                int errcode = sqlite3_prepare_v2(
                    db_connection, stmt.c_str(), -1, &prepared_stmt, nullptr
                );
                if (errcode != SQLITE_OK) {
                    std::cout <<
                        "Error preparing a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else if ((errcode = sqlite3_step(prepared_stmt)) == SQLITE_ROW) {
                    std::cout <<
                        "Error evaluating a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    return true;
                }
            }

            bool prepare_and_evaluate_no_row_query(std::string& stmt) {
                char* errmsg = nullptr;

                int errcode = sqlite3_exec(db_connection, stmt.c_str(), nullptr, nullptr, &errmsg);
                if (errcode != SQLITE_OK) {
                    std::cout <<
                        "Error executing a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    return false;
                } else {
                    return true;
                }
            }
    };

    /**
     * @brief   Send a `MSG_DEL_APPLIANCE` message for a handler test.
     */
    int msg_del_appliance_send(messenger* this_messenger, id_t id) {
        messages::storage::msg_del_appliance_request msg_del = {
            id : id
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_DEL_APPLIANCE,
            modules::STORAGE,
            this_messenger->serialize(msg_del),
            nullptr
        );
    }

    /**
     * @brief   Send a `MSG_DEL_TASK` message for a handler test.
     */
    int msg_del_task_send(messenger* this_messenger, id_t id) {
        messages::storage::msg_del_task_request msg_del = {
            id : id
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_DEL_TASK,
            modules::STORAGE,
            this_messenger->serialize(msg_del),
            nullptr
        );
    }

    /**
     * @brief   Send a `MSG_DEL_AUTO_PROFILE` message for a handler test.
     */
    int msg_del_auto_profile_send(messenger* this_messenger, id_t id) {
        messages::storage::msg_del_auto_profile_request msg_del = {
            id : id
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_DEL_AUTO_PROFILE,
            modules::STORAGE,
            this_messenger->serialize(msg_del),
            nullptr
        );
    }

    /**
     * @brief   Send a `MSG_DEL_ENERGY_CONSUMPTION` message for a handler test.
     */
    int msg_del_energy_consumption_send(messenger* this_messenger, ptime time, id_t appliance_id) {
        messages::storage::msg_del_energy_consumption_request msg_del = {
            time            : time,
            appliance_id    : appliance_id
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_DEL_ENERGY_CONSUMPTION,
            modules::STORAGE,
            this_messenger->serialize(msg_del),
            nullptr
        );
    }

    /**
     * @brief   Send a `MSG_DEL_ENERGY_PRODUCTION` message for a handler test.
     */
    int msg_del_energy_production_send(messenger* this_messenger, ptime time) {
        messages::storage::msg_del_energy_production_request msg_del = {
            time : time
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_DEL_ENERGY_PRODUCTION,
            modules::STORAGE,
            this_messenger->serialize(msg_del),
            nullptr
        );
    }

    /**
     * @brief   Send a `MSG_DEL_WEATHER` message for a handler test.
     */
    int msg_del_weather_send(messenger* this_messenger, ptime start_time, id_t station) {
        messages::storage::msg_del_weather_request msg_del = {
            time    : start_time,
            station : station
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_DEL_WEATHER,
            modules::STORAGE,
            this_messenger->serialize(msg_del),
            nullptr
        );
    }

    /**
     * @brief   Write settings into database for the `MSG_DEL_WEATHER` test.
     */
    bool write_settings(hems_storage_test* this_instance, settings_t settings) {
        current_settings = settings;
        std::string stmt1 =
            "INSERT INTO settings ("
                "id, longitude, latitude, timezone, pv_uri, interval_energy_production, "
                "interval_energy_consumption, interval_automation"
            ") "
            "VALUES ("
                "0, " +
                std::to_string(settings.longitude) + ", " +
                std::to_string(settings.latitude) + ", " +
                std::to_string(settings.timezone) + ", " +
                "'" + settings.pv_uri + "', " +
                std::to_string(settings.interval_energy_production) + ", " +
                std::to_string(settings.interval_energy_consumption) + ", " +
                std::to_string(settings.interval_automation) +
            ") "
            "ON CONFLICT (id) DO UPDATE SET "
                "longitude=excluded.longitude, latitude=excluded.latitude, timezone=excluded.timezone, "
                "pv_uri=excluded.pv_uri, "
                "interval_energy_production=excluded.interval_energy_production, "
                "interval_energy_consumption=excluded.interval_energy_consumption, "
                "interval_automation=excluded.interval_automation "
            "WHERE id = 0";
        if (!this_instance->prepare_and_evaluate_no_row_query(stmt1)) {
            return false;
        }

        std::string stmt2 =
            "INSERT INTO settings_stations (station_id, settings_id, interval) "
            "VALUES ";
        for (const auto& station_interval : settings.station_intervals) {
             stmt2 +=
                "(" +
                    std::to_string(station_interval.first) + ", " +
                    "0, " +
                    std::to_string(station_interval.second) +
                "), ";
        }
        stmt2.pop_back();
        stmt2.pop_back();
        stmt2 +=
            " ON CONFLICT (station_id) DO UPDATE SET"
            " station_id=excluded.station_id, settings_id=excluded.settings_id, interval=excluded.interval";
        if (!this_instance->prepare_and_evaluate_no_row_query(stmt2)) {
            return false;
        }

        return true;
    }


    /**
     * @brief   Test the handler for `MSG_DEL_APPLIANCE` messages.
     */
    bool test_handler_msg_del_appliance_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        /* Add entries into `appliances` and the associated child tables. */

        for (int i=1; i<=2; ++i) {
            std::string stmt1 =
                "INSERT INTO appliances (name, rating, duty_cycle, schedules_per_week) "
                "VALUES ('Lorem ipsum', 5.5, 4, 1); "
                "INSERT INTO tasks (name, start_time, end_time, auto_profile, is_user_declared) "
                "VALUES ("
                    "'Lorem ipsum', "
                    "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 02:02:02.000")) + "', " +
                    "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 22:02:02.000")) + "', " +
                    "NULL, "
                    "0" +
                "); "
                "INSERT INTO auto_profiles (name, profile) "
                "VALUES ('Lorem ipsum', 'Lorem ipsum'); ";

            if (!this_instance->prepare_and_evaluate_no_row_query(stmt1)) {
                return false;
            }
        }

        std::string stmt2 =
            "INSERT INTO appliances_tasks (appliance_id, task_id) VALUES (1, 1); "
            "INSERT INTO appliances_tasks (appliance_id, task_id) VALUES (2, 1); "
            "INSERT INTO appliances_auto_profiles (appliance_id, auto_profile) VALUES (1, 1); "
            "INSERT INTO appliances_auto_profiles (appliance_id, auto_profile) VALUES (2, 1); "
            "INSERT INTO energy_consumption (time, appliance_id, energy) "
            "VALUES "
            "("
                "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 02:02:02.000")) + "', " +
                "1, 4"
            "), "
            "("
                "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 02:02:02.000")) + "', " +
                "2, 4"
            ")";

        if (!this_instance->prepare_and_evaluate_no_row_query(stmt2)) {
            return false;
        }


        /* Check that the entries were successfully added. */

        for (int i=1; i<=2; ++i) {
            std::vector<std::string> stmts3 = {
                "SELECT * FROM appliances WHERE id=" + std::to_string(i),
                "SELECT * FROM appliances_tasks WHERE appliance_id=" + std::to_string(i),
                "SELECT * FROM appliances_auto_profiles WHERE appliance_id=" + std::to_string(i),
                "SELECT * FROM energy_consumption WHERE appliance_id=" + std::to_string(i)
            };
            for (const auto& stmt3 : stmts3) {
                sqlite3_stmt* prepared_stmt3 = nullptr;
                if (!this_instance->prepare_and_evaluate(stmt3, prepared_stmt3)) {
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt3);
                }
            }
        }


        /* Delete entry. */

        res = msg_del_appliance_send(this_messenger, 0);
        if (res != response_code::MSG_DEL_CONSTRAINT_VIOLATION) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_CONSTRAINT_VIOLATION) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        res = msg_del_appliance_send(this_messenger, 3);
        if (res != response_code::MSG_DEL_DELETE_NON_EXISTING) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_DELETE_NON_EXISTING) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        res = msg_del_appliance_send(this_messenger, 1);
        if (res != response_code::SUCCESS) {
            std::cout <<
                "Query that should have been successful failed with error " + std::to_string(res) +
                " instead.\n";
            return false;
        }


        /* Check that the deleted entry is indeed gone. */

        std::vector<std::string> stmts4 = {
            "SELECT * FROM appliances WHERE id=1",
            "SELECT * FROM appliances_tasks WHERE appliance_id=1",
            "SELECT * FROM appliances_auto_profiles WHERE appliance_id=1",
            "SELECT * FROM energy_consumption WHERE appliance_id=1"
        };
        for (auto& stmt4 : stmts4) {
            sqlite3_stmt* prepared_stmt4 = nullptr;
            if (!this_instance->prepare_and_evaluate_no_row_expected(stmt4, prepared_stmt4)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt4);
            }
        }


        /* Check that the other entry is still there. */

        std::vector<std::string> stmts5 = {
            "SELECT * FROM appliances WHERE id=2",
            "SELECT * FROM appliances_tasks WHERE appliance_id=2",
            "SELECT * FROM appliances_auto_profiles WHERE appliance_id=2",
            "SELECT * FROM energy_consumption WHERE appliance_id=2"
        };
        for (const auto& stmt5 : stmts5) {
            sqlite3_stmt* prepared_stmt5 = nullptr;
            if (!this_instance->prepare_and_evaluate(stmt5, prepared_stmt5)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt5);
            }
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_DEL_TASK` messages.
     */
    bool test_handler_msg_del_task_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        /* Add entries into `tasks` and the associated child tables. */

        for (int i=1; i<=2; ++i) {
            std::string stmt1 =
                "INSERT INTO appliances (name, rating, duty_cycle, schedules_per_week) "
                "VALUES ('Lorem ipsum', 5.5, 4, 1); "
                "INSERT INTO tasks (name, start_time, end_time, auto_profile, is_user_declared) "
                "VALUES ("
                    "'Lorem ipsum', "
                    "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 02:02:02.000")) + "', " +
                    "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 22:02:02.000")) + "', " +
                    "NULL, "
                    "0" +
                "); ";

            if (!this_instance->prepare_and_evaluate_no_row_query(stmt1)) {
                return false;
            }
        }

        std::string stmt2 =
            "INSERT INTO appliances_tasks (appliance_id, task_id) VALUES (1, 1); "
            "INSERT INTO appliances_tasks (appliance_id, task_id) VALUES (1, 2); ";
        if (!this_instance->prepare_and_evaluate_no_row_query(stmt2)) {
            return false;
        }


        /* Check that the entries were successfully added. */

        for (int i=1; i<=2; ++i) {
            std::vector<std::string> stmts3 = {
                "SELECT * FROM tasks WHERE id=" + std::to_string(i),
                "SELECT * FROM appliances_tasks WHERE task_id=" + std::to_string(i)
            };
            for (const auto& stmt3 : stmts3) {
                sqlite3_stmt* prepared_stmt3 = nullptr;
                if (!this_instance->prepare_and_evaluate(stmt3, prepared_stmt3)) {
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt3);
                }
            }
        }


        /* Delete entry. */

        res = msg_del_task_send(this_messenger, 0);
        if (res != response_code::MSG_DEL_CONSTRAINT_VIOLATION) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_CONSTRAINT_VIOLATION) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        res = msg_del_task_send(this_messenger, 3);
        if (res != response_code::MSG_DEL_DELETE_NON_EXISTING) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_DELETE_NON_EXISTING) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        res = msg_del_task_send(this_messenger, 1);
        if (res != response_code::SUCCESS) {
            std::cout <<
                "Query that should have been successful failed with error " + std::to_string(res) +
                " instead.\n";
            return false;
        }


        /* Check that the deleted entry is indeed gone. */

        std::vector<std::string> stmts4 = {
            "SELECT * FROM tasks WHERE id=1",
            "SELECT * FROM appliances_tasks WHERE task_id=1"
        };
        for (auto& stmt4 : stmts4) {
            sqlite3_stmt* prepared_stmt4 = nullptr;
            if (!this_instance->prepare_and_evaluate_no_row_expected(stmt4, prepared_stmt4)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt4);
            }
        }


        /* Check that the other entry is still there. */

        std::vector<std::string> stmts5 = {
            "SELECT * FROM tasks WHERE id=2",
            "SELECT * FROM appliances_tasks WHERE task_id=2"
        };
        for (const auto& stmt5 : stmts5) {
            sqlite3_stmt* prepared_stmt5 = nullptr;
            if (!this_instance->prepare_and_evaluate(stmt5, prepared_stmt5)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt5);
            }
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_DEL_AUTO_PROFILE` messages.
     */
    bool test_handler_msg_del_auto_profile_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Add entries into `auto_profiles` and the associated child tables. */

        for (int i=1; i<=2; ++i) {
            std::string stmt1 =
                "INSERT INTO appliances (name, rating, duty_cycle, schedules_per_week) "
                "VALUES ('Lorem ipsum', 5.5, 4, 1); "
                "INSERT INTO auto_profiles (name, profile) "
                "VALUES ('Lorem ipsum', 'Lorem ipsum'); ";

            if (!this_instance->prepare_and_evaluate_no_row_query(stmt1)) {
                return false;
            }
        }

        std::string stmt2 =
            "INSERT INTO appliances_auto_profiles (appliance_id, auto_profile) VALUES (1, 1); "
            "INSERT INTO appliances_auto_profiles (appliance_id, auto_profile) VALUES (1, 2); "
            "INSERT INTO tasks (name, start_time, end_time, auto_profile, is_user_declared) "
                "VALUES ("
                    "'Lorem ipsum', "
                    "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 02:02:02.000")) + "', " +
                    "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 22:02:02.000")) + "', " +
                    "1, "
                    "0" +
                "); "
            "INSERT INTO tasks (name, start_time, end_time, auto_profile, is_user_declared) "
                "VALUES ("
                    "'Lorem ipsum', "
                    "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 02:02:02.000")) + "', " +
                    "'" + boost::posix_time::to_iso_string(time_from_string("2020-02-20 22:02:02.000")) + "', " +
                    "2, "
                    "0" +
                "); ";
        if (!this_instance->prepare_and_evaluate_no_row_query(stmt2)) {
            return false;
        }


        /* Check that the entries were successfully added. */

        for (int i=1; i<=2; ++i) {
            std::vector<std::string> stmts3 = {
                "SELECT * FROM auto_profiles WHERE id=" + std::to_string(i),
                "SELECT * FROM tasks WHERE auto_profile=" + std::to_string(i),
                "SELECT * FROM appliances_auto_profiles WHERE auto_profile=" + std::to_string(i)
            };
            for (const auto& stmt3 : stmts3) {
                sqlite3_stmt* prepared_stmt3 = nullptr;
                if (!this_instance->prepare_and_evaluate(stmt3, prepared_stmt3)) {
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt3);
                }
            }
        }


        /* Delete entry. */

        res = msg_del_auto_profile_send(this_messenger, 0);
        if (res != response_code::MSG_DEL_CONSTRAINT_VIOLATION) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_CONSTRAINT_VIOLATION) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        res = msg_del_auto_profile_send(this_messenger, 3);
        if (res != response_code::MSG_DEL_DELETE_NON_EXISTING) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_DELETE_NON_EXISTING) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        res = msg_del_auto_profile_send(this_messenger, 1);
        if (res != response_code::SUCCESS) {
            std::cout <<
                "Query that should have been successful failed with error " + std::to_string(res) +
                " instead.\n";
            return false;
        }


        /* Check that the deleted entry is indeed gone. */

        std::vector<std::string> stmts4 = {
            "SELECT * FROM auto_profiles WHERE id=1",
            "SELECT * FROM tasks WHERE auto_profile=1",
            "SELECT * FROM appliances_auto_profiles WHERE auto_profile=1"
        };
        for (auto& stmt4 : stmts4) {
            sqlite3_stmt* prepared_stmt4 = nullptr;
            if (!this_instance->prepare_and_evaluate_no_row_expected(stmt4, prepared_stmt4)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt4);
            }
        }


        /* Check that the other entry is still there. */

        std::vector<std::string> stmts5 = {
            "SELECT * FROM auto_profiles WHERE id=2",
            "SELECT * FROM tasks WHERE auto_profile=2",
            "SELECT * FROM appliances_auto_profiles WHERE auto_profile=2"
        };
        for (const auto& stmt5 : stmts5) {
            sqlite3_stmt* prepared_stmt5 = nullptr;
            if (!this_instance->prepare_and_evaluate(stmt5, prepared_stmt5)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt5);
            }
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_DEL_ENERGY_CONSUMPTION` messages.
     */
    bool test_handler_msg_del_energy_consumption_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Add entries into `energy_consumption`. */

        for (int i=1; i<=2; ++i) {
            std::string stmt1 =
                "INSERT INTO energy_consumption (time, appliance_id, energy) "
                "VALUES ("
                    "'20200220T0" + std::to_string(i) + "0000', " +
                    "NULL, " +
                    "5" +
                ")";

            if (!this_instance->prepare_and_evaluate_no_row_query(stmt1)) {
                return false;
            }
        }


        /* Check that the entries were successfully added. */

        for (int i=1; i<=2; ++i) {
            std::string stmt2 =
                "SELECT * FROM energy_consumption WHERE time='20200220T0" + std::to_string(i) + "0000'";
            sqlite3_stmt* prepared_stmt2 = nullptr;
            if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt2);
            }
        }


        /* Delete entry. */

        boost::posix_time::ptime time3 = time_from_string("2020-02-20 00:00:22.000");
        res = msg_del_energy_consumption_send(this_messenger, time3, 0);
        if (res != response_code::MSG_DEL_CONSTRAINT_VIOLATION) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_CONSTRAINT_VIOLATION) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        boost::posix_time::ptime time4 = time_from_string("2020-02-20 03:00:00.000");
        res = msg_del_energy_consumption_send(this_messenger, time4, 0);
        if (res != response_code::MSG_DEL_DELETE_NON_EXISTING) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_DELETE_NON_EXISTING) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        boost::posix_time::ptime time5 = time_from_string("2020-02-20 01:00:00.000");
        res = msg_del_energy_consumption_send(this_messenger, time5, 0);
        if (res != response_code::SUCCESS) {
            std::cout <<
                "Query that should have been successful failed with error " + std::to_string(res) +
                " instead.\n";
            return false;
        }


        /* Check that the deleted entry is indeed gone. */

        std::string stmt6 = "SELECT * FROM energy_consumption WHERE time='20200220T010000'";
        sqlite3_stmt* prepared_stmt6 = nullptr;
        if (!this_instance->prepare_and_evaluate_no_row_expected(stmt6, prepared_stmt6)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt6);
        }


        /* Check that the other entry is still there. */

        std::string stmt7 = "SELECT * FROM energy_consumption WHERE time='20200220T020000'";
        sqlite3_stmt* prepared_stmt7 = nullptr;
        if (!this_instance->prepare_and_evaluate(stmt7, prepared_stmt7)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt7);
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_DEL_ENERGY_PRODUCTION` messages.
     */
    bool test_handler_msg_del_energy_production_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Add entries into `energy_production`. */

        for (int i=1; i<=2; ++i) {
            std::string stmt1 =
                "INSERT INTO energy_production (time, energy) "
                "VALUES ("
                    "'20200220T0" + std::to_string(i) + "0000', " +
                    "5" +
                ")";

            if (!this_instance->prepare_and_evaluate_no_row_query(stmt1)) {
                return false;
            }
        }


        /* Check that the entries were successfully added. */

        for (int i=1; i<=2; ++i) {
            std::string stmt2 =
                "SELECT * FROM energy_production WHERE time='20200220T0" + std::to_string(i) + "0000'";
            sqlite3_stmt* prepared_stmt2 = nullptr;
            if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt2);
            }
        }


        /* Delete entry. */

        boost::posix_time::ptime time3 = time_from_string("2020-02-20 00:00:22.000");
        res = msg_del_energy_production_send(this_messenger, time3);
        if (res != response_code::MSG_DEL_CONSTRAINT_VIOLATION) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_CONSTRAINT_VIOLATION) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        boost::posix_time::ptime time4 = time_from_string("2020-02-20 03:00:00.000");
        res = msg_del_energy_production_send(this_messenger, time4);
        if (res != response_code::MSG_DEL_DELETE_NON_EXISTING) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_DELETE_NON_EXISTING) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        boost::posix_time::ptime time5 = time_from_string("2020-02-20 01:00:00.000");
        res = msg_del_energy_production_send(this_messenger, time5);
        if (res != response_code::SUCCESS) {
            std::cout <<
                "Query that should have been successful failed with error " + std::to_string(res) +
                " instead.\n";
            return false;
        }


        /* Check that the deleted entry is indeed gone. */

        std::string stmt6 = "SELECT * FROM energy_production WHERE time='20200220T010000'";
        sqlite3_stmt* prepared_stmt6 = nullptr;
        if (!this_instance->prepare_and_evaluate_no_row_expected(stmt6, prepared_stmt6)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt6);
        }


        /* Check that the other entry is still there. */

        std::string stmt7 = "SELECT * FROM energy_production WHERE time='20200220T020000'";
        sqlite3_stmt* prepared_stmt7 = nullptr;
        if (!this_instance->prepare_and_evaluate(stmt7, prepared_stmt7)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt7);
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_DEL_WEATHER` messages.
     */
    bool test_handler_msg_del_weather_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        /* Write settings into database, as required because of the `station` foreign key. */
        settings_t settings = {
            longitude                   : 52.455864,
            latitude                    : 13.296937,
            timezone                    : 1,
            pv_uri                      : "",
            station_intervals           : { {1, 15}, {2, 30} },
            station_uris                : {},
            interval_energy_production  : 10,
            interval_energy_consumption : 20,
            interval_automation         : 36
        };
        if (!write_settings(this_instance, settings)) {
            return false;
        }

        /* Add entries into `weather`. */

        for (int i=1; i<=2; ++i) {
            std::string stmt1 =
                "INSERT INTO weather (time, station, temperature, humidity, pressure, cloud_cover, radiation) "
                "VALUES ("
                    "'20200220T0" + std::to_string(i) + "0000', " +
                    "1, 5, 6, 9, 17, 70" +
                ")";

            if (!this_instance->prepare_and_evaluate_no_row_query(stmt1)) {
                return false;
            }
        }


        /* Check that the entries were successfully added. */

        for (int i=1; i<=2; ++i) {
            std::string stmt2 =
                "SELECT * FROM weather WHERE time='20200220T0" + std::to_string(i) + "0000'";
            sqlite3_stmt* prepared_stmt2 = nullptr;
            if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt2);
            }
        }


        /* Delete entry. */

        boost::posix_time::ptime time3 = time_from_string("2020-02-20 00:00:22.000");
        res = msg_del_weather_send(this_messenger, time3, 1);
        if (res != response_code::MSG_DEL_CONSTRAINT_VIOLATION) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_CONSTRAINT_VIOLATION) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        boost::posix_time::ptime time4 = time_from_string("2020-02-20 03:00:00.000");
        res = msg_del_weather_send(this_messenger, time4, 1);
        if (res != response_code::MSG_DEL_DELETE_NON_EXISTING) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_DEL_DELETE_NON_EXISTING) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        boost::posix_time::ptime time5 = time_from_string("2020-02-20 01:00:00.000");
        res = msg_del_weather_send(this_messenger, time5, 1);
        if (res != response_code::SUCCESS) {
            std::cout <<
                "Query that should have been successful failed with error " + std::to_string(res) +
                " instead.\n";
            return false;
        }


        /* Check that the deleted entry is indeed gone. */

        std::string stmt6 = "SELECT * FROM weather WHERE time='20200220T010000'";
        sqlite3_stmt* prepared_stmt6 = nullptr;
        if (!this_instance->prepare_and_evaluate_no_row_expected(stmt6, prepared_stmt6)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt6);
        }


        /* Check that the other entry is still there. */

        std::string stmt7 = "SELECT * FROM weather WHERE time='20200220T020000'";
        sqlite3_stmt* prepared_stmt7 = nullptr;
        if (!this_instance->prepare_and_evaluate(stmt7, prepared_stmt7)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt7);
        }


        return true;
    }


    enum msg_del_test_types {
        APPLIANCE, TASK, AUTO_PROFILE, ENERGY_CONSUMPTION, ENERGY_PRODUCTION, WEATHER
    };

    bool test_handler_msg_del(msg_del_test_types test_type) {
        logger::this_logger = new dummy_logger();

        char db_path[11];
        do {
            generate_random_file_name(db_path);
        } while (boost::filesystem::exists(db_path));

        /* Delete message queues to remove junk messages from previous runs. */
        mq_unlink(messenger::mq_names.at(modules::type::STORAGE).c_str());
        mq_unlink(messenger::mq_names.at(modules::type::LAUNCHER).c_str());

        /* Create the message queues for the Data Storage Module so that its constructor does not fail. */
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

        /*  Create message queues under the identity of the HEMS Launcher that we can use to
            communicate with the Data Storage Module. */
        mq_close(mq_open(
            messenger::mq_names.at(modules::type::LAUNCHER).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));
        mq_close(mq_open(
            messenger::mq_res_names.at(modules::type::LAUNCHER).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));

        try {
            hems_storage::this_instance = new hems_storage_test(db_path);
        } catch (int err) {
            std::cout <<
                "Constructing Data Storage Module threw an exception " + std::to_string(err) +
                ", test failed.\n";
            return false;
        }

        messenger* this_messenger = new messenger(modules::LAUNCHER, true);
        const messenger::msg_handler_map handler_map = {};
        this_messenger->listen(handler_map);
        this_messenger->start_handlers();

        hems_storage_test* this_instance =
            reinterpret_cast<hems_storage_test*>(hems_storage::this_instance);

        bool success;
        switch (test_type) {
            case msg_del_test_types::APPLIANCE:
                success = test_handler_msg_del_appliance_(this_messenger, this_instance);
                break;
            case msg_del_test_types::TASK:
                success = test_handler_msg_del_task_(this_messenger, this_instance);
                break;
            case msg_del_test_types::AUTO_PROFILE:
                success = test_handler_msg_del_auto_profile_(this_messenger, this_instance);
                break;
            case msg_del_test_types::ENERGY_CONSUMPTION:
                success = test_handler_msg_del_energy_consumption_(this_messenger, this_instance);
                break;
            case msg_del_test_types::ENERGY_PRODUCTION:
                success = test_handler_msg_del_energy_production_(this_messenger, this_instance);
                break;
            case msg_del_test_types::WEATHER:
                success = test_handler_msg_del_weather_(this_messenger, this_instance);
                break;
        }

        delete hems_storage::this_instance;
        delete this_messenger;
        delete logger::this_logger;

        remove(db_path);
        mq_unlink(messenger::mq_names.at(modules::type::STORAGE).c_str());
        mq_unlink(messenger::mq_names.at(modules::type::LAUNCHER).c_str());

        return success;
    }

    static inline bool test_handler_msg_del_appliance() {
        return test_handler_msg_del(msg_del_test_types::APPLIANCE);
    }

    static inline bool test_handler_msg_del_task() {
        return test_handler_msg_del(msg_del_test_types::TASK);
    }

    static inline bool test_handler_msg_del_auto_profile() {
        return test_handler_msg_del(msg_del_test_types::AUTO_PROFILE);
    }

    static inline bool test_handler_msg_del_energy_consumption() {
        return test_handler_msg_del(msg_del_test_types::ENERGY_CONSUMPTION);
    }

    static inline bool test_handler_msg_del_energy_production() {
        return test_handler_msg_del(msg_del_test_types::ENERGY_PRODUCTION);
    }

    static inline bool test_handler_msg_del_weather() {
        return test_handler_msg_del(msg_del_test_types::WEATHER);
    }

}}}

int main() {
    return run_tests({
        {
            "01 Storage: Message handler test for MSG_DEL_APPLIANCE",
            &hems::modules::storage::test_handler_msg_del_appliance
        },
        {
            "02 Storage: Message handler test for MSG_DEL_TASK",
            &hems::modules::storage::test_handler_msg_del_task
        },
        {
            "03 Storage: Message handler test for MSG_DEL_AUTO_PROFILE",
            &hems::modules::storage::test_handler_msg_del_auto_profile
        },
        {
            "04 Storage: Message handler test for MSG_DEL_ENERGY_CONSUMPTION",
            &hems::modules::storage::test_handler_msg_del_energy_consumption
        },
        {
            "05 Storage: Message handler test for MSG_DEL_ENERGY_PRODUCTION",
            &hems::modules::storage::test_handler_msg_del_energy_production
        },
        {
            "06 Storage: Message handler test for MSG_DEL_WEATHER",
            &hems::modules::storage::test_handler_msg_del_weather
        }
    });
}
