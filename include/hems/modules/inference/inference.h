/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Knowledge Inference Module.
 * This module is responsible for inference knowledge from the energy production model. Knowledge 
 * is pulled from its source through an appropriate submodule at a given schedule.
 */

#ifndef HEMS_MODULES_INFERENCE_INFERENCE_H
#define HEMS_MODULES_INFERENCE_INFERENCE_H

#include <vector>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems { namespace modules { namespace inference {

    using boost::archive::text_iarchive;
    using boost::archive::text_oarchive;

    using boost::posix_time::ptime;

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
     * @brief       Wrapper for the message handler for `MSG_GET_PREDICTIONS` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                 if success.
     *              INVALID_DATA            if failed due to invalid data.
     *              UNREACHABLE_SOURCE      if failed due to unreachable model.
     *              INVALID_RESPONSE_SOURCE if failed due to invalid response from model.
     *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
     */
    int handler_wrapper_msg_get_predictions(text_iarchive& ia, text_oarchive* oa);


    /**
     * @brief   The Knowledge Inference Module class.
     *          This module is responsible for inference knowledge from the energy production model.
     *          Knowledge is pulled from its source through an appropriate submodule at a given
     *          schedule.
     */
    class hems_inference {
        public:
            /**
             * @brief       Constructs the Knowledge Inference Module.
             * 
             * @param[in]   test_mode       Whether the Knowledge Inference Module should run in
             *                              test mode, in which case no settings initialization
             *                              takes place and the messenger is also launched in test
             *                              mode.
             */
            hems_inference(bool test_mode);

            ~hems_inference();

            static const modules::type module_type = modules::type::INFERENCE; /** The type of this module. */

            static hems_inference* this_instance;   /** The Knowledge Inference Module is
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
             * @brief       Message handler for `MSG_GET_PREDICTIONS` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                 if success.
             *              INVALID_DATA            if failed due to invalid data.
             *              UNREACHABLE_SOURCE      if failed due to unreachable model.
             *              INVALID_RESPONSE_SOURCE if failed due to invalid response from model.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int handler_msg_get_predictions(text_iarchive& ia, text_oarchive* oa);

            /* END Message handlers. */

        private:
            static const messenger::msg_handler_map handler_map;    /** The message handler map for
                                                                        this module. */

            /**
             * @brief       Gets the energy production predictions for a week beginning at the given
             *              time.
             * 
             * @param[in]   start_time          The beginning of the one-week interval for which to
             *                                  get energy production predictions.
             * @param[out]  energy_production   A reference to the map to store the energy
             *                                  production predictions in.
             * 
             * @return      SUCCESS                 if success.
             *              INVALID_DATA            if failed due to invalid data.
             *              UNREACHABLE_SOURCE      if failed due to unreachable model.
             *              INVALID_RESPONSE_SOURCE if failed due to invalid response from model.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int get_energy_production_predictions(
                ptime start_time, std::map<ptime, types::energy_production_t>& energy_production
            );


            friend class inference_test; /* Friend class to let tests access private members. */
    };

}}}

#endif /* HEMS_MODULES_INFERENCE_INFERENCE_H */
