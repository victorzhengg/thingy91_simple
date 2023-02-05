#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(udp)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/ui_rgb_control.c)
target_sources(app PRIVATE src/ui_buzzer_control.c)
target_sources(app PRIVATE src/user_shell_cmd.c)
# NORDIC SDK APP END

zephyr_include_directories(src)

zephyr_library_include_directories(
	src/ui
	src
	)

add_subdirectory(src/ui)