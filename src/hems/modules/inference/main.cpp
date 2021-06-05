/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Knowledge Inference Module.
 * This module is responsible for inference knowledge from the energy production model. Knowledge 
 * is pulled from its source through an appropriate submodule at a given schedule.
 */

#include <iostream>
#include <string>
#include <vector>

#include "hems/modules/inference/inference.h"
#include "hems/common/logger.h"
#include "hems/common/exit.h"

int main(int argc, char* argv[]) {
    using namespace hems::modules::inference;

    bool debug = false;

    std::string valid_args =
        "--help, -h         Get a list of permitted arguments.\n"
        "--debug, -d        Launch with the debug configuration.\n"
        "                   This will log and print debug messages.\n";

    for (int i = 1; i < argc; ++i) {
        std::string arg = std::string(argv[i]);
        if (arg == "--help" || arg == "-h") {
            std::cout << valid_args;
            return EXIT_FAILURE;
        } else if (arg == "--debug" || arg == "-d") {
            debug = true;
        } else {
            std::cout << "Invalid usage, permitted arguments are:\n\n" + valid_args;
            return EXIT_FAILURE;
        }
    }

    hems::logger::this_logger = new hems::remote_logger(hems_inference::module_type, debug);

    /* Register signal handlers to ensure that destructors are called before termination. */
    signal(SIGTERM, hems::signal_handler);
    signal(SIGQUIT, hems::signal_handler);
    signal(SIGINT, hems::signal_handler);

    try {
        hems_inference::this_instance = new hems_inference(false);

        /*  The main thread remains blocked until the exit semaphore is notified through a call to
            the `exit()` function, whereupon the thread leaves this block and the program terminates. */
        hems::exit_sem.wait();
    } catch (int err) {
        hems::logger::this_logger->log(
            "FATAL: Could not start instance of " +
                hems::modules::to_string_extended(hems_inference::module_type) + ".",
            hems::logger::level::ERR
        );
        hems::exit(err);
    }

    delete hems_inference::this_instance;
    delete hems::logger::this_logger;
    return hems::exit_status;
}
