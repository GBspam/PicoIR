cmake_minimum_required(VERSION 3.16)
if (NOT DEFINED PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()
if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "Set PICO_SDK_PATH to a Raspberry Pi Pico SDK checkout")
endif()
include(\${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(picoir_pico_backend C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
pico_sdk_init()

add_library(picoir_pico_backend STATIC src/picoir_pico.c)
target_include_directories(picoir_pico_backend PUBLIC
    \${CMAKE_CURRENT_LIST_DIR}/include
    \${CMAKE_CURRENT_LIST_DIR}/../../include
)
target_link_libraries(picoir_pico_backend PUBLIC
    pico_stdlib hardware_clocks hardware_dma hardware_pio hardware_sync
)
pico_generate_pio_header(picoir_pico_backend \${CMAKE_CURRENT_LIST_DIR}/pio/ir_rx_sampler.pio)
pico_generate_pio_header(picoir_pico_backend \${CMAKE_CURRENT_LIST_DIR}/pio/ir_tx_carrier.pio)

add_library(picoir_core STATIC
    \${CMAKE_CURRENT_LIST_DIR}/../../src/capture_frame.c
    \${CMAKE_CURRENT_LIST_DIR}/../../src/console.c
    \${CMAKE_CURRENT_LIST_DIR}/../../src/gameboy_ir.c
    \${CMAKE_CURRENT_LIST_DIR}/../../src/gameboy_link.c
    \${CMAKE_CURRENT_LIST_DIR}/../../src/ir_protocol.c
    \${CMAKE_CURRENT_LIST_DIR}/../../src/ir_timing.c
    \${CMAKE_CURRENT_LIST_DIR}/../../src/remote_db.c
    \${CMAKE_CURRENT_LIST_DIR}/../../src/signal_analysis.c
)
target_include_directories(picoir_core PUBLIC \${CMAKE_CURRENT_LIST_DIR}/../../include)

option(PICOIR_BUILD_FIRMWARE "Build the TinyUSB dual-CDC application" ON)
if (PICOIR_BUILD_FIRMWARE)
    add_executable(picoir_firmware src/main.c src/usb_descriptors.c)
    target_include_directories(picoir_firmware PRIVATE \${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(picoir_firmware PRIVATE
        picoir_pico_backend picoir_core tinyusb_device tinyusb_board pico_stdlib hardware_gpio
    )
    pico_add_extra_outputs(picoir_firmware)
endif()

option(PICOIR_BUILD_HARDWARE_TESTS "Build RP2040-only stream stress test firmware" OFF)
if (PICOIR_BUILD_HARDWARE_TESTS)
    add_executable(picoir_rx_stream_stress tests/pico_rx_stream_stress.c)
    target_link_libraries(picoir_rx_stream_stress PRIVATE
        picoir_pico_backend pico_stdlib hardware_gpio hardware_sync
    )
    pico_enable_stdio_usb(picoir_rx_stream_stress 1)
    pico_enable_stdio_uart(picoir_rx_stream_stress 0)
    pico_add_extra_outputs(picoir_rx_stream_stress)
endif()
