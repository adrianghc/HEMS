/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Model Training Module.
 * This module is responsible for training of the energy production model. The model can be trained
 * on a schedule or by request.
 */

#include <list>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "hems/modules/training/training.h"
#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/training.h"

namespace hems { namespace modules { namespace training {

    using namespace hems::messages::training;

    const messenger::msg_handler_map hems_training::handler_map = {
        { messenger::special_subtype::SETTINGS_INIT, handler_wrapper_settings_init },
        { messenger::special_subtype::SETTINGS_CHECK, handler_wrapper_settings_check },
        { messenger::special_subtype::SETTINGS_COMMIT, handler_wrapper_settings_commit },
        { msg_type::MSG_TRAIN, handler_wrapper_msg_train }
    };


    int handler_wrapper_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return hems_training::this_instance->handler_settings_init(ia, oa);
    }

    int hems_training::handler_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return hems_training::this_instance->handler_settings_check(ia, oa);
    }

    int hems_training::handler_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return hems_training::this_instance->handler_settings_commit(ia, oa);
    }

    int hems_training::handler_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_msg_train(text_iarchive& ia, text_oarchive* oa) {
        return hems_training::this_instance->handler_msg_train(ia, oa);
    }

    int hems_training::handler_msg_train(text_iarchive& ia, text_oarchive* oa) {
        /* TODO */
        return response_code::UNREACHABLE_SOURCE;
    }

}}}
