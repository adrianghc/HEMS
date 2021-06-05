/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Measurement Collection Module.
 * This module is responsible for collecting measurement data. Each type of continuously
 * accumulating measurement data is pulled from its source through an appropriate submodule at a
 * given schedule.
 */

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "hems/modules/collection/collection.h"
#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/collection.h"

namespace hems { namespace modules { namespace collection {

    using namespace hems::messages::collection;

    const messenger::msg_handler_map hems_collection::handler_map = {
        { messenger::special_subtype::SETTINGS_INIT, handler_wrapper_settings_init },
        { messenger::special_subtype::SETTINGS_CHECK, handler_wrapper_settings_check },
        { messenger::special_subtype::SETTINGS_COMMIT, handler_wrapper_settings_commit },
        { msg_type::MSG_DOWNLOAD_ENERGY_PRODUCTION, handler_wrapper_msg_download_energy_production },
        { msg_type::MSG_DOWNLOAD_ENERGY_CONSUMPTION, handler_wrapper_msg_download_energy_consumption },
        { msg_type::MSG_DOWNLOAD_WEATHER_DATA, handler_wrapper_msg_download_weather_data },
    };


    int handler_wrapper_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return hems_collection::this_instance->handler_settings_init(ia, oa);
    }

    int hems_collection::handler_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return hems_collection::this_instance->handler_settings_check(ia, oa);
    }

    int hems_collection::handler_settings_check(text_iarchive& ia, text_oarchive* oa) {
        types::settings_t settings;
        ia >> settings;

        if (!check_location_validity(settings.longitude, settings.latitude, settings.timezone)) {
            logger::this_logger->log(
                "Could not download sunlight data for invalid location: "
                    "(" + std::to_string(settings.longitude) + ", " + std::to_string(latitude) + ") "
                    "at timezone " + get_timezone_str(timezone),
                logger::level::ERR
            );
            return messenger::settings_code::INVALID;
        } else {
            return messenger::settings_code::SUCCESS;
        }
    }


    int handler_wrapper_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return hems_collection::this_instance->handler_settings_commit(ia, oa);
    }

    int hems_collection::handler_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_msg_download_energy_production(text_iarchive& ia, text_oarchive* oa) {
        return hems_collection::this_instance->handler_msg_download_energy_production(ia, oa);
    }

    int hems_collection::handler_msg_download_energy_production(text_iarchive& ia, text_oarchive* oa) {
        msg_download_energy_production_request request;
        ia >> request;

        return download_energy_production(request.time);
    }

    int handler_wrapper_msg_download_energy_consumption(text_iarchive& ia, text_oarchive* oa) {
        return hems_collection::this_instance->handler_msg_download_energy_consumption(ia, oa);
    }

    int hems_collection::handler_msg_download_energy_consumption(text_iarchive& ia, text_oarchive* oa) {
        return 0;
    }


    int handler_wrapper_msg_download_weather_data(text_iarchive& ia, text_oarchive* oa) {
        return hems_collection::this_instance->handler_msg_download_weather_data(ia, oa);
    }

    int hems_collection::handler_msg_download_weather_data(text_iarchive& ia, text_oarchive* oa) {
        msg_download_weather_data_request request;
        ia >> request;

        int res = download_weather_data(request.time, request.station);
        return res;
    }

}}}
