cmake_minimum_required(VERSION 3.5)
project(glad)

# Usage:
# set(GLAD_VERSION 4.4)
# set(GLAD_PROFILE CORE)


if (NOT GLAD_VERSION)
    message(ERROR "No GLAD_VERSION set")
endif ()
if (NOT GLAD_PROFILE)
    message(ERROR "No GLAD_PROFILE set")
endif ()

set(GLAD_PATH "${GLAD_PROFILE}-${GLAD_VERSION}")
string(TOLOWER ${GLAD_PATH} GLAD_PATH)

add_library(glad STATIC
        ${GLAD_PATH}/src/glad.c
        ${GLAD_PATH}/include/glad/glad.h
        ${GLAD_PATH}/include/KHR/khrplatform.h
)
target_include_directories(glad PUBLIC ${GLAD_PATH}/include)
