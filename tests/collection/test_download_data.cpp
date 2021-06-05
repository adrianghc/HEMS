/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * These are unit tests for the Measurement Collection Module.
 */

#include <cstdlib>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "../test.hpp"
#include "../extras/dummy_logger.hpp"

#include "hems/modules/collection/collection.h"
#include "hems/messages/storage.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems { namespace modules { namespace collection {

    using boost::posix_time::ptime;
    using boost::posix_time::time_from_string;
    using hems::types::id_t;

    /**
     * @brief   This class exists to provide access to private members of `hems_collection`.
     */
    class collection_test : public hems_collection {
        public:
            collection_test() : hems_collection(true) {};

            int download_energy_production_(ptime time) {
                return download_energy_production(time);
            }

            // int download_energy_consumption_(ptime time, id_t appliance) {
            //     return download_energy_consumption(time, appliance);
            // }

            int download_weather_data_(ptime time, id_t station) {
                return download_weather_data(time, station);
            }

            enum response_code {
                SUCCESS = 0x00,
                INVALID_DATA,
                UNREACHABLE_SOURCE,
                INVALID_RESPONSE_SOURCE,
                DATA_STORAGE_MODULE_ERR
            };

    };


    int handler_set_energy_production_success(text_iarchive& ia, text_oarchive* oa) {
        return 0;
    }

    int handler_set_energy_production_fail(text_iarchive& ia, text_oarchive* oa) {
        return 1;
    }

    int handler_del_energy_production(text_iarchive& ia, text_oarchive* oa) {
        return 0;
    }

    int handler_set_weather_success(text_iarchive& ia, text_oarchive* oa) {
        return 0;
    }

    int handler_set_weather_fail(text_iarchive& ia, text_oarchive* oa) {
        return 1;
    }

    int handler_del_weather(text_iarchive& ia, text_oarchive* oa) {
        return 0;
    }


    pid_t start_source_server(std::string path) {
        const char* argv[] = { path.c_str(), (char*) nullptr };
        switch (pid_t pid = fork()) {
            case -1:
                return 0;
                break;
            case 0:
                execv(path.c_str(), const_cast<char* const*>(argv));
                /*  `exec` only returns if something failed, so if the next line is ever
                    reached, something went wrong. We use `std::exit()` directly here
                    because we don't want a fork of the launcher module to call its destructor
                    as well. */
                std::exit(EXIT_FAILURE);
                break;
            default:
                return pid;
                break;
        }
    }


    bool init_test(
        collection_test* this_instance, messenger* messenger_storage, const messenger::msg_handler_map& handler_map
    ) {
        hems::logger::this_logger = new hems::dummy_logger();

        /* Open message queues. */
        struct mq_attr attr = { 
            mq_flags    : 0,
            mq_maxmsg   : 10,
            mq_msgsize  : sizeof(messenger::msg_t),
            mq_curmsgs  : 0
        };
        mq_close(mq_open(
            messenger::mq_names.at(modules::type::STORAGE).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));
        mq_close(mq_open(
            messenger::mq_res_names.at(modules::type::STORAGE).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));
        mq_close(mq_open(
            messenger::mq_names.at(modules::type::COLLECTION).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));
        mq_close(mq_open(
            messenger::mq_res_names.at(modules::type::COLLECTION).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));

        /* Start listen loop for fake Data Storage Module. */
        messenger_storage = new hems::messenger(modules::type::STORAGE, true);
        if (!messenger_storage->listen(handler_map)) {
            std::cout << "Starting listen loop failed.\n";
            return false;
        }
        messenger_storage->start_handlers();

        /* Construct Measurement Collection Module. */
        try {
            this_instance = new collection_test();
        } catch (int err) {
            std::cout <<
                "Constructing Data Storage Module threw an exception " + std::to_string(err) + ", test failed.\n";
            return false;
        }

        return true;
    }

    void end_test(collection_test* this_instance, messenger* messenger_storage) {
        delete messenger_storage;
        delete this_instance;

        mq_unlink(messenger::mq_names.at(modules::type::COLLECTION).c_str());
        mq_unlink(messenger::mq_names.at(modules::type::STORAGE).c_str());

        delete logger::this_logger;
    }


    bool test_energy_production() {
        const messenger::msg_handler_map handler_map_fail = {
            { messages::storage::MSG_SET_ENERGY_PRODUCTION, &handler_set_energy_production_fail },
            { messages::storage::MSG_DEL_ENERGY_PRODUCTION, &handler_del_energy_production },
        };
        const messenger::msg_handler_map handler_map_success = {
            { messages::storage::MSG_SET_ENERGY_PRODUCTION, &handler_set_energy_production_success },
            { messages::storage::MSG_DEL_ENERGY_PRODUCTION, &handler_del_energy_production },
        };

        bool success = true;
        int res;

        collection_test* this_instance = nullptr;
        messenger* messenger_storage = nullptr;

        if (!init_test(this_instance, messenger_storage, handler_map_fail)) {
            return false;
        }


        /* Test unreachable source server. */
        res = this_instance->download_energy_production_(time_from_string("9999-01-01 00:00:00.000"));
        if (res != collection_test::response_code::UNREACHABLE_SOURCE) {
            std::cout <<
                "Call that should have returned " +
                std::to_string(collection_test::response_code::UNREACHABLE_SOURCE) +
                " returned " + std::to_string(res) + " instead.\n";
            success = false;
        }


        /* Start source server. */
        pid_t pid = start_source_server("collection/energy_production_provider/run.sh");
        if (!pid) {
            std::cout << "Could not start source server.\n";
            return false;
        }
        sleep(2);


        /* Test invalid values. */
        std::vector<std::string> time_strings_1 = {
            "2020-02-20 20:00:00.123",
            "2020-02-20 20:00:20.000",
            "2020-02-20 20:20:00.000"
        };
        for (const auto& time_string : time_strings_1) {
            res = this_instance->download_energy_production_(time_from_string(time_string));
            if (res != collection_test::response_code::INVALID_DATA) {
                std::cout <<
                    "Call that should have returned " +
                    std::to_string(collection_test::response_code::INVALID_DATA) + " returned " +
                    std::to_string(res) + " instead.\n";
                success = false;
            }
        }

        /* Test valid value, invalid response from source server. */
        res = this_instance->download_energy_production_(time_from_string("9999-01-01 00:00:00.000"));
        if (res != collection_test::response_code::INVALID_RESPONSE_SOURCE) {
            std::cout <<
                "Call that should have returned " +
                std::to_string(collection_test::response_code::INVALID_RESPONSE_SOURCE) +
                " returned " + std::to_string(res) + " instead.\n";
            success = false;
        }

        /* Test valid value, error from Data Storage Module. */
        res = this_instance->download_energy_production_(time_from_string("2020-02-20 20:00:00.000"));
        if (res != collection_test::response_code::DATA_STORAGE_MODULE_ERR) {
            std::cout <<
                "Call that should have returned " +
                std::to_string(collection_test::response_code::DATA_STORAGE_MODULE_ERR) +
                " returned " + std::to_string(res) + " instead.\n";
            success = false;
        }

        end_test(this_instance, messenger_storage);


        /* Test valid value, no errors. */
        if (!init_test(this_instance, messenger_storage, handler_map_success)) {
            return false;
        }
        res = this_instance->download_energy_production_(time_from_string("2020-02-20 20:00:00.000"));
        if (res != collection_test::response_code::SUCCESS) {
            std::cout <<
                "Call that should have returned " +
                std::to_string(collection_test::response_code::SUCCESS) + " returned " +
                std::to_string(res) + " instead.\n";
            success = false;
        }
        end_test(this_instance, messenger_storage);


        kill(pid, SIGKILL);
        return success;
    }

    bool test_weather() {
        const messenger::msg_handler_map handler_map_fail = {
            { messages::storage::MSG_SET_WEATHER, &handler_set_weather_fail },
            { messages::storage::MSG_DEL_WEATHER, &handler_del_weather },
        };
        const messenger::msg_handler_map handler_map_success = {
            { messages::storage::MSG_SET_WEATHER, &handler_set_weather_success },
            { messages::storage::MSG_DEL_WEATHER, &handler_del_weather },
        };

        bool success = true;
        int res;

        collection_test* this_instance = nullptr;
        messenger* messenger_storage = nullptr;

        if (!init_test(this_instance, messenger_storage, handler_map_fail)) {
            return false;
        }


        double longitude_valid = 13.296937;
        double latitude_valid = 52.455864;
        double timezone_valid = 1;
        std::string time_string_valid = "2020-02-20 20:00:00.000";
        id_t station_valid = 1;

        types::settings_t settings = {
            longitude                   : longitude_valid,
            latitude                    : latitude_valid,
            timezone                    : timezone_valid,
            pv_uri                      : "",
            station_intervals           : { {1, 15}, {2, 30} },
            station_uris                : {},
            interval_energy_production  : 10,
            interval_energy_consumption : 20,
            interval_automation         : 36
        };
        current_settings = settings;


        /* Test unreachable source server. */
        res = this_instance->download_weather_data_(
            time_from_string("9999-01-01 00:00:00.000"), station_valid
        );
        if (res != collection_test::response_code::UNREACHABLE_SOURCE) {
            std::cout <<
                "Call that should have returned " +
                std::to_string(collection_test::response_code::UNREACHABLE_SOURCE) +
                " returned " + std::to_string(res) + " instead.\n";
            success = false;
        }


        /* Start source server. */
        pid_t pid = start_source_server("collection/weather_provider/run.sh");
        if (!pid) {
            std::cout << "Could not start source server.\n";
            return false;
        }
        sleep(2);


        /* Test invalid values (time). */
        std::vector<std::string> time_strings_1 = {
            "2020-02-20 20:00:00.123",
            "2020-02-20 20:00:20.000",
            "2020-02-20 20:33:00.000"
        };
        for (const auto& time_string : time_strings_1) {
            res = this_instance->download_weather_data_(
                time_from_string(time_string), station_valid
            );
            if (res != collection_test::response_code::INVALID_DATA) {
                std::cout <<
                    "Call that should have returned " +
                    std::to_string(collection_test::response_code::INVALID_DATA) + " returned " +
                    std::to_string(res) + " instead.\n";
                success = false;
            }
        }

        /* Test valid value, invalid response from source server. */
        res = this_instance->download_weather_data_(
            time_from_string("9999-01-01 00:00:00.000"), station_valid
        );
        if (res != collection_test::response_code::INVALID_RESPONSE_SOURCE) {
            std::cout <<
                "Call that should have returned " +
                std::to_string(collection_test::response_code::INVALID_RESPONSE_SOURCE) +
                " returned " + std::to_string(res) + " instead.\n";
            success = false;
        }

        /* Test valid value, error from Data Storage Module. */
        res = this_instance->download_weather_data_(
            time_from_string("2020-02-20 20:00:00.000"), station_valid
        );
        if (res != collection_test::response_code::DATA_STORAGE_MODULE_ERR) {
            std::cout <<
                "Call that should have returned " +
                std::to_string(collection_test::response_code::DATA_STORAGE_MODULE_ERR) +
                " returned " + std::to_string(res) + " instead.\n";
            success = false;
        }

        end_test(this_instance, messenger_storage);


        /* Test valid value, no errors. */
        if (!init_test(this_instance, messenger_storage, handler_map_success)) {
            return false;
        }
        res = this_instance->download_weather_data_(
            time_from_string("2020-02-20 20:00:00.000"), station_valid
        );
        if (res != collection_test::response_code::SUCCESS) {
            std::cout <<
                "Call that should have returned " +
                std::to_string(collection_test::response_code::SUCCESS) + " returned " +
                std::to_string(res) + " instead.\n";
            success = false;
        }
        end_test(this_instance, messenger_storage);


        kill(pid, SIGKILL);
        return success;
    }

}}}


int main() {
    return run_tests({
        {
            "01 Collection: Download energy production data test",
            &hems::modules::collection::test_energy_production
        },
        {
            "02 Collection: Download weather data test",
            &hems::modules::collection::test_weather
        }
    });
}
