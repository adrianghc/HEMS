/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * These are unit tests for the HEMS Launcher.
 */

#include <iostream>
#include <functional>
#include <map>

#include "../test.hpp"
#include "../extras/dummy_logger.hpp"

#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/modules/launcher/launcher.h"

namespace hems { namespace modules { namespace launcher {

    bool test_message_queues() {
        hems::logger::this_logger = new hems::dummy_logger();

        /* Instantiate with debug mode on so that no other modules are launched for this test. */
        try {
            hems_launcher::this_instance = new hems_launcher(true, true, "", "", "", "", "", "", "", "");
        } catch (int err) {
            std::cout << "Constructing HEMS Launcher threw an exception " + std::to_string(err) + ", test failed.\n";
            return false;
        }

        /* Check for each message queue that it has been created. */
        int count_open = 0;
        for (const auto& item : messenger::mq_names) {
            modules::type owner = item.first;
            std::string mq_name = item.second;

            if (mq_open(hems::messenger::mq_names.at(owner).c_str(), O_RDONLY | O_CLOEXEC) < 0) {
                ++count_open;
            }
        }

        if (count_open) {
            std::cout << std::to_string(count_open) + " message queues were not opened successfully.\n";
        }

        delete hems_launcher::this_instance;

        /* Check for each message queue that it has been closed. */
        int count_close = 0;
        for (const auto& item : messenger::mq_names) {
            modules::type owner = item.first;
            std::string mq_name = item.second;

            if (mq_open(hems::messenger::mq_names.at(owner).c_str(), O_RDONLY | O_CLOEXEC) >= 0) {
                ++count_close;
            }
        }

        if (count_close) {
            std::cout << std::to_string(count_open) + " message queues were not closed successfully.\n";
        }

        delete hems::logger::this_logger;

        return !count_open && !count_close;
    }

    bool test_watch_modules () {
        hems::logger::this_logger = new hems::dummy_logger();


        /* BEGIN Dummy module with return value 0. */

        /* Instantiate with debug mode off so that the dummy module can be launched. */
        try {
            hems_launcher::this_instance = new hems_launcher(
                false, true, "dummy_module_0", "", "dummy_module_0", "dummy_module_0", "",
                "dummy_module_0", "dummy_module_0", "dummy_module_0"
            );
        } catch (int err) {
            std::cout <<
                "Constructing HEMS Launcher threw an exception " + std::to_string(err) + ", test failed.\n";
            return false;
        }

        /* Wait five seconds to be sure ... */
        std::cout << "Wait five seconds ...\n";
        sleep(5);

        bool exit0 = (hems::exit_status == 0);
        if (!exit0) {
            std::cout << "Module watchers did not detect return value 0.\n";
        }

        delete hems_launcher::this_instance;

        hems::exit_status = -1;

        /* END Dummy module with return value 0. */


        /* BEGIN Dummy module with return value 1. */

        /* Instantiate with debug mode off so that the dummy module can be launched. */
        try {
            hems_launcher::this_instance = new hems_launcher(
                false, true, "dummy_module_1", "", "dummy_module_1", "dummy_module_1", "",
                "dummy_module_1", "dummy_module_1", "dummy_module_1"
            );
        } catch (int err) {
            std::cout <<
                "Constructing HEMS Launcher threw an exception " + std::to_string(err) + ", test failed.\n";
            return false;
        }

        /* Wait five seconds to be sure ... */
        std::cout << "Wait five seconds ...\n";
        sleep(5);

        bool exit1 = (hems::exit_status == 1);
        if (!exit1) {
            std::cout << "Module watchers did not detect return value 1.\n";
        }

        delete hems_launcher::this_instance;

        hems::exit_status = -1;

        /* END Dummy module with return value 1. */


        /* BEGIN Dummy module with return value 2. */

        /* Instantiate with debug mode off so that the dummy module (return value 2) can be launched. */
        try {
            hems_launcher::this_instance = new hems_launcher(
                false, true, "dummy_module_2", "", "dummy_module_2", "dummy_module_2", "",
                "dummy_module_2", "dummy_module_2", "dummy_module_2"
            );
        } catch (int err) {
            std::cout <<
                "Constructing HEMS Launcher threw an exception " + std::to_string(err) + ", test failed.\n";
            return false;
        }

        /* Wait five seconds to be sure ... */
        std::cout << "Wait five seconds ...\n";
        sleep(5);

        bool exit2 = (hems::exit_status == 2);
        if (!exit2) {
            std::cout << "Module watchers did not detect return value 2.\n";
        }

        delete hems_launcher::this_instance;

        /* END Dummy module with return value 2. */


        delete hems::logger::this_logger;

        return exit0 && exit1 && exit2;
    }

}}}


int main() {
    return run_tests({
        { "01 Launcher: Message queue test", &hems::modules::launcher::test_message_queues },
        { "02 Launcher: Module watcher test", &hems::modules::launcher::test_watch_modules }
    });
}
