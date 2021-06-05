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
#include <set>
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
    using boost::posix_time::from_iso_string;

    /**
     * @brief   This class exists to provide helper functions that need access to private members of
     *          `hems_storage`.
     */
    class hems_storage_test : public hems_storage {
        public:
            hems_storage_test(std::string db_path) : hems_storage(true, db_path) {};

            bool prepare_and_evaluate(std::string& stmt, sqlite3_stmt*& prepared_stmt) {
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
     * @brief   Send a `MSG_SET_APPLIANCE` message for a handler test.
     */
    int msg_set_appliance_send(
        messenger* this_messenger, appliance_t payload, std::string* response
    ) {
        messages::storage::msg_set_appliance_request msg_set = {
            appliance : payload
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_SET_APPLIANCE,
            modules::STORAGE,
            this_messenger->serialize(msg_set),
            response
        );
    }

    /**
     * @brief   Send a `MSG_SET_TASK` message for a handler test.
     */
    int msg_set_task_send(messenger* this_messenger, task_t payload, std::string* response) {
        messages::storage::msg_set_task_request msg_set = {
            task : payload
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_SET_TASK,
            modules::STORAGE,
            this_messenger->serialize(msg_set),
            response
        );
    }

    /**
     * @brief   Send a `MSG_SET_AUTO_PROFILE` message for a handler test.
     */
    int msg_set_auto_profile_send(
        messenger* this_messenger, auto_profile_t payload, std::string* response
    ) {
        messages::storage::msg_set_auto_profile_request msg_set = {
            auto_profile : payload
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_SET_AUTO_PROFILE,
            modules::STORAGE,
            this_messenger->serialize(msg_set),
            response
        );
    }

    /**
     * @brief   Send a `MSG_SET_ENERGY_CONSUMPTION` message for a handler test.
     */
    int msg_set_energy_consumption_send(messenger* this_messenger, energy_consumption_t payload) {
        messages::storage::msg_set_energy_consumption_request msg_set = {
            energy_consumption : payload
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_SET_ENERGY_CONSUMPTION,
            modules::STORAGE,
            this_messenger->serialize(msg_set),
            nullptr
        );
    }

    /**
     * @brief   Send a `MSG_SET_ENERGY_PRODUCTION` message for a handler test.
     */
    int msg_set_energy_production_send(messenger* this_messenger, energy_production_t payload) {
        messages::storage::msg_set_energy_production_request msg_set = {
            energy_production : payload
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_SET_ENERGY_PRODUCTION,
            modules::STORAGE,
            this_messenger->serialize(msg_set),
            nullptr
        );
    }

    /**
     * @brief   Send a `MSG_SET_WEATHER` message for a handler test.
     */
    int msg_set_weather_send(messenger* this_messenger, weather_t payload) {
        messages::storage::msg_set_weather_request msg_set = {
            weather : payload
        };
        return this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_SET_WEATHER,
            modules::STORAGE,
            this_messenger->serialize(msg_set),
            nullptr
        );
    }

    /**
     * @brief   Write settings into database for the `MSG_SET_WEATHER` test.
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
     * @brief   Test the handler for `MSG_SET_APPLIANCE` messages.
     */
    bool test_handler_msg_set_appliance_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        /* Test replacing an appliance_t before it exists. */
        types::appliance_t appliance1 = {
            id                  : 1,
            name                : "appliance",
            uri                 : "",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 1,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>()
        };
        res = msg_set_appliance_send(this_messenger, appliance1, nullptr);
        if (res != response_code::MSG_SET_REPLACE_NON_EXISTING) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_REPLACE_NON_EXISTING) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }


        /* 0 for a task or auto_profile id should be treated as if there was no id. */
        std::vector<std::set<id_t>> ids_vector = {
            std::set<id_t>(),
            std::set<id_t>({0, 0})
        };

        for (const auto& ids : ids_vector) {
            /* Test adding a new valid appliance_t without foreign keys. */
            types::appliance_t appliance2 = {
                id                  : 0,
                name                : "Lorem ipsum",
                uri                 : "Tomato",
                rating              : 5.5,
                duty_cycle          : 4,
                schedules_per_week  : 1,
                tasks               : ids,
                auto_profiles       : ids
            };
            res = msg_set_appliance_send(this_messenger, appliance2, nullptr);
            if (res) {
                std::cout <<
                    "Query that should have been successful failed with code " + std::to_string(res) +
                    " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt = nullptr;
                std::string stmt = "SELECT * FROM appliances WHERE name='" + appliance2.name + "'";

                if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                    return false;
                } else {
                    id_t id = sqlite3_column_int64(prepared_stmt, 0);
                    std::string name =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));
                    std::string uri =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 2));
                    double rating = sqlite3_column_double(prepared_stmt, 3);
                    uint32_t duty_cycle = sqlite3_column_int(prepared_stmt, 4);
                    uint8_t schedules_per_week = sqlite3_column_int(prepared_stmt, 5);

                    if (id == 0 || name != appliance2.name || uri != appliance2.uri ||
                        rating != appliance2.rating || duty_cycle != appliance2.duty_cycle ||
                        schedules_per_week != appliance2.schedules_per_week) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt);
                    }
                }
            }


            /*  Test replacing an existing appliance_t with valid values (without foreign keys,
                non-schedulable). */
            types::appliance_t appliance3 = {
                id                  : 1,
                name                : "dolor sit amet",
                uri                 : "potato",
                rating              : 6.1,
                duty_cycle          : 9,
                schedules_per_week  : 0,
                tasks               : ids,
                auto_profiles       : ids
            };
            res = msg_set_appliance_send(this_messenger, appliance3, nullptr);
            if (res) {
                std::cout <<
                    "Query that should have been successful failed with code " + std::to_string(res) +
                    " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt;
                std::string stmt = "SELECT * FROM appliances WHERE id=" + std::to_string(appliance3.id);

                if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                    return false;
                } else {
                    id_t id = sqlite3_column_int64(prepared_stmt, 0);
                    std::string name =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));
                    std::string uri =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 2));
                    double rating = sqlite3_column_double(prepared_stmt, 3);
                    uint32_t duty_cycle = sqlite3_column_int(prepared_stmt, 4);
                    uint8_t schedules_per_week = sqlite3_column_int(prepared_stmt, 5);

                    if (name == appliance2.name || uri == appliance2.uri ||
                        rating == appliance2.rating || duty_cycle == appliance2.duty_cycle ||
                        schedules_per_week == appliance2.schedules_per_week) {
                        std::cout << "Item in database was not replaced with all new values.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else if ( id != appliance3.id || name != appliance3.name ||
                                rating != appliance3.rating || duty_cycle != appliance3.duty_cycle ||
                                schedules_per_week != appliance3.schedules_per_week) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt);
                    }
                }
            }


            /*  Test replacing an existing appliance_t with valid values (without foreign keys,
                schedulable). */
            types::appliance_t appliance4 = {
                id                  : 1,
                name                : "consetetur sadipscing elitr",
                uri                 : "If I sound lazy just ignore my tone",
                rating              : 88.4,
                duty_cycle          : 5,
                schedules_per_week  : 2,
                tasks               : ids,
                auto_profiles       : ids
            };
            res = msg_set_appliance_send(this_messenger, appliance4, nullptr);
            if (res) {
                std::cout <<
                    "Query that should have been successful failed with code " + std::to_string(res) +
                    " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt;
                std::string stmt = "SELECT * FROM appliances WHERE id=" + std::to_string(appliance3.id);

                if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                    return false;
                } else {
                    id_t id = sqlite3_column_int64(prepared_stmt, 0);
                    std::string name =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));
                    std::string uri =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 2));
                    double rating = sqlite3_column_double(prepared_stmt, 3);
                    uint32_t duty_cycle = sqlite3_column_int(prepared_stmt, 4);
                    uint8_t schedules_per_week = sqlite3_column_int(prepared_stmt, 5);

                    if (name == appliance3.name || uri == appliance3.uri ||
                        rating == appliance3.rating || duty_cycle == appliance3.duty_cycle ||
                        schedules_per_week == appliance3.schedules_per_week) {
                        std::cout << "Item in database was not replaced with all new values.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else if ( id != appliance4.id || name != appliance4.name ||
                                rating != appliance4.rating || duty_cycle != appliance4.duty_cycle ||
                                schedules_per_week != appliance4.schedules_per_week) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt);
                    }
                }
            }
        }


        /* Test adding an invalid appliance_t with non-existing foreign keys (tasks). */
        types::appliance_t appliance5 = {
            id                  : 0,
            name                : "sed diam nonumy",
            uri                 : "Cause I'm always gonna answer when you call my phone",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 1,
            tasks               : std::set<id_t>({4, 5}),
            auto_profiles       : std::set<id_t>()
        };
        res = msg_set_appliance_send(this_messenger, appliance5, nullptr);
        if (res != response_code::MSG_SET_SQL_ERROR) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_SQL_ERROR) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        /* Test adding an invalid appliance_t with non-existing foreign keys (auto_profiles). */
        types::appliance_t appliance6 = {
            id                  : 0,
            name                : "sed diam nonumy",
            uri                 : "What's up danger",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 1,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>({6, 7})
        };
        res = msg_set_appliance_send(this_messenger, appliance6, nullptr);
        if (res != response_code::MSG_SET_SQL_ERROR) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_SQL_ERROR) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }


        /* Test adding an invalid appliance_t with invalid rating. */
        types::appliance_t appliance7 = {
            id                  : 0,
            name                : "Lorem ipsum",
            uri                 : "The spice is vital to space travel",
            rating              : -4.6,
            duty_cycle          : 5,
            schedules_per_week  : 1,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>()
        };
        res = msg_set_appliance_send(this_messenger, appliance7, nullptr);
        if (res != response_code::MSG_SET_CONSTRAINT_VIOLATION) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_CONSTRAINT_VIOLATION) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_SET_APPLIANCE` messages with valid foreign keys.
     */
    bool test_handler_msg_set_appliance_fk_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Add valid auto_profile_t twice. */
        types::auto_profile_t auto_profile = {
            id      : 0,
            name    : "Lorem ipsum",
            profile : "I am Iron Man."
        };
        msg_set_auto_profile_send(this_messenger, auto_profile, nullptr);
        msg_set_auto_profile_send(this_messenger, auto_profile, nullptr);

        /* Add valid task_t twice. */
        types::task_t task = {
            id                  : 0,
            name                : "Lorem ipsum",
            start_time          : time_from_string("2020-02-20 02:02:02.000"),
            end_time            : time_from_string("2020-02-20 20:20:20.000"),
            auto_profile        : 0,
            is_user_declared    : true
        };
        msg_set_task_send(this_messenger, task, nullptr);
        msg_set_task_send(this_messenger, task, nullptr);

        /* Test adding a new valid appliance_t with valid foreign keys. */
        types::appliance_t appliance = {
            id                  : 0,
            name                : "Lorem ipsum",
            uri                 : "Don't be a stranger",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 2,
            tasks               : std::set<id_t>({0, 1, 2}),
            auto_profiles       : std::set<id_t>({0, 1, 2})
        };
        res = msg_set_appliance_send(this_messenger, appliance, nullptr);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt1 = nullptr;
            std::string stmt1 = "SELECT * FROM appliances WHERE name='" + appliance.name + "'";
            if (!this_instance->prepare_and_evaluate(stmt1, prepared_stmt1)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt1);
            }

            sqlite3_stmt* prepared_stmt2 = nullptr;
            std::string stmt2 = "SELECT * FROM appliances_tasks WHERE appliance_id=1 AND task_id=1";
            if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt2);
            }

            sqlite3_stmt* prepared_stmt3 = nullptr;
            std::string stmt3 = "SELECT * FROM appliances_tasks WHERE appliance_id=1 AND task_id=2";
            if (!this_instance->prepare_and_evaluate(stmt3, prepared_stmt3)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt3);
            }

            sqlite3_stmt* prepared_stmt4 = nullptr;
            std::string stmt4 =
                "SELECT * FROM appliances_auto_profiles WHERE appliance_id=1 AND auto_profile=1";
            if (!this_instance->prepare_and_evaluate(stmt4, prepared_stmt4)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt4);
            }

            sqlite3_stmt* prepared_stmt5 = nullptr;
            std::string stmt5 =
                "SELECT * FROM appliances_auto_profiles WHERE appliance_id=1 AND auto_profile=2";
            if (!this_instance->prepare_and_evaluate(stmt5, prepared_stmt5)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt5);
            }
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_SET_TASK` messages.
     */
    bool test_handler_msg_set_task_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        /* Test replacing a task_t before it exists. */
        types::task_t task1 = {
            id                  : 1,
            name                : "task",
            start_time          : time_from_string("2020-02-20 02:02:02.000"),
            end_time            : time_from_string("2020-02-20 20:20:20.000"),
            auto_profile        : 0,
            is_user_declared    : true,
            appliances          : std::set<id_t>()
        };
        res = msg_set_task_send(this_messenger, task1, nullptr);
        if (res != response_code::MSG_SET_REPLACE_NON_EXISTING) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_REPLACE_NON_EXISTING) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }


        /* 0 for an appliance id should be treated as if there was no appliance id. */
        std::vector<std::set<id_t>> appliances_vector = {
            std::set<id_t>(),
            std::set<id_t>({0, 0})
        };

        for (const auto& appliances : appliances_vector) {
            /* Test adding a new valid task_t without foreign keys. */
            types::task_t task2 = {
                id                  : 0,
                name                : "Lorem ipsum",
                start_time          : time_from_string("2020-02-20 02:02:02.000"),
                end_time            : time_from_string("2020-02-20 20:20:20.000"),
                auto_profile        : 0,
                is_user_declared    : true,
                appliances          : appliances
            };
            res = msg_set_task_send(this_messenger, task2, nullptr);
            if (res) {
                std::cout <<
                    "Query that should have been successful failed with code " + std::to_string(res) +
                    " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt;
                std::string stmt = "SELECT * FROM tasks WHERE name='" + task2.name + "'";

                if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                    return false;
                } else {
                    id_t id = sqlite3_column_int64(prepared_stmt, 0);
                    std::string name =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));
                    ptime start_time = from_iso_string(
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 2))
                    );
                    ptime end_time = from_iso_string(
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 3))
                    );
                    bool is_user_declared = !sqlite3_column_int(prepared_stmt, 5) ? false : true;

                    if (id == 0 || name != task2.name || start_time != task2.start_time ||
                        end_time != task2.end_time || is_user_declared != task2.is_user_declared) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else if (sqlite3_column_type(prepared_stmt, 4) != SQLITE_NULL) {
                        std::cout << "Unprovided foreign key was not stored as null in database.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt);
                    }
                }
            }


            /* Test replacing an existing task_t with valid values (without foreign keys). */
            types::task_t task3 = {
                id                  : 1,
                name                : "dolor sit amet",
                start_time          : time_from_string("2002-02-02 02:02:02.000"),
                end_time            : time_from_string("2002-02-02 20:20:20.000"),
                auto_profile        : 0,
                is_user_declared    : false,
                appliances          : appliances
            };
            res = msg_set_task_send(this_messenger, task3, nullptr);
            if (res) {
                std::cout <<
                    "Query that should have been successful failed with code " + std::to_string(res) +
                    " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt;
                std::string stmt = "SELECT * FROM tasks WHERE id=" + std::to_string(task3.id);

                if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                    return false;
                } else {
                    id_t id = sqlite3_column_int64(prepared_stmt, 0);
                    std::string name =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));
                    ptime start_time = from_iso_string(
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 2))
                    );
                    ptime end_time = from_iso_string(
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 3))
                    );
                    bool is_user_declared = !sqlite3_column_int(prepared_stmt, 5) ? false : true;

                    if (name == task2.name || start_time == task2.start_time ||
                        end_time == task2.end_time || is_user_declared == task2.is_user_declared) {
                        std::cout << "Item in database was not replaced with all new values.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else if ( id != task3.id || name != task3.name || start_time != task3.start_time ||
                                end_time != task3.end_time || is_user_declared != task3.is_user_declared) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else if (sqlite3_column_type(prepared_stmt, 4) != SQLITE_NULL) {
                        std::cout << "Unprovided foreign key was not stored as null in database.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt);
                    }
                }
            }
        }


        /* Test adding an invalid task_t with constraint violation (start_time = end_time). */
        types::task_t task4 = {
            id                  : 0,
            name                : "consetetur sadipscing elitr",
            start_time          : time_from_string("2020-02-20 02:00:00.000"),
            end_time            : time_from_string("2020-02-20 02:00:00.000"),
            auto_profile        : 0,
            is_user_declared    : true,
            appliances          : std::set<id_t>()
        };
        res = msg_set_task_send(this_messenger, task4, nullptr);
        if (res != response_code::MSG_SET_CONSTRAINT_VIOLATION) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_CONSTRAINT_VIOLATION) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        /* Test adding an invalid task_t with constraint violation (start_time > end_time). */
        types::task_t task5 = {
            id                  : 0,
            name                : "consetetur sadipscing elitr",
            start_time          : time_from_string("2020-02-20 20:00:00.000"),
            end_time            : time_from_string("2020-02-20 02:00:00.000"),
            auto_profile        : 0,
            is_user_declared    : true,
            appliances          : std::set<id_t>()
        };
        res = msg_set_task_send(this_messenger, task5, nullptr);
        if (res != response_code::MSG_SET_CONSTRAINT_VIOLATION) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_CONSTRAINT_VIOLATION) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }


        /* Test adding an invalid task_t with non-existing foreign keys (auto_profile). */
        types::task_t task6 = {
            id                  : 0,
            name                : "consetetur sadipscing elitr",
            start_time          : time_from_string("2020-02-20 02:02:02.000"),
            end_time            : time_from_string("2020-02-20 20:20:20.000"),
            auto_profile        : 5,
            is_user_declared    : true,
            appliances          : std::set<id_t>()
        };
        res = msg_set_task_send(this_messenger, task6, nullptr);
        if (res != response_code::MSG_SET_SQL_ERROR) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_SQL_ERROR) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        /* Test adding an invalid task_t with non-existing foreign keys (appliances). */
        types::task_t task7 = {
            id                  : 0,
            name                : "consetetur sadipscing elitr",
            start_time          : time_from_string("2020-02-20 02:02:02.000"),
            end_time            : time_from_string("2020-02-20 20:20:20.000"),
            auto_profile        : 0,
            is_user_declared    : true,
            appliances          : std::set<id_t>({4, 5})
        };
        res = msg_set_task_send(this_messenger, task7, nullptr);
        if (res != response_code::MSG_SET_SQL_ERROR) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_SQL_ERROR) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_SET_TASK` messages with valid foreign keys.
     */
    bool test_handler_msg_set_task_fk_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Add a new valid auto_profile_t. */
        types::auto_profile_t auto_profile = {
            id      : 0,
            name    : "Lorem ipsum",
            profile : "I am Iron Man."
        };
        msg_set_auto_profile_send(this_messenger, auto_profile, nullptr);

        /* Add new valid appliance_t twice. */
        types::appliance_t appliance = {
            id                  : 0,
            name                : "Lorem ipsum",
            uri                 : "There goes the last great American dynasty",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 3,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>()
        };
        msg_set_appliance_send(this_messenger, appliance, nullptr);
        msg_set_appliance_send(this_messenger, appliance, nullptr);

        /* Test adding a new valid task_t with valid foreign keys. */
        types::task_t task = {
            id                  : 0,
            name                : "Lorem ipsum",
            start_time          : time_from_string("2020-02-20 02:02:02.000"),
            end_time            : time_from_string("2020-02-20 20:20:20.000"),
            auto_profile        : 1,
            is_user_declared    : true,
            appliances          : std::set<id_t>({0, 1, 2})
        };
        res = msg_set_task_send(this_messenger, task, nullptr);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt1;
            std::string stmt = "SELECT * FROM tasks WHERE name='" + task.name + "'";

            if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt1)) {
                return false;
            } else {
                id_t auto_profile = sqlite3_column_int64(prepared_stmt1, 5);

                if (auto_profile != task.auto_profile) {
                    std::cout << "Foreign key was not handled correctly.\n";
                    sqlite3_finalize(prepared_stmt1);
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt1);
                }
            }

            sqlite3_stmt* prepared_stmt2 = nullptr;
            std::string stmt2 = "SELECT * FROM appliances_tasks WHERE appliance_id=1 AND task_id=1";
            if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt2);
            }

            sqlite3_stmt* prepared_stmt3 = nullptr;
            std::string stmt3 = "SELECT * FROM appliances_tasks WHERE appliance_id=2 AND task_id=1";
            if (!this_instance->prepare_and_evaluate(stmt3, prepared_stmt3)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt3);
            }
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_SET_AUTO_PROFILE` messages.
     */
    bool test_handler_msg_set_auto_profile_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Test replacing an auto_profile_t before it exists. */
        types::auto_profile_t auto_profile_1 = {
            id          : 1,
            name        : "auto_profile",
            profile     : "I am Iron Man.",
            appliances  : std::set<id_t>(),
            tasks       : std::set<id_t>()
        };
        res = msg_set_auto_profile_send(this_messenger, auto_profile_1, nullptr);
        if (res != response_code::MSG_SET_REPLACE_NON_EXISTING) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_REPLACE_NON_EXISTING) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }


        /* 0 for an appliance or task id should be treated as if there was no id. */
        std::vector<std::set<id_t>> ids_vector = {
            std::set<id_t>(),
            std::set<id_t>({0, 0})
        };

        for (const auto& ids : ids_vector) {
            /* Test adding a new valid auto_profile_t. */
            types::auto_profile_t auto_profile_2 = {
                id          : 0,
                name        : "Lorem ipsum",
                profile     : "I am Iron Man.",
                appliances  : ids,
                tasks       : ids
            };
            res = msg_set_auto_profile_send(this_messenger, auto_profile_2, nullptr);
            if (res) {
                std::cout <<
                    "Query that should have been successful failed with code " + std::to_string(res) +
                    " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt;
                std::string stmt =
                    "SELECT * FROM auto_profiles WHERE name='" + auto_profile_2.name + "'";

                if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                    return false;
                } else {
                    id_t id = sqlite3_column_int64(prepared_stmt, 0);
                    std::string name =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));
                    std::string profile =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 2));

                    if (id == 0 || name != auto_profile_2.name || profile != auto_profile_2.profile) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt);
                    }
                }
            }


            /* Test replacing an existing auto_profile_t with valid values. */
            types::auto_profile_t auto_profile_3 = {
                id          : 1,
                name        : "dolor sit amet",
                profile     : "And I'm Batman.",
                appliances  : ids,
                tasks       : ids
            };
            res = msg_set_auto_profile_send(this_messenger, auto_profile_3, nullptr);
            if (res) {
                std::cout <<
                    "Query that should have been successful failed with code " + std::to_string(res) +
                    " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt;
                std::string stmt =
                    "SELECT * FROM auto_profiles WHERE id=" + std::to_string(auto_profile_3.id);

                if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                    return false;
                } else {
                    id_t id = sqlite3_column_int64(prepared_stmt, 0);
                    std::string name =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));
                    std::string profile =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 2));

                    if (name == auto_profile_2.name || profile == auto_profile_2.profile) {
                        std::cout << "Item in database was not replaced with all new values.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else if ( id != auto_profile_3.id || name != auto_profile_3.name ||
                                profile != auto_profile_3.profile) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt);
                    }
                }
            }
        }


        /* Test adding an invalid auto_profile_t with non-existing foreign keys (appliances). */
        types::auto_profile_t auto_profile_4 = {
            id          : 1,
            name        : "consetetur sadipscing elitr",
            profile     : "And I'm Batman.",
            appliances  : std::set<id_t>({4, 5}),
            tasks       : std::set<id_t>()
        };
        res = msg_set_auto_profile_send(this_messenger, auto_profile_4, nullptr);
        if (res != response_code::MSG_SET_SQL_ERROR) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_SQL_ERROR) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }

        /*  TODO Test adding an invalid auto_profile_t with non-existing foreign keys (tasks).
            Before this can be tested, `hander_msg_set_auto_profile()` needs to implement setting
            the auto_profile id for all tasks, as that is when it notices if a task id is invalid. */
        // types::auto_profile_t auto_profile_5 = {
        //     id          : 1,
        //     name        : "consetetur sadipscing elitr",
        //     profile     : "And I'm Batman.",
        //     appliances  : std::vector<id_t>(),
        //     tasks       : std::vector<id_t>({4, 5})
        // };
        // res = msg_set_auto_profile_send(this_messenger, auto_profile_5, nullptr);
        // if (res != response_code::MSG_SET_SQL_ERROR) {
        //     std::cout <<
        //         "Query that should have failed with error " +
        //         std::to_string(response_code::MSG_SET_SQL_ERROR) + " returned " +
        //         std::to_string(res) + " instead.\n";
        //     return false;
        // }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_SET_AUTO_PROFILE` messages with valid foreign keys.
     */
    bool test_handler_msg_set_auto_profile_fk_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Add new valid appliance_t twice. */
        types::appliance_t appliance = {
            id                  : 0,
            name                : "Lorem ipsum",
            uri                 : "And when I felt like I was an old cardigan under someone's bed",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 1,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>()
        };
        msg_set_appliance_send(this_messenger, appliance, nullptr);
        msg_set_appliance_send(this_messenger, appliance, nullptr);

        /* Add new valid task_t twice. */
        types::task_t task = {
            id                  : 0,
            name                : "Lorem ipsum",
            start_time          : time_from_string("2020-02-20 02:02:02.000"),
            end_time            : time_from_string("2020-02-20 20:20:20.000"),
            auto_profile        : 0,
            is_user_declared    : true
        };
        msg_set_task_send(this_messenger, task, nullptr);
        msg_set_task_send(this_messenger, task, nullptr);

        /* Test adding a new valid auto_profile_t with valid foreign keys. */
        types::auto_profile_t auto_profile = {
            id          : 0,
            name        : "Lorem ipsum",
            profile     : "I am Iron Man.",
            appliances  : std::set<id_t>({0, 1, 2}),
            tasks       : std::set<id_t>({0, 1, 2})
        };
        res = msg_set_auto_profile_send(this_messenger, auto_profile, nullptr);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt1 = nullptr;
            std::string stmt1 =
                "SELECT * FROM appliances_auto_profiles WHERE appliance_id=1 AND auto_profile=1";
            if (!this_instance->prepare_and_evaluate(stmt1, prepared_stmt1)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt1);
            }

            sqlite3_stmt* prepared_stmt2 = nullptr;
            std::string stmt2 =
                "SELECT * FROM appliances_auto_profiles WHERE appliance_id=2 AND auto_profile=1";
            if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt2);
            }

            /* TODO Test that auto_profile id was set for all tasks. */
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_SET_ENERGY_CONSUMPTION` messages.
     */
    bool test_handler_msg_set_energy_consumption_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Test adding new valid energy_consumption_t without foreign keys and with valid start_times. */
        std::vector<std::string> time_strings_1 = {
            "2020-02-20 20:00:00.000",
            "2020-02-20 20:15:00.000",
            "2020-02-20 20:30:00.000",
            "2020-02-20 20:45:00.000"
        };
        double energy1 = 8.7;
        for (const auto& time_string : time_strings_1) {
            types::energy_consumption_t energy_consumption_1 = {
                time            : time_from_string(time_string),
                appliance_id    : 0,
                energy          : energy1
            };
            res = msg_set_energy_consumption_send(this_messenger, energy_consumption_1);
            if (res) {
                std::cout <<
                    "Query that should have been successful failed with code " + std::to_string(res) +
                    " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt;
                std::string stmt =
                    "SELECT * FROM energy_consumption WHERE time='" +
                    boost::posix_time::to_iso_string(energy_consumption_1.time) + "'";

                if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                    return false;
                } else {
                    ptime time = from_iso_string(
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 0))
                    );
                    id_t appliance_id = sqlite3_column_int64(prepared_stmt, 1);
                    double energy = sqlite3_column_double(prepared_stmt, 2);

                    if (time != energy_consumption_1.time || energy != energy_consumption_1.energy ||
                        appliance_id != energy_consumption_1.appliance_id) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt);
                    }
                }
            }
        }


        /* Test replacing an existing energy_consumption_t with valid values (without foreign keys). */
        types::energy_consumption_t energy_consumption_2 = {
            time            : time_from_string(time_strings_1.at(0)),
            appliance_id    : 0,
            energy          : 9.15
        };
        res = msg_set_energy_consumption_send(this_messenger, energy_consumption_2);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt;
            std::string stmt =
                "SELECT * FROM energy_consumption WHERE time='" +
                boost::posix_time::to_iso_string(energy_consumption_2.time) + "'";

            if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                return false;
            } else {
                ptime time = from_iso_string(
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 0))
                );
                id_t appliance_id = sqlite3_column_int64(prepared_stmt, 1);
                double energy = sqlite3_column_double(prepared_stmt, 2);

                if (energy == energy1) {
                    std::cout << "Item in database was not replaced with all new values.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else if ( time != energy_consumption_2.time || energy != energy_consumption_2.energy ||
                            appliance_id != energy_consumption_2.appliance_id) {
                    std::cout << "Item in database is not identical to the one sent.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt);
                }
            }
        }


        /* Test adding new energy_consumption_t with invalid start_times. */
        std::vector<std::string> time_strings_3 = {
            "2020-02-20 20:00:00.123",
            "2020-02-20 20:00:20.000",
            "2020-02-20 20:20:00.000"
        };
        for (const auto& time_string : time_strings_3) {
            types::energy_consumption_t energy_consumption_3 = {
                time            : time_from_string(time_string),
                appliance_id    : 0,
                energy          : 8.7
            };
            res = msg_set_energy_consumption_send(this_messenger, energy_consumption_3);
            if (res != response_code::MSG_SET_CONSTRAINT_VIOLATION) {
                std::cout <<
                    "Query that should have failed with error " +
                    std::to_string(response_code::MSG_SET_CONSTRAINT_VIOLATION) + " returned " +
                    std::to_string(res) + " instead.\n";
                return false;
            }
        }


        /* Test adding an invalid energy_consumption_t with non-existing foreign keys. */
        types::energy_consumption_t energy_consumption_4 = {
            time            : time_from_string(time_strings_1.at(0)),
            appliance_id    : 1,
            energy          : 9.15
        };
        res = msg_set_energy_consumption_send(this_messenger, energy_consumption_4);
        if (res != response_code::MSG_SET_SQL_ERROR) {
            std::cout <<
                "Query that should have failed with error " +
                std::to_string(response_code::MSG_SET_SQL_ERROR) + " returned " +
                std::to_string(res) + " instead.\n";
            return false;
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_SET_ENERGY_CONSUMPTION` messages with valid foreign keys.
     */
    bool test_handler_msg_set_energy_consumption_fk_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Add a new valid appliance_t. */
        types::appliance_t appliance = {
            id                  : 0,
            name                : "Lorem ipsum",
            uri                 : "You put me on and said I was your favorite",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 0,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>()
        };
        msg_set_appliance_send(this_messenger, appliance, nullptr);

        /* Test adding new valid energy_consumption_t with valid foreign keys. */
        types::energy_consumption_t energy_consumption = {
            time            : time_from_string("2020-02-20 20:00:00.000"),
            appliance_id    : 1,
            energy          : 8.7
        };
        res = msg_set_energy_consumption_send(this_messenger, energy_consumption);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt;
            std::string stmt =
                "SELECT * FROM energy_consumption WHERE time='" +
                boost::posix_time::to_iso_string(energy_consumption.time) + "'";

            if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                return false;
            } else {
                id_t appliance_id = sqlite3_column_int64(prepared_stmt, 1);

                if (appliance_id != energy_consumption.appliance_id) {
                    std::cout << "Foreign key was not handled correctly.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt);
                }
            }
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_SET_ENERGY_PRODUCTION` messages.
     */
    bool test_handler_msg_set_energy_production_(
        messenger* this_messenger, hems_storage_test* this_instance
    ) {
        int res;

        /* Test adding new valid energy_production_t with valid start_times. */
        std::vector<std::string> time_strings_1 = {
            "2020-02-20 20:00:00.000",
            "2020-02-20 20:15:00.000",
            "2020-02-20 20:30:00.000",
            "2020-02-20 20:45:00.000"
        };
        double energy1 = 8.7;
        for (const auto& time_string : time_strings_1) {
            types::energy_production_t energy_production_1 = {
                time    : time_from_string(time_string),
                energy  : energy1
            };
            res = msg_set_energy_production_send(this_messenger, energy_production_1);
            if (res) {
                std::cout <<
                    "Query that should have been successful failed with code " + std::to_string(res) +
                    " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt;
                std::string stmt =
                    "SELECT * FROM energy_production WHERE time='" +
                    boost::posix_time::to_iso_string(energy_production_1.time) + "'";

                if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                    return false;
                } else {
                    ptime time = from_iso_string(
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 0))
                    );
                    double energy = sqlite3_column_double(prepared_stmt, 1);

                    if (time != energy_production_1.time || energy != energy_production_1.energy) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt);
                    }
                }
            }
        }


        /* Test replacing an existing energy_consumption_t with valid values. */
        types::energy_production_t energy_production_2 = {
            time    : time_from_string(time_strings_1.at(0)),
            energy  : 9.15
        };
        res = msg_set_energy_production_send(this_messenger, energy_production_2);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt;
            std::string stmt =
                "SELECT * FROM energy_production WHERE time='" +
                boost::posix_time::to_iso_string(energy_production_2.time) + "'";

            if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                return false;
            } else {
                ptime time = from_iso_string(
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 0))
                );
                double energy = sqlite3_column_double(prepared_stmt, 1);

                if (energy == energy1) {
                    std::cout << "Item in database was not replaced with all new values.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else if (time != energy_production_2.time || energy != energy_production_2.energy) {
                    std::cout << "Item in database is not identical to the one sent.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt);
                }
            }
        }


        /* Test adding new energy_production_t with invalid start_times. */
        std::vector<std::string> time_strings_3 = {
            "2020-02-20 20:00:00.123",
            "2020-02-20 20:00:20.000",
            "2020-02-20 20:20:00.000"
        };
        for (const auto& time_string : time_strings_3) {
            types::energy_production_t energy_production_3 = {
                time    : time_from_string(time_string),
                energy  : 8.7
            };
            res = msg_set_energy_production_send(this_messenger, energy_production_3);
            if (res != response_code::MSG_SET_CONSTRAINT_VIOLATION) {
                std::cout <<
                    "Query that should have failed with error " +
                    std::to_string(response_code::MSG_SET_CONSTRAINT_VIOLATION) + " returned " +
                    std::to_string(res) + " instead.\n";
                return false;
            }
        }


        return true;
    }

    /**
     * @brief   Test the handler for `MSG_SET_WEATHER` messages.
     */
    bool test_handler_msg_set_weather_(messenger* this_messenger, hems_storage_test* this_instance) {
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

        /* Test adding new valid weather_t with valid time. */
        types::weather_t weather1 = {
            time        : time_from_string("2020-02-20 20:00:00.000"),
            station     : 1,
            temperature : 25.5,
            humidity    : 30,
            pressure    : 1000,
            cloud_cover : 70,
            radiation   : 765.43
        };
        res = msg_set_weather_send(this_messenger, weather1);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt;
            std::string stmt =
                "SELECT * FROM weather WHERE time='" + boost::posix_time::to_iso_string(weather1.time) + "'";

            if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                return false;
            } else {
                ptime time = from_iso_string(
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 0))
                );
                id_t station = sqlite3_column_int64(prepared_stmt, 1);
                double temperature = sqlite3_column_double(prepared_stmt, 2);
                unsigned int humidity = sqlite3_column_int(prepared_stmt, 3);
                double pressure = sqlite3_column_double(prepared_stmt, 4);
                unsigned int cloud_cover = sqlite3_column_int(prepared_stmt, 5);
                double radiation = sqlite3_column_double(prepared_stmt, 6);

                if (time != weather1.time || station != weather1.station ||
                    temperature != weather1.temperature || humidity != weather1.humidity ||
                    pressure != weather1.pressure || cloud_cover != weather1.cloud_cover ||
                    radiation != weather1.radiation) {
                    std::cout << "Item in database is not identical to the one sent.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt);
                }
            }
        }


        /* Test replacing an existing weather_t with valid values. */
        types::weather_t weather2 = {
            time        : weather1.time,
            station     : weather1.station,
            temperature : 15.89,
            humidity    : 19,
            pressure    : 802,
            cloud_cover : 17,
            radiation   : 196
        };
        res = msg_set_weather_send(this_messenger, weather2);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt;
            std::string stmt =
                "SELECT * FROM weather WHERE time='" + boost::posix_time::to_iso_string(weather2.time) + "'";

            if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                return false;
            } else {
                ptime time = from_iso_string(
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 0))
                );
                id_t station = sqlite3_column_int64(prepared_stmt, 1);
                double temperature = sqlite3_column_double(prepared_stmt, 2);
                unsigned int humidity = sqlite3_column_int(prepared_stmt, 3);
                double pressure = sqlite3_column_double(prepared_stmt, 4);
                unsigned int cloud_cover = sqlite3_column_int(prepared_stmt, 5);
                double radiation = sqlite3_column_double(prepared_stmt, 6);

                if (temperature == weather1.temperature || humidity == weather1.humidity ||
                    pressure == weather1.pressure || cloud_cover == weather1.cloud_cover ||
                    radiation == weather1.radiation) {
                    std::cout << "Item in database was not replaced with all new values.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else if ( time != weather2.time || station != weather2.station ||
                            temperature != weather2.temperature || humidity != weather2.humidity ||
                            pressure != weather2.pressure || cloud_cover != weather2.cloud_cover ||
                            radiation != weather2.radiation) {
                    std::cout << "Item in database is not identical to the one sent.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt);
                }
            }
        }


        /* Test adding new weather_t with invalid time, humidity, cloud cover and radiation. */
        types::weather_t weather3 = weather2;
        weather3.time = time_from_string("2020-02-20 20:00:00.123");

        types::weather_t weather4 = weather2;
        weather4.time = time_from_string("2020-02-20 20:00:20.000");

        types::weather_t weather5 = weather2;
        weather5.time = time_from_string("2020-02-20 20:33:00.000");
        weather5.station = 2;

        types::weather_t weather6 = weather2;
        weather6.station = 2;
        weather6.humidity = 101;

        types::weather_t weather7 = weather2;
        weather7.station = 2;
        weather7.humidity = -1;

        types::weather_t weather8 = weather2;
        weather8.station = 2;
        weather8.cloud_cover = 101;

        types::weather_t weather9 = weather2;
        weather9.station = 2;
        weather9.cloud_cover = -1;

        types::weather_t weather10 = weather2;
        weather10.station = 2;
        weather10.radiation = -1;

        std::vector<types::weather_t> weather_vector_11 = {
            weather3, weather4, weather5, weather6, weather7, weather8, weather9, weather10
        };
        for (const auto& weather3 : weather_vector_11) {
            res = msg_set_weather_send(this_messenger, weather3);
            if (res != response_code::MSG_SET_CONSTRAINT_VIOLATION) {
                std::cout <<
                    "Query that should have failed with error " +
                    std::to_string(response_code::MSG_SET_CONSTRAINT_VIOLATION) + " returned " +
                    std::to_string(res) + " instead.\n";
                return false;
            }
        }


        return true;
    }

    /**
     * @brief   Test the handlers for `MSG_SET` messages with payloads containing quote and double
     *          quote characters.
     */
    bool test_handler_msg_set_quote_chars_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        /* Test adding new valid appliance_t with quote and double quote characters in the name. */
        types::appliance_t appliance = {
            id                  : 0,
            name                : "\"'Tis the season to be jolly\"",
            uri                 : "Na-na na-na na-na na-na",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 1,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>()
        };
        res = msg_set_appliance_send(this_messenger, appliance, nullptr);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt;
            std::string stmt = "SELECT * FROM appliances WHERE id=1";

            if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                return false;
            } else {
                std::string name =
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));

                if (name != appliance.name) {
                    std::cout <<
                        "Field with quote and double quote characters was not written or read "
                        "correctly.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt);
                }
            }
        }


        /* Test adding new valid task_t with quote and double quote characters in the name. */
        types::task_t task = {
            id                  : 0,
            name                : "\"'Tis the season to be jolly\"",
            start_time          : time_from_string("2020-02-20 02:02:02.000"),
            end_time            : time_from_string("2020-02-20 20:20:20.000"),
            auto_profile        : 0,
            is_user_declared    : true
        };
        res = msg_set_task_send(this_messenger, task, nullptr);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt;
            std::string stmt = "SELECT * FROM tasks WHERE id=1";

            if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                return false;
            } else {
                std::string name =
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));

                if (name != appliance.name) {
                    std::cout <<
                        "Field with quote and double quote characters was not written or read "
                        "correctly.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt);
                }
            }
        }


        /* Test adding new valid auto_profile_t with quote and double quote characters in the name. */
        types::auto_profile_t auto_profile = {
            id      : 0,
            name    : "\"'Tis the season to be jolly\"",
            profile : "Isn't aren't haven't \"text\""
        };
        res = msg_set_auto_profile_send(this_messenger, auto_profile, nullptr);
        if (res) {
            std::cout <<
                "Query that should have been successful failed with code " + std::to_string(res) +
                " instead.\n";
            return false;
        } else {
            sqlite3_stmt* prepared_stmt;
            std::string stmt = "SELECT * FROM auto_profiles WHERE id=1";

            if (!this_instance->prepare_and_evaluate(stmt, prepared_stmt)) {
                return false;
            } else {
                std::string name =
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));
                std::string profile =
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 2));

                if (name != appliance.name || profile != auto_profile.profile) {
                    std::cout <<
                        "Field with quote and double quote characters was not written or read "
                        "correctly.\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt);
                }
            }
        }


        return true;
    }

    /**
     * @brief   Test the handlers for `MSG_SET` where responses can be received.
     */
    bool test_handler_msg_set_responses_(messenger* this_messenger, hems_storage_test* this_instance) {
        std::string serialized_response;

        /* Test appliance_t. */
        types::appliance_t appliance = {
            id                  : 0,
            name                : "Lorem ipsum",
            uri                 : "BATMAN!",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 3,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>()
        };
        for (unsigned int i=1; i<3; i++) {
            msg_set_appliance_send(this_messenger, appliance, &serialized_response);
            if (serialized_response.empty()) {
                std::cout << "No response received.\n";
                return false;
            }
            messages::storage::msg_set_response response =
                this_messenger->deserialize<messages::storage::msg_set_response>(serialized_response);
            if (response.id != i) {
                std::cout << "Response did not return expected id.\n";
                return false;
            }
            serialized_response.clear();
        }

        /* Test task_t. */
        types::task_t task = {
            id                  : 0,
            name                : "Lorem ipsum",
            start_time          : time_from_string("2020-02-20 02:02:02.000"),
            end_time            : time_from_string("2020-02-20 20:20:20.000"),
            auto_profile        : 0,
            is_user_declared    : true
        };
        for (unsigned int i=1; i<3; i++) {
            msg_set_task_send(this_messenger, task, &serialized_response);
            if (serialized_response.empty()) {
                std::cout << "No response received.\n";
                return false;
            }
            messages::storage::msg_set_response response =
                this_messenger->deserialize<messages::storage::msg_set_response>(serialized_response);
            if (response.id != i) {
                std::cout << "Response did not return expected id.\n";
                return false;
            }
            serialized_response.clear();
        }

        /* Test auto_profile_t. */
        types::auto_profile_t auto_profile = {
            id      : 0,
            name    : "Lorem ipsum",
            profile : "I am Iron Man."
        };
        for (unsigned int i=1; i<3; i++) {
            msg_set_auto_profile_send(this_messenger, auto_profile, &serialized_response);
            if (serialized_response.empty()) {
                std::cout << "No response received.\n";
                return false;
            }
            messages::storage::msg_set_response response =
                this_messenger->deserialize<messages::storage::msg_set_response>(serialized_response);
            if (response.id != i) {
                std::cout << "Response did not return expected id.\n";
                return false;
            }
            serialized_response.clear();
        }


        return true;
    }


    enum msg_set_test_types {
        APPLIANCE, APPLIANCE_T_FK, TASK, TASK_T_FK, AUTO_PROFILE, AUTO_PROFILE_FK, ENERGY_CONSUMPTION,
        ENERGY_CONSUMPTION_FK, ENERGY_PRODUCTION, WEATHER, QUOTE_CHARS, RESPONSES
    };

    bool test_handler_msg_set(msg_set_test_types test_type) {
        logger::this_logger = new dummy_logger();

        char db_path[11];
        do {
            generate_random_file_name(db_path);
        } while (boost::filesystem::exists(db_path));

        /* Delete message queues to remove junk messages from previous runs. */
        mq_unlink(messenger::mq_names.at(modules::type::STORAGE).c_str());
        mq_unlink(messenger::mq_names.at(modules::type::LAUNCHER).c_str());

        /* Create the message queue for the Data Storage Module so that its constructor does not fail. */
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
            case msg_set_test_types::APPLIANCE:
                success = test_handler_msg_set_appliance_(this_messenger, this_instance);
                break;
            case msg_set_test_types::APPLIANCE_T_FK:
                success = test_handler_msg_set_appliance_fk_(this_messenger, this_instance);
                break;
            case msg_set_test_types::TASK:
                success = test_handler_msg_set_task_(this_messenger, this_instance);
                break;
            case msg_set_test_types::TASK_T_FK:
                success = test_handler_msg_set_task_fk_(this_messenger, this_instance);
                break;
            case msg_set_test_types::AUTO_PROFILE:
                success = test_handler_msg_set_auto_profile_(this_messenger, this_instance);
                break;
            case msg_set_test_types::AUTO_PROFILE_FK:
                success = test_handler_msg_set_auto_profile_fk_(this_messenger, this_instance);
                break;
            case msg_set_test_types::ENERGY_CONSUMPTION:
                success = test_handler_msg_set_energy_consumption_(this_messenger, this_instance);
                break;
            case msg_set_test_types::ENERGY_CONSUMPTION_FK:
                success = test_handler_msg_set_energy_consumption_fk_(this_messenger, this_instance);
                break;
            case msg_set_test_types::ENERGY_PRODUCTION:
                success = test_handler_msg_set_energy_production_(this_messenger, this_instance);
                break;
            case msg_set_test_types::WEATHER:
                success = test_handler_msg_set_weather_(this_messenger, this_instance);
                break;
            case msg_set_test_types::QUOTE_CHARS:
                success = test_handler_msg_set_quote_chars_(this_messenger, this_instance);
                break;
            case msg_set_test_types::RESPONSES:
                success = test_handler_msg_set_responses_(this_messenger, this_instance);
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

    static inline bool test_handler_msg_set_appliance() {
        return test_handler_msg_set(msg_set_test_types::APPLIANCE);
    }

    static inline bool test_handler_msg_set_appliance_fk() {
        return test_handler_msg_set(msg_set_test_types::APPLIANCE_T_FK);
    }

    static inline bool test_handler_msg_set_task() {
        return test_handler_msg_set(msg_set_test_types::TASK);
    }

    static inline bool test_handler_msg_set_task_fk() {
        return test_handler_msg_set(msg_set_test_types::TASK_T_FK);
    }

    static inline bool test_handler_msg_set_auto_profile() {
        return test_handler_msg_set(msg_set_test_types::AUTO_PROFILE);
    }

    static inline bool test_handler_msg_set_auto_profile_fk() {
        return test_handler_msg_set(msg_set_test_types::AUTO_PROFILE_FK);
    }

    static inline bool test_handler_msg_set_energy_consumption() {
        return test_handler_msg_set(msg_set_test_types::ENERGY_CONSUMPTION);
    }

    static inline bool test_handler_msg_set_energy_consumption_fk() {
        return test_handler_msg_set(msg_set_test_types::ENERGY_CONSUMPTION_FK);
    }

    static inline bool test_handler_msg_set_energy_production() {
        return test_handler_msg_set(msg_set_test_types::ENERGY_PRODUCTION);
    }

    static inline bool test_handler_msg_set_weather() {
        return test_handler_msg_set(msg_set_test_types::WEATHER);
    }

    static inline bool test_handler_msg_set_quote_chars() {
        return test_handler_msg_set(msg_set_test_types::QUOTE_CHARS);
    }

    static inline bool test_handler_msg_set_responses() {
        return test_handler_msg_set(msg_set_test_types::RESPONSES);
    }

}}}

int main() {
    return run_tests({
        {
            "01 Storage: Message handler test for MSG_SET_APPLIANCE",
            &hems::modules::storage::test_handler_msg_set_appliance
        },
        {
            "02 Storage: Message handler test for MSG_SET_APPLIANCE (valid foreign keys)",
            &hems::modules::storage::test_handler_msg_set_appliance_fk
        },
        {
            "03 Storage: Message handler test for MSG_SET_TASK",
            &hems::modules::storage::test_handler_msg_set_task
        },
        {
            "04 Storage: Message handler test for MSG_SET_TASK (valid foreign keys)",
            &hems::modules::storage::test_handler_msg_set_task_fk
        },
        {
            "05 Storage: Message handler test for MSG_SET_AUTO_PROFILE",
            &hems::modules::storage::test_handler_msg_set_auto_profile
        },
        {
            "06 Storage: Message handler test for MSG_SET_AUTO_PROFILE (valid foreign keys)",
            &hems::modules::storage::test_handler_msg_set_auto_profile_fk
        },
        {
            "07 Storage: Message handler test for MSG_SET_ENERGY_CONSUMPTION",
            &hems::modules::storage::test_handler_msg_set_energy_consumption
        },
        {
            "08 Storage: Message handler test for MSG_SET_ENERGY_CONSUMPTION (valid foreign keys)",
            &hems::modules::storage::test_handler_msg_set_energy_consumption_fk
        },
        {
            "09 Storage: Message handler test for MSG_SET_ENERGY_PRODUCTION",
            &hems::modules::storage::test_handler_msg_set_energy_production
        },
        {
            "10 Storage: Message handler test for MSG_SET_WEATHER",
            &hems::modules::storage::test_handler_msg_set_weather
        },
        {
            "11 Storage: Message handler test for MSG_SET messages (quote and double quote characters)",
            &hems::modules::storage::test_handler_msg_set_quote_chars
        },
        {
            "12 Storage: Message handler test for MSG_SET responses",
            &hems::modules::storage::test_handler_msg_set_responses
        }
    });
}
