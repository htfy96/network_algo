cmake_minimum_required(VERSION 2.8)
project (networkalgo)

set(SOCI_STATIC true)
set(SOCI_TEST false)
set(WITH_BOOST true)
set(WITH_ORACLE false)
set(CMAKE_INCLUDE_CURRENT_DIR true)

IF (MYSQL_INCLUDE_DIR)
    # Already in cache, be silent
    SET(MYSQL_FIND_QUIETLY TRUE)
ENDIF (MYSQL_INCLUDE_DIR)

FIND_PATH(MYSQL_INCLUDE_DIR mysql.h
    /usr/local/include/mysql
    /usr/include/mysql
    )

add_subdirectory(${networkalgo_SOURCE_DIR}/soci)

include_directories(${MYSQL_INCLUDE_DIR})
include_directories(soci/include/soci)
include_directories(soci/include)
include_directories(${networkalgo_BINARY_DIR}/soci/include)

message(STATUS "${networkalgo_SOURCE_DIR}/soci/include")
message("SDfsfa")
add_executable(test src/test.cpp)
target_link_libraries(test soci_core soci_mysql)
