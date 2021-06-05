/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header defines the interface for the Model Training Module.
 * The interface consists of message types for messages that are put in the module's message queue.
 */

#ifndef HEMS_MESSAGES_TRAINING_H
#define HEMS_MESSAGES_TRAINING_H

namespace hems { namespace messages { namespace training {

    /**
     * @brief       Identifiers for message types of the Model Trainings Module.
     */
    enum msg_type {
        MSG_TRAIN
    };

    /**
     * @brief       Response codes for the messages of the Model Training Module.
     */
    enum response_code {
        SUCCESS = 0x00,
        UNREACHABLE_SOURCE,
        INVALID_RESPONSE_SOURCE,
        DATA_STORAGE_MODULE_ERR
    };

}}}

#endif /* HEMS_MESSAGES_TRAINING_H */
