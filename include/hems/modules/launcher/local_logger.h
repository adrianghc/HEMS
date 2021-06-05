/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header declares the `local_logger` implementation of the abstract `logger` class.
 * This class collects errors, outputs and log messages from all modules and writes them to a file
 * and prints them to standard I/O streams.
 */

#ifndef HEMS_MODULES_LAUNCHER_LOCAL_LOGGER_H
#define HEMS_MODULES_LAUNCHER_LOCAL_LOGGER_H

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "hems/common/logger.h"

namespace hems { namespace modules { namespace launcher {

    /**
     * @brief   The `local_logger` class.
     *          This class collects errors, outputs and log messages from all modules and writes
     *          them to a file and prints them to standard I/O streams.
     */
    class local_logger : public logger {

        public:
            /**
             * @brief       Constructs a local logger.
             * 
             * @param[in]   owner       The owner of this instance.
             * @param[in]   debug       Whether to print and write debug messages or not.
             * @param[in]   log_path    The path of the file to write messages to.
             */
            local_logger(modules::type owner, bool debug, std::string log_path);

            ~local_logger();

            /**
             * @brief       Logs a message from a given source and a given level.
             * 
             * @param[in]   msg         The log message.
             * @param[in]   log_level   The log level.
             */
            void log(std::string msg, level log_level) override;

            /**
             * @brief       Logs a message from a given source and a given level.
             * 
             * @param[in]   msg         The log message.
             * @param[in]   log_level   The log level.
             * @param[in]   src         The source of this message, i.e. an identifier of the
             *                          logging module.
             */
            void log(std::string msg, level log_level, modules::type src);

        private:
            std::ofstream log_file; /** File stream for the log file. */

            static const int source_strings_maxlen = 10;    /** The maximum length of all module
                                                                identifier strings. */
            static const int log_strings_maxlen = 5;        /** The maximum length of all log level
                                                                strings. */

            /**
             * @brief   Maps log levels to standard I/O streams.
             */
            static const std::map<level, std::ostream&> log_streams;

            friend class local_logger_test; /* Friend class to let tests access private members. */

    };

}}}

#endif /* HEMS_MODULES_LAUNCHER_LOCAL_LOGGER_H */
