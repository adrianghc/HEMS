/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This code file contains functions and variables for signal handling and termination.
 */

#include <csignal>
#include <mutex>

#include "extras/semaphore.hpp"

#include "hems/common/exit.h"

namespace hems {

    semaphore exit_sem;

    volatile sig_atomic_t exit_status = -1;

    void signal_handler(int signal) {
        if (signal == SIGTERM || signal == SIGQUIT || signal == SIGINT) {
            exit(EXIT_SUCCESS);
        }
    }

    void exit(int status) {
        /*  Prevent a situation where `exit()` is called twice in a row by separate threads, and the
            second call overwrites the status that was set by the first call before the process was
            able to exit. Subsequent calls to `exit()` will simply skip setting `exit_status`. */
        static std::mutex exit_status_mutex;

        exit_status_mutex.lock();
        if (exit_status == -1) {
            exit_status = status;
        }
        exit_status_mutex.unlock();

        exit_sem.notify();
    }

}
