/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Automation and Recommendation Module.
 * This module is responsible for providing recommendations for task scheduling and automating
 * appliances.
 */

#include <list>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/vector.hpp>

#include "hems/modules/automation/automation.h"
#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/automation.h"

namespace hems { namespace modules { namespace automation {

    using namespace hems::messages::automation;

    using boost::posix_time::ptime;

    const messenger::msg_handler_map hems_automation::handler_map = {
        { messenger::special_subtype::SETTINGS_INIT, handler_wrapper_settings_init },
        { messenger::special_subtype::SETTINGS_CHECK, handler_wrapper_settings_check },
        { messenger::special_subtype::SETTINGS_COMMIT, handler_wrapper_settings_commit },
        { msg_type::MSG_GET_RECOMMENDATIONS, handler_wrapper_msg_get_recommendations }
    };


    int handler_wrapper_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return hems_automation::this_instance->handler_settings_init(ia, oa);
    }

    int hems_automation::handler_settings_init(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return hems_automation::this_instance->handler_settings_check(ia, oa);
    }

    int hems_automation::handler_settings_check(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return hems_automation::this_instance->handler_settings_commit(ia, oa);
    }

    int hems_automation::handler_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    int handler_wrapper_msg_get_recommendations(text_iarchive& ia, text_oarchive* oa) {
        return hems_automation::this_instance->handler_msg_get_recommendations(ia, oa);
    }

    int hems_automation::handler_msg_get_recommendations(text_iarchive& ia, text_oarchive* oa) {
        msg_get_recommendations_request msg;
        ia >> msg;
        std::list<task_t> recommendations;

        int res = get_recommendations(
            msg.start_time, msg.rect_order, msg.alloc_heuristic, recommendations
        );
        if (res == response_code::SUCCESS && oa != nullptr) {
            /* Prepare response message containing the energy production predictions. */
            msg_get_recommendations_response response {
                recommendations : recommendations
            };
            *oa << response;
        }

        return res;
    }

}}}
