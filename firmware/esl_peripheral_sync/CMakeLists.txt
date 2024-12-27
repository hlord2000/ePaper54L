# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esl_peripheral_sync)

target_sources(app PRIVATE 
	src/peripheral_sync.c
)

target_sources_ifdef(CONFIG_PAWR_EPD app PRIVATE 
					 src/display.c
					 src/images/boston.c
)

target_sources_ifdef(CONFIG_SENSOR app PRIVATE src/temp_humidity_sensor.c)
target_sources_ifdef(CONFIG_INPUT app PRIVATE src/buttons.c)

target_include_directories(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)
