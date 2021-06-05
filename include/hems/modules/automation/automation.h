/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Automation and Recommendation Module.
 * This module is responsible for providing recommendations for task scheduling and automating
 * appliances.
 */

#ifndef HEMS_MODULES_AUTOMATION_AUTOMATION_H
#define HEMS_MODULES_AUTOMATION_AUTOMATION_H

#include <list>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/automation.h"

namespace hems { namespace modules { namespace automation {

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
     * @brief       Wrapper for the message handler for `MSG_GET_RECOMMENDATIONS` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                 if success.
     *              INVALID_DATA            if failed due to invalid data.
     *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
     */
    int handler_wrapper_msg_get_recommendations(text_iarchive& ia, text_oarchive* oa);


    /**
     * @brief   The Automation and Recommendation Module class.
     *          This module is responsible for providing recommendations for task scheduling and
     *          automating appliances.
     */
    class hems_automation {
        public:
            /**
             * @brief       Constructs the Automation and Recommendation Module.
             * 
             * @param[in]   test_mode       Whether the Automation and Recommendation Module should
             *                              run in test mode, in which case no settings initialization
             *                              takes place and the messenger is also launched in test
             *                              mode.
             */
            hems_automation(bool test_mode);

            ~hems_automation();

            static const modules::type module_type = modules::type::AUTOMATION; /** The type of this module. */

            static hems_automation* this_instance;  /** The Automation and Recommendation Module is
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
             * @brief       Message handler for `MSG_GET_RECOMMENDATIONS` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                 if success.
             *              INVALID_DATA            if failed due to invalid data.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int handler_msg_get_recommendations(text_iarchive& ia, text_oarchive* oa);

            /* END Message handlers. */

        private:
            static const messenger::msg_handler_map handler_map;    /** The message handler map for
                                                                        this module. */

            /**
             * @brief       Gets a list of task recommendations for a week beginning at a given time.
             * 
             * @param[in]   start_time          The beginning of the week for which to get
             *                                  recommendations.
             * @param[in]   rect_order          The order in which to sort rectangles in the
             *                                  allocation algorithm.
             * @param[in]   alloc_heuristic     The heuristic to use for rectangle allocation.
             * @param[out]  recommendations     A reference to the list to store the recommendations
             *                                  in.
             * 
             * @return      SUCCESS                 if success.
             *              INVALID_DATA            if failed due to invalid data.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int get_recommendations(
                ptime start_time, messages::automation::order rect_order,
                messages::automation::heuristic alloc_heuristic, std::list<types::task_t>& recommendations
            );


            friend class automation_test;   /* Friend class to let tests access private members. */
    };

}}}

#endif /* HEMS_MODULES_AUTOMATION_AUTOMATION_H */
