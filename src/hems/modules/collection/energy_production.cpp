/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is a submodule of the Measurement Collection Module.
 * This module is responsible for collecting measurement data. Each type of continuously
 * accumulating measurement data is pulled from its source through an appropriate submodule at a
 * given schedule.
 */

#include <string>

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

    #ifndef COLLECTION_ENERGY_PRODUCTION_HOST
    static std::string host = "127.0.0.1";
    #else
    static std::string host = COLLECTION_ENERGY_PRODUCTION_HOST;
    #endif

    #ifndef COLLECTION_ENERGY_PRODUCTION_PORT
    static std::string port = "2020";
    #else
    static std::string port = std::to_string(COLLECTION_ENERGY_PRODUCTION_PORT);
    #endif

    int hems_collection::download_energy_production(ptime start_time) {
        types::energy_production_t energy_production;

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
        energy_production.time = start_time;

        /* Format the time into a string for the URL. */
        boost::posix_time::time_facet* facet = new boost::posix_time::time_facet();
        facet->format("%Y%m%d%H%M");

        std::stringstream time_stream;
        time_stream.imbue(std::locale(std::locale::classic(), facet));
        time_stream << start_time;

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
        http::request<http::string_body> req{http::verb::get, path, 11};
        req.set(http::field::host, host);

        /* Send HTTP request and read response. */
        http::write(stream, req);
        http::response<http::string_body> response;
        http::read(stream, buf, response);

        /* Extract content from HTTP response body. */
        try {
            energy_production.energy = std::stod(response.body());
        } catch (const std::invalid_argument& e) {
            logger::this_logger->log(
                "Could not extract energy production from HTTP response body.", logger::level::ERR
            );
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            return response_code::INVALID_RESPONSE_SOURCE;
        }

        /* Close connection. */
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        /* Send result to Data Storage Module. */
        messages::storage::msg_set_energy_production_request msg_set = {
            energy_production : energy_production
        };

        int res = messenger::this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_SET_ENERGY_PRODUCTION,
            modules::STORAGE,
            messenger::serialize(msg_set),
            nullptr
        );
        if (res != messages::storage::response_code::SUCCESS) {
            logger::this_logger->log(
                "Error " + std::to_string(res) + " adding energy production data: " +
                    types::to_string(energy_production),
                logger::level::ERR
            );
            return response_code::DATA_STORAGE_MODULE_ERR;
        }

        return response_code::SUCCESS;
    }

}}}
