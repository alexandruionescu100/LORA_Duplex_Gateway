cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

# Pull in SDK (must be before project)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

project(receive2)

#include_directories(LoRa-RP2040/Print)
include_directories("LoRa-RP2040" "ssd1306" "sd_card")
file(GLOB SOURCES "LoRa-RP2040/*.cpp" "ssd1306/*.cpp" "sd_card/*.c")

add_executable(receive2 LoRaSimpleNode.cpp ${SOURCES})
#add_executable(test2 LoRa-RP2040.cpp)
#add_library(test2 LoRa-RP2040.cpp LoRa-RP2040.h)
#add_library(test2  Print.cpp Print.h)

#target_link_libraries(test2 pico_stdlib hardware_spi hardware_interp LoRa_print)
target_link_libraries(receive2 pico_stdlib hardware_spi hardware_interp hardware_i2c hardware_clocks hardware_dma hardware_pio hardware_rtc)

 # enable usb output, disable uart output
pico_enable_stdio_usb(receive2 1)
pico_enable_stdio_uart(receive2 0)
pico_add_extra_outputs(receive2)