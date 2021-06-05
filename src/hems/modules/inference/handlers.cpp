/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Knowledge Inference Module.
 * This module is responsible for inference knowledge from the energy production model. Knowledge 
 * is pulled from its source through an appropriate submodule at a given schedule.
 */

#include <map>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/vector.hpp>

#include "hems/modules/inference/inference.h"
#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/inference.h"

namespace hems { namespace modules { namespace inference {

    using namespace hems::messages::inference;

    using boost::posix_time::ptime;

    const messenger::msg_handler_map hems_inference::handler_map = {
        { messenger::special_subtype::SETTINGS_INIT, handler_wrapper_settings_init },
        { messenger::special_subtype::SETTINGS_CHECK, handler_wrapper_settings_check },
        { messenger::special_subtype::SETTINGS_COMMIT, handler_wrapper_settings_commit },
        { msg_type::MSG_GET_PREDICTIONS, handler_wrapper_msg_get_predictions }
    };


    int handler_wrapper_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return hems_inference::this_instance->handler_settings_init(ia, oa);
    }

    int hems_inference::handler_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return hems_inference::this_instance->handler_settings_check(ia, oa);
    }

    int hems_inference::handler_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return hems_inference::this_instance->handler_settings_commit(ia, oa);
    }

    int hems_inference::handler_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_msg_get_predictions(text_iarchive& ia, text_oarchive* oa) {
        return hems_inference::this_instance->handler_msg_get_predictions(ia, oa);
    }

    int hems_inference::handler_msg_get_predictions(text_iarchive& ia, text_oarchive* oa) {
        msg_get_predictions_request msg;
        ia >> msg;
        ptime start_time = msg.start_time;
        std::map<ptime, types::energy_production_t> energy_production;

        int res = get_energy_production_predictions(start_time, energy_production);
        if (res == response_code::SUCCESS && oa != nullptr) {
            /* Prepare response message containing the energy production predictions. */
            msg_get_predictions_response response {
                energy : energy_production
            };
            *oa << response;
        }

        return res;
    }

}}}
