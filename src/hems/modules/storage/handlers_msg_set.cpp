/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Data Storage Module.
 * This module is responsible for managing access to data storage for all other modules.
 * Whenever other modules need to read or write measurements or other data, they can issue messages
 * to the Data Storage Module.
 */

#include <numeric>

#include <boost/algorithm/string/replace.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

#include "hems/modules/storage/storage.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/storage.h"

#include "sqlite/sqlite3.h"

namespace hems { namespace modules { namespace storage {

    using namespace hems::messages::storage;

    int handler_wrapper_msg_set_appliance(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_set_appliance(ia, oa);
    }

    int handler_wrapper_msg_set_task(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_set_task(ia, oa);
    }

    int handler_wrapper_msg_set_auto_profile(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_set_auto_profile(ia, oa);
    }

    int handler_wrapper_msg_set_energy_consumption(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_set_energy_consumption(ia, oa);
    }

    int handler_wrapper_msg_set_energy_production(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_set_energy_production(ia, oa);
    }

    int handler_wrapper_msg_set_weather(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_set_weather(ia, oa);
    }


    int hems_storage::handler_msg_set_with_id(
        types::id_t& new_id,
        std::string& stmt1, std::string& stmt2, std::string& stmt3, std::string& stmt4
    ) {
        int code;
        char* errmsg = nullptr; /*  It's important to initialize this to nullptr or `sqlite3_free()`
                                    will fail if `sqlite3_malloc()` was not previously called. */

        if (!db_begin()) {
            return response_code::MSG_SET_SQL_ERROR;
        }

        if (new_id) {
            /* Replace an existing entry. */
            if (sqlite3_exec(db_connection, stmt1.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
                logger::this_logger->log(
                    "Error replacing an entry: '" + stmt1 + "'. The error was: " + errmsg,
                    logger::level::ERR
                );
                code = response_code::MSG_SET_SQL_ERROR;
            } else if (!sqlite3_changes(db_connection)) {
                logger::this_logger->log(
                    "Attempted to replace a non-existing entry: '" + stmt1 + "'.",
                    logger::level::ERR
                );
                code = response_code::MSG_SET_REPLACE_NON_EXISTING;
            } else {
                logger::this_logger->log(
                    "Successfully replaced an entry: '" + stmt1 + "'.", logger::level::LOG
                );

                /* Delete all existing entries in the compound tables for the id. */
                if (sqlite3_exec(db_connection, stmt3.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
                    logger::this_logger->log(
                        "Error deleting entries: '" + stmt3 + "'. The error was: " + errmsg,
                        logger::level::ERR
                    );
                    code = response_code::MSG_SET_SQL_ERROR;
                } else {
                    logger::this_logger->log(
                        "Successfully deleted entries: '" + stmt3 + "'.", logger::level::LOG
                    );
                    code = response_code::SUCCESS;
                }
            }
        } else {
            /* Insert a new entry. */
            if (sqlite3_exec(db_connection, stmt2.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
                logger::this_logger->log(
                    "Error adding a new entry: '" + stmt2 + "'. The error was: " + errmsg,
                    logger::level::ERR
                );
                code = response_code::MSG_SET_SQL_ERROR;
            } else {
                logger::this_logger->log(
                    "Successfully added a new entry: '" + stmt2 + "'.", logger::level::LOG
                );
                code = response_code::SUCCESS;
            }
            new_id = static_cast<id_t>(sqlite3_last_insert_rowid(db_connection));
        }

        /*  Insert new entries into the compound tables for the id. */
        if (stmt4.size()) {
            boost::replace_all(stmt4, "0", std::to_string(new_id));
            if (sqlite3_exec(db_connection, stmt4.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
                logger::this_logger->log(
                    "Error adding new entries: '" + stmt4 + "'. The error was: " + errmsg,
                    logger::level::ERR
                );
                code = response_code::MSG_SET_SQL_ERROR;
            } else {
                logger::this_logger->log(
                    "Successfully added new entries: '" + stmt4 + "'.", logger::level::LOG
                );
                code = response_code::SUCCESS;
            }
        }

        if (code != response_code::SUCCESS) {
            hems_storage::db_commit(false);
        } else {
            hems_storage::db_commit(true);
        }

        sqlite3_free(errmsg);
        return code;
    };

    int hems_storage::handler_msg_set_without_id(std::string& stmt1, std::string& stmt2, std::string& stmt3) {
        sqlite3_stmt* prepared_stmt;
        int errcode, code;
        char* errmsg = nullptr; /*  It's important to initialize this to nullptr or `sqlite3_free()`
                                    will fail if `sqlite3_malloc()` was not previously called. */

        if (!db_begin()) {
            return response_code::MSG_SET_SQL_ERROR;
        }

        errcode = sqlite3_prepare_v2(
            db_connection, stmt1.c_str(), -1, &prepared_stmt, nullptr
        );
        if (errcode != SQLITE_OK) {
            logger::this_logger->log(
                "Error preparing a statement: '" + stmt1 + "'. The error was: " +
                    sqlite3_errstr(errcode),
                logger::level::ERR
            );
            hems_storage::db_commit(false);
            return response_code::MSG_SET_SQL_ERROR;
        }
        if ((errcode = sqlite3_step(prepared_stmt)) != SQLITE_ROW) {
            logger::this_logger->log(
                "Error evaluating a statement: '" + stmt1 + "'. The error was: " +
                    sqlite3_errstr(errcode),
                logger::level::ERR
            );
            hems_storage::db_commit(false);
            return response_code::MSG_SET_SQL_ERROR;
        }

        int num_entries = sqlite3_column_int(prepared_stmt, 0);
        sqlite3_finalize(prepared_stmt);
        if (num_entries == 1) {
            /* Replace an existing entry. */
            if (sqlite3_exec(db_connection, stmt2.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
                logger::this_logger->log(
                    "Error replacing an entry: '" + stmt2 + "'. The error was: " + errmsg,
                    logger::level::ERR
                );
                code = response_code::MSG_SET_SQL_ERROR;
            } else if (!sqlite3_changes(db_connection)) {
                logger::this_logger->log(
                    "Attempted to replace a non-existing entry: '" + stmt1 + "'.",
                    logger::level::ERR
                );
                code = response_code::MSG_SET_REPLACE_NON_EXISTING;
            } else {
                logger::this_logger->log(
                    "Successfully replaced an entry: '" + stmt2 + "'.", logger::level::LOG
                );
                code = response_code::SUCCESS;
            }
        } else if (num_entries == 0) {
            /* Insert a new entry. */
            if (sqlite3_exec(db_connection, stmt3.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
                logger::this_logger->log(
                    "Error adding a new entry: '" + stmt3 + "'. The error was: " + errmsg,
                    logger::level::ERR
                );
                code = response_code::MSG_SET_SQL_ERROR;
            } else {
                logger::this_logger->log(
                    "Successfully added a new entry: '" + stmt3 + "'.", logger::level::LOG
                );
                code = response_code::SUCCESS;
            }
        } else {
            /* This REALLY shouldn't happen. */
            logger::this_logger->log(
                "ERROR: '" + stmt1 + "' returned more than one entry - what is going on???",
                logger::level::ERR
            );
            code = response_code::MSG_SET_FATAL_DUPLICATE;
        }

        if (code != response_code::SUCCESS) {
            hems_storage::db_commit(false);
        } else {
            hems_storage::db_commit(true);
        }

        sqlite3_free(errmsg);
        return code;
    };


    int hems_storage::handler_msg_set_appliance(text_iarchive& ia, text_oarchive* oa) {
        msg_set_appliance_request msg;
        ia >> msg;
        appliance_t& entry = msg.appliance;

        if (entry.rating < 0) {
            logger::this_logger->log(
                "Invalid value provided for rating: Must be at least 0 but was " +
                    std::to_string(entry.rating),
                logger::level::ERR
            );
            return response_code::MSG_SET_CONSTRAINT_VIOLATION;
        }

        boost::replace_all(entry.name, "'", "''");
        boost::replace_all(entry.uri, "'", "''");

        std::string stmt1 =
            "UPDATE appliances "
            "SET "
                "name='" + entry.name + "', "
                "uri='" + entry.uri + "', "
                "rating=" + std::to_string(entry.rating) + ", "
                "duty_cycle=" + std::to_string(entry.duty_cycle) + ", "
                "schedules_per_week=" + std::to_string(entry.schedules_per_week) + " ";
            "WHERE id=" + std::to_string(entry.id);

        std::string stmt2 =
            "INSERT INTO appliances (name, uri, rating, duty_cycle, schedules_per_week) "
            "VALUES ("
                "'" + entry.name + "', " +
                "'" + entry.uri + "', " +
                std::to_string(entry.rating) + ", " +
                std::to_string(entry.duty_cycle) + ", " +
                std::to_string(entry.schedules_per_week) +
            ")";

        std::string stmt3 =
            "DELETE FROM appliances_tasks WHERE appliance_id=" + std::to_string(entry.id) + "; " +
            "DELETE FROM appliances_auto_profiles WHERE appliance_id=" + std::to_string(entry.id);

        std::string stmt4;
        if (entry.tasks.size() && std::accumulate(entry.tasks.begin(), entry.tasks.end(), 0)) {
            stmt4 += "INSERT INTO appliances_tasks (appliance_id, task_id) VALUES ";
            for (const auto& task : entry.tasks) {
                if (!task) {
                    continue;
                }
                stmt4 += "(" + std::to_string(entry.id) + ", " + std::to_string(task) + "), ";
            }
            stmt4.pop_back();
            stmt4.pop_back();
            stmt4 += "; ";
        }
        if (entry.auto_profiles.size() &&
            std::accumulate(entry.auto_profiles.begin(), entry.auto_profiles.end(), 0)) {
            stmt4 += "INSERT INTO appliances_auto_profiles (appliance_id, auto_profile) VALUES ";
            for (const auto& auto_profile : entry.auto_profiles) {
                if (!auto_profile) {
                    continue;
                }
                stmt4 += "(" + std::to_string(entry.id) + ", " + std::to_string(auto_profile) + "), ";
            }
            stmt4.pop_back();
            stmt4.pop_back();
        }

        int code = hems_storage::handler_msg_set_with_id(entry.id, stmt1, stmt2, stmt3, stmt4);
        if (oa != nullptr) {
            /* Prepare response message containing the id of the new entry. */
            msg_set_response response {
                id : entry.id
            };
            *oa << response;
        }
        return code;
    }

    int hems_storage::handler_msg_set_task(text_iarchive& ia, text_oarchive* oa) {
        msg_set_task_request msg;
        ia >> msg;
        task_t& entry = msg.task;

        if (entry.end_time <= entry.start_time) {
            logger::this_logger->log(
                "Invalid value provided for end_time (" +
                    boost::posix_time::to_iso_string(entry.end_time) + 
                    "): Must be greater than start_time (" +
                    boost::posix_time::to_iso_string(entry.start_time) + ")",
                logger::level::ERR
            );
            return response_code::MSG_SET_CONSTRAINT_VIOLATION;
        }

        boost::replace_all(entry.name, "'", "''");

        std::string stmt1 =
            "UPDATE tasks SET "
                "name='" +  entry.name + "', " +
                "start_time='" + boost::posix_time::to_iso_string(entry.start_time) + "', " +
                "end_time='" + boost::posix_time::to_iso_string(entry.end_time) + "', "
                "auto_profile=" + (entry.auto_profile ? std::to_string(entry.auto_profile) : "NULL") + ", " +
                "is_user_declared=" + (entry.is_user_declared ? "1" : "0") + " " +
            "WHERE id=" + std::to_string(entry.id);

        std::string stmt2 =
            "INSERT INTO tasks (id, name, start_time, end_time, auto_profile, is_user_declared) "
            "VALUES ("
                "NULL, "
                "'" + entry.name + "', "
                "'" + boost::posix_time::to_iso_string(entry.start_time) + "', " +
                "'" + boost::posix_time::to_iso_string(entry.end_time) + "', " +
                (entry.auto_profile ? std::to_string(entry.auto_profile) : "NULL") + ", " +
                (entry.is_user_declared ? "1" : "0") +
            ")";

        std::string stmt3 =
            "DELETE FROM appliances_tasks WHERE task_id=" + std::to_string(entry.id);

        std::string stmt4;
        if (entry.appliances.size() && std::accumulate(entry.appliances.begin(), entry.appliances.end(), 0)) {
            stmt4 += "INSERT INTO appliances_tasks (appliance_id, task_id) VALUES ";
            for (const auto& appliance : entry.appliances) {
                if (!appliance) {
                    continue;
                }
                stmt4 += "(" + std::to_string(appliance) + ", " + std::to_string(entry.id) + "), ";
            }
            stmt4.pop_back();
            stmt4.pop_back();
        }

        int code = hems_storage::handler_msg_set_with_id(entry.id, stmt1, stmt2, stmt3, stmt4);
        if (oa != nullptr) {
            /* Prepare response message containing the id of the new entry. */
            msg_set_response response {
                id : entry.id
            };
            *oa << response;
        }
        return code;
    }

    int hems_storage::handler_msg_set_auto_profile(text_iarchive& ia, text_oarchive* oa) {
        msg_set_auto_profile_request msg;
        ia >> msg;
        auto_profile_t& entry = msg.auto_profile;

        boost::replace_all(entry.name, "'", "''");
        boost::replace_all(entry.profile, "'", "''");

        std::string stmt1 =
            "UPDATE auto_profiles SET "
                "name='" +  entry.name + "', " +
                "profile='" + entry.profile + "' " +
            "WHERE id=" + std::to_string(entry.id);

        std::string stmt2 =
            "INSERT INTO auto_profiles (id, name, profile) "
            "VALUES ("
                "NULL, "
                "'" + entry.name + "', " +
                "'" + entry.profile + "'" +
            ")";

        std::string stmt3 =
            "DELETE FROM appliances_auto_profiles WHERE auto_profile=" + std::to_string(entry.id);

        std::string stmt4;
        if (entry.appliances.size() && std::accumulate(entry.appliances.begin(), entry.appliances.end(), 0)) {
            stmt4 += "INSERT INTO appliances_auto_profiles (appliance_id, auto_profile) VALUES ";
            for (const auto& appliance : entry.appliances) {
                if (!appliance) {
                    continue;
                }
                stmt4 += "(" + std::to_string(appliance) + ", " + std::to_string(entry.id) + "), ";
            }
            stmt4.pop_back();
            stmt4.pop_back();
        }

        int code = hems_storage::handler_msg_set_with_id(entry.id, stmt1, stmt2, stmt3, stmt4);
        if (oa != nullptr) {
            /* TODO Set auto_profile id for all tasks. */
            /* Prepare response message containing the id of the new entry. */
            msg_set_response response {
                id : entry.id
            };
            *oa << response;
        }
        return code;
    }

    int hems_storage::handler_msg_set_energy_consumption(text_iarchive& ia, text_oarchive* oa) {
        msg_set_energy_consumption_request msg;
        ia >> msg;
        energy_consumption_t& entry = msg.energy_consumption;

        const auto& time = entry.time.time_of_day();

        if (time.fractional_seconds() || time.seconds() || time.minutes() % 15) {
            logger::this_logger->log(
                "Invalid value provided for time: Must be a quarter-hour but was " +
                    boost::posix_time::to_iso_string(entry.time),
                logger::level::ERR
            );
            return response_code::MSG_SET_CONSTRAINT_VIOLATION;
        }

        std::string stmt1 =
            "SELECT COUNT(*) FROM energy_consumption WHERE "
                "time='" + boost::posix_time::to_iso_string(entry.time) + "' AND "
                "appliance_id" +
                    (entry.appliance_id ? "=" + std::to_string(entry.appliance_id) : " IS NULL");

        std::string stmt2 =
            "UPDATE energy_consumption SET energy=" + std::to_string(entry.energy) + " "
            "WHERE time='" + boost::posix_time::to_iso_string(entry.time) + "' " +
            "AND appliance_id" +
                (entry.appliance_id ? "=" + std::to_string(entry.appliance_id) : " IS NULL");

        std::string stmt3 =
            "INSERT INTO energy_consumption (time, appliance_id, energy) "
            "VALUES ("
                "'" + boost::posix_time::to_iso_string(entry.time) + "', " +
                (entry.appliance_id ? std::to_string(entry.appliance_id) : "NULL") + ", " +
                std::to_string(entry.energy) +
            ")";

        return hems_storage::handler_msg_set_without_id(stmt1, stmt2, stmt3);
    }

    int hems_storage::handler_msg_set_energy_production(text_iarchive& ia, text_oarchive* oa) {
        msg_set_energy_production_request msg;
        ia >> msg;
        energy_production_t& entry = msg.energy_production;

        const auto& time = entry.time.time_of_day();

        if (time.fractional_seconds() || time.seconds() || time.minutes() % 15) {
            logger::this_logger->log(
                "Invalid value provided for time: Must be a quarter-hour but was " +
                    boost::posix_time::to_iso_string(entry.time),
                logger::level::ERR
            );
            return response_code::MSG_SET_CONSTRAINT_VIOLATION;
        }

        std::string stmt1 =
            "SELECT COUNT(*) FROM energy_production WHERE "
                "time='" + boost::posix_time::to_iso_string(entry.time) + "'";

        std::string stmt2 =
            "UPDATE energy_production SET energy=" + std::to_string(entry.energy) + " "
            "WHERE time='" + boost::posix_time::to_iso_string(entry.time) + "'";

        std::string stmt3 =
            "INSERT INTO energy_production (time, energy) "
            "VALUES ("
                "'" + boost::posix_time::to_iso_string(entry.time) + "', " +
                std::to_string(entry.energy) +
            ")";

        return hems_storage::handler_msg_set_without_id(stmt1, stmt2, stmt3);
    }

    int hems_storage::handler_msg_set_weather(text_iarchive& ia, text_oarchive* oa) {
        msg_set_weather_request msg;
        ia >> msg;
        weather_t& entry = msg.weather;

        const auto& time = entry.time.time_of_day();
        auto interval = current_settings.station_intervals.at(entry.station);

        if (time.fractional_seconds() || time.seconds() || time.minutes() % interval) {
            logger::this_logger->log(
                "Invalid value provided for time: Must be a multiple of " + std::to_string(interval) +
                    " full minutes but was " + boost::posix_time::to_iso_string(entry.time),
                logger::level::ERR
            );
            return response_code::MSG_SET_CONSTRAINT_VIOLATION;
        }

        if (entry.humidity < 0 || entry.humidity > 100) {
            logger::this_logger->log(
                "Invalid value provided for humidity: Must be between 0 and 100 but was " +
                    std::to_string(entry.humidity),
                logger::level::ERR
            );
            return response_code::MSG_SET_CONSTRAINT_VIOLATION;
        }

        if (entry.cloud_cover < 0 || entry.cloud_cover > 100) {
            logger::this_logger->log(
                "Invalid value provided for cloud cover: Must be between 0 and 100 but was " +
                    std::to_string(entry.cloud_cover),
                logger::level::ERR
            );
            return response_code::MSG_SET_CONSTRAINT_VIOLATION;
        }

        if (entry.radiation < 0) {
            logger::this_logger->log(
                "Invalid value provided for radiation: Must be at least 0 but was " +
                    std::to_string(entry.radiation),
                logger::level::ERR
            );
            return response_code::MSG_SET_CONSTRAINT_VIOLATION;
        }

        std::string stmt1 =
            "SELECT COUNT(*) FROM weather WHERE "
                "time='" + boost::posix_time::to_iso_string(entry.time) + "' AND "
                "station=" + std::to_string(entry.station);

        std::string stmt2 =
            "UPDATE weather SET "
                "temperature=" + std::to_string(entry.temperature) + ", " +
                "humidity=" + std::to_string(entry.humidity) + ", " +
                "pressure=" + std::to_string(entry.pressure) + ", " +
                "cloud_cover=" + std::to_string(entry.cloud_cover) + ", " +
                "radiation=" + std::to_string(entry.radiation) + " " +
            "WHERE time='" + boost::posix_time::to_iso_string(entry.time) + "' "
            "AND station=" + std::to_string(entry.station);

        std::string stmt3 =
            "INSERT INTO weather (time, station, temperature, humidity, pressure, cloud_cover, radiation) "
            "VALUES ("
                "'" + boost::posix_time::to_iso_string(entry.time) + "', " +
                std::to_string(entry.station) + ", " +
                std::to_string(entry.temperature) + ", " +
                std::to_string(entry.humidity) + ", " +
                std::to_string(entry.pressure) + ", " +
                std::to_string(entry.cloud_cover) + ", " +
                std::to_string(entry.radiation) +
            ")";

        return hems_storage::handler_msg_set_without_id(stmt1, stmt2, stmt3);
    }

}}}
