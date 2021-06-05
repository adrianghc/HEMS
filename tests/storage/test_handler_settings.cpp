/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * These are unit tests for the Data Storage Module.
 */

#include <boost/archive/text_oarchive.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "../test.hpp"
#include "../extras/dummy_logger.hpp"
#include "../extras/random_file_name.hpp"

#include "hems/modules/storage/storage.h"
#include "hems/messages/storage.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

#include "sqlite/sqlite3.h"

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

namespace hems { namespace modules { namespace storage {

    using namespace hems::messages::storage;
    using namespace hems::types;

    /**
     * @brief   This class exists to provide helper functions that need access to private members of
     *          `hems_storage`.
     */
    class hems_storage_test : public hems_storage {
        public:
            hems_storage_test(std::string db_path) : hems_storage(true, db_path) {};

            bool prepare_and_evaluate(const std::string& stmt, sqlite3_stmt*& prepared_stmt) {
                int errcode = sqlite3_prepare_v2(
                    db_connection, stmt.c_str(), -1, &prepared_stmt, nullptr
                );
                if (errcode != SQLITE_OK) {
                    std::cout <<
                        "Error preparing a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else if ((errcode = sqlite3_step(prepared_stmt)) != SQLITE_ROW) {
                    std::cout <<
                        "Error evaluating a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    return true;
                }
            }

            bool prepare_and_evaluate_no_row_expected(std::string& stmt, sqlite3_stmt*& prepared_stmt) {
                int errcode = sqlite3_prepare_v2(
                    db_connection, stmt.c_str(), -1, &prepared_stmt, nullptr
                );
                if (errcode != SQLITE_OK) {
                    std::cout <<
                        "Error preparing a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else if ((errcode = sqlite3_step(prepared_stmt)) == SQLITE_ROW) {
                    std::cout <<
                        "Error evaluating a statement: '" + stmt + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    sqlite3_finalize(prepared_stmt);
                    return false;
                } else {
                    return true;
                }
            }

            sqlite3* get_db_connection() {
                return db_connection;
            }
    };


    /**
     * @brief   Test the handler for `SETTINGS_CHECK` messages.
     */
    bool test_handler_settings_check_(messenger_test* this_messenger, hems_storage_test* this_instance) {
        /* Test valid settings. */

        std::vector<settings_t> valid_settings_1 = {
            {
                longitude                   : 52.455864,
                latitude                    : 13.296937,
                timezone                    : 1,
                pv_uri                      : "'Tis the season",
                station_intervals           : { {1, 15}, {2, 30} },
                station_uris                : { {1, "'Tis the season"} },
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 52.455864,
                latitude                    : 13.296937,
                timezone                    : 1,
                pv_uri                      : "",
                station_intervals           : {},
                station_uris                : {},
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            }
        };
        for (const auto& settings1 : valid_settings_1) {
            int res = this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messenger::special_subtype::SETTINGS_CHECK,
                modules::STORAGE,
                this_messenger->serialize(settings1),
                nullptr
            );

            if (res != messenger::settings_code::SUCCESS) {
                std::cout <<
                    "Settings check that should have succeeded returned error " + std::to_string(res) +
                        " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt1;
                std::string stmt1 = "SELECT * FROM settings WHERE id=0";

                if (!this_instance->prepare_and_evaluate_no_row_expected(stmt1, prepared_stmt1)) {
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt1);
                }

                sqlite3_stmt* prepared_stmt2;
                std::string stmt2 = "SELECT * FROM settings_stations WHERE settings_id=0";

                if (!this_instance->prepare_and_evaluate_no_row_expected(stmt2, prepared_stmt2)) {
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt2);
                }
            }
        }

        /* Test invalid settings. */

        std::vector<settings_t> invalid_settings_2 = {
            {
                longitude                   : 52.455864,
                latitude                    : 13.296937,
                timezone                    : 1,
                pv_uri                      : "",
                station_intervals           : { {1, 0} },
                station_uris                : {},
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 52.455864,
                latitude                    : 13.296937,
                timezone                    : 1,
                pv_uri                      : "",
                station_intervals           : { {1, 61} },
                station_uris                : {},
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 98.67342,
                latitude                    : 0.8662,
                timezone                    : -2.5,
                pv_uri                      : "",
                station_intervals           : { {2, 7} },
                station_uris                : {},
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 98.67342,
                latitude                    : 0.8662,
                timezone                    : -2.5,
                pv_uri                      : "",
                station_intervals           : { {1, 60} },
                station_uris                : { {2, ""} },
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 98.67342,
                latitude                    : 0.8662,
                timezone                    : -2.5,
                pv_uri                      : "",
                station_intervals           : {},
                station_uris                : {},
                interval_energy_production  : 7,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 98.67342,
                latitude                    : 0.8662,
                timezone                    : -2.5,
                pv_uri                      : "",
                station_intervals           : {},
                station_uris                : {},
                interval_energy_production  : 10,
                interval_energy_consumption : 8,
                interval_automation         : 36
            }
        };
        for (const auto& settings2 : invalid_settings_2) {
            int res = this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messenger::special_subtype::SETTINGS_CHECK,
                modules::STORAGE,
                this_messenger->serialize(settings2),
                nullptr
            );

            if (res != messenger::settings_code::INVALID) {
                std::cout <<
                    "Settings check that should have failed with error " +
                    std::to_string(messenger::settings_code::INVALID) + " returned error " +
                    std::to_string(res) + " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt1;
                std::string stmt1 = "SELECT * FROM settings WHERE id=0";

                if (!this_instance->prepare_and_evaluate_no_row_expected(stmt1, prepared_stmt1)) {
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt1);
                }

                sqlite3_stmt* prepared_stmt2;
                std::string stmt2 = "SELECT * FROM settings_stations WHERE settings_id=0";

                if (!this_instance->prepare_and_evaluate_no_row_expected(stmt2, prepared_stmt2)) {
                    return false;
                } else {
                    sqlite3_finalize(prepared_stmt2);
                }
            }
        }

        return true;
    }

    /**
     * @brief   Test the handler for `SETTINGS_COMMIT` messages.
     */
    bool test_handler_settings_commit_(messenger_test* this_messenger, hems_storage_test* this_instance) {
        /* Test valid settings. */

        std::vector<settings_t> valid_settings_1 = {
            {
                longitude                   : 52.455864,
                latitude                    : 13.296937,
                timezone                    : 1,
                pv_uri                      : "",
                station_intervals           : { {1, 15}, {2, 30} },
                station_uris                : {},
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 52.455864,
                latitude                    : 13.296937,
                timezone                    : 11,
                pv_uri                      : "'Tis the season",
                station_intervals           : { {1, 15}, {2, 30}, {3, 10} },
                station_uris                : { {2, "Lorem"}, {3, "ipsum"} },
                interval_energy_production  : 20,
                interval_energy_consumption : 5,
                interval_automation         : 50
            },
            {
                longitude                   : 52.455864,
                latitude                    : 13.296937,
                timezone                    : 10.5,
                pv_uri                      : "",
                station_intervals           : {},
                station_uris                : {},
                interval_energy_production  : 60,
                interval_energy_consumption : 30,
                interval_automation         : 100
            }
        };
        for (const auto& settings1 : valid_settings_1) {
            int res = this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messenger::special_subtype::SETTINGS_COMMIT,
                modules::STORAGE,
                this_messenger->serialize(settings1),
                nullptr
            );

            if (res != messenger::settings_code::SUCCESS) {
                std::cout <<
                    "Settings check that should have succeeded returned error " + std::to_string(res) +
                        " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt1;
                std::string stmt1 = "SELECT * FROM settings WHERE id=0";

                if (!this_instance->prepare_and_evaluate(stmt1, prepared_stmt1)) {
                    return false;
                } else {
                    id_t id = sqlite3_column_int64(prepared_stmt1, 0);
                    double longitude = sqlite3_column_double(prepared_stmt1, 1);
                    double latitude = sqlite3_column_double(prepared_stmt1, 2);
                    double timezone = sqlite3_column_double(prepared_stmt1, 3);
                    std::string pv_uri =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt1, 4));
                    unsigned int interval_energy_production = sqlite3_column_int(prepared_stmt1, 5);
                    unsigned int interval_energy_consumption = sqlite3_column_int(prepared_stmt1, 6);
                    unsigned int interval_automation = sqlite3_column_int(prepared_stmt1, 7);

                    if (id || longitude != settings1.longitude || latitude != settings1.latitude ||
                        timezone != settings1.timezone || pv_uri != settings1.pv_uri ||
                        interval_energy_production != settings1.interval_energy_production ||
                        interval_energy_consumption != settings1.interval_energy_consumption ||
                        interval_automation != settings1.interval_automation) {
                        std::cout << "Item in database is not identical to the one sent.\n";
                        sqlite3_finalize(prepared_stmt1);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt1);
                    }
                }

                sqlite3_stmt* prepared_stmt2;
                std::string stmt2 = "SELECT * FROM settings_stations WHERE settings_id=0";
                std::map<id_t, unsigned int> station_intervals;

                int errcode = sqlite3_prepare_v2(
                    this_instance->get_db_connection(), stmt2.c_str(), -1, &prepared_stmt2, nullptr
                );
                if (errcode != SQLITE_OK) {
                    std::cout <<
                        "Error preparing a statement: '" + stmt2 + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    return false;
                } else {
                    while ((errcode = sqlite3_step(prepared_stmt2)) == SQLITE_ROW) {
                        id_t station_id = sqlite3_column_int64(prepared_stmt2, 0);
                        unsigned int interval = sqlite3_column_int(prepared_stmt2, 2);

                        station_intervals.emplace(station_id, interval);
                    }
                    if (errcode != SQLITE_DONE) {
                        std::cout <<
                            "Error evaluating a statement: '" + stmt2 + "'. The error was: " +
                            sqlite3_errstr(errcode) + "\n";
                        return false;
                    }
                }
                sqlite3_finalize(prepared_stmt2);
                if (station_intervals != settings1.station_intervals) {
                    std::cout << "Item in database is not identical to the one sent.\n";
                    return false;
                }
            }
        }

        /* Test invalid settings. */

        std::vector<settings_t> invalid_settings_2 = {
            {
                longitude                   : 56.235346,
                latitude                    : 7.135687,
                timezone                    : 2.5,
                pv_uri                      : "",
                station_intervals           : { {1, 0} },
                station_uris                : {},
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 0,
                latitude                    : -1.85,
                timezone                    : -3,
                pv_uri                      : "",
                station_intervals           : { {1, 61} },
                station_uris                : {},
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 0,
                latitude                    : -1.85,
                timezone                    : -3,
                pv_uri                      : "",
                station_intervals           : { {1, 60} },
                station_uris                : { {2, ""} },
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 98.67342,
                latitude                    : 0.8662,
                timezone                    : -2.5,
                pv_uri                      : "",
                station_intervals           : { {2, 7} },
                station_uris                : {},
                interval_energy_production  : 10,
                interval_energy_consumption : 20,
                interval_automation         : 70
            },
            {
                longitude                   : 98.67342,
                latitude                    : 0.8662,
                timezone                    : -2.5,
                pv_uri                      : "",
                station_intervals           : {},
                station_uris                : {},
                interval_energy_production  : 9,
                interval_energy_consumption : 20,
                interval_automation         : 36
            },
            {
                longitude                   : 98.67342,
                latitude                    : 0.8662,
                timezone                    : -2.5,
                pv_uri                      : "",
                station_intervals           : {},
                station_uris                : {},
                interval_energy_production  : 11,
                interval_energy_consumption : 8,
                interval_automation         : 36
            }
        };
        for (const auto& settings2 : invalid_settings_2) {
            int res = this_messenger->send(
                DEFAULT_SEND_TIMEOUT,
                messenger::special_subtype::SETTINGS_COMMIT,
                modules::STORAGE,
                this_messenger->serialize(settings2),
                nullptr
            );

            if (res != messenger::settings_code::INVALID) {
                std::cout <<
                    "Settings check that should have failed with error " +
                    std::to_string(messenger::settings_code::INVALID) + " returned error " +
                    std::to_string(res) + " instead.\n";
                return false;
            } else {
                sqlite3_stmt* prepared_stmt1;
                std::string stmt1 = "SELECT * FROM settings WHERE id=0";

                if (!this_instance->prepare_and_evaluate(stmt1, prepared_stmt1)) {
                    return false;
                } else {
                    id_t id = sqlite3_column_int64(prepared_stmt1, 0);
                    double longitude = sqlite3_column_double(prepared_stmt1, 1);
                    double latitude = sqlite3_column_double(prepared_stmt1, 2);
                    double timezone = sqlite3_column_double(prepared_stmt1, 3);
                    std::string pv_uri =
                        reinterpret_cast<const char*>(sqlite3_column_text(prepared_stmt1, 4));
                    unsigned int interval_energy_production = sqlite3_column_int(prepared_stmt1, 5);
                    unsigned int interval_energy_consumption = sqlite3_column_int(prepared_stmt1, 6);
                    unsigned int interval_automation = sqlite3_column_int(prepared_stmt1, 7);

                    settings_t& settings1 = valid_settings_1.back();

                    if (id || longitude != settings1.longitude || latitude != settings1.latitude ||
                        timezone != settings1.timezone || pv_uri != settings1.pv_uri ||
                        interval_energy_production != settings1.interval_energy_production ||
                        interval_energy_consumption != settings1.interval_energy_consumption ||
                        interval_automation != settings1.interval_automation) {
                        std::cout << "Old valid entry was erroneously overwritten.\n";
                        sqlite3_finalize(prepared_stmt1);
                        return false;
                    } else {
                        sqlite3_finalize(prepared_stmt1);
                    }
                }

                sqlite3_stmt* prepared_stmt2;
                std::string stmt2 = "SELECT * FROM settings_stations WHERE settings_id=0";
                std::map<id_t, unsigned int> station_intervals;

                int errcode = sqlite3_prepare_v2(
                    this_instance->get_db_connection(), stmt2.c_str(), -1, &prepared_stmt2, nullptr
                );
                if (errcode != SQLITE_OK) {
                    std::cout <<
                        "Error preparing a statement: '" + stmt2 + "'. The error was: " +
                        sqlite3_errstr(errcode) + "\n";
                    return false;
                } else {
                    while ((errcode = sqlite3_step(prepared_stmt2)) == SQLITE_ROW) {
                        id_t station_id = sqlite3_column_int64(prepared_stmt2, 0);
                        unsigned int interval = sqlite3_column_int(prepared_stmt2, 2);

                        station_intervals.emplace(station_id, interval);
                    }
                    if (errcode != SQLITE_DONE) {
                        std::cout <<
                            "Error evaluating a statement: '" + stmt2 + "'. The error was: " +
                            sqlite3_errstr(errcode) + "\n";
                        return false;
                    }
                }
                sqlite3_finalize(prepared_stmt2);
                if (station_intervals != valid_settings_1.back().station_intervals) {
                    std::cout << "Old valid entry was erroneously overwritten.\n";
                    return false;
                }
            }
        }

        return true;
    }


    bool test_handler_settings(messenger::special_subtype test_type) {
        logger::this_logger = new dummy_logger();

        char db_path[11];
        do {
            generate_random_file_name(db_path);
        } while (boost::filesystem::exists(db_path));

        /* Delete message queues to remove junk messages from previous runs. */
        mq_unlink(messenger::mq_names.at(modules::type::STORAGE).c_str());
        mq_unlink(messenger::mq_names.at(modules::type::LAUNCHER).c_str());

        /* Create the message queue for the Data Storage Module so that its constructor does not fail. */
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

        /*  Create message queues under the identity of the HEMS Launcher that we can use to
            communicate with the Data Storage Module. */
        mq_close(mq_open(
            messenger::mq_names.at(modules::type::LAUNCHER).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));
        mq_close(mq_open(
            messenger::mq_res_names.at(modules::type::LAUNCHER).c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr
        ));

        try {
            hems_storage::this_instance = new hems_storage_test(db_path);
        } catch (int err) {
            std::cout <<
                "Constructing Data Storage Module threw an exception " + std::to_string(err) +
                ", test failed.\n";
            return false;
        }

        messenger_test* this_messenger = new messenger_test(modules::LAUNCHER);
        const messenger::msg_handler_map handler_map = {
            /* These three handlers are necessary to avoid an error, but they won't be called. */
            {
                messenger::special_subtype::SETTINGS_INIT,
                std::function<int(text_iarchive&, text_oarchive*)>()
            },
            {
                messenger::special_subtype::SETTINGS_CHECK,
                std::function<int(text_iarchive&, text_oarchive*)>()
            },
            {
                messenger::special_subtype::SETTINGS_COMMIT,
                std::function<int(text_iarchive&, text_oarchive*)>()
            }
        };
        this_messenger->listen(handler_map);
        this_messenger->start_handlers();

        hems_storage_test* this_instance =
            reinterpret_cast<hems_storage_test*>(hems_storage::this_instance);

        bool success;
        switch (test_type) {
            case messenger::special_subtype::SETTINGS_CHECK:
                success = test_handler_settings_check_(this_messenger, this_instance);
                break;
            case messenger::special_subtype::SETTINGS_COMMIT:
                success = test_handler_settings_commit_(this_messenger, this_instance);
                break;
            default:
                success = false;
                break;
        }

        delete hems_storage::this_instance;
        delete this_messenger;
        delete logger::this_logger;

        remove(db_path);
        mq_unlink(messenger::mq_names.at(modules::type::STORAGE).c_str());
        mq_unlink(messenger::mq_names.at(modules::type::LAUNCHER).c_str());

        return success;
    }

    static inline bool test_handler_settings_check() {
        return test_handler_settings(messenger::special_subtype::SETTINGS_CHECK);
    }

    static inline bool test_handler_settings_commit() {
        return test_handler_settings(messenger::special_subtype::SETTINGS_COMMIT);
    }

}}}

int main() {
    return run_tests({
        {
            "01 Storage: Message handler test for SETTINGS_CHECK",
            &hems::modules::storage::test_handler_settings_check
        },
        {
            "02 Storage: Message handler test for SETTINGS_COMMIT",
            &hems::modules::storage::test_handler_settings_commit
        }
    });
}
