/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Knowledge Inference Module.
 * This module is responsible for inference knowledge from the energy production model. Knowledge 
 * is pulled from its source through an appropriate submodule at a given schedule.
 */

#include <sstream>
#include <set>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "hems/modules/inference/inference.h"
#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/common/exit.h"
#include "hems/messages/inference.h"
#include "hems/messages/storage.h"

namespace hems { namespace modules { namespace inference {

    using namespace hems::messages::inference;

    namespace http = boost::beast::http;

    using boost::archive::xml_iarchive;
    using boost::archive::xml_oarchive;
    using boost::asio::ip::tcp;
    using boost::posix_time::ptime;
    using boost::posix_time::minutes;

    #ifndef INFERENCE_HOST
    static std::string host = "127.0.0.1";
    #else
    static std::string host = INFERENCE_HOST;
    #endif

    #ifndef INFERENCE_PORT
    static std::string port = "2024";
    #else
    static std::string port = std::to_string(INFERENCE_PORT);
    #endif

    int hems_inference::get_energy_production_predictions(
        ptime start_time, std::map<ptime, types::energy_production_t>& energy_production
    ) {
        /* Check that time is within the interval. */
        auto interval = current_settings.interval_energy_production;
        if (start_time.time_of_day().fractional_seconds() || start_time.time_of_day().seconds() ||
            start_time.time_of_day().minutes() % interval) {
            logger::this_logger->log(
                "Invalid value provided for start_time: Must be a multiple of " +
                    std::to_string(interval) + " full minutes but was " +
                    boost::posix_time::to_simple_string(start_time) + ".",
                logger::level::ERR
            );
            return response_code::INVALID_DATA;
        }

        /* Format the time into a string for the URL. */
        boost::posix_time::time_facet* facet = new boost::posix_time::time_facet();
        facet->format("%Y%m%d%H%M");

        std::stringstream time_stream;
        time_stream.imbue(std::locale(std::locale::classic(), facet));
        time_stream << start_time;


        /* Get the past and the next week of weather data. */
        std::set<types::id_t> stations;
        for (const auto& station_interval : current_settings.station_intervals) {
            stations.emplace(station_interval.first);
        }

        messages::storage::msg_get_weather_request weather_request = {
            start_time  : start_time - boost::posix_time::hours(24*7),
            end_time    : start_time + boost::posix_time::hours(24*7),
            stations    : stations
        };

        std::string weather_response_serialized;

        int res = messenger::this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_WEATHER,
            modules::STORAGE,
            messenger::serialize(weather_request),
            &weather_response_serialized
        );

        if (res != messages::storage::response_code::SUCCESS) {
            logger::this_logger->log(
                "Error " + std::to_string(res) + " getting weather data.",
                logger::level::ERR
            );
            return response_code::DATA_STORAGE_MODULE_ERR;
        }

        messages::storage::msg_get_weather_response weather_response =
            messenger::deserialize<messages::storage::msg_get_weather_response>(weather_response_serialized);

        /* Serialize the weather data into an XML string. */
        std::ostringstream ostream_weather;
        boost::archive::xml_oarchive oa_weather(ostream_weather);

        oa_weather << BOOST_SERIALIZATION_NVP(weather_response.time_to_weather);


        /* Get the past week of energy production data. */
        messages::storage::msg_get_energy_production_request energy_production_request = {
            start_time  : start_time - boost::posix_time::hours(24*7),
            end_time    : start_time
        };

        std::string energy_production_response_serialized;

        res = messenger::this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_ENERGY_PRODUCTION,
            modules::STORAGE,
            messenger::serialize(energy_production_request),
            &energy_production_response_serialized
        );

        if (res != messages::storage::response_code::SUCCESS) {
            logger::this_logger->log(
                "Error " + std::to_string(res) + " getting energy production data.",
                logger::level::ERR
            );
            return response_code::DATA_STORAGE_MODULE_ERR;
        }

        messages::storage::msg_get_energy_production_response energy_production_response =
            messenger::deserialize<messages::storage::msg_get_energy_production_response>(
                energy_production_response_serialized
            );

        /* Serialize the energy production data into an XML string. */
        std::ostringstream ostream_energy_production;
        boost::archive::xml_oarchive oa_energy_production(ostream_energy_production);

        oa_energy_production << BOOST_SERIALIZATION_NVP(energy_production_response.energy);


        /* Create request path. */
        std::string path = "/?time=" + time_stream.str();

        /* Prepare buffer, resolver and TCP stream for Boost HTTP client. */
        boost::beast::flat_buffer buf;
        boost::asio::io_context ioc;
        tcp::resolver resolver(ioc);
        boost::beast::tcp_stream stream(ioc);
        boost::system::error_code ec;

        try {
            stream.connect(resolver.resolve(host, port));
        } catch (const boost::wrapexcept<boost::system::system_error>& e) {
            logger::this_logger->log(
                "Could not connect to source server for energy production data at " + host + ":" + port + ".",
                logger::level::ERR
            );
            return response_code::UNREACHABLE_SOURCE;
        }

        /* Prepare HTTP request. */
        std::string delimiter = "\n-----\n";
        std::string body = ostream_weather.str() + delimiter + ostream_energy_production.str();

        http::request<http::string_body> req{http::verb::post, path, 11};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "text/plain");
        req.body() = body;
        req.prepare_payload();

        /* Send HTTP request and read response. */
        http::write(stream, req);
        http::response<http::string_body> response;
        http::read(stream, buf, response);

        /* Extract content from HTTP response body. */
        try {
            std::vector<std::string> energy_vector;
            boost::split(energy_vector, response.body(), boost::is_any_of("\n"));

            if (energy_vector.size() != 24*7 * 60/current_settings.interval_energy_production) {
                logger::this_logger->log(
                    "Received invalid energy production from HTTP response body.", logger::level::ERR
                );
                stream.socket().shutdown(tcp::socket::shutdown_both, ec);
                return response_code::INVALID_RESPONSE_SOURCE;
            } else {
                for (std::size_t i = 0; i < energy_vector.size(); ++i) {
                    ptime time = start_time + minutes(i * current_settings.interval_energy_production);
                    types::energy_production_t energy = {
                        time    : time,
                        energy  : std::stod(energy_vector.at(i))
                    };
                    energy_production.emplace(time, energy);
                }
            }
        } catch (const std::invalid_argument& e) {
            logger::this_logger->log(
                "Could not extract energy production from HTTP response body.", logger::level::ERR
            );
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            return response_code::INVALID_RESPONSE_SOURCE;
        }

        /* Close connection. */
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return response_code::SUCCESS;
    }

}}}
