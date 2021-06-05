/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header defines the interface for the HEMS Launcher.
 * The interface consists of message types for messages that are put in the launcher's message queue.
 */

#ifndef HEMS_MESSAGES_LAUNCHER_H
#define HEMS_MESSAGES_LAUNCHER_H

#include <boost/serialization/string.hpp>

#include "hems/modules/launcher/local_logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems { namespace messages { namespace launcher {

    using namespace hems::modules::launcher;

    /**
     * @brief       Identifiers for message types of the launcher.
     */
    enum msg_type {
        MSG_LOG
    };


    /**
     * @brief       Use this message to send a log message to the central logger in the
     *              HEMS Launcher.
     */
    typedef struct {
        modules::type   source;     /** The source of this message, i.e. an identifier of the
                                        sending module. */
        logger::level   log_level;  /** The log level. */
        std::string     message;    /** The message content. */
    } msg_log;

}}}


namespace boost { namespace serialization {

    using namespace hems::messages::launcher;

    template<typename Archive>
    void serialize(Archive& ar, msg_log& msg, const unsigned int version) {
        ar & msg.source;
        ar & msg.log_level;
        ar & msg.message;
    }

}}

#endif /* HEMS_MESSAGES_LAUNCHER_H */
