cmake_minimum_required (VERSION 2.8.3)

## Project Metadata

project (zmqcpp)
set (zmqcpp_VERSION_MAJOR 1)
set (zmqcpp_VERSION_MINOR 0)
set (zmqcpp_VERSION_PATCH 2)
set (zmqcpp_VERSION "${zmqcpp_VERSION_MAJOR}.${zmqcpp_VERSION_MINOR}.${zmqcpp_VERSION_PATCH}")


## Configuration

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -O2 -Wno-long-long -pedantic")


include_directories(
    ${zmqcpp_SOURCE_DIR}/include
    ${zmqcpp_BINARY_DIR}
)

install(DIRECTORY ${zmqcpp_SOURCE_DIR}/include
    DESTINATION include
)

set(PKGCONFIG_FILE ${zmqcpp_BINARY_DIR}/zmqcpp.pc)
configure_file(zmqcpp.pc.cmake ${PKGCONFIG_FILE} @ONLY)

install(FILES ${PKGCONFIG_FILE}
    DESTINATION lib/pkgconfig
)

install(FILES include/zmqcpp.h
    DESTINATION include
)


## Testing

enable_testing()
add_subdirectory(test)


## Report

message("** Build Summary **")
message("  Version:            ${zmqcpp_VERSION}")
message("  Prefix:             ${CMAKE_INSTALL_PREFIX}")
message("  Build Type:         ${CMAKE_BUILD_TYPE}")
message("  Architecture:       ${CMAKE_SYSTEM_PROCESSOR}")
