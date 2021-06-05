/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Automation and Recommendation Module.
 * This module is responsible for providing recommendations for task scheduling and automating
 * appliances.
 */

#include "hems/modules/automation/automation.h"
#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/common/exit.h"

namespace hems { namespace modules { namespace automation {

    hems_automation* hems_automation::this_instance = nullptr;

    hems_automation::hems_automation(bool test_mode) {
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

    hems_automation::~hems_automation() {
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

}}}
