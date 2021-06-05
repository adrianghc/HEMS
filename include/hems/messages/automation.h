/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header defines the interface for the Automation and Recommendation Module.
 * The interface consists of message types for messages that are put in the module's message queue.
 */

#ifndef HEMS_MESSAGES_AUTOMATION_H
#define HEMS_MESSAGES_AUTOMATION_H

#include <list>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/list.hpp>

#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems { namespace messages { namespace automation {

    using namespace hems::types;

    using boost::posix_time::ptime;

    /**
     * @brief       Identifiers for message types of the Automation and Recommendation Module.
     */
    enum msg_type {
        MSG_SET_TASKS,
        MSG_SET_AUTOMATION,
        MSG_DEL_TASKS,
        MSG_DEL_AUTOMATION,
        MSG_GET_RECOMMENDATIONS
    };

    /**
     * @brief       Response codes for the messages of the Measurement Collection Module and its
     *              data download methods.
     */
    enum response_code {
        SUCCESS = 0x00,
        INVALID_DATA,
        DATA_STORAGE_MODULE_ERR
    };


    /**
     * @brief       The order in which to sort rectangles in the allocation algorithm.
     */
    enum order { WIDTH_DESC, HEIGHT_DESC, AREA_DESC };

    /**
     * @brief       The heuristic to use for rectangle allocation.
     */
    enum heuristic { FIRST_FIT, NEXT_FIT, BEST_FIT };


    /**
     * @brief       Use this message to request a recommendation for a week beginning at a given
     *              time.
     */
    typedef struct {
        ptime start_time;           /** The beginning of the week for the recommendation. */
        order rect_order;           /** The order in which to sort rectangles in the allocation
                                        algorithm. */
        heuristic alloc_heuristic;  /** The heuristic to use for rectangle allocation. */
    } msg_get_recommendations_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_RECOMMENDATIONS` request.
     */
    typedef struct {
        std::list<task_t> recommendations;  /** A list of recommendations in form of uncommitted tasks. */
    } msg_get_recommendations_response;

}}}


namespace boost { namespace serialization {

    using namespace hems::messages::automation;

    template<typename Archive>
    void serialize(Archive& ar, msg_get_recommendations_request& msg, const unsigned int version) {
        ar & msg.start_time;
        ar & msg.rect_order;
        ar & msg.alloc_heuristic;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_recommendations_response& msg, const unsigned int version) {
        ar & msg.recommendations;
    }

}}

#endif /* HEMS_MESSAGES_AUTOMATION_H */
