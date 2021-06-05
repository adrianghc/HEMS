/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is a dummy logger for unit tests. All it does is receive and discard log messages.
 */

#include <iostream>
#include "hems/common/logger.h"
#include "hems/common/modules.h"

namespace hems {

    class dummy_logger : public logger {
        public:
            dummy_logger() : logger(modules::type::LAUNCHER, false) {};
            ~dummy_logger() {};

            void log(std::string msg, level log_level) override {
                return;
            }
    };

}
