/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Measurement Collection Module.
 * This module is responsible for collecting measurement data. Each type of continuously
 * accumulating measurement data is pulled from its source through an appropriate submodule at a
 * given schedule.
 */

#include <iomanip>

#include "hems/modules/collection/collection.h"
#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/common/exit.h"
#include "hems/messages/collection.h"
#include "hems/messages/storage.h"

namespace hems { namespace modules { namespace collection {

    hems_collection* hems_collection::this_instance = nullptr;

    hems_collection::hems_collection(bool test_mode) {
        /* Open messenger object. */
        messenger::this_messenger = new messenger(module_type, test_mode);

        logger::this_logger->log(
            "Starting " + modules::to_string_extended(module_type) + ".",
            logger::level::LOG
        );

        /* Begin listening for messages. */
        if (!messenger::this_messenger->listen(handler_map)) {
            logger::this_logger->log("Cannot listen for messages, aborting.", logger::level::ERR);
            throw EXIT_FAILURE;
        } else {
            logger::this_logger->log("Listening for messages.", logger::level::LOG);
        }

        /* Message handlers must not be called before the module's constructor has finished. */
        logger::this_logger->log("Begin handling incoming messages.", logger::level::LOG);
        messenger::this_messenger->start_handlers();
    }

    hems_collection::~hems_collection() {
        logger::this_logger->log(
            "Shutting down " + modules::to_string_extended(module_type) + ".",
            logger::level::LOG
        );

        logger::this_logger->log(
            "Successfully shut down " + modules::to_string_extended(module_type) + ", stop "
                "listening for messages.",
            logger::level::LOG
        );

        /* Delete messenger object. */
        delete messenger::this_messenger;
    }


    bool hems_collection::check_location_validity(double longitude, double latitude, double timezone) {
        if (latitude < -90 || latitude > 90 ||
            longitude < -180 || longitude > 180 ||
            timezone < -12 || timezone > 12) {
            return false;
        } else {
            return true;
        }
    }

    std::string hems_collection::get_timezone_str(double timezone) {
        std::stringstream timezone_str;
        timezone_str << std::setprecision(2);
        timezone_str << timezone;
        return timezone_str.str();
    }

}}}
