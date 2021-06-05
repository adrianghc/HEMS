/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * These are unit tests for the `messenger` class.
 */

#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <mqueue.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "../test.hpp"
#include "../extras/dummy_logger.hpp"

#include "hems/common/logger.h"
#include "hems/common/messenger.h"

namespace hems { namespace messages {

    std::string payload = "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, ";
    std::string expected_response = "sed diam nonumy eirmod tempor";
    int expected_code = 11;

    bool success = true;


    int handler1(text_iarchive& ia, text_oarchive* oa) {
        return 1;
    }

    int handler2(text_iarchive& ia, text_oarchive* oa) {
        std::string msg;
        ia >> msg;
        if (msg != payload) {
            success = false;
            std::cout << "Message handler did not receive the right payload!\n";
            return expected_code;
        } else if (oa == nullptr) {
            success = false;
            std::cout <<
                "Message handler for request message was not given text_oarchive to write response in.\n";
            return expected_code;
        } else {
            *oa << expected_response;
            return expected_code;
        }
    }

    int handler3(text_iarchive& ia, text_oarchive* oa) {
        usleep(1100 * DEFAULT_SEND_TIMEOUT);
        return 0;
    }


    bool test_messenger() {
        const messenger::msg_handler_map handler_map = {
            { 1, &handler1 },
            { 2, &handler2 },
            { 3, &handler3 },
        };

        hems::logger::this_logger = new hems::dummy_logger();


        /* Open all message queues. */
        modules::type this_module = modules::type::LAUNCHER;

        auto create = [] (const std::map<modules::type, std::string>& names) {
            for (const auto& item : names) {
                modules::type owner = item.first;
                std::string name = item.second;

                struct mq_attr attr = { 
                    mq_flags    : 0,
                    mq_maxmsg   : 10,
                    mq_msgsize  : sizeof(messenger::msg_t),
                    mq_curmsgs  : 0
                };

                mqd_t id = mq_open(name.c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr);
                if (id < 0) {
                    std::cout <<
                        "Could not create message queue for " + modules::to_string_extended(owner) + ": " +
                        strerror(errno);
                    return false;
                }
            }
            return true;
        };

        if (!create(messenger::mq_names)) {
            return false;
        }
        if (!create(messenger::mq_res_names)) {
            return false;
        }

        messenger* this_messenger = new messenger(this_module, true);

        std::string payload_serialized = this_messenger->serialize(payload);
        std::string response_serialized;

        if (!this_messenger->listen(handler_map)) {
            success = false;
            std::cout << "Could not begin listen loop.\n";
        }
        this_messenger->start_handlers();

        /* Test command message (timeout 0). */
        int code = this_messenger->send(0, 1, this_module, payload_serialized, &response_serialized);
        if (code != 0) {
            success = false;
            std::cout << "Send() for command message returned with an error (" + std::to_string(code) + ").\n";
        }
        if (!response_serialized.empty()) {
            success = false;
            response_serialized.clear();
            std::cout << "Response was not empty for command message.\n";
        }

        /* Test request message (standard timeout). */
        code = this_messenger->send(DEFAULT_SEND_TIMEOUT, 2, this_module, payload_serialized, &response_serialized);
        if (code != expected_code) {
            success = false;
            std::cout <<
                "Send() for request message did not return with expected code. "
                "(Expected: " + std::to_string(expected_code) + ", Received: " + std::to_string(code) + ").\n";
        }
        if (response_serialized.empty()) {
            success = false;
            std::cout << "Response was empty for request message.\n";
        } else {
            std::string response = messenger::deserialize<std::string>(response_serialized);

            if (response != expected_response) {
                success = false;
                std::cout <<
                    "Response was not as expected (expected: '" + expected_response
                    + "', received: '" + response + "').\n";
            }

            response_serialized.clear();
        }

        /* Test timeout. */
        code = this_messenger->send(DEFAULT_SEND_TIMEOUT, 3, this_module, payload_serialized, &response_serialized);
        if (code != messenger::send_error::SEND_TIMEOUT) {
            success = false;
            std::cout <<
                "Send() for request message did not return with expected code. "
                "(Expected: " + std::to_string(messenger::send_error::SEND_TIMEOUT) + ", "
                "Received: " + std::to_string(code) + ").\n";
        }
        if (!response_serialized.empty()) {
            success = false;
            response_serialized.clear();
            std::cout << "Response was not empty for timed out request message.\n";
        }


        delete this_messenger;
        delete hems::logger::this_logger;
        for (const auto& item : messenger::mq_names) {
            mq_unlink(item.second.c_str());
        }

        return success;
    }

}}


int main() {
    return run_tests({
        { "01 Common: Messenger test", &hems::messages::test_messenger }
    });
}
