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
     * @brief   Write settings into database for the `MSG_GET_WEATHER` and `MSG_GET_SETTINGS` tests.
     */
    bool write_settings(hems_storage_test* this_instance, settings_t settings) {
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
            "INSERT INTO settings_stations (station_id, settings_id, interval, uri) "
            "VALUES ";
        for (const auto& station_interval : settings.station_intervals) {
            std::string uri;
            try {
                uri = settings.station_uris.at(station_interval.first);
            } catch (std::out_of_range& e) {
                uri = "";
            }
             stmt2 +=
                "(" +
                    std::to_string(station_interval.first) + ", " +
                    "0, " +
                    std::to_string(station_interval.second) + ", " +
                    "'" + uri + "'" +
                "), ";
        }
        stmt2.pop_back();
        stmt2.pop_back();
        stmt2 +=
            " ON CONFLICT (station_id) DO UPDATE SET"
            " station_id=excluded.station_id, settings_id=excluded.settings_id,"
            " interval=excluded.interval, uri=excluded.uri";
        if (!this_instance->prepare_and_evaluate_no_row_query(stmt2)) {
            return false;
        }

        return true;
    }


    /**
     * @brief   Test the handler for `MSG_GET_SETTINGS` messages.
     */
    bool test_handler_msg_get_settings_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        /* Test getting entry when there is none. */
        res = this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_SETTINGS,
            modules::STORAGE,
            this_messenger->serialize(""),
            nullptr
        );
        if (res != response_code::MSG_GET_NONE_AVAILABLE) {
            std::cout <<
                "Expected code " + std::to_string(response_code::MSG_GET_NONE_AVAILABLE) + " but "
                "got " + std::to_string(res) + " instead.\n";
            return false;
        }

        /* Add an entry into `settings`. */
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

        /* Check that the entry was successfully added. */
        std::string stmt3 =
            "SELECT * FROM settings LEFT JOIN settings_stations ON settings.id = settings_stations.settings_id "
            "WHERE settings.id=0";
        sqlite3_stmt* prepared_stmt3 = nullptr;
        if (!this_instance->prepare_and_evaluate(stmt3, prepared_stmt3)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt3);
        }

        /* Get entry. */
        std::string response;
        res = this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_SETTINGS,
            modules::STORAGE,
            this_messenger->serialize(""),
            &response
        );

        if (res != response_code::SUCCESS) {
            std::cout <<
                "Expected code " + std::to_string(response_code::SUCCESS) + " but got " +
                std::to_string(res) + " instead.\n";
            return false;
        } else {
            settings_t settings_response = this_messenger->deserialize<settings_t>(response);
            if (settings != settings_response) {
                std::cout <<
                    "GET message returned a wrong item: " + to_string(settings_response) + "\n";
                return false;
            }
        }

        return true;
    }


    /**
     * @brief   Test the handlers for `MSG_GET_APPLIANCES` messages.
     */
    bool test_handler_msg_get_appliances_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        msg_get_appliances_request request1 = { ids : {1, 2} };

        /* Test getting entry when there is none. */
        res = this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_APPLIANCES,
            modules::STORAGE,
            this_messenger->serialize(request1),
            nullptr
        );
        if (res != response_code::MSG_GET_NONE_AVAILABLE) {
            std::cout <<
                "Expected code " + std::to_string(response_code::MSG_GET_NONE_AVAILABLE) + " but "
                "got " + std::to_string(res) + " instead.\n";
            return false;
        }


        appliance_t appliance = {
            id                  : 0,
            name                : "appliance",
            uri                 : "",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 1,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>()
        };
        std::string stmt =
            "INSERT INTO appliances (name, uri, rating, duty_cycle, schedules_per_week) "
            "VALUES ("
                "'" + appliance.name + "', " +
                "'" + appliance.uri + "', " +
                std::to_string(appliance.rating) + ", " +
                std::to_string(appliance.duty_cycle) + ", " +
                std::to_string(appliance.schedules_per_week) +
            ")";

        if (!this_instance->prepare_and_evaluate_no_row_query(stmt)) {
            return false;
        }

        /* Check that the entry was successfully added. */
        std::string stmt2 =
            "SELECT * FROM appliances WHERE name='" + appliance.name + "'";

        sqlite3_stmt* prepared_stmt2 = nullptr;
        if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt2);
        }

        /* Get entry. */
        std::vector<msg_get_appliances_request> requests_valid = {
            { ids: {1} },
            { ids: {1, 2} },
        };
        for (const auto& request2 : requests_valid) {
            std::string response_str2;
            res = this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messages::storage::msg_type::MSG_GET_APPLIANCES,
                modules::STORAGE,
                this_messenger->serialize(request2),
                &response_str2
            );

            if (res != response_code::SUCCESS) {
                std::cout <<
                    "Expected code " + std::to_string(response_code::SUCCESS) + " but got " +
                    std::to_string(res) + " instead.\n";
                return false;
            } else {
                msg_get_appliances_response response =
                    this_messenger->deserialize<msg_get_appliances_response>(response_str2);
                if (response.appliances.find(1) == response.appliances.end()) {
                    std::cout << "GET message returned a different item than expected.\n";
                    return false;
                } else if (response.appliances.at(1) != appliance) {
                    std::cout << "GET message returned a wrong item.\n";
                    return false;
                }
            }
        }

        /* Make sure that the previously added entry is not returned for wrong id. */
        msg_get_appliances_request request_invalid = { ids : {3} };
        std::string response_str3;
        res = this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_APPLIANCES,
            modules::STORAGE,
            this_messenger->serialize(request_invalid),
            &response_str3
        );

        if (res != response_code::MSG_GET_NONE_AVAILABLE) {
            std::cout <<
                "Expected code " + std::to_string(response_code::MSG_GET_NONE_AVAILABLE) + " but "
                "got " + std::to_string(res) + " instead.\n";
            return false;
        } else {
            msg_get_appliances_response response =
                this_messenger->deserialize<msg_get_appliances_response>(response_str3);
            if (int n = response.appliances.size()) {
                std::cout <<
                    std::to_string(n) + " entries were returned despite error code saying otherwise.\n";
                return false;
            }
        }


        /* TODO Test foreign key rows. */

        return true;
    }

    /**
     * @brief   Test the handlers for `MSG_GET_APPLIANCES_ALL` messages.
     */
    bool test_handler_msg_get_appliances_all_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        msg_get_appliances_all_request request1 = { tribool::TRUE };

        /* Test getting entry when there is none. */
        res = this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_APPLIANCES_ALL,
            modules::STORAGE,
            this_messenger->serialize(request1),
            nullptr
        );
        if (res != response_code::MSG_GET_NONE_AVAILABLE) {
            std::cout <<
                "Expected code " + std::to_string(response_code::MSG_GET_NONE_AVAILABLE) + " but "
                "got " + std::to_string(res) + " instead.\n";
            return false;
        }


        /* Test setting and getting for different values in the tribool field. */
        auto test_with_tribool = [this_instance, this_messenger] (
            appliance_t& appliance, std::vector<msg_get_appliances_all_request>& requests_valid,
            msg_get_appliances_all_request& request_invalid
        ) {
            int res;

            std::string stmt =
                "INSERT INTO appliances (name, uri, rating, duty_cycle, schedules_per_week) "
                "VALUES ("
                    "'" + appliance.name + "', " +
                    "'" + appliance.uri + "', " +
                    std::to_string(appliance.rating) + ", " +
                    std::to_string(appliance.duty_cycle) + ", " +
                    std::to_string(appliance.schedules_per_week) +
                ")";

            if (!this_instance->prepare_and_evaluate_no_row_query(stmt)) {
                return false;
            }

            /* Check that the entry was successfully added. */
            std::string stmt2 =
                "SELECT * FROM appliances WHERE name='" + appliance.name + "'";

            sqlite3_stmt* prepared_stmt2 = nullptr;
            if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
                return false;
            } else {
                sqlite3_finalize(prepared_stmt2);
            }

            /* Get entry. */
            for (const auto& request2 : requests_valid) {
                std::string response_str2;
                res = this_messenger->send(
                    DEFAULT_SEND_TIMEOUT,
                    messages::storage::msg_type::MSG_GET_APPLIANCES_ALL,
                    modules::STORAGE,
                    this_messenger->serialize(request2),
                    &response_str2
                );

                if (res != response_code::SUCCESS) {
                    std::cout <<
                        "Expected code " + std::to_string(response_code::SUCCESS) + " but got " +
                        std::to_string(res) + " instead.\n";
                    return false;
                } else {
                    msg_get_appliances_all_response response =
                        this_messenger->deserialize<msg_get_appliances_all_response>(response_str2);
                    if (response.appliances.at(0) != appliance && response.appliances.at(1) != appliance) {
                        std::cout << "GET message returned a wrong item.\n";
                        return false;
                    }
                }
            }

            /* Make sure that the previously added entry is not returned for wrong tribool value. */
            std::string response_str3;
            res = this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messages::storage::msg_type::MSG_GET_APPLIANCES_ALL,
                modules::STORAGE,
                this_messenger->serialize(request_invalid),
                &response_str3
            );

            msg_get_appliances_all_response response =
                this_messenger->deserialize<msg_get_appliances_all_response>(response_str3);
            for (const auto& response_appliance: response.appliances) {
                if (response_appliance == appliance) {
                    std::cout << "GET message returned a wrong item.\n";
                    return false;
                }
            }

            return true;
        };


        /* Test with is_schedulable = True. */
        appliance_t appliance1 = {
            id                  : 0,
            name                : "appliance",
            uri                 : "",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 1,
            tasks               : std::set<id_t>(),
            auto_profiles       : std::set<id_t>()
        };

        std::vector<msg_get_appliances_all_request> requests2 = {
            { is_schedulable  : tribool::INDETERMINATE },
            { is_schedulable  : tribool::TRUE },
        };

        msg_get_appliances_all_request request3 = { is_schedulable  : tribool::FALSE };

        if (!test_with_tribool(appliance1, requests2, request3)) {
            return false;
        }

        /* Test with is_schedulable = False. */
        appliance_t appliance2 = appliance1;
        appliance2.schedules_per_week = 0;

        std::vector<msg_get_appliances_all_request> requests4 = {
            { is_schedulable : tribool::INDETERMINATE },
            { is_schedulable : tribool::FALSE }
        };

        msg_get_appliances_all_request request5 = { is_schedulable : tribool::TRUE };

        if (!test_with_tribool(appliance2, requests4, request5)) {
            return false;
        }


        /* TODO Test foreign key rows. */

        return true;
    }


    /**
     * @brief   Test the handler for `MSG_GET_ENERGY_PRODUCTION` messages.
     */
    bool test_handler_msg_get_energy_production_(messenger* this_messenger, hems_storage_test* this_instance) {
        int res;

        msg_get_energy_production_request request1 = {
            start_time  : time_from_string("2020-01-01 00:00:00.000"),
            end_time    : time_from_string("9999-01-01 00:00:00.000")
        };

        /* Test getting entry when there is none. */
        res = this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_ENERGY_PRODUCTION,
            modules::STORAGE,
            this_messenger->serialize(request1),
            nullptr
        );
        if (res != response_code::MSG_GET_NONE_AVAILABLE) {
            std::cout <<
                "Expected code " + std::to_string(response_code::MSG_GET_NONE_AVAILABLE) + " but "
                "got " + std::to_string(res) + " instead.\n";
            return false;
        }

        /* Add an entry into `energy_production`. */
        energy_production_t energy_production = {
            time    : time_from_string("2020-01-01 00:00:00.000"),
            energy  : 76.3
        };
        std::string stmt =
            "INSERT INTO energy_production (time, energy) "
            "VALUES ("
                "'" + boost::posix_time::to_iso_string(energy_production.time) + "', " +
                std::to_string(energy_production.energy) +
            ")";

        if (!this_instance->prepare_and_evaluate_no_row_query(stmt)) {
            return false;
        }

        /* Check that the entry was successfully added. */
        std::string stmt2 =
            "SELECT * FROM energy_production WHERE "
            "time='" + boost::posix_time::to_iso_string(energy_production.time) + "'";

        sqlite3_stmt* prepared_stmt2 = nullptr;
        if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt2);
        }

        /* Get entry. */
        msg_get_energy_production_request request2 = {
            start_time  : energy_production.time,
            end_time    : time_from_string("9999-01-01 00:00:00.000")
        };

        std::string response_str2;
        res = this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_ENERGY_PRODUCTION,
            modules::STORAGE,
            this_messenger->serialize(request2),
            &response_str2
        );

        if (res != response_code::SUCCESS) {
            std::cout <<
                "Expected code " + std::to_string(response_code::SUCCESS) + " but got " +
                std::to_string(res) + " instead.\n";
            return false;
        } else {
            msg_get_energy_production_response response =
                this_messenger->deserialize<msg_get_energy_production_response>(response_str2);
            if (response.energy.find(energy_production.time) == response.energy.end()) {
                std::cout << "GET message returned a different item than expected.\n";
                return false;
            } else if (response.energy.at(energy_production.time) != energy_production) {
                std::cout << "GET message returned a wrong item.\n";
                return false;
            }
        }

        /* Make sure that the previously added entry is not returned for wrong request parameters. */
        std::vector<msg_get_energy_production_request> requests3 = {
            {
                start_time  : time_from_string("2020-01-01 00:00:00.001"),
                end_time    : time_from_string("9999-01-01 00:00:00.000")
            },
            {
                start_time  : time_from_string("2000-01-01 00:00:00.000"),
                end_time    : time_from_string("2019-12-31 23:59:59.999"),
            }
        };
        for (const auto& request3 : requests3) {
            std::string response_str3;
            res = this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messages::storage::msg_type::MSG_GET_ENERGY_PRODUCTION,
                modules::STORAGE,
                this_messenger->serialize(request3),
                &response_str3
            );

            if (res != response_code::MSG_GET_NONE_AVAILABLE) {
                std::cout <<
                    "Expected code " + std::to_string(response_code::MSG_GET_NONE_AVAILABLE) + " but "
                    "got " + std::to_string(res) + " instead.\n";
                return false;
            } else {
                msg_get_energy_production_response response =
                    this_messenger->deserialize<msg_get_energy_production_response>(response_str3);
                if (int n = response.energy.size()) {
                    std::cout <<
                        std::to_string(n) + " entries were returned despite error code saying otherwise.\n";
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * @brief   Test the handler for `MSG_GET_WEATHER` messages.
     */
    bool test_handler_msg_get_weather_(messenger* this_messenger, hems_storage_test* this_instance) {
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

        msg_get_weather_request request1 = {
            start_time  : time_from_string("2020-01-01 00:00:00.000"),
            end_time    : time_from_string("9999-01-01 00:00:00.000"),
            stations    : std::set<id_t>()
        };

        /* Test getting entry when there is none. */
        res = this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_WEATHER,
            modules::STORAGE,
            this_messenger->serialize(request1),
            nullptr
        );
        if (res != response_code::MSG_GET_NONE_AVAILABLE) {
            std::cout <<
                "Expected code " + std::to_string(response_code::MSG_GET_NONE_AVAILABLE) + " but "
                "got " + std::to_string(res) + " instead.\n";
            return false;
        }

        /* Add an entry into `weather`. */
        weather_t weather = {
            time        : time_from_string("2020-01-01 00:00:00.000"),
            station     : 2,
            temperature : 10.5,
            humidity    : 32,
            pressure    : 970,
            cloud_cover : 25,
            radiation   : 500,
        };
        std::string stmt =
            "INSERT INTO weather (time, station, temperature, humidity, pressure, cloud_cover, radiation) "
            "VALUES ("
                "'" + boost::posix_time::to_iso_string(weather.time) + "', " +
                std::to_string(weather.station) + ", " +
                std::to_string(weather.temperature) + ", " +
                std::to_string(weather.humidity) + ", " +
                std::to_string(weather.pressure) + ", " +
                std::to_string(weather.cloud_cover) + ", " +
                std::to_string(weather.radiation) +
            ")";

        if (!this_instance->prepare_and_evaluate_no_row_query(stmt)) {
            return false;
        }

        /* Check that the entry was successfully added. */
        std::string stmt2 =
            "SELECT * FROM weather WHERE "
            "time='" + boost::posix_time::to_iso_string(weather.time) + "' AND "
            "station=" + std::to_string(weather.station);

        sqlite3_stmt* prepared_stmt2 = nullptr;
        if (!this_instance->prepare_and_evaluate(stmt2, prepared_stmt2)) {
            return false;
        } else {
            sqlite3_finalize(prepared_stmt2);
        }

        /* Get entry. */
        std::vector<msg_get_weather_request> requests2 = {
            {
                start_time  : weather.time,
                end_time    : time_from_string("9999-01-01 00:00:00.000"),
                stations    : std::set<id_t>({1, 2}) /* Stations explicitly given. */
            },
            {
                start_time  : weather.time,
                end_time    : time_from_string("9999-01-01 00:00:00.000"),
                stations    : std::set<id_t>() /* No stations given. */
            },
        };
        for (const auto& request2 : requests2) {
            std::string response_str2;
            res = this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messages::storage::msg_type::MSG_GET_WEATHER,
                modules::STORAGE,
                this_messenger->serialize(request2),
                &response_str2
            );

            if (res != response_code::SUCCESS) {
                std::cout <<
                    "Expected code " + std::to_string(response_code::SUCCESS) + " but got " +
                    std::to_string(res) + " instead.\n";
                return false;
            } else {
                msg_get_weather_response response =
                    this_messenger->deserialize<msg_get_weather_response>(response_str2);
                if (response.time_to_weather.find(weather.time) == response.time_to_weather.end() ||
                    response.station_to_weather.find(weather.station) == response.station_to_weather.end()) {
                    std::cout << "GET message returned a different item than expected.\n";
                    return false;
                } else if ( response.time_to_weather.at(weather.time) != std::vector<weather_t>({weather}) ||
                            response.station_to_weather.at(weather.station) != std::vector<weather_t>({weather})) {
                    std::cout << "GET message returned a wrong item.\n";
                    return false;
                }
            }
        }

        /* Make sure that the previously added entry is not returned for wrong request parameters. */
        std::vector<msg_get_weather_request> requests3 = {
            {
                start_time  : time_from_string("2020-01-01 00:00:00.001"),
                end_time    : time_from_string("9999-01-01 00:00:00.000"),
                stations    : std::set<id_t>()
            },
            {
                start_time  : time_from_string("2000-01-01 00:00:00.000"),
                end_time    : time_from_string("2019-12-31 23:59:59.999"),
                stations    : std::set<id_t>()
            },
            {
                start_time  : time_from_string("2000-01-01 00:00:00.000"),
                end_time    : time_from_string("9999-01-01 00:00:00.000"),
                stations    : std::set<id_t>({3, 4})
            },
        };
        for (const auto& request3 : requests3) {
            std::string response_str3;
            res = this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messages::storage::msg_type::MSG_GET_WEATHER,
                modules::STORAGE,
                this_messenger->serialize(request3),
                &response_str3
            );

            if (res != response_code::MSG_GET_NONE_AVAILABLE) {
                std::cout <<
                    "Expected code " + std::to_string(response_code::MSG_GET_NONE_AVAILABLE) + " but "
                    "got " + std::to_string(res) + " instead.\n";
                return false;
            } else {
                msg_get_weather_response response =
                    this_messenger->deserialize<msg_get_weather_response>(response_str3);
                if (int n = response.time_to_weather.size() + response.station_to_weather.size()) {
                    std::cout <<
                        std::to_string(n) + " entries were returned despite error code saying otherwise.\n";
                    return false;
                }
            }
        }

        return true;
    }


    enum msg_get_test_types {
        SETTINGS, APPLIANCES, APPLIANCES_ALL, ENERGY_PRODUCTION, WEATHER
    };

    bool test_handler_msg_get(msg_get_test_types test_type) {
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
            case msg_get_test_types::SETTINGS:
                success = test_handler_msg_get_settings_(this_messenger, this_instance);
                break;
            case msg_get_test_types::APPLIANCES:
                success = test_handler_msg_get_appliances_(this_messenger, this_instance);
                break;
            case msg_get_test_types::APPLIANCES_ALL:
                success = test_handler_msg_get_appliances_all_(this_messenger, this_instance);
                break;
            case msg_get_test_types::ENERGY_PRODUCTION:
                success = test_handler_msg_get_energy_production_(this_messenger, this_instance);
                break;
            case msg_get_test_types::WEATHER:
                success = test_handler_msg_get_weather_(this_messenger, this_instance);
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

    static inline bool test_handler_msg_get_settings() {
        return test_handler_msg_get(msg_get_test_types::SETTINGS);
    }

    static inline bool test_handler_msg_get_appliances() {
        return test_handler_msg_get(msg_get_test_types::APPLIANCES);
    }

    static inline bool test_handler_msg_get_appliances_all() {
        return test_handler_msg_get(msg_get_test_types::APPLIANCES_ALL);
    }

    static inline bool test_handler_msg_get_energy_production() {
        return test_handler_msg_get(msg_get_test_types::ENERGY_PRODUCTION);
    }

    static inline bool test_handler_msg_get_weather() {
        return test_handler_msg_get(msg_get_test_types::WEATHER);
    }

}}}

int main() {
    return run_tests({
        {
            "01 Storage: Message handler test for MSG_GET_SETTINGS",
            &hems::modules::storage::test_handler_msg_get_settings
        },
        {
            "02 Storage: Message handler test for MSG_GET_APPLIANCES",
            &hems::modules::storage::test_handler_msg_get_appliances
        },
        {
            "03 Storage: Message handler test for MSG_GET_APPLIANCES_ALL",
            &hems::modules::storage::test_handler_msg_get_appliances_all
        },
        {
            "04 Storage: Message handler test for MSG_GET_ENERGY_PRODUCTION",
            &hems::modules::storage::test_handler_msg_get_energy_production
        },
        {
            "05 Storage: Message handler test for MSG_GET_WEATHER",
            &hems::modules::storage::test_handler_msg_get_weather
        }
    });
}
