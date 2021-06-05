/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the User Interface Module.
 * This module is responsible for offering an interface for the user to interact with the HEMS,
 * presenting the current state and offering commands.
 */

#include <string>
#include <thread>

#include "hems/modules/ui/ui.h"
#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/common/exit.h"

namespace hems { namespace modules { namespace ui {

    hems_ui* hems_ui::this_instance = nullptr;

    hems_ui::hems_ui(bool test_mode, std::string ui_server_root) : ui_server_root(ui_server_root) {
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

        /*  Starting command server. The command server is stateless, so it does not have to be
            joined later. */
        logger::this_logger->log("Starting command server.", logger::level::LOG);
        ui_server_worker = new std::thread(&hems_ui::listen, this);

        /* Message handlers must not be called before the module's constructor has finished. */
        logger::this_logger->log("Begin handling incoming messages.", logger::level::LOG);
        messenger::this_messenger->start_handlers();
    }

    hems_ui::~hems_ui() {
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
