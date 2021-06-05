/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the User Interface Module.
 * This module is responsible for offering an interface for the user to interact with the HEMS,
 * presenting the current state and offering commands.
 */

#include <exception>
#include <functional>
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "extras/http_worker.hpp"

#include "hems/modules/ui/ui.h"
#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/automation.h"
#include "hems/messages/collection.h"
#include "hems/messages/inference.h"
#include "hems/messages/storage.h"

namespace hems { namespace modules { namespace ui {

    namespace http = boost::beast::http;

    using boost::asio::ip::tcp;
    using boost::posix_time::ptime;
    using boost::posix_time::time_from_string;

    static std::string host = "127.0.0.1";

    #ifndef UI_PORT
    static std::string port = "2222";
    #else
    static std::string port = std::to_string(UI_PORT);
    #endif

    static std::string dynamic_content_area = "<!-- DYNAMIC_CONTENT_AREA -->";


    std::string handler_wrapper_set_stations(std::map<std::string, std::string>& request_map) {
        return hems_ui::this_instance->handler_set_stations(request_map);
    }

    std::string hems_ui::handler_set_stations(std::map<std::string, std::string>& request_map) {
        std::string res;

        std::string station_number_str, station_interval_str;
        try {
            station_number_str = request_map.at("station_number");
            station_interval_str = request_map.at("station_interval");
        } catch (std::out_of_range& e) {
            res = "Station number or interval for settings was missing.";
            logger::this_logger->log(res, logger::level::LOG);
            return res;
        }

        types::id_t station_number;
        unsigned int station_interval;
        try {
            station_number = std::stoul(station_number_str);
            station_interval = std::stoul(station_interval_str);
        } catch (boost::bad_lexical_cast& e) {
            res = "Station number or interval for settings was formatted incorrectly.";
            logger::this_logger->log(res, logger::level::LOG);
            return res;
        }

        std::map<types::id_t, unsigned int> station_intervals = current_settings.station_intervals;
        station_intervals[station_number] = station_interval;
        types::settings_t new_settings = current_settings;
        new_settings.station_intervals = station_intervals;
        int settings_update_res = messenger::this_messenger->broadcast_settings(new_settings);
        if (settings_update_res == messenger::settings_code::SUCCESS) {
            res =
                "Successfully set interval " + std::to_string(station_interval) + " for station " +
                std::to_string(station_number) + ".";
            logger::this_logger->log(res, logger::level::LOG);
        } else {
            res =
                "Error " + std::to_string(settings_update_res) + " setting interval " +
                std::to_string(station_interval) + " for station " + std::to_string(station_number) + ".";
            logger::this_logger->log(res, logger::level::ERR);
        }

        return res;
    }


    std::string handler_wrapper_download_weather(std::map<std::string, std::string>& request_map) {
        return hems_ui::this_instance->handler_download_weather(request_map);
    }

    std::string hems_ui::handler_download_weather(std::map<std::string, std::string>& request_map) {
        std::string res;

        std::string start_time_str, end_time_str;
        try {
            start_time_str = request_map.at("start_time");
            end_time_str = request_map.at("end_time");
        } catch (std::out_of_range& e) {
            res = "Start or end time for weather data download was missing.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        ptime start_time, end_time;
        try {
            start_time = time_from_string(start_time_str);
            end_time = time_from_string(end_time_str);
        } catch (boost::bad_lexical_cast& e) {
            res = "Start or end time for weather data download was formatted incorrectly.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        if (start_time >= end_time) {
            res =
                "Start time for weather data was greater than end time: start_time=" +
                start_time_str + ", end_time=" + end_time_str;

            logger::this_logger->log(res, logger::level::LOG);
            return res;
        }

        std::map<types::id_t, std::vector<ptime>> errors;

        for (const auto& station_interval : current_settings.station_intervals) {
            boost::posix_time::time_iterator it(
                start_time, boost::posix_time::minutes(station_interval.second)
            );

            while (it <= end_time) {
                messages::collection::msg_download_weather_data_request req = {
                    time        : *it,
                    station     : station_interval.first
                };

                int code = messenger::this_messenger->send(
                    3 * DEFAULT_SEND_TIMEOUT, // Use a bigger timeout here to account for connection issues.
                    messages::collection::MSG_DOWNLOAD_WEATHER_DATA,
                    modules::COLLECTION,
                    messenger::serialize(req),
                    nullptr
                );
                if (code) {
                    if (errors.find(station_interval.first) == errors.end()) {
                        errors.emplace(station_interval.first, std::vector<ptime>({*it}));
                    } else {
                        errors.at(station_interval.first).emplace_back(*it);
                    }
                    if (code == messages::collection::response_code::UNREACHABLE_SOURCE) {
                        break;
                    }
                }

                ++it;
            }
        }

        if (errors.size()) {
            res = "There was an error downloading weather data for the following parameters: ";
            for (const auto& error : errors) {
                res += "station: " + std::to_string(error.first) + ", time: ";
                if (!error.second.size()) {
                    res += "None";
                } else {
                    for (const auto& time : error.second) {
                        res += boost::posix_time::to_simple_string(time) + ", ";
                    }
                    res.pop_back();
                    res.pop_back();
                }
                res += "; ";
            }
            res.pop_back();
            res.pop_back();
            logger::this_logger->log(res, logger::level::ERR);
        } else {
            res =
                "All weather data between " + start_time_str + " and " + end_time_str +
                " was downloaded successfully for the following stations: ";
            if (!current_settings.station_intervals.size()) {
                res += "None";
            } else {
                for (const auto& station_interval : current_settings.station_intervals) {
                    res += std::to_string(station_interval.first) + ", ";
                }
                res.pop_back();
                res.pop_back();
            }
            logger::this_logger->log(res, logger::level::LOG);
        }

        return res;
    }


    std::string handler_wrapper_download_energy_production(std::map<std::string, std::string>& request_map) {
        return hems_ui::this_instance->handler_download_energy_production(request_map);
    }

    std::string hems_ui::handler_download_energy_production(std::map<std::string, std::string>& request_map) {
        std::string res;

        std::string start_time_str, end_time_str;
        try {
            start_time_str = request_map.at("start_time");
            end_time_str = request_map.at("end_time");
        } catch (std::out_of_range& e) {
            res = "Start or end time for energy production data download was missing.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        ptime start_time, end_time;
        try {
            start_time = time_from_string(start_time_str);
            end_time = time_from_string(end_time_str);
        } catch (boost::bad_lexical_cast& e) {
            res = "Start or end time for energy production data download was formatted incorrectly.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        if (start_time >= end_time) {
            res =
                "Start time for energy production data was greater than end time: start_time=" +
                start_time_str + ", end_time=" + end_time_str;

            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        std::vector<ptime> errors;

        boost::posix_time::time_iterator it(
            start_time, boost::posix_time::minutes(current_settings.interval_energy_production)
        );

        while (it <= end_time) {
            messages::collection::msg_download_energy_production_request req = {
                time : *it
            };

            int code = messenger::this_messenger->send(
                3 * DEFAULT_SEND_TIMEOUT, // Use a bigger timeout here to account for connection issues.
                messages::collection::MSG_DOWNLOAD_ENERGY_PRODUCTION,
                modules::COLLECTION,
                messenger::serialize(req),
                nullptr
            );
            if (code) {
                errors.emplace_back(*it);
                if (code == messages::collection::response_code::UNREACHABLE_SOURCE) {
                    break;
                }
            }

            ++it;
        }

        if (errors.size()) {
            res = "There was an error downloading energy production data for the following times: ";
            for (const auto& time : errors) {
                res += boost::posix_time::to_simple_string(time) + ", ";
            }
            res.pop_back();
            res.pop_back();
            logger::this_logger->log(res, logger::level::ERR);
        } else {
            res =
                "All energy production data between " + start_time_str + " and " + end_time_str +
                " was downloaded successfully.";
            logger::this_logger->log(res, logger::level::LOG);
        }

        return res;
    }


    std::string handler_wrapper_get_energy_production_predictions(std::map<std::string, std::string>& request_map) {
        return hems_ui::this_instance->handler_get_energy_production_predictions(request_map);
    }

    std::string hems_ui::handler_get_energy_production_predictions(std::map<std::string, std::string>& request_map) {
        std::string res;

        std::string start_time_str;
        try {
            start_time_str = request_map.at("start_time");
        } catch (std::out_of_range& e) {
            res = "Start time for getting energy production predictions was missing.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        ptime start_time;
        try {
            start_time = time_from_string(start_time_str);
        } catch (boost::bad_lexical_cast& e) {
            res = "Start time for getting energy production predictions was formatted incorrectly.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        messages::inference::msg_get_predictions_request request = {
            start_time : start_time
        };

        std::string response_serialized;

        int code = messenger::this_messenger->send(
            3 * DEFAULT_SEND_TIMEOUT, // Use a bigger timeout here to account for connection issues.
            messages::inference::MSG_GET_PREDICTIONS,
            modules::INFERENCE,
            messenger::serialize(request),
            &response_serialized
        );

        if (code) {
            res =
                "Error " + std::to_string(code) + " getting energy production predictions for the " +
                "week starting at " + start_time_str;
            logger::this_logger->log(res, logger::level::ERR);
        } else {
            messages::inference::msg_get_predictions_response response =
                messenger::deserialize<messages::inference::msg_get_predictions_response>(
                    response_serialized
                );

            res =
                "Successfully got energy production predictions for the week starting at " +
                start_time_str + ": ";

            for (const auto& energy : response.energy) {
                res += to_string(energy.second);
                res += ", ";
            }
            res.pop_back();
            res.pop_back();

            logger::this_logger->log(
                "Successfully got energy production predictions for the week starting at " +
                    start_time_str + ".",
                logger::level::LOG
            );
        }

        return res;
    }


    std::string handler_wrapper_set_appliance(std::map<std::string, std::string>& request_map) {
        return hems_ui::this_instance->handler_set_appliance(request_map);
    }

    std::string hems_ui::handler_set_appliance(std::map<std::string, std::string>& request_map) {
        std::string res;

        std::string id_str, name_str, rating_str, duty_cycle_str, schedules_per_week_str;

        try {
            id_str = request_map.at("id");
            name_str = request_map.at("name");
            rating_str = request_map.at("rating");
            duty_cycle_str = request_map.at("duty_cycle");
            schedules_per_week_str = request_map.at("schedules_per_week");
        } catch (std::out_of_range& e) {
            res = "One or more parameters for setting an appliance were missing.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        types::appliance_t appliance;
        try {
            appliance.id = std::stoul(id_str);
            appliance.name = name_str;
            appliance.rating = std::stod(rating_str);
            appliance.duty_cycle = std::stoul(duty_cycle_str);
            appliance.schedules_per_week = std::stoul(schedules_per_week_str);
        } catch (boost::bad_lexical_cast& e) {
            res = "One or more parameters for setting an appliance were formatted incorrectly.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        messages::storage::msg_set_appliance_request request = {
            appliance : appliance
        };

        std::string response_serialized;

        int code = messenger::this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::MSG_SET_APPLIANCE,
            modules::STORAGE,
            messenger::serialize(request),
            &response_serialized
        );

        if (code) {
            res = "Error " + std::to_string(code) + " setting an appliance: " + to_string(appliance);
            logger::this_logger->log(res, logger::level::ERR);
        } else {
            messages::storage::msg_set_response response =
                messenger::deserialize<messages::storage::msg_set_response>(response_serialized);

            res = "Successfully set an appliance with id " + std::to_string(response.id) + ".";
            logger::this_logger->log(res, logger::level::LOG);
        }

        return res;
    }


    std::string handler_wrapper_get_appliances(std::map<std::string, std::string>& request_map) {
        return hems_ui::this_instance->handler_get_appliances(request_map);
    }

    std::string hems_ui::handler_get_appliances(std::map<std::string, std::string>& request_map) {
        std::string res;

        messages::storage::msg_get_appliances_all_request request = {
            is_schedulable : messages::storage::tribool::INDETERMINATE
        };

        std::string response_serialized;

        int code = messenger::this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::MSG_GET_APPLIANCES_ALL,
            modules::STORAGE,
            messenger::serialize(request),
            &response_serialized
        );

        if (code == messages::storage::response_code::SUCCESS) {
             messages::storage::msg_get_appliances_all_response response =
                messenger::deserialize<messages::storage::msg_get_appliances_all_response>(response_serialized);

            for (const auto& appliance : response.appliances) {
                res += to_string(appliance) + ", ";
            }
            res.pop_back();
            res.pop_back();

            logger::this_logger->log("Successfully got all appliances.", logger::level::LOG);
        } else if (code == messages::storage::response_code::MSG_GET_NONE_AVAILABLE) {
            res = "No appliances available.";
            logger::this_logger->log(res, logger::level::LOG);
        } else {
            res = "Error " + std::to_string(code) + " getting appliances.";
            logger::this_logger->log(res, logger::level::ERR);
        }

        return res;
    }


    std::string handler_wrapper_del_appliance(std::map<std::string, std::string>& request_map) {
        return hems_ui::this_instance->handler_del_appliance(request_map);
    }

    std::string hems_ui::handler_del_appliance(std::map<std::string, std::string>& request_map) {
        std::string res;

        std::string id_str;
        try {
            id_str = request_map.at("id");
        } catch (std::out_of_range& e) {
            res = "Parameter 'id' for deleting an appliance was missing.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        id_t id;
        try {
            id = std::stoul(id_str);
        } catch (boost::bad_lexical_cast& e) {
            res = "Parameter 'id' for deleting an appliance was formatted incorrectly.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        if (id) {
            messages::storage::msg_del_appliance_request request = {
                id : id
            };

            int code = messenger::this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messages::storage::MSG_DEL_APPLIANCE,
                modules::STORAGE,
                messenger::serialize(request),
                nullptr
            );

            if (code) {
                res =
                    "Error " + std::to_string(code) + " deleting appliance with id " +
                    std::to_string(id) + ".";
                logger::this_logger->log(res, logger::level::ERR);
            } else {
                res = "Successfully deleted appliance with id " + std::to_string(id) + ".";
                logger::this_logger->log(res, logger::level::LOG);
            }
        } else {
            messages::storage::msg_get_appliances_all_request request = {
                is_schedulable : messages::storage::tribool::INDETERMINATE
            };

            std::string response_serialized;

            int code = messenger::this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messages::storage::MSG_GET_APPLIANCES_ALL,
                modules::STORAGE,
                messenger::serialize(request),
                &response_serialized
            );

            if (code) {
                res = "Error " + std::to_string(code) + " getting appliances.";
                logger::this_logger->log(res, logger::level::ERR);
            } else {
                messages::storage::msg_get_appliances_all_response response =
                    messenger::deserialize<messages::storage::msg_get_appliances_all_response>(response_serialized);

                int code = 0;
                for (const auto& appliance : response.appliances) {
                    messages::storage::msg_del_appliance_request request = {
                        id : appliance.id
                    };

                    code = messenger::this_messenger->send(
                        DEFAULT_SEND_TIMEOUT,
                        messages::storage::MSG_DEL_APPLIANCE,
                        modules::STORAGE,
                        messenger::serialize(request),
                        nullptr
                    );

                    if (code) {
                        res =
                            "Error " + std::to_string(code) + " deleting appliance with id " +
                            std::to_string(id) + ".";
                        logger::this_logger->log(res, logger::level::ERR);
                        break;
                    }
                }

                if (!code) {
                    res = "Successfully deleted all appliances.";
                    logger::this_logger->log(res, logger::level::LOG);
                }
            }
        }

        return res;
    }


    std::string handler_wrapper_get_recommendations(std::map<std::string, std::string>& request_map) {
        return hems_ui::this_instance->handler_get_recommendations(request_map);
    }

    std::string hems_ui::handler_get_recommendations(std::map<std::string, std::string>& request_map) {
        std::string res;

        std::string start_time_str, order_str, heuristic_str;
        try {
            start_time_str = request_map.at("start_time");
            order_str = request_map.at("order");
            heuristic_str = request_map.at("heuristic");
        } catch (std::out_of_range& e) {
            res = "Parameters for requesting task recommendations were missing.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        ptime start_time;
        try {
            start_time = time_from_string(start_time_str);
        } catch (boost::bad_lexical_cast& e) {
            res = "Start time for getting energy production predictions was formatted incorrectly.";
            logger::this_logger->log(res, logger::level::ERR);
            return res;
        }

        messages::automation::order order;
        if (order_str == "width") {
            order = messages::automation::order::WIDTH_DESC;
        } else if (order_str == "height") {
            order = messages::automation::order::HEIGHT_DESC;
        } else {
            order = messages::automation::order::AREA_DESC;
        }

        messages::automation::heuristic heuristic;
        if (heuristic_str == "first") {
            heuristic = messages::automation::heuristic::FIRST_FIT;
        } else if (heuristic_str == "next") {
            heuristic = messages::automation::heuristic::NEXT_FIT;
        } else {
            heuristic = messages::automation::heuristic::BEST_FIT;
        }

        messages::automation::msg_get_recommendations_request request = {
            start_time      : start_time,
            rect_order      : order,
            alloc_heuristic : heuristic
        };

        std::string response_serialized;

        int code = messenger::this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::automation::MSG_GET_RECOMMENDATIONS,
            modules::AUTOMATION,
            messenger::serialize(request),
            &response_serialized
        );

        if (code) {
            res =
                "Error " + std::to_string(code) + " getting task recommendations for the week " +
                "starting at " + start_time_str;
            logger::this_logger->log(res, logger::level::ERR);
        } else {
            messages::automation::msg_get_recommendations_response response =
                messenger::deserialize<messages::automation::msg_get_recommendations_response>(
                    response_serialized
                );

            res =
                "Successfully got task recommendations for the week starting at " + start_time_str + ": ";

            for (const auto& recommendation : response.recommendations) {
                res += to_string(recommendation);
                res += ", ";
            }
            res.pop_back();
            res.pop_back();

            logger::this_logger->log(
                "Successfully got task recommendations for the week starting at " + start_time_str + ".",
                logger::level::LOG
            );
        }

        return res;
    }


    const std::map<std::string, const std::function<std::string(std::map<std::string, std::string>&)>>
    hems_ui::request_handlers = {
        { "set_stations", handler_wrapper_set_stations },
        { "download_weather", handler_wrapper_download_weather },
        { "download_energy_production", handler_wrapper_download_energy_production },
        { "get_energy_production_predictions", handler_wrapper_get_energy_production_predictions },
        { "set_appliance", handler_wrapper_set_appliance },
        { "get_appliances", handler_wrapper_get_appliances },
        { "del_appliances", handler_wrapper_del_appliance },
        { "get_recommendations", handler_wrapper_get_recommendations }
    };


    std::string hems_ui::set_dynamic_content_of_file(std::string filename, std::string dynamic_content) {
        std::ifstream file(filename);
        std::stringstream buf;
        buf << file.rdbuf();
        std::string buf_str = buf.str();

        auto dynamic_content_pos = buf_str.find(dynamic_content_area);
        if (dynamic_content_pos != std::string::npos) {
            buf_str.replace(dynamic_content_pos, dynamic_content_area.size(), dynamic_content);
        }

        return buf_str;
    }

    static void sanitize(std::string& input) {
        boost::replace_all(input, "+", " ");
        boost::replace_all(input, "%3A", ":");
    }


    std::tuple<const std::string, const std::string, bool> post_callback_wrapper(
        const std::string& body, const std::string& target
    ) {
        return hems_ui::this_instance->post_callback(body, target);
    }

    std::tuple<const std::string, const std::string, bool> hems_ui::post_callback(
        const std::string& body, const std::string& target
    ) {
        logger::this_logger->log(
            "Received a HTTP POST request: Body: " + body + ", target: " + target,
            logger::level::DBG
        );

        std::vector<std::string> body_vector;
        boost::split(body_vector, body, boost::is_any_of("&"));

        std::map<std::string, std::string> request_map;

        for (const auto& entry : body_vector) {
            std::vector<std::string> entry_vector;
            boost::split(entry_vector, entry, boost::is_any_of("="));

            std::string key = entry_vector.at(0);
            std::string value = entry_vector.at(1);
            sanitize(value);
            request_map.emplace(key, value);
        }

        std::string res;

        try {
            std::string request_type = request_map.at("action");
            logger::this_logger->log("Handling action: " + body, logger::level::LOG);
            res = request_handlers.at(request_type)(request_map);
        } catch (std::out_of_range& e) {
            res = "No action was specified, or it was unknown: " + body;
            logger::this_logger->log(res, logger::level::LOG);
        }

        res = set_dynamic_content_of_file(ui_server_root + "/" + target, res);
        return std::make_tuple(res, "text/html", false);
    }

    void hems_ui::listen() {
        boost::asio::io_context ioc(1);

        try {
            /*  This pragma is required because for whatever bizarre reason C++ does not have a
                string-to-unsigned-int function... Since a port is never going to be beyond the max
                value for unsigned int, it is okay to simply ignore the GCC warning here. */
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wnarrowing"
            tcp::acceptor acceptor(
                ioc, { boost::asio::ip::make_address(host), std::stoul(port) }
            );
            #pragma GCC diagnostic pop

            logger::this_logger->log(
                "Starting user interface HTTP server under " + host + ":" + port + ".",
                logger::level::LOG
            );

            /*  One worker is sufficient, as only one user at a time is really plausible for this
                application. */
            http_worker worker(acceptor, ui_server_root, "/html/hems.html", post_callback_wrapper);

            worker.start();
            ioc.run();
        } catch (boost::wrapexcept<boost::system::system_error>& e) {
            logger::this_logger->log(
                "Could not start worker for the user interface HTTP server: " + std::string(e.what()) +
                    ", terminating.",
                logger::level::ERR
            );
            hems::exit(EXIT_FAILURE);
        } catch (std::exception& e) {
            logger::this_logger->log(
                "Worker for the user interface HTTP server has thrown an exception: '" +
                    std::string(e.what()) + "', terminating.",
                logger::level::ERR
            );
            hems::exit(EXIT_FAILURE);
        }
    }

}}}
