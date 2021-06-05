/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the User Interface Module.
 * This module is responsible for offering an interface for the user to interact with the HEMS,
 * presenting the current state and offering commands.
 */

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/vector.hpp>

#include "hems/modules/ui/ui.h"
#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems { namespace modules { namespace ui {

    const messenger::msg_handler_map hems_ui::handler_map = {
        { messenger::special_subtype::SETTINGS_INIT, handler_wrapper_settings_init },
        { messenger::special_subtype::SETTINGS_CHECK, handler_wrapper_settings_check },
        { messenger::special_subtype::SETTINGS_COMMIT, handler_wrapper_settings_commit }
    };


    int handler_wrapper_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return hems_ui::this_instance->handler_settings_init(ia, oa);
    }

    int hems_ui::handler_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return hems_ui::this_instance->handler_settings_check(ia, oa);
    }

    int hems_ui::handler_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return hems_ui::this_instance->handler_settings_commit(ia, oa);
    }

    int hems_ui::handler_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }

}}}
