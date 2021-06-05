#!/bin/sh

FAIL=0
cd build/native/tests

run_test () {
    $1
    ret=$?
    if [ $ret -eq "127" ] || [ $ret -eq "134" ]; then
        FAIL=$(($FAIL + 1))
    else
        FAIL=$(($FAIL + $ret))
    fi
    echo "\n"
}

# Run common tests

run_test "./common/test_messenger"
run_test "./common/test_settings"
run_test "./common/test_types"

# Run tests for Launcher Module

run_test "./launcher/test_launcher"
run_test "./launcher/test_local_logger"

# Run tests for Data Storage Module

run_test "./storage/test_storage"
run_test "./storage/test_handler_settings"
run_test "./storage/test_handler_msg_set"
run_test "./storage/test_handler_msg_del"
run_test "./storage/test_handler_msg_get"

# Run tests for Measurement Collection Module (requires additional shell scripts)

chmod u+x collection/energy_production_provider/run.sh
chmod u+x collection/weather_provider/run.sh
run_test "./collection/test_download_data"

echo "TOTAL TEST FAILURES: $FAIL"
