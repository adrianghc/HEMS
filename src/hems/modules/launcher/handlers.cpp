/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the code for the HEMS Launcher. The HEMS Launcher is the executable that starts the
 * entire HEMS, launching all modules and initializing their message queues. The lifecycle of the
 * HEMS is identical with the launcher's lifecycle, therefore when the launcher terminates, the
 * entire HEMS does. The launcher also collects errors, outputs and log messages from all modules
 * and writes them to files and standard I/O streams.
 */

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "hems/modules/launcher/launcher.h"
#include "hems/modules/launcher/local_logger.h"
#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/launcher.h"

namespace hems { namespace modules { namespace launcher {

    using namespace hems::messages::launcher;

    const messenger::msg_handler_map hems_launcher::handler_map = {
        { messenger::special_subtype::SETTINGS_INIT, handler_wrapper_settings },
        { messenger::special_subtype::SETTINGS_CHECK, handler_wrapper_settings },
        { messenger::special_subtype::SETTINGS_COMMIT, handler_wrapper_settings },
        { msg_type::MSG_LOG, handler_wrapper_msg_log }
    };


    int handler_wrapper_settings(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }

    int handler_wrapper_msg_log(text_iarchive& ia, text_oarchive* oa) {
        return hems_launcher::this_instance->handler_msg_log(ia, oa);
    }

    int hems_launcher::handler_msg_log(text_iarchive& ia, text_oarchive* oa) {
        msg_log msg;
        ia >> msg;
        dynamic_cast<local_logger*>(logger::this_logger)->log(msg.message, msg.log_level, msg.source);
        return 0;
    }

}}}
