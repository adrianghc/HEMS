/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Model Training Module.
 * This module is responsible for training of the energy production model. The model can be trained
 * on a schedule or by request.
 */

#ifndef HEMS_MODULES_TRAINING_TRAINING_H
#define HEMS_MODULES_TRAINING_TRAINING_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems { namespace modules { namespace training {

    using boost::archive::text_iarchive;
    using boost::archive::text_oarchive;

    /**
     * @brief       Wrapper for the message handler for `SETTINGS_INIT` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code.
     */
    int handler_wrapper_settings_init(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `SETTINGS_CHECK` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code.
     */
    int handler_wrapper_settings_check(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `SETTINGS_COMMIT` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code.
     */
    int handler_wrapper_settings_commit(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_TRAIN` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                 if success.
     *              UNREACHABLE_SOURCE      if failed due to unreachable model.
     *              INVALID_RESPONSE_SOURCE if failed due to invalid response from model.
     *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
     */
    int handler_wrapper_msg_train(text_iarchive& ia, text_oarchive* oa);


    /**
     * @brief   The Model Training Module class.
     *          This module is responsible for training of the energy production model. The model
     *          can be trained on a schedule or by request.
     */
    class hems_training {
        public:
            /**
             * @brief       Constructs the Model Training Module.
             * 
             * @param[in]   test_mode       Whether the Model Training Module should run in
             *                              test mode, in which case no settings initialization
             *                              takes place and the messenger is also launched in test
             *                              mode.
             */
            hems_training(bool test_mode);

            ~hems_training();

            static const modules::type module_type = modules::type::TRAINING;   /** The type of this module. */

            static hems_training* this_instance;    /** The Model Training Module is
                                                        conceptually a singleton, a pointer to which
                                                        is stored here. */

            /* BEGIN Message handlers. */

            /**
             * @brief       Message handler for `SETTINGS_INIT` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The settings broadcast response code.
             */
            int handler_settings_init(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `SETTINGS_CHECK` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The settings broadcast response code.
             */
            int handler_settings_check(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `SETTINGS_COMMIT` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The settings broadcast response code.
             */
            int handler_settings_commit(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_TRAIN` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                 if success.
             *              UNREACHABLE_SOURCE      if failed due to unreachable model.
             *              INVALID_RESPONSE_SOURCE if failed due to invalid response from model.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int handler_msg_train(text_iarchive& ia, text_oarchive* oa);

            /* END Message handlers. */

        private:
            static const messenger::msg_handler_map handler_map;    /** The message handler map for
                                                                        this module. */

            friend class training_test; /* Friend class to let tests access private members. */
    };

}}}

#endif /* HEMS_MODULES_TRAINING_TRAINING_H */
