# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esl_peripheral_sync)

target_sources(app PRIVATE 
	src/peripheral_sync.c
)

target_sources_ifdef(CONFIG_PAWR_EPD app PRIVATE 
					 src/display_manager.c
					 src/state_manager.c
					 src/ui_manager.c
					 src/routes/boot.c
					 src/routes/config.c
					 src/routes/nametag.c
					 src/images/nordic.c
					 src/images/boston.c
					 src/images/philadelphia.c
					 src/images/spartanburg.c
					 src/images/trondheim.c
					 src/images/sanjose.c
)

target_sources_ifdef(CONFIG_SENSOR app PRIVATE src/environmental_sensor.c)
target_sources_ifdef(CONFIG_INPUT app PRIVATE src/input_manager.c)

target_include_directories(app PRIVATE
	include
	${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)
