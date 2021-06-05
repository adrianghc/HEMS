/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This contains the code for the `remote_logger` implementation of the abstract `logger` class.
 * This class sends errors, outputs and log messages to the HEMS Launcher Module. It is used by all
 * modules except the launcher module.
 */

#include <string>

#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/messages/launcher.h"

namespace hems {

    using namespace hems::messages::launcher;

    logger* logger::this_logger = nullptr;

    std::string logger::to_string(logger::level level) {
        switch (level) {
            case logger::level::LOG: return "LOG";
            case logger::level::DBG: return "DEBUG";
            case logger::level::ERR: return "ERROR";
            default: return "???";
        }
    }

    void remote_logger::log(std::string msg, level log_level) {
        if (!debug && log_level == level::DBG) {
            return;
        }

        msg_log payload = {
            .source     = owner,
            .log_level  = log_level,
            .message    = msg
        };
        messenger::this_messenger->send(
            0,
            messages::launcher::msg_type::MSG_LOG,
            modules::type::LAUNCHER,
            messenger::serialize(payload),
            nullptr
        );
    }

}
