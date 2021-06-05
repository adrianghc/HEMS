/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Data Storage Module.
 * This module is responsible for managing access to data storage for all other modules.
 * Whenever other modules need to read or write measurements or other data, they can issue messages
 * to the Data Storage Module.
 */

#include <algorithm>
#include <iterator>
#include <mutex>

#include <boost/algorithm/string/replace.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "hems/modules/storage/storage.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/storage.h"

#include "sqlite/sqlite3.h"

namespace hems { namespace modules { namespace storage {

    using namespace hems::messages::storage;

    int handler_wrapper_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }

    int handler_wrapper_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_settings(ia, oa, true);
    }

    int handler_wrapper_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_settings(ia, oa, false);
    }

    int hems_storage::handler_settings(text_iarchive& ia, text_oarchive* oa, bool check_only) {
        types::settings_t settings;
        ia >> settings;

        std::vector<id_t> new_stations1;    /** A vector containing all the stations from the new
                                                settings (in `station_intervals`). */
        std::vector<id_t> new_stations2;    /** A vector containing all the stations from the new
                                                settings (in `station_uris`). */
        std::vector<id_t> old_stations;     /** A vector containing all the stations from the old
                                                settings. */

        for (const auto& station_interval : settings.station_intervals) {
            new_stations1.emplace_back(station_interval.first);
            if (station_interval.second <= 0 || 60 % station_interval.second) {
                logger::this_logger->log(
                    "Invalid value provided for a station interval: Must be between 1 and 60 and "
                        "a divisor of 60 but was " + std::to_string(station_interval.second) + ".",
                    logger::level::ERR
                );
                return messenger::settings_code::INVALID;
            }
        }

        for (const auto& station_uri : settings.station_uris) {
            new_stations2.emplace_back(station_uri.first);
        }


        if (settings.interval_energy_production <= 0 || 60 % settings.interval_energy_production) {
            logger::this_logger->log(
                "Invalid value provided for a energy production collection interval: Must be "
                    "between 1 and 60 and a divisor of 60 but was " +
                    std::to_string(settings.interval_energy_production) + ".",
                logger::level::ERR
            );
            return messenger::settings_code::INVALID;
        }

        if (settings.interval_energy_consumption <= 0 || 60 % settings.interval_energy_consumption) {
            logger::this_logger->log(
                "Invalid value provided for a energy consumption collection interval: Must be "
                    "between 1 and 60 and a divisor of 60 but was " +
                    std::to_string(settings.interval_energy_consumption) + ".",
                logger::level::ERR
            );
            return messenger::settings_code::INVALID;
        }

        int code = messenger::settings_code::SUCCESS;
        char* errmsg = nullptr; /*  It's important to initialize this to nullptr or `sqlite3_free()`
                                    will fail if `sqlite3_malloc()` was not previously called. */

        if (!db_begin()) {
            return messenger::settings_code::INTERNAL_ERROR;
        }


        /* Start with the settings table. */

        boost::replace_all(settings.pv_uri, "'", "''");

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
                "longitude=excluded.longitude, latitude=excluded.latitude, "
                "timezone=excluded.timezone, pv_uri=excluded.pv_uri, "
                "interval_energy_production=excluded.interval_energy_production, "
                "interval_energy_consumption=excluded.interval_energy_consumption, "
                "interval_automation=excluded.interval_automation "
            "WHERE id = 0";

        if (sqlite3_exec(db_connection, stmt1.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
            logger::this_logger->log(
                "Could not update settings to " + types::to_string(settings) + ", the new settings "
                    "are rejected: " + std::string(errmsg),
                logger::level::ERR
            );
            code = messenger::settings_code::INVALID;
        }

        sqlite3_free(errmsg);

        if (code != messenger::settings_code::SUCCESS) {
            hems_storage::db_commit(false);
            return code;
        }


        /* Check that the keys of `station_intervals` are a superset of `station_uris`. */
        std::vector<id_t> stations_diff;
        std::set_difference(
            new_stations2.begin(), new_stations2.end(), new_stations1.begin(), new_stations1.end(),
            std::inserter(stations_diff, stations_diff.begin())
        );
        if (stations_diff.size()) {
            logger::this_logger->log(
                "An URI was given for a station for which no interval was given.", logger::level::ERR
            );
            code = messenger::settings_code::INVALID;
        }

        if (code != messenger::settings_code::SUCCESS) {
            hems_storage::db_commit(false);
            return code;
        }


    	/* Before continuing with settings_stations, we must know what it currently looks like. */
        errmsg = nullptr;
        std::string stmt2 = "SELECT * FROM settings_stations WHERE settings_id=0";
        sqlite3_stmt* prepared_stmt2;

        std::map<id_t, unsigned int> station_intervals;

        int errcode = sqlite3_prepare_v2(db_connection, stmt2.c_str(), -1, &prepared_stmt2, nullptr);
        if (errcode != SQLITE_OK) {
            logger::this_logger->log(
                "Could not gather existing settings, the new settings are rejected: " +
                    std::string(errmsg),
                logger::level::ERR
            );
            code = messenger::settings_code::INVALID;
        } else {
            while ((errcode = sqlite3_step(prepared_stmt2)) == SQLITE_ROW) {
                id_t station_id = sqlite3_column_int64(prepared_stmt2, 0);
                unsigned int interval = sqlite3_column_int(prepared_stmt2, 2);

                old_stations.emplace_back(station_id);
                station_intervals.emplace(station_id, interval);
            }
            if (errcode != SQLITE_DONE) {
                logger::this_logger->log(
                    "Error gathering existing settings, the new settings are rejected: " +
                        std::string(sqlite3_errmsg(db_connection)),
                    logger::level::ERR
                );
                code = messenger::settings_code::INVALID;
            }
        }

        sqlite3_finalize(prepared_stmt2);
        sqlite3_free(errmsg);

        if (code != messenger::settings_code::SUCCESS) {
            hems_storage::db_commit(false);
            return code;
        }

        /* Check if any stations have been removed. */
        std::vector<id_t> removed_stations;
        std::set_difference(
            old_stations.begin(), old_stations.end(), new_stations1.begin(), new_stations1.end(),
            std::inserter(removed_stations, removed_stations.begin())
        );


        /* Remove stations, if applicable. */
        if (removed_stations.size()) {
            errmsg = nullptr;
            std::string stmt3 = "DELETE FROM settings_stations WHERE ";
            for (const auto& station : removed_stations) {
                stmt3 += "station_id=" + std::to_string(station) + " OR ";
            }
            stmt3.pop_back();
            stmt3.pop_back();
            stmt3.pop_back();
            stmt3.pop_back();

            if (sqlite3_exec(db_connection, stmt3.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
                logger::this_logger->log(
                    "Could not update settings to " + types::to_string(settings) + ", the new settings "
                        "are rejected: " + std::string(errmsg),
                    logger::level::ERR
                );
                code = messenger::settings_code::INVALID;
            }

            sqlite3_free(errmsg);
        }

        if (!new_stations1.size()) {
            if (check_only) {
                hems_storage::db_commit(false);
                code = messenger::settings_code::SUCCESS;
            } else if (!hems_storage::db_commit(true)) {
                code = messenger::settings_code::INTERNAL_ERROR;
            } else {
                code = messenger::settings_code::SUCCESS;
            }
            return code;
        } else if (code != messenger::settings_code::SUCCESS) {
            hems_storage::db_commit(false);
            return code;
        }


        /* Insert new entries and update existing ones. */
        errmsg = nullptr;
        std::string stmt4 =
            "INSERT INTO settings_stations (station_id, settings_id, interval, uri) "
            "VALUES ";
        for (const auto& station_interval : settings.station_intervals) {
            std::string uri;
            try {
                uri = settings.station_uris.at(station_interval.first);
                boost::replace_all(uri, "'", "''");
            } catch (std::out_of_range& e) {
                uri = "";
            }
             stmt4 +=
                "(" +
                    std::to_string(station_interval.first) + ", " +
                    "0, " +
                    std::to_string(station_interval.second) + ", " +
                    "'" + uri + "'" +
                "), ";
        }
        stmt4.pop_back();
        stmt4.pop_back();
        stmt4 +=
            " ON CONFLICT (station_id) DO UPDATE SET"
            " station_id=excluded.station_id, settings_id=excluded.settings_id,"
            " interval=excluded.interval, uri=excluded.uri";

        if (sqlite3_exec(db_connection, stmt4.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
            logger::this_logger->log(
                "Could not update settings to " + types::to_string(settings) + ", the new settings "
                    "are rejected: " + std::string(errmsg),
                logger::level::ERR
            );
            hems_storage::db_commit(false);
            code = messenger::settings_code::INVALID;
        } else {
            if (check_only) {
                hems_storage::db_commit(false);
                code = messenger::settings_code::SUCCESS;
            } else if (!hems_storage::db_commit(true)) {
                code = messenger::settings_code::INTERNAL_ERROR;
            } else {
                code = messenger::settings_code::SUCCESS;
            }
        }

        sqlite3_free(errmsg);

        return code;
    }

}}}
