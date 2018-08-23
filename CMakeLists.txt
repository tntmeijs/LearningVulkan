cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(LearningVulkan)

set(SOURCE_FILES
    source/Main.cpp
    source/Utility.cpp
    source/Renderer.cpp)

set(HEADER_FILES
    headers/LearningVulkan/Utility.hpp
    headers/LearningVulkan/Renderer.hpp)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-std=c++11)

find_package(Vulkan REQUIRED)

add_executable(LearningVulkan WIN32 ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(LearningVulkan PRIVATE headers)

target_link_libraries(LearningVulkan Vulkan::Vulkan)