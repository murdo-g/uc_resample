cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)

project(uc_resample C)

include_directories(${PROJECT_SOURCE_DIR}/src/)

add_executable(sine_test src/sine_test.c)


# Add the standard library to the build
# target_link_libraries(SamplePlayer pico_stdlib pico_audio_i2s pico_sd_card)

# Add any user requested libraries
# target_link_libraries(SamplePlayer
#         hardware_gpio
#         hardware_spi
#         hardware_irq
#         )
