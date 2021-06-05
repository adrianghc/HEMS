/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is a submodule of the Measurement Collection Module.
 * This module is responsible for collecting measurement data. Each type of continuously
 * accumulating measurement data is pulled from its source through an appropriate submodule at a
 * given schedule.
 */

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "hems/modules/collection/collection.h"
#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/common/exit.h"
#include "hems/messages/collection.h"
#include "hems/messages/storage.h"

namespace hems { namespace modules { namespace collection {

    using namespace hems::messages::collection;

    namespace http = boost::beast::http;

    using boost::asio::ip::tcp;
    using boost::posix_time::ptime;

    #ifndef COLLECTION_WEATHER_HOST
    static std::string host = "127.0.0.1";
    #else
    static std::string host = COLLECTION_WEATHER_HOST;
    #endif

    #ifndef COLLECTION_WEATHER_PORT
    static std::string port = "2022";
    #else
    static std::string port = std::to_string(COLLECTION_WEATHER_PORT);
    #endif

    int hems_collection::download_weather_data(ptime time, id_t station) {
        if (current_settings.station_intervals.find(station) == current_settings.station_intervals.end()) {
            logger::this_logger->log(
                "Invalid value for weather station provided: " + std::to_string(station),
                logger::level::ERR
            );
            return response_code::INVALID_DATA;
        }

        types::weather_t weather;

        /* Check that time is within the interval. */
        auto interval = current_settings.station_intervals.at(station);
        if (time.time_of_day().fractional_seconds() || time.time_of_day().seconds() ||
            time.time_of_day().minutes() % interval) {
            logger::this_logger->log(
                "Invalid value provided for time: Must be a multiple of " + std::to_string(interval) +
                    " full minutes but was " + boost::posix_time::to_simple_string(time) + ".",
                logger::level::ERR
            );
            return response_code::INVALID_DATA;
        }
        weather.time = time;
        weather.station = station;

        /* Format the time into a string for the URL. */
        boost::posix_time::time_facet* facet = new boost::posix_time::time_facet();
        facet->format("%Y%m%d%H%M");

        std::stringstream time_stream;
        time_stream.imbue(std::locale(std::locale::classic(), facet));
        time_stream << time;

        std::string path = "/?time=" + time_stream.str() + "&station=" + std::to_string(station);

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
                "Could not connect to source server for weather data at " + host + ":" + port + ".",
                logger::level::ERR
            );
            return response_code::UNREACHABLE_SOURCE;
        }

        /* Prepare HTTP request. */
        http::request<http::string_body> req{http::verb::get, path, 11};
        req.set(http::field::host, host);

        /* Send HTTP request and read response. */
        http::write(stream, req);
        http::response<http::string_body> response;
        http::read(stream, buf, response);

        /* Extract content from HTTP response body. */
        try {
            std::vector<std::string> weather_vector;
            boost::split(weather_vector, response.body(), boost::is_any_of("\n"));

            if (weather_vector.size() != 5) {
                logger::this_logger->log(
                    "Received invalid energy production from HTTP response body.", logger::level::ERR
                );
                stream.socket().shutdown(tcp::socket::shutdown_both, ec);
                return response_code::INVALID_RESPONSE_SOURCE;
            } else {
                weather.temperature = std::stod(weather_vector.at(0));
                weather.humidity = std::stoul(weather_vector.at(1));
                weather.pressure = std::stod(weather_vector.at(2));
                weather.cloud_cover = std::stoul(weather_vector.at(3));
                weather.radiation = std::stoul(weather_vector.at(4));
            }
        } catch (const std::invalid_argument& e) {
            logger::this_logger->log(
                "Could not extract weather from HTTP response body.", logger::level::ERR
            );
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            return response_code::INVALID_RESPONSE_SOURCE;
        }

        /* Close connection. */
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        /* Send result to Data Storage Module. */
        messages::storage::msg_set_weather_request msg_set = {
            weather : weather
        };

        int res = messenger::this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_SET_WEATHER,
            modules::STORAGE,
            messenger::serialize(msg_set),
            nullptr
        );
        if (res != messages::storage::response_code::SUCCESS) {
            logger::this_logger->log(
                "Error " + std::to_string(res) + " adding weather data: " +
                    types::to_string(weather),
                logger::level::ERR
            );
            return response_code::DATA_STORAGE_MODULE_ERR;
        }

        return response_code::SUCCESS;
    }

}}}
