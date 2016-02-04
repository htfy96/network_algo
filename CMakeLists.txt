cmake_minimum_required(VERSION 2.8)
project (networkalgo)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# configuration for soci
set(SOCI_STATIC ON CACHE BOOL "Build static")
set(SOCI_TESTS OFF CACHE BOOL "Build soci tests(would fail if you don't have all databases built" FORCE)
set(WITH_BOOST ON CACHE BOOL "Build with boost")

#set(SOCI_CXX_C11 )
set(WITH_THREADS ON)

# Enable database(s)
#set(WITH_ORACLE OFF CACHE BOOL "Build with oracle database")
#set(WITH_SQLITE3 ON CACHE BOOL "Build with sqlite3 database")
#set(WITH_POSTGRESQL OFF CACHE BOOL "Build with postgresql database")
#set(WITH_ODBC OFF CACHE BOOL "Build with odbc database")
#set(WITH_FIREBIRD OFF CACHE BOOL "Build with firebird database")
#set(WITH_MYSQL ON CACHE BOOL "Build with mysql database")
#set(WITH_DB2 OFF CACHE BOOL "Build with db2 database")


set(WITH_TESTS ON CACHE BOOL "Build tests of networkalgo(recommented)")

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
include_directories(NanoSQL)

if (${WITH_TESTS})

    include_directories(googletest/googletest/include)

    add_executable(graphtest test/graphtest.cpp googletest/googletest/src/gtest_main.cc)
    target_link_libraries(graphtest gtest gtest_main soci_core soci_mysql)

# ----------------
    enable_testing()
    add_test(NAME graphtest COMMAND graphtest)
endif(${WITH_TESTS})
