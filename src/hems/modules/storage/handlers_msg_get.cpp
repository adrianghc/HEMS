/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Data Storage Module.
 * This module is responsible for managing access to data storage for all other modules.
 * Whenever other modules need to read or write measurements or other data, they can issue messages
 * to the Data Storage Module.
 */

#include <map>
#include <string>
#include <vector>

#include <boost/algorithm/string/replace.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include "hems/modules/storage/storage.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/storage.h"

#include "sqlite/sqlite3.h"

namespace hems { namespace modules { namespace storage {

    using namespace hems::messages::storage;

    using boost::posix_time::ptime;
    using boost::posix_time::from_iso_string;

    int handler_wrapper_msg_get_settings(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_settings(ia, oa);
    }

    int hems_storage::handler_msg_get_settings(text_iarchive& ia, text_oarchive* oa) {
        types::settings_t settings;
        std::string stmt = "SELECT * FROM settings WHERE id = 0";

        int code = response_code::SUCCESS;
        sqlite3_stmt* prepared_stmt = nullptr;

        int errcode = sqlite3_prepare_v2(
            db_connection, stmt.c_str(), -1, &prepared_stmt, nullptr
        );
        if (errcode != SQLITE_OK) {
            logger::this_logger->log(
                "Error preparing a statement: '" + stmt + "'. The error was: " +
                    sqlite3_errstr(errcode) + "\n",
                logger::level::ERR
            );
            code = response_code::MSG_GET_SQL_ERROR;
        } else {
            errcode = sqlite3_step(prepared_stmt);
            if (errcode == SQLITE_DONE) {
                code = response_code::MSG_GET_NONE_AVAILABLE;
            } else if (errcode == SQLITE_ROW) {
                    double longitude = sqlite3_column_double(prepared_stmt, 1);
                    double latitude = sqlite3_column_double(prepared_stmt, 2);
                    double timezone = sqlite3_column_double(prepared_stmt, 3);
                    const char* pv_uri_ptr =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 4));
                    std::string pv_uri = pv_uri_ptr ? pv_uri_ptr : "";
                    boost::replace_all(pv_uri, "'", "''");
                    unsigned int interval_energy_production = sqlite3_column_int(prepared_stmt, 5);
                    unsigned int interval_energy_consumption = sqlite3_column_int(prepared_stmt, 6);
                    unsigned int interval_automation = sqlite3_column_int(prepared_stmt, 7);

                settings = {
                    longitude                   : longitude,
                    latitude                    : latitude,
                    timezone                    : timezone,
                    pv_uri                      : pv_uri,
                    station_intervals           : {},
                    station_uris                : {},
                    interval_energy_production  : interval_energy_production,
                    interval_energy_consumption : interval_energy_consumption,
                    interval_automation         : interval_automation
                };
            } else {
                logger::this_logger->log(
                    "Error evaluating a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n",
                    logger::level::ERR
                );
                code = response_code::MSG_GET_SQL_ERROR;
            }
        }
        sqlite3_finalize(prepared_stmt);

        if (code != response_code::SUCCESS) {
            return code;
        }

        stmt = "SELECT * FROM settings_stations WHERE settings_id = 0";
        prepared_stmt = nullptr;
        std::map<id_t, unsigned int> station_intervals;
        std::map<id_t, std::string> station_uris;

        errcode = sqlite3_prepare_v2(
            db_connection, stmt.c_str(), -1, &prepared_stmt, nullptr
        );
        if (errcode != SQLITE_OK) {
            logger::this_logger->log(
                "Error preparing a statement: '" + stmt + "'. The error was: " +
                    sqlite3_errstr(errcode) + "\n",
                logger::level::ERR
            );
            code = response_code::MSG_GET_SQL_ERROR;
        } else {
            while ((errcode = sqlite3_step(prepared_stmt)) == SQLITE_ROW) {
                id_t station_id = sqlite3_column_int64(prepared_stmt, 0);
                unsigned int interval = sqlite3_column_int(prepared_stmt, 2);
                const char* uri_ptr =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 3));
                std::string uri = uri_ptr ? uri_ptr : "";
                boost::replace_all(uri, "'", "''");

                station_intervals.emplace(station_id, interval);
                if (uri.size()) {
                    station_uris.emplace(station_id, uri);
                }
            }
            if (errcode != SQLITE_DONE) {
                logger::this_logger->log(
                    "Error evaluating a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n",
                    logger::level::ERR
                );
                code = response_code::MSG_GET_SQL_ERROR;
            }
        }

        sqlite3_finalize(prepared_stmt);

        if (code == response_code::SUCCESS) {
            settings.station_intervals = station_intervals;
            *oa << settings;
        }

        return code;
    }


    int hems_storage::handler_msg_get_appliances_common(
        std::string& stmt1, std::string& stmt2, std::string& stmt3, std::map<types::id_t, types::appliance_t>& appliances
    ) {
        int code = response_code::SUCCESS;
        sqlite3_stmt* prepared_stmt = nullptr;


        /* Get items from `appliances`. */
        int errcode = sqlite3_prepare_v2(
            db_connection, stmt1.c_str(), -1, &prepared_stmt, nullptr
        );
        if (errcode != SQLITE_OK) {
            logger::this_logger->log(
                "Error preparing a statement: '" + stmt1 + "'. The error was: " +
                    sqlite3_errstr(errcode) + "\n",
                logger::level::ERR
            );
            code = response_code::MSG_GET_SQL_ERROR;
        } else {
            while ((errcode = sqlite3_step(prepared_stmt)) == SQLITE_ROW) {
                id_t id = sqlite3_column_int64(prepared_stmt, 0);

                const char* name_ptr =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 1));
                std::string name = name_ptr ? name_ptr : "";
                boost::replace_all(name, "'", "''");

                const char* uri_ptr =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 2));
                std::string uri = uri_ptr ? uri_ptr : "";
                boost::replace_all(uri, "'", "''");

                double rating = sqlite3_column_double(prepared_stmt, 3);
                unsigned int duty_cycle = sqlite3_column_int(prepared_stmt, 4);
                uint8_t schedules_per_week = sqlite3_column_int(prepared_stmt, 5);

                types::appliance_t appliance = {
                    id                  : id,
                    name                : name,
                    uri                 : uri,
                    rating              : rating,
                    duty_cycle          : duty_cycle,
                    schedules_per_week  : schedules_per_week,
                    tasks               : {},
                    auto_profiles       : {}
                };

                appliances.emplace(id, appliance);
            }
            if (errcode != SQLITE_DONE) {
                logger::this_logger->log(
                    "Error evaluating a statement: '" + stmt1 + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n",
                    logger::level::ERR
                );
                code = response_code::MSG_GET_SQL_ERROR;
            }
        }

        sqlite3_finalize(prepared_stmt);

        if (!appliances.size()) {
            code = response_code::MSG_GET_NONE_AVAILABLE;
        }

        if (code != response_code::SUCCESS) {
            return code;
        }


        /* Get items from `appliances_tasks`. */
        prepared_stmt = nullptr;
        errcode = sqlite3_prepare_v2(
            db_connection, stmt2.c_str(), -1, &prepared_stmt, nullptr
        );
        if (errcode != SQLITE_OK) {
            logger::this_logger->log(
                "Error preparing a statement: '" + stmt2 + "'. The error was: " +
                    sqlite3_errstr(errcode) + "\n",
                logger::level::ERR
            );
            code = response_code::MSG_GET_SQL_ERROR;
        } else {
            while ((errcode = sqlite3_step(prepared_stmt)) == SQLITE_ROW) {
                id_t appliance_id = sqlite3_column_int64(prepared_stmt, 0);
                id_t task_id = sqlite3_column_int64(prepared_stmt, 1);
                if (appliances.find(appliance_id) != appliances.end()) {
                    types::appliance_t& appliance = appliances.at(appliance_id);
                    std::set<id_t>& tasks = appliance.tasks;
                    if (std::find(tasks.begin(), tasks.end(), task_id) == tasks.end()) {
                        tasks.emplace(task_id);
                    }
                }
            }
            if (errcode != SQLITE_DONE) {
                logger::this_logger->log(
                    "Error evaluating a statement: '" + stmt2 + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n",
                    logger::level::ERR
                );
                code = response_code::MSG_GET_SQL_ERROR;
            }
        }

        sqlite3_finalize(prepared_stmt);

        if (code != response_code::SUCCESS) {
            return code;
        }


        /* Get items from `appliances_auto_profiles`. */
        prepared_stmt = nullptr;
        errcode = sqlite3_prepare_v2(
            db_connection, stmt3.c_str(), -1, &prepared_stmt, nullptr
        );
        if (errcode != SQLITE_OK) {
            logger::this_logger->log(
                "Error preparing a statement: '" + stmt3 + "'. The error was: " +
                    sqlite3_errstr(errcode) + "\n",
                logger::level::ERR
            );
            code = response_code::MSG_GET_SQL_ERROR;
        } else {
            while ((errcode = sqlite3_step(prepared_stmt)) == SQLITE_ROW) {
                id_t appliance_id = sqlite3_column_int64(prepared_stmt, 0);
                id_t auto_profile = sqlite3_column_int64(prepared_stmt, 1);
                if (appliances.find(appliance_id) != appliances.end()) {
                    types::appliance_t& appliance = appliances.at(appliance_id);
                    std::set<id_t>& auto_profiles = appliance.auto_profiles;
                    if (std::find(auto_profiles.begin(), auto_profiles.end(), auto_profile) == auto_profiles.end()) {
                        auto_profiles.emplace(auto_profile);
                    }
                }
            }
            if (errcode != SQLITE_DONE) {
                logger::this_logger->log(
                    "Error evaluating a statement: '" + stmt3 + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n",
                    logger::level::ERR
                );
                code = response_code::MSG_GET_SQL_ERROR;
            }
        }

        sqlite3_finalize(prepared_stmt);

        return code;
    }

    int handler_wrapper_msg_get_appliances(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_appliances(ia, oa);
    }

    int hems_storage::handler_msg_get_appliances(text_iarchive& ia, text_oarchive* oa) {
        msg_get_appliances_request entry;
        ia >> entry;

        std::map<id_t, appliance_t> appliances;

        std::string stmt1 = "SELECT * FROM appliances WHERE ";
        for (const auto& id : entry.ids) {
            stmt1 += "id=" + std::to_string(id) + " OR ";
        }
        stmt1.pop_back();
        stmt1.pop_back();
        stmt1.pop_back();

        std::string stmt2 = "SELECT * FROM appliances_tasks WHERE ";
        for (const auto& id : entry.ids) {
            stmt2 += "appliance_id=" + std::to_string(id) + " OR ";
        }
        stmt2.pop_back();
        stmt2.pop_back();
        stmt2.pop_back();

        std::string stmt3 = "SELECT * FROM appliances_auto_profiles WHERE ";
        for (const auto& id : entry.ids) {
            stmt3 += "appliance_id=" + std::to_string(id) + " OR ";
        }
        stmt3.pop_back();
        stmt3.pop_back();
        stmt3.pop_back();

        int code = handler_msg_get_appliances_common(stmt1, stmt2, stmt3, appliances);

        msg_get_appliances_response response = { appliances : appliances };
        *oa << response;

        return code;
    }

    int handler_wrapper_msg_get_appliances_all(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_appliances_all(ia, oa);
    }

    int hems_storage::handler_msg_get_appliances_all(text_iarchive& ia, text_oarchive* oa) {
        msg_get_appliances_all_request entry;
        ia >> entry;

        std::map<id_t, appliance_t> appliances;

        std::string stmt1 = "SELECT * FROM appliances";
        if (entry.is_schedulable == tribool::TRUE) {
            stmt1 += " WHERE schedules_per_week > 0";
        } else if (entry.is_schedulable == tribool::FALSE) {
            stmt1 += " WHERE schedules_per_week = 0";
        }

        std::string stmt2 = "SELECT * FROM appliances_tasks";
        std::string stmt3 = "SELECT * FROM appliances_auto_profiles";

        int code = handler_msg_get_appliances_common(stmt1, stmt2, stmt3, appliances);

        std::vector<appliance_t> appliances_vec;
        if (code == response_code::SUCCESS) {
            appliances_vec.reserve(appliances.size());
            for (const auto& appliance : appliances) {
                appliances_vec.push_back(appliance.second);
            }
        }

        msg_get_appliances_all_response response = { appliances : appliances_vec };
        *oa << response;

        return code;
    }

    int handler_wrapper_msg_get_tasks_by_id(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_tasks_by_id(ia, oa);
    }

    int hems_storage::handler_msg_get_tasks_by_id(text_iarchive& ia, text_oarchive* oa) {
        return 1; // TODO
    }

    int handler_wrapper_msg_get_tasks_by_time(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_tasks_by_time(ia, oa);
    }

    int hems_storage::handler_msg_get_tasks_by_time(text_iarchive& ia, text_oarchive* oa) {
        return 1; // TODO
    }

    int handler_wrapper_msg_get_tasks_all(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_tasks_all(ia, oa);
    }

    int hems_storage::handler_msg_get_tasks_all(text_iarchive& ia, text_oarchive* oa) {
        return 1; // TODO
    }

    int handler_wrapper_msg_get_auto_profiles(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_auto_profiles(ia, oa);
    }

    int hems_storage::handler_msg_get_auto_profiles(text_iarchive& ia, text_oarchive* oa) {
        return 1; // TODO
    }

    int handler_wrapper_msg_get_auto_profiles_all(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_auto_profiles_all(ia, oa);
    }

    int hems_storage::handler_msg_get_auto_profiles_all(text_iarchive& ia, text_oarchive* oa) {
        return 1; // TODO
    }

    int handler_wrapper_msg_get_energy_production(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_energy_production(ia, oa);
    }

    int hems_storage::handler_msg_get_energy_production(text_iarchive& ia, text_oarchive* oa) {
        msg_get_energy_production_request entry;
        ia >> entry;

        std::string stmt =
            "SELECT * FROM energy_production WHERE time BETWEEN '" +
            boost::posix_time::to_iso_string(entry.start_time) + "' AND '" +
            boost::posix_time::to_iso_string(entry.end_time) + "'";

        std::map<ptime, energy_production_t> energy_productions;

        int code = response_code::SUCCESS;
        sqlite3_stmt* prepared_stmt = nullptr;

        int errcode = sqlite3_prepare_v2(
            db_connection, stmt.c_str(), -1, &prepared_stmt, nullptr
        );
        if (errcode != SQLITE_OK) {
            logger::this_logger->log(
                "Error preparing a statement: '" + stmt + "'. The error was: " +
                    sqlite3_errstr(errcode) + "\n",
                logger::level::ERR
            );
            code = response_code::MSG_GET_SQL_ERROR;
        } else {
            while ((errcode = sqlite3_step(prepared_stmt)) == SQLITE_ROW) {
                ptime time = from_iso_string(
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 0))
                );
                double energy = sqlite3_column_double(prepared_stmt, 1);

                types::energy_production_t energy_production = {
                    time    : time,
                    energy  : energy
                };

                energy_productions.emplace(time, energy_production);
            }
            if (errcode != SQLITE_DONE) {
                logger::this_logger->log(
                    "Error evaluating a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n",
                    logger::level::ERR
                );
                code = response_code::MSG_GET_SQL_ERROR;
            }
        }

        sqlite3_finalize(prepared_stmt);

        if (!energy_productions.size()) {
            code = response_code::MSG_GET_NONE_AVAILABLE;
        }

        msg_get_energy_production_response response = {
            energy : energy_productions
        };

        *oa << response;
        return code;
    }

    int handler_wrapper_msg_get_energy_consumption(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_energy_consumption(ia, oa);
    }

    int hems_storage::handler_msg_get_energy_consumption(text_iarchive& ia, text_oarchive* oa) {
        return 1; // TODO
    }

    int handler_wrapper_msg_get_energy_consumption_all(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_energy_consumption_all(ia, oa);
    }

    int hems_storage::handler_msg_get_energy_consumption_all(text_iarchive& ia, text_oarchive* oa) {
        return 1; // TODO
    }

    int handler_wrapper_msg_get_weather(text_iarchive& ia, text_oarchive* oa) {
        return hems_storage::this_instance->handler_msg_get_weather(ia, oa);
    }

    int hems_storage::handler_msg_get_weather(text_iarchive& ia, text_oarchive* oa) {
        msg_get_weather_request entry;
        ia >> entry;

        std::set<id_t>& stations = entry.stations;
        std::map<ptime, std::vector<weather_t>> time_to_weather;
        std::map<id_t, std::vector<weather_t>> station_to_weather;

        std::string stmt =
            "SELECT * FROM weather WHERE time BETWEEN '" +
            boost::posix_time::to_iso_string(entry.start_time) + "' AND '" +
            boost::posix_time::to_iso_string(entry.end_time) + "'";

        unsigned int i = 0;
        for (const auto& station : stations) {
            if (i == 0) {
                stmt += " AND (";
            }
            stmt += "station=" + std::to_string(station);
            if (i < stations.size() - 1) {
                stmt += " OR ";
            } else {
                stmt += ")";
            }
            ++i;
        }

        int code = response_code::SUCCESS;
        sqlite3_stmt* prepared_stmt = nullptr;

        int errcode = sqlite3_prepare_v2(
            db_connection, stmt.c_str(), -1, &prepared_stmt, nullptr
        );
        if (errcode != SQLITE_OK) {
            logger::this_logger->log(
                "Error preparing a statement: '" + stmt + "'. The error was: " +
                    sqlite3_errstr(errcode) + "\n",
                logger::level::ERR
            );
            code = response_code::MSG_GET_SQL_ERROR;
        } else {
            while ((errcode = sqlite3_step(prepared_stmt)) == SQLITE_ROW) {
                ptime time = from_iso_string(
                    reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt, 0))
                );
                id_t station = sqlite3_column_int64(prepared_stmt, 1);
                double temperature = sqlite3_column_double(prepared_stmt, 2);
                unsigned int humidity = sqlite3_column_int(prepared_stmt, 3);
                double pressure = sqlite3_column_double(prepared_stmt, 4);
                unsigned int cloud_cover = sqlite3_column_int(prepared_stmt, 5);
                double radiation = sqlite3_column_double(prepared_stmt, 6);

                types::weather_t weather = {
                    time        : time,
                    station     : station,
                    temperature : temperature,
                    humidity    : humidity,
                    pressure    : pressure,
                    cloud_cover : cloud_cover,
                    radiation   : radiation
                };

                if (time_to_weather.find(time) == time_to_weather.end()) {
                    time_to_weather.emplace(time, std::vector<weather_t>({weather}));
                } else {
                    time_to_weather.at(time).emplace_back(weather);
                }

                if (station_to_weather.find(station) == station_to_weather.end()) {
                    station_to_weather.emplace(station, std::vector<weather_t>({weather}));
                } else {
                    station_to_weather.at(station).emplace_back(weather);
                }
            }
            if (errcode != SQLITE_DONE) {
                logger::this_logger->log(
                    "Error evaluating a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n",
                    logger::level::ERR
                );
                code = response_code::MSG_GET_SQL_ERROR;
            }
        }

        sqlite3_finalize(prepared_stmt);

        if (!time_to_weather.size() || !station_to_weather.size()) {
            code = response_code::MSG_GET_NONE_AVAILABLE;
        }

        msg_get_weather_response response = {
            time_to_weather     : time_to_weather,
            station_to_weather  : station_to_weather
        };

        *oa << response;
        return code;
    }

}}}
