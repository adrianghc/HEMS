/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * These are unit tests for the HEMS Launcher.
 */

#include <fstream>
#include <iostream>
#include <functional>
#include <map>

#include <boost/filesystem.hpp>

#include "../test.hpp"
#include "../extras/random_file_name.hpp"

#include "hems/common/modules.h"
#include "hems/modules/launcher/launcher.h"
#include "hems/modules/launcher/local_logger.h"

namespace hems { namespace modules { namespace launcher {

    /**
     * @brief   This class exists to be able to access private members of `local_logger`.
     */
    class local_logger_test : public local_logger {
        public:
            local_logger_test(bool debug, std::string log_path) :
                local_logger(modules::type::LAUNCHER, debug, log_path) {};

            static int source_strings_maxlen() {
                return local_logger::source_strings_maxlen;
            };

            static int log_strings_maxlen() {
                return local_logger::log_strings_maxlen;
            }

            std::string to_string(logger::level level) {
                return local_logger::to_string(level);
            }
    };

    bool test_local_logger() {
        bool success = true;
        int count, order;
        std::string log_line;

        char log_path[11];
        do {
            generate_random_file_name(log_path);
        } while (boost::filesystem::exists(log_path));

        int source_strings_maxlen = local_logger_test::source_strings_maxlen();
        int log_strings_maxlen = local_logger_test::log_strings_maxlen();

        std::string msg_log = "Lorem ipsum ";
        std::string msg_err = "dolor sit amet, ";
        std::string msg_dbg = "consetetur sadipscing elitr,";

        std::string str_log = "[" + logger::to_string(logger::level::LOG) + "] ";
        int cur_size_log1 = logger::to_string(logger::level::LOG).size();
        if (cur_size_log1 < log_strings_maxlen) {
            str_log.append(std::string(log_strings_maxlen - cur_size_log1, ' '));
        }
        str_log += "[" + modules::to_string(modules::type::LAUNCHER) + "] ";
        int cur_size_src1 = modules::to_string(modules::type::LAUNCHER).size();
        if (cur_size_src1 < source_strings_maxlen) {
            str_log.append(std::string(source_strings_maxlen - cur_size_src1, ' '));
        }
        str_log += msg_log;

        std::string str_err = "[" + logger::to_string(logger::level::ERR) + "] ";
        int cur_size_log2 = logger::to_string(logger::level::ERR).size();
        if (cur_size_log2 < log_strings_maxlen) {
            str_err.append(std::string(log_strings_maxlen - cur_size_log2, ' '));
        }
        str_err += "[" + modules::to_string(modules::type::LAUNCHER) + "] ";
        int cur_size_src2 = modules::to_string(modules::type::LAUNCHER).size();
        if (cur_size_src2 < source_strings_maxlen) {
            str_err.append(std::string(source_strings_maxlen - cur_size_src2, ' '));
        }
        str_err += msg_err;

        std::string str_dbg = "[" + logger::to_string(logger::level::DBG) + "] ";
        int cur_size_log3 = logger::to_string(logger::level::DBG).size();
        if (cur_size_log3 < log_strings_maxlen) {
            str_dbg.append(std::string(log_strings_maxlen - cur_size_log3, ' '));
        }
        str_dbg += "[" + modules::to_string(modules::type::LAUNCHER) + "] ";
        int cur_size_src3 = modules::to_string(modules::type::LAUNCHER).size();
        if (cur_size_src3 < source_strings_maxlen) {
            str_dbg.append(std::string(source_strings_maxlen - cur_size_src3, ' '));
        }
        str_dbg += msg_dbg;

        std::ifstream log_file;


        /* BEGIN Test with debug mode on. */

        std::cout << "Testing with debug mode on.\n";

        logger::this_logger = new local_logger_test(true, std::string(log_path));

        logger::this_logger->log(msg_log, logger::level::LOG);
        logger::this_logger->log(msg_err, logger::level::ERR);
        logger::this_logger->log(msg_dbg, logger::level::DBG);

        log_file.open(log_path);

        count = 0;
        order = 0;

        log_line.clear();
        while (std::getline(log_file, log_line)) {
            if (log_line.find(str_log) != std::string::npos) {
                ++count;
                if (order++ != 0) {
                    success = false;
                    std::cout << "Messages written in wrong order!\n";
                }
            } else if (log_line.find(str_err) != std::string::npos) {
                ++count;
                if (order++ != 1) {
                    success = false;
                    std::cout << "Messages written in wrong order!\n";
                }
            } else if (log_line.find(str_dbg) != std::string::npos) {
                ++count;
                if (order++ != 2) {
                    success = false;
                    std::cout << "Messages written in wrong order!\n";
                }
            }
        }

        if (count != 3) {
            success = false;
            std::cout << "One message was not written to the log file!\n";
        }

        log_file.close();
        remove(log_path);

        /* END Test with debug mode on. */


        /* BEGIN Test with debug mode off. */

        std::cout << "Testing with debug mode off.\n";

        logger::this_logger = new local_logger_test(false, std::string(log_path));

        logger::this_logger->log(msg_log, logger::level::LOG);
        logger::this_logger->log(msg_err, logger::level::ERR);
        logger::this_logger->log(msg_dbg, logger::level::DBG);

        log_file.open(log_path);

        count = 0;
        order = 0;

        log_line.clear();
        while (std::getline(log_file, log_line)) {
            if (log_line.find(str_log) != std::string::npos) {
                ++count;
                if (order++ != 0) {
                    success = false;
                    std::cout << "Messages written in wrong order!\n";
                }
            } else if (log_line.find(str_err) != std::string::npos) {
                ++count;
                if (order++ != 1) {
                    success = false;
                    std::cout << "Messages written in wrong order!\n";
                }
            } else if (log_line.find(str_dbg) != std::string::npos) {
                ++count;
                success = false;
                std::cout << "Debug message was written despite debug mode being off!\n";
            }
        }

        if (count != 2) {
            success = false;
            std::cout << "One message was not written to the log file!\n";
        }

        delete logger::this_logger;

        log_file.close();
        remove(log_path);

        /* END Test with debug mode off. */

        return success;
    }

}}}


int main() {
    return run_tests({
        { "01 Launcher: Local logger test", &hems::modules::launcher::test_local_logger }
    });
}
