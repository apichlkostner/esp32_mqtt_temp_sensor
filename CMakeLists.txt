cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(temp_sens)
target_add_binary_data(temp_sens.elf "main/mqtt_self_ca.pem" TEXT)
