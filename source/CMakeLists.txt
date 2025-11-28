# OpenGL with ImGui Template
# Project Name: OpenGL Template
# Version: 0.1
# Author: Mustafa Alotbah
# Description: This project sets up a basic OpenGL environment integrated with ImGui for GUI rendering.
# Terms of Use: 'Creative Commons Attribution 4.0 International (CC BY 4.0)'
# Creation Date: 08 Dec 2023
# Last Updated: 08 Dec 2023


cmake_minimum_required(VERSION 3.26)
project(opengl_template VERSION 0.1)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ASSETS_PATH "${CMAKE_SOURCE_DIR}/../assets")
add_compile_definitions(ASSETS_PATH="${ASSETS_PATH}")
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")

# Find libraries
find_package(OpenGL REQUIRED)


# Dependencies
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_DOCS OFF)
option(GLFW_INSTALL OFF)

add_subdirectory(dependency/glfw)

set(GLAD_VERSION 4.4)
set(GLAD_PROFILE CORE)

add_subdirectory(dependency/glad)
add_subdirectory(dependency/glm)


# sources and headers
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB IMGUI_SOURCES "dependency/imgui/*.cpp")
add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${IMGUI_SOURCES}
        "dependency/imgui/backends/imgui_impl_opengl3.cpp"
        "dependency/imgui/backends/imgui_impl_glfw.cpp"
)


# include
target_include_directories(${PROJECT_NAME} PRIVATE "include")
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL glfw glad glm)
target_include_directories(${PROJECT_NAME} PRIVATE "dependency/imgui")
target_include_directories(${PROJECT_NAME} PRIVATE "dependency/stb")