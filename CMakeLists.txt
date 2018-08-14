cmake_minimum_required(VERSION 3.11)

project(LearningVulkan)

set(SOURCE_FILES
    source/Main.cpp)

# set(HEADER_FILES
#     headers/LearningVulkan/WinWindow.hpp)

add_executable(LearningVulkan ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(LearningVulkan PRIVATE headers)