/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header defines the interface for the Knowledge Inference Module.
 * The interface consists of message types for messages that are put in the module's message queue.
 */

#ifndef HEMS_MESSAGES_INFERENCE_H
#define HEMS_MESSAGES_INFERENCE_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

#include "hems/common/types.h"

namespace hems { namespace messages { namespace inference {

    using boost::posix_time::ptime;

    /**
     * @brief       Identifiers for message types of the Knowledge Inference Module.
     */
    enum msg_type {
        MSG_GET_PREDICTIONS
    };

    /**
     * @brief       Response codes for the messages of the Knowledge Inference Module.
     */
    enum response_code {
        SUCCESS = 0x00,
        INVALID_DATA,
        UNREACHABLE_SOURCE,
        INVALID_RESPONSE_SOURCE,
        DATA_STORAGE_MODULE_ERR
    };


    /**
     * @brief       Use this message to request a prediction for a given one-week interval.
     */
    typedef struct {
        ptime start_time;   /** The beginning of the one-week interval. */
    } msg_get_predictions_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_PREDICTIONS` request.
     */
    typedef struct {
        std::map<ptime, types::energy_production_t> energy; /** The predicted energy available for
                                                                the initially given one-week
                                                                interval, per 15 minutes. */
    } msg_get_predictions_response;

}}}


namespace boost { namespace serialization {

    using namespace hems::messages::inference;

    template<typename Archive>
    void serialize(Archive& ar, msg_get_predictions_request& msg, const unsigned int version) {
        ar & msg.start_time;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_predictions_response& msg, const unsigned int version) {
        ar & msg.energy;
    }

}}

#endif /* HEMS_MESSAGES_INFERENCE_H */
