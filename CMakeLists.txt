cmake_minimum_required(VERSION 3.12)
project(logi_pop_emoji_button C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} udev)
