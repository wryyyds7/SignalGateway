cmake_minimum_required(VERSION 3.16)
project(SignalGateway VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()
include_directories(${CMAKE_SOURCE_DIR}/src)

add_library(common INTERFACE)
if(UNIX)
    target_link_libraries(common INTERFACE pthread)
endif()

file(GLOB CAN_SOURCES CONFIGURE_DEPENDS "src/can/*.cpp" "src/can/*.h")
add_library(can_layer STATIC ${CAN_SOURCES})
target_link_libraries(can_layer PUBLIC common)

file(GLOB ROUTING_SOURCES CONFIGURE_DEPENDS "src/routing/*.cpp" "src/routing/*.h")
add_library(routing STATIC ${ROUTING_SOURCES})
target_link_libraries(routing PUBLIC common)

file(GLOB SERVICE_SOURCES CONFIGURE_DEPENDS "src/service/*.cpp" "src/service/*.h")
add_library(service STATIC ${SERVICE_SOURCES})
target_link_libraries(service PUBLIC common routing)

file(GLOB RECORDER_SOURCES CONFIGURE_DEPENDS "src/recorder/*.cpp" "src/recorder/*.h")
add_library(recorder STATIC ${RECORDER_SOURCES})
target_link_libraries(recorder PUBLIC common routing service)

add_executable(signal_gateway src/main.cpp)
target_link_libraries(signal_gateway PRIVATE common can_layer routing service recorder)

add_executable(signal_replay src/replay_main.cpp)
target_link_libraries(signal_replay PRIVATE common recorder routing service)

enable_testing()
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
    foreach(test_src ${TEST_SOURCES})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_link_libraries(${test_name} PRIVATE
            common can_layer routing service recorder
            GTest::gtest GTest::gtest_main)
        target_compile_definitions(${test_name} PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests")
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

install(TARGETS signal_gateway signal_replay RUNTIME DESTINATION bin)
