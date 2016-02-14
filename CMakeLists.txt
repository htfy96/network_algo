cmake_minimum_required(VERSION 2.8)
project (networkalgo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(COMMON_MINIMUM_GCC_VERSION 5.0)
include(Compiler)

find_package(MySQL)
find_package(SQLite3 REQUIRED)
find_package(LevelDB REQUIRED)

include_directories(${MYSQL_INCLUDE_DIR})
include_directories(${SQLITE3_INCLUDE_DIR})
include_directories(${LEVELDB_INCLUDE})


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

set(SOCI_CXX_C11 ON)
set(WITH_THREADS ON)



set(WITH_TESTS ON CACHE BOOL "Build tests of networkalgo(recommented)")

set(CMAKE_INCLUDE_CURRENT_DIR true)

set(CMAKE_CXX_FLAGS_DEBUG "-DMYDEBUG")
add_subdirectory(${networkalgo_SOURCE_DIR}/soci)
add_subdirectory(${networkalgo_SOURCE_DIR}/googletest)

include_directories(soci/include/soci)
include_directories(soci/include)
include_directories(include/)
include_directories(src/)
include_directories(${networkalgo_BINARY_DIR}/soci/include)
include_directories(NanoSQL)
include_directories(cereal/include)

if (${WITH_TESTS})

    include_directories(googletest/googletest/include)
    set(GTEST_SRC googletest/googletest/src/gtest_main.cc)
    set(GTEST_LIB gtest gtest_main)

    add_executable(graphtest test/graphtest.cpp ${GTEST_SRC})
    target_link_libraries(graphtest ${GTEST_LIB} soci_core soci_mysql)

    add_executable(lazycachetest test/lazycachetest.cpp ${GTEST_SRC})
    target_link_libraries(lazycachetest ${GTEST_LIB} soci_core soci_sqlite3)

    add_executable(graphdsltest test/graphdsltest.cpp ${GTEST_SRC})
    target_link_libraries(graphdsltest ${GTEST_LIB})

# ----------------
    enable_testing()
    add_test(NAME graphtest COMMAND graphtest)
    add_test(NAME lazycachetest COMMAND lazycachetest)
    add_test(NAME graphdsltest COMMAND graphdsltest)
endif(${WITH_TESTS})
