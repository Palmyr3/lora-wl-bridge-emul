#!/usr/bin/env bash

gcc -Wall -lpthread \
	-DCONFIG_WL_LOSE_PACKETS=false \
	-DCONFIG_WL_LOSE_PACKETS_SCORE=10 \
	-DCONFIG_WL_ERR_DATA=false \
	-DCONFIG_WL_ERR_DATA_FIRST_SCORE=2 \
	-DCONFIG_WL_ERR_DATA_MULTIP_SCORE=80 \
	-DCONFIG_WL_ADD_PACKETS=false \
	-DCONFIG_WL_ADD_PACKETS_SCORE=2 \
	-DCONFIG_WL_RANDOMIZE_TOA=false \
	-DCONFIG_WL_STD_TIME_ON_AIR_MS=100 \
	mt-test-env-core.c env-core.c stack-comm.c timeout-timer-internal.c cc-traceinfo.c -o mt-test-env-core
gcc -Wall -lpthread \
	-DCONFIG_WL_LOSE_PACKETS=false \
	-DCONFIG_WL_LOSE_PACKETS_SCORE=10 \
	-DCONFIG_WL_ERR_DATA=true \
	-DCONFIG_WL_ERR_DATA_FIRST_SCORE=2 \
	-DCONFIG_WL_ERR_DATA_MULTIP_SCORE=80 \
	-DCONFIG_WL_ADD_PACKETS=false \
	-DCONFIG_WL_ADD_PACKETS_SCORE=2 \
	-DCONFIG_WL_RANDOMIZE_TOA=false \
	-DCONFIG_WL_STD_TIME_ON_AIR_MS=100 \
	mt-test-env-core.c env-core.c stack-comm.c timeout-timer-internal.c cc-traceinfo.c -o mt-test-err-env-core

gcc -Wall -lpthread \
	-DCONFIG_WL_LOSE_PACKETS=false \
	-DCONFIG_WL_LOSE_PACKETS_SCORE=10 \
	-DCONFIG_WL_ERR_DATA=true \
	-DCONFIG_WL_ERR_DATA_FIRST_SCORE=2 \
	-DCONFIG_WL_ERR_DATA_MULTIP_SCORE=80 \
	-DCONFIG_WL_ADD_PACKETS=false \
	-DCONFIG_WL_ADD_PACKETS_SCORE=2 \
	-DCONFIG_WL_RANDOMIZE_TOA=false \
	-DCONFIG_WL_STD_TIME_ON_AIR_MS=100 \
	mt-test-env-core-w-timout.c env-core.c stack-comm.c timeout-timer-internal.c cc-traceinfo.c -o mt-test-env-core-w-timout

#gcc -Wall -lpthread \
#	-DCONFIG_WL_LOSE_PACKETS=true \
#	-DCONFIG_WL_LOSE_PACKETS_SCORE=10 \
#	-DCONFIG_WL_ERR_DATA=true \
#	-DCONFIG_WL_ERR_DATA_FIRST_SCORE=2 \
#	-DCONFIG_WL_ERR_DATA_MULTIP_SCORE=80 \
#	-DCONFIG_WL_ADD_PACKETS=false \
#	-DCONFIG_WL_ADD_PACKETS_SCORE=2 \
#	-DCONFIG_WL_RANDOMIZE_TOA=false \
#	-DCONFIG_WL_STD_TIME_ON_AIR_MS=100 \
#	mt-test-stk.c env-core.c stack-comm.c -o mt-test-stk

gcc -Wall -lpthread \
	-DCONFIG_WL_LOSE_PACKETS=true \
	-DCONFIG_WL_LOSE_PACKETS_SCORE=10 \
	-DCONFIG_WL_ERR_DATA=true \
	-DCONFIG_WL_ERR_DATA_FIRST_SCORE=2 \
	-DCONFIG_WL_ERR_DATA_MULTIP_SCORE=80 \
	-DCONFIG_WL_ADD_PACKETS=false \
	-DCONFIG_WL_ADD_PACKETS_SCORE=2 \
	-DCONFIG_WL_RANDOMIZE_TOA=false \
	-DCONFIG_WL_STD_TIME_ON_AIR_MS=100 \
	-fno-omit-frame-pointer -g -rdynamic \
	mt-test-stk-v2.c env-core.c stack-comm.c timeout-timer-internal.c cc-traceinfo.c -o mt-test-stk-v2

gcc -Wall -lpthread \
	-DCONFIG_WL_LOSE_PACKETS=true \
	-DCONFIG_WL_LOSE_PACKETS_SCORE=10 \
	-DCONFIG_WL_ERR_DATA=true \
	-DCONFIG_WL_ERR_DATA_FIRST_SCORE=2 \
	-DCONFIG_WL_ERR_DATA_MULTIP_SCORE=80 \
	-DCONFIG_WL_ADD_PACKETS=false \
	-DCONFIG_WL_ADD_PACKETS_SCORE=2 \
	-DCONFIG_WL_RANDOMIZE_TOA=false \
	-DCONFIG_WL_STD_TIME_ON_AIR_MS=100 \
	mt-test-env-core.c env-core.c stack-comm.c timeout-timer-internal.c cc-traceinfo.c -o mt-test-lose-env-core

gcc -Wall -lpthread \
	-DCONFIG_WL_LOSE_PACKETS=false \
	-DCONFIG_WL_LOSE_PACKETS_SCORE=10 \
	-DCONFIG_WL_ERR_DATA=false \
	-DCONFIG_WL_ERR_DATA_FIRST_SCORE=2 \
	-DCONFIG_WL_ERR_DATA_MULTIP_SCORE=80 \
	-DCONFIG_WL_ADD_PACKETS=false \
	-DCONFIG_WL_ADD_PACKETS_SCORE=2 \
	-DCONFIG_WL_RANDOMIZE_TOA=false \
	-DCONFIG_WL_STD_TIME_ON_AIR_MS=100 \
	st-test-env-core.c env-core.c -o st-test-env-core
