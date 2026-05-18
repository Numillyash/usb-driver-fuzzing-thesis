cmake_minimum_required(VERSION 3.13...3.27)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH is not set")
endif()

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(portable_demo C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(NOT PICO_BOARD)
    set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Target board")
endif()

pico_sdk_init()

add_executable(portable_demo
    src/crc8.c
    src/main.c
    src/nrf24_radio.c
    src/scheduler.c
    src/watchdog_guard.c
    src/mode_manager.c
    src/status.c
    src/command_dispatch.c
)

target_include_directories(portable_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(portable_demo
    pico_stdlib
    hardware_gpio
    hardware_spi
)

pico_enable_stdio_usb(portable_demo 1)
pico_enable_stdio_uart(portable_demo 0)

pico_add_extra_outputs(portable_demo)

add_executable(usb_case_demo
    src/usb_case_main.c
    src/nrf24_radio.c
    src/usb_case_descriptor_layer.c
)

target_include_directories(usb_case_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(usb_case_demo
    pico_stdlib
    hardware_gpio
    hardware_spi
)

pico_enable_stdio_usb(usb_case_demo 1)
pico_enable_stdio_uart(usb_case_demo 0)

pico_add_extra_outputs(usb_case_demo)

add_executable(usb_case_custom_demo
    src/usb_case_main.c
    src/nrf24_radio.c
    src/usb_case_descriptor_layer.c
)

target_include_directories(usb_case_custom_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(usb_case_custom_demo
    pico_stdlib
    hardware_gpio
    hardware_spi
    tinyusb_device
    tinyusb_board
)

target_compile_definitions(usb_case_custom_demo PRIVATE
    CFG_TUD_CDC=1
    CFG_TUD_HID=1
    CFG_TUD_HID_EP_BUFSIZE=16
    USB_CASE_ENABLE_TINYUSB_CUSTOM_DESCRIPTORS=1
    USB_CASE_MANUAL_TINYUSB_INIT=1
)

pico_enable_stdio_usb(usb_case_custom_demo 0)
pico_enable_stdio_uart(usb_case_custom_demo 0)

pico_add_extra_outputs(usb_case_custom_demo)

add_executable(usb_case_msc_demo
    src/usb_case_main.c
    src/nrf24_radio.c
    src/usb_case_descriptor_layer.c
)

target_include_directories(usb_case_msc_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(usb_case_msc_demo
    pico_stdlib
    hardware_gpio
    hardware_spi
    tinyusb_device
    tinyusb_board
)

target_compile_definitions(usb_case_msc_demo PRIVATE
    CFG_TUD_CDC=0
    CFG_TUD_HID=0
    CFG_TUD_MSC=1
    CFG_TUD_MSC_EP_BUFSIZE=64
    USB_CASE_ENABLE_TINYUSB_CUSTOM_DESCRIPTORS=1
    USB_CASE_MANUAL_TINYUSB_INIT=1
)

pico_enable_stdio_usb(usb_case_msc_demo 0)
pico_enable_stdio_uart(usb_case_msc_demo 0)

pico_add_extra_outputs(usb_case_msc_demo)

add_executable(usb_case_composite_msc_demo
    src/usb_case_main.c
    src/nrf24_radio.c
    src/usb_case_descriptor_layer.c
)

target_include_directories(usb_case_composite_msc_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(usb_case_composite_msc_demo
    pico_stdlib
    hardware_gpio
    hardware_spi
    tinyusb_device
    tinyusb_board
)

target_compile_definitions(usb_case_composite_msc_demo PRIVATE
    CFG_TUD_CDC=1
    CFG_TUD_HID=0
    CFG_TUD_MSC=1
    CFG_TUD_MSC_EP_BUFSIZE=64
    USB_CASE_ENABLE_TINYUSB_CUSTOM_DESCRIPTORS=1
    USB_CASE_MANUAL_TINYUSB_INIT=1
)

pico_enable_stdio_usb(usb_case_composite_msc_demo 0)
pico_enable_stdio_uart(usb_case_composite_msc_demo 0)

pico_add_extra_outputs(usb_case_composite_msc_demo)

add_executable(usb_case_composite_hid_msc_demo
    src/usb_case_main.c
    src/nrf24_radio.c
    src/usb_case_descriptor_layer.c
)

target_include_directories(usb_case_composite_hid_msc_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(usb_case_composite_hid_msc_demo
    pico_stdlib
    hardware_gpio
    hardware_spi
    tinyusb_device
    tinyusb_board
)

target_compile_definitions(usb_case_composite_hid_msc_demo PRIVATE
    CFG_TUD_CDC=0
    CFG_TUD_HID=1
    CFG_TUD_MSC=1
    CFG_TUD_HID_EP_BUFSIZE=16
    CFG_TUD_MSC_EP_BUFSIZE=64
    USB_CASE_ENABLE_TINYUSB_CUSTOM_DESCRIPTORS=1
    USB_CASE_MANUAL_TINYUSB_INIT=1
)

pico_enable_stdio_usb(usb_case_composite_hid_msc_demo 0)
pico_enable_stdio_uart(usb_case_composite_hid_msc_demo 0)

pico_add_extra_outputs(usb_case_composite_hid_msc_demo)
