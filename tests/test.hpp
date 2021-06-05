/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This contains functions for all unit tests.
 */

#include <iostream>
#include <functional>
#include <map>

int run_tests(std::map<std::string, const std::function<bool()>> tests) {
    int total = tests.size();
    int success = 0;
    int fail = 0;

    for (const auto& test : tests) {
        std::cout << "### BEGIN '" + test.first + "' ###\n";
        if (test.second()) {
            ++success;
        } else {
            ++fail;
        }
        std::cout << "### END '" + test.first + "' ###\n";
    }

    std::cout << "\n";

    std::cout << "SUCCESS: " + std::to_string(success) + "/" + std::to_string(total) + "\n";
    std::cout << "FAIL: " + std::to_string(fail) + "/" + std::to_string(total) + "\n";

    return fail;
}
