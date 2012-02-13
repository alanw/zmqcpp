set(CTEST_OUTPUT_ON_FAILURE TRUE)

find_package (GTest REQUIRED)

# find Gmock library
find_library(GMOCK_LIBRARIES gmock
    HINTS
    $ENV{GMOCK_ROOT}
    ${GMOCK_ROOT}
    $ENV{GMOCK_ROOT}/make
    ${GMOCK_ROOT}/make
)

find_path(GMOCK_INCLUDE_DIR gmock/gmock.h
    HINTS
    $ENV{GMOCK_ROOT}/include
    ${GMOCK_ROOT}/include
)

if(NOT DEFINED GMOCK_INCLUDE_DIR OR NOT DEFINED GMOCK_LIBRARIES)
    message(SEND_ERROR "gmock not found")
endif()

include_directories(
    ${GTEST_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
)

file(GLOB test_sources *.cc)
add_executable(test_zmqcpp ${test_sources})

target_link_libraries(test_zmqcpp
    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_BOTH_LIBRARIES}
    zmq3
)
gtest_add_tests(test_zmqcpp "--gtest_output=xml" ${test_sources})
