set(CTEST_OUTPUT_ON_FAILURE TRUE)
INCLUDE(External.cmake)

INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR})
LINK_DIRECTORIES(${GTEST_LIB_DIR} ${GMOCK_LIB_DIR})
file(GLOB test_sources *.cc)
add_executable(test_zmqcpp ${test_sources})

ADD_DEPENDENCIES(test_zmqcpp googletest googlemock)
target_link_libraries(test_zmqcpp zmq gtest gmock)
add_test(testzmq test_zmqcpp --gtest_output=xml)
