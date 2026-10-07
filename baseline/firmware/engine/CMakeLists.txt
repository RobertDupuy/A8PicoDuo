cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(a8_pico_cart)

set(PICO_BOARD_HEADER_DIRS ${CMAKE_SOURCE_DIR}) 
set(PICO_BOARD myboard)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(a8_pico_cart)

# 16megs of flash on purple pico clones
target_compile_definitions(a8_pico_cart PRIVATE
    PICO_FLASH_SIZE_BYTES=16777216
)

pico_set_linker_script(a8_pico_cart ${CMAKE_CURRENT_SOURCE_DIR}/memmap_preserved.ld)

target_sources(a8_pico_cart PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/block_client.c
    ${CMAKE_CURRENT_LIST_DIR}/dual_menu.c
    ${CMAKE_CURRENT_LIST_DIR}/game_virtual.c
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/atari_cart.c
    ${CMAKE_CURRENT_LIST_DIR}/msc_disk.c
    ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
    ${CMAKE_CURRENT_LIST_DIR}/fatfs_disk.c
    ${CMAKE_CURRENT_LIST_DIR}/flash_fs.c
    ${CMAKE_CURRENT_LIST_DIR}/fatfs/ff.c
    ${CMAKE_CURRENT_LIST_DIR}/fatfs/ffunicode.c
    ${CMAKE_CURRENT_LIST_DIR}/fatfs/diskio.c
) 

target_include_directories(a8_pico_cart PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/fatfs
    ${CMAKE_CURRENT_LIST_DIR}/../shared
)

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support
target_link_libraries(a8_pico_cart PUBLIC pico_stdlib hardware_flash hardware_uart tinyusb_device)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(a8_pico_cart)
pico_enable_stdio_uart(a8_pico_cart 0)
pico_enable_stdio_usb(a8_pico_cart 0)
target_link_options(a8_pico_cart PRIVATE -Wl,--wrap=f_mount,--wrap=f_open,--wrap=f_opendir,--wrap=f_readdir,--wrap=f_closedir)

set_target_properties(a8_pico_cart PROPERTIES LINKER_LANGUAGE C)
target_link_options(a8_pico_cart PRIVATE -Wl,--wrap=f_read,--wrap=f_write,--wrap=f_lseek,--wrap=f_sync,--wrap=f_close)
