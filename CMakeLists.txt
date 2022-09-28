set(PROJECT_NAME vgmplayer)
cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(${PROJECT_NAME} C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME}
	main.c
	vgmcb.c
	ltc6903.c
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/ym2151.pio)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/TinyVGM)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/no-OS-FatFS/FatFs_SPI)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
	pico_stdlib
	TinyVGM
	FatFs_SPI
)