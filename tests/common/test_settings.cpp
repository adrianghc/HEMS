/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * These are unit tests for the settings mechanism.
 */

#include <functional>
#include <iostream>
#include <map>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "../test.hpp"
#include "../extras/dummy_logger.hpp"

#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems {
    /**
     * @brief   This class exists to provide access to private members of `messenger`.
     */
    class messenger_test : public messenger {
        public:
            messenger_test(modules::type owner) : messenger(owner, true) {};

            int send(unsigned int timeout, int subtype, modules::type recipient, std::string payload, std::string* response) {
                return send_(timeout, subtype, recipient, payload, response);
            }
    };
}

namespace hems { namespace messages {

    int handler_settings_check_timeout(text_iarchive& ia, text_oarchive* oa) {
        sleep(2 * DEFAULT_SEND_TIMEOUT/1000 + 2);
        return messenger::settings_code::SUCCESS;
    }

    int handler_settings_check_invalid(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::INVALID;
    }

    int handler_settings_check_success(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }

    int handler_settings_commit(text_iarchive& ia, text_oarchive* oa) {
        return messenger::settings_code::SUCCESS;
    }


    bool test_settings_no_handlers() {
        hems::logger::this_logger = new hems::dummy_logger();

        const messenger::msg_handler_map handler_map;

        messenger* messenger = new hems::messenger(modules::type::LAUNCHER);
        if (messenger->listen(handler_map)) {
            std::cout <<
                "Starting listen loop should fail due to missing settings handler but didn't.\n";
            delete messenger;
            return false;
        } else {
            delete messenger;
            return true;
        }
    }

    bool test_settings(
        const messenger::msg_handler_map& handler_map1, const messenger::msg_handler_map& handler_map2,
        int expected_response
    ) {
        hems::logger::this_logger = new hems::dummy_logger();

        bool continue_test = true;
        bool success = true;

        /* First, create all message queues. */
        struct mq_attr attr = {
            mq_flags    : 0,
            mq_maxmsg   : 10,
            mq_msgsize  : sizeof(messenger::msg_t),
            mq_curmsgs  : 0
        };
        for (const auto& item : messenger::mq_names) {
            mq_close(mq_open(item.second.c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr));
        }
        for (const auto& item : messenger::mq_res_names) {
            mq_close(mq_open(item.second.c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr));
        }

        /* Start listen loops. */
        std::map<modules::type, messenger*> messengers;
        for (const auto& item : messenger::mq_names) {
            messenger_test* messenger = new hems::messenger_test(item.first);
            int res;
            if (item.first == modules::type::STORAGE) {
                res = messenger->listen(handler_map1);
            } else {
                res = messenger->listen(handler_map2);
            }
            if (!res) {
                std::cout << "Starting listen loop failed.\n";
                continue_test = false;
                success = false;
            }
            messengers[item.first] = messenger;
            messenger->start_handlers();
        }

        /* Broadcast settings. */
        if (continue_test) {
            types::settings_t settings;
            int response = messengers.at(modules::type::LAUNCHER)->broadcast_settings(settings);
            if (response != expected_response) {
                std::cout <<
                    "Broadcasting settings should have returned " + std::to_string(expected_response) +
                    ", returned " + std::to_string(response) + " instead.\n";
                success = false;
            }
        }

        for (const auto& item : messengers) {
            delete item.second;
        }

        /* Delete message queues. */
        for (const auto& item : messenger::mq_names) {
            mq_unlink(item.second.c_str());
        }
        for (const auto& item : messenger::mq_res_names) {
            mq_unlink(item.second.c_str());
        }

        return success;
    }

    bool test_settings_timeout() {
        const messenger::msg_handler_map handler_map = {
            { messenger::special_subtype::SETTINGS_INIT, std::function<int(text_iarchive&, text_oarchive*)>() },
            { messenger::special_subtype::SETTINGS_CHECK, &handler_settings_check_timeout },
            { messenger::special_subtype::SETTINGS_COMMIT, &handler_settings_commit }
        };
        std::cout << "Wait ...\n";
        return test_settings(handler_map, handler_map, messenger::settings_code::TIMEOUT);
    }

    bool test_settings_invalid() {
        /*  This handler map will be used for one single module. This is to test that it is
            sufficient for only one module to return an error in order for the entire operation to
            fail. */
        const messenger::msg_handler_map handler_map1 = {
            { messenger::special_subtype::SETTINGS_INIT, std::function<int(text_iarchive&, text_oarchive*)>() },
            { messenger::special_subtype::SETTINGS_CHECK, &handler_settings_check_invalid },
            { messenger::special_subtype::SETTINGS_COMMIT, &handler_settings_commit }
        };

        /* This handler map will be used for all other modules. */
        const messenger::msg_handler_map handler_map2 = {
            { messenger::special_subtype::SETTINGS_INIT, std::function<int(text_iarchive&, text_oarchive*)>() },
            { messenger::special_subtype::SETTINGS_CHECK, &handler_settings_check_success },
            { messenger::special_subtype::SETTINGS_COMMIT, &handler_settings_commit }
        };

        return test_settings(handler_map1, handler_map2, messenger::settings_code::INVALID);
    }

    bool test_settings_success() {
        const messenger::msg_handler_map handler_map = {
            { messenger::special_subtype::SETTINGS_INIT, std::function<int(text_iarchive&, text_oarchive*)>() },
            { messenger::special_subtype::SETTINGS_CHECK, &handler_settings_check_success },
            { messenger::special_subtype::SETTINGS_COMMIT, &handler_settings_commit }
        };
        return test_settings(handler_map, handler_map, messenger::settings_code::SUCCESS);
    }

}}


int main() {
    return run_tests({
        { "01 Common: Settings test (no handlers provided)", &hems::messages::test_settings_no_handlers },
        { "02 Common: Settings test (timeout)", &hems::messages::test_settings_timeout },
        { "03 Common: Settings test (invalid)", &hems::messages::test_settings_invalid },
        { "04 Common: Settings test (success)", &hems::messages::test_settings_success },
    });
}
