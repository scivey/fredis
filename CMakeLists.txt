cmake_minimum_required(VERSION 2.8)

project(fredis)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 -Wall -O0 -g")

set(EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(GTEST ${EXTERNAL}/googletest)

include_directories(
    ${GTEST}/googletest/include
    ${GTEST}/googlemock/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/local/include
)

link_directories(
    ${GTEST}/build/googlemock
    /usr/local/lib
)

set(COMMON_LIBS
    folly
    hiredis
    memcached
    event
    glog
    pthread
    atomic
)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)
FILE(GLOB FREDIS_CORE_SRC
    ${SRC_ROOT}/fredis/redis/**/*.cpp
    ${SRC_ROOT}/fredis/redis/*.cpp
    ${SRC_ROOT}/fredis/memcached/*.cpp
    ${SRC_ROOT}/fredis/memcached/**/*.cpp
    ${SRC_ROOT}/fredis/FredisError.cpp

)

add_library(fredis
    ${FREDIS_CORE_SRC}
)
target_link_libraries(fredis ${COMMON_LIBS})

add_executable(runner
    ${SRC_ROOT}/main.cpp
)

add_dependencies(runner fredis)
target_link_libraries(runner fredis ${COMMON_LIBS})


FILE(GLOB INTEGRATION_SRC
    ${SRC_ROOT}/fredis/integration_tests/*.cpp
)
add_executable(integration_runner
    ${INTEGRATION_SRC}
    ${SRC_ROOT}/run_tests.cpp
)
add_dependencies(integration_runner fredis)

target_link_libraries(integration_runner
    fredis
    gmock
    ${COMMON_LIBS}
)
