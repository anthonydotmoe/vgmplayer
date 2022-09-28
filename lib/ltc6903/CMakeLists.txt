add_library(ltc6903 INTERFACE)
target_sources(ltc6903 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ltc6903.c
)
target_include_directories(ltc6903 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(ltc6903 INTERFACE
    hardware_spi
    hardware_gpio
    pico_stdlib
)