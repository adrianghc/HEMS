/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Data Storage Module.
 * This module is responsible for managing access to data storage for all other modules.
 * Whenever other modules need to read or write measurements or other data, they can issue messages
 * to the Data Storage Module.
 */

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

    int handler_wrapper_msg_del_appliance(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_del_appliance(ia, oa);
    }

    int handler_wrapper_msg_del_task(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_del_task(ia, oa);
    }

    int handler_wrapper_msg_del_auto_profile(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_del_auto_profile(ia, oa);
    }

    int handler_wrapper_msg_del_energy_consumption(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_del_energy_consumption(ia, oa);
    }

    int handler_wrapper_msg_del_energy_production(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_del_energy_production(ia, oa);
    }

    int handler_wrapper_msg_del_weather(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_del_weather(ia, oa);
    }


    int hems_storage::handler_msg_del(std::string& stmt) {
        int code;
        char* errmsg = nullptr; /*  It's important to initialize this to nullptr or `sqlite3_free()`
                                    will fail if `sqlite3_malloc()` was not previously called. */

        if (!db_begin()) {
            return response_code::MSG_DEL_SQL_ERROR;
        }

        if (sqlite3_exec(db_connection, stmt.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
            logger::this_logger->log(
                "Error deleting an entry: '" + stmt + "'. The error was: " + errmsg,
                logger::level::ERR
            );
            code = response_code::MSG_DEL_SQL_ERROR;
        } else if (!sqlite3_changes(db_connection)) {
            logger::this_logger->log(
                "Attempted to delete a non-existing entry: '" + stmt + "'.",
                logger::level::ERR
            );
            code = response_code::MSG_DEL_DELETE_NON_EXISTING;
        } else {
            code = response_code::SUCCESS;
        }

        if (code != response_code::SUCCESS) {
            hems_storage::db_commit(false);
        } else {
            hems_storage::db_commit(true);
        }

        return code;
    }


    int hems_storage::handler_msg_del_appliance(text_iarchive& ia, text_oarchive* oa) {
        msg_del_appliance_request entry;
        ia >> entry;

        if (!entry.id) {
            logger::this_logger->log(
                "Attempted to delete an appliance entry with invalid id 0.",
                logger::level::ERR
            );
            return response_code::MSG_DEL_CONSTRAINT_VIOLATION;
        }

        std::string stmt = "DELETE FROM appliances WHERE id=" + std::to_string(entry.id);

        return hems_storage::handler_msg_del(stmt);
    }

    int hems_storage::handler_msg_del_task(text_iarchive& ia, text_oarchive* oa) {
        msg_del_task_request entry;
        ia >> entry;

        if (!entry.id) {
            logger::this_logger->log(
                "Attempted to delete a task entry with invalid id 0.",
                logger::level::ERR
            );
            return response_code::MSG_DEL_CONSTRAINT_VIOLATION;
        }

        std::string stmt = "DELETE FROM tasks WHERE id=" + std::to_string(entry.id);

        return hems_storage::handler_msg_del(stmt);
    }

    int hems_storage::handler_msg_del_auto_profile(text_iarchive& ia, text_oarchive* oa) {
        msg_del_auto_profile_request entry;
        ia >> entry;

        if (!entry.id) {
            logger::this_logger->log(
                "Attempted to delete an auto_profile entry with invalid id 0.",
                logger::level::ERR
            );
            return response_code::MSG_DEL_CONSTRAINT_VIOLATION;
        }

        std::string stmt = "DELETE FROM auto_profiles WHERE id=" + std::to_string(entry.id);

        return hems_storage::handler_msg_del(stmt);
    }

    int hems_storage::handler_msg_del_energy_consumption(text_iarchive& ia, text_oarchive* oa) {
        msg_del_energy_consumption_request entry;
        ia >> entry;

        const auto& time = entry.time.time_of_day();
        if (time.fractional_seconds() || time.seconds() || time.minutes() % 15) {
            logger::this_logger->log(
                "Attempted to delete an energy consumption entry with invalid time: "
                "Must be a quarter-hour but was " + boost::posix_time::to_iso_string(entry.time),
                logger::level::ERR
            );
            return response_code::MSG_DEL_CONSTRAINT_VIOLATION;
        }

        std::string stmt =
            "DELETE FROM energy_consumption WHERE "
                "time='" + boost::posix_time::to_iso_string(entry.time) + "' AND "
                "appliance_id" +
                    (entry.appliance_id ? "=" + std::to_string(entry.appliance_id) : " IS NULL");

        return hems_storage::handler_msg_del(stmt);
    }

    int hems_storage::handler_msg_del_energy_production(text_iarchive& ia, text_oarchive* oa) {
        msg_del_energy_production_request entry;
        ia >> entry;

        const auto& time = entry.time.time_of_day();
        if (time.fractional_seconds() || time.seconds() || time.minutes() % 15) {
            logger::this_logger->log(
                "Attempted to delete an energy production entry with invalid time: "
                "Must be a quarter-hour but was " + boost::posix_time::to_iso_string(entry.time),
                logger::level::ERR
            );
            return response_code::MSG_DEL_CONSTRAINT_VIOLATION;
        }

        std::string stmt =
            "DELETE FROM energy_production WHERE "
                "time='" + boost::posix_time::to_iso_string(entry.time) + "'";

        return hems_storage::handler_msg_del(stmt);
    }

    int hems_storage::handler_msg_del_weather(text_iarchive& ia, text_oarchive* oa) {
        msg_del_weather_request entry;
        ia >> entry;

        const auto& time = entry.time.time_of_day();
        auto interval = current_settings.station_intervals.at(entry.station);
        if (time.fractional_seconds() || time.seconds() || time.minutes() % interval) {
            logger::this_logger->log(
                "Attempted to delete a weather entry with invalid time: "
                    "Must be a multiple of " + std::to_string(interval) + " full minutes but was " +
                    boost::posix_time::to_iso_string(entry.time),
                logger::level::ERR
            );
            return response_code::MSG_DEL_CONSTRAINT_VIOLATION;
        }

        std::string stmt =
            "DELETE FROM weather WHERE "
                "time='" + boost::posix_time::to_iso_string(entry.time) + "' AND "
                "station=" + std::to_string(entry.station);

        return hems_storage::handler_msg_del(stmt);
    }

}}}
