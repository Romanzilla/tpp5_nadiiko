cmake_minimum_required(VERSION 3.20)
project(SFML_Rotation_Lab5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML COMPONENTS Graphics Window System CONFIG REQUIRED)

add_executable(RotationSim main.cpp)

target_link_libraries(RotationSim PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)

# Автоматичне копіювання папки resources у каталог збірки після компіляції
add_custom_command(TARGET RotationSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:RotationSim>/resources"
)
