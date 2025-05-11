#!/bin/bash

# This script is adapted from https://github.com/embedd-actions/nrf-connect-sdk-ci
# Original project is licensed under the MIT License:
# Copyright (c) 2023 Sergey Ladanov

. /workdir/ncs/zephyr/zephyr-env.sh
/bin/bash /workdir/zephyr-sdk-${ZEPHYR_TAG}/setup.sh -t arm-zephyr-eabi
/bin/bash /workdir/zephyr-sdk-${ZEPHYR_TAG}/setup.sh -c

./app/setup_mcuboot.sh /app/priv.pem
west build --build-dir /app/build --pristine --no-sysbuild --board ${BOARD} /app
