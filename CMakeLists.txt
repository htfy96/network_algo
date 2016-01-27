cmake_minimum_required(VERSION 2.8)
project (networkalgo)

set(SOCI_STATIC ON)
set(SOCI_TESTS OFF)
set(WITH_BOOST ON)


set(WITH_ORACLE OFF)
set(WITH_SQLITE3 OFF)
set(WITH_POSTGRESQL OFF)
set(WITH_ODBC OFF)
set(WITH_FIREBIRD OFF)
set(WITH_MYSQL ON)
set(WITH_DB2 OFF)


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
add_subdirectory(${networkalgo_SOURCE_DIR}/googletest)

include_directories(${MYSQL_INCLUDE_DIR})
include_directories(soci/include/soci)
include_directories(soci/include)
include_directories(include/)
include_directories(${networkalgo_BINARY_DIR}/soci/include)
include_directories(googletest/googletest/include)

add_executable(test src/test.cpp)
target_link_libraries(test soci_core soci_mysql)

add_executable(testtest test/testtest.cpp googletest/googletest/src/gtest_main.cc)
target_link_libraries(testtest gtest gtest_main)

# ----------------
enable_testing()
add_test(NAME testtest COMMAND testtest)
