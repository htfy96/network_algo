cmake_minimum_required(VERSION 2.8)
project (networkalgo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(USE_COTIRE_PCH ON CACHE BOOL "Use cotire to speed up compilation(Python required)")

if (USE_COTIRE_PCH)
    include(cotire)
    macro(use_pch target_)
        cotire(${target_})
    endmacro(use_pch)
else(USE_COTIRE_PCH)
    macro(use_pch target_)
    endmacro(use_pch)
endif(USE_COTIRE_PCH)

set(COMMON_MINIMUM_GCC_VERSION 5.0)
include(Compiler)

find_package(MySQL)
find_package(SQLite3 REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Protobuf REQUIRED)
find_package(snappy REQUIRED)

include_directories(${MYSQL_INCLUDE_DIR})
include_directories(${SQLITE3_INCLUDE_DIR})
include_directories(${LEVELDB_INCLUDE})
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${snappy_INCLUDE_DIRS})


set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -Wno-error -O2 -g -DMYDEBUG -Wno-maybe-uninitialized -Wno-return-type -Wno-reorder")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-error -O2 -Wno-maybe-uninitialized -Wno-return-type -Wno-reorder")

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

message(STATUS "Build flags: ${CMAKE_CXX_FLAGS}")
# configuration for soci
set(SOCI_STATIC ON CACHE BOOL "Build static")
set(SOCI_TESTS OFF CACHE BOOL "Build soci tests(would fail if you don't have all databases built" FORCE)

set(SOCI_CXX_C11 ON)
set(WITH_THREADS ON)

set(WITH_TESTS ON CACHE BOOL "Build tests of networkalgo(recommented)")

set(CMAKE_INCLUDE_CURRENT_DIR true)


add_subdirectory(${networkalgo_SOURCE_DIR}/soci)
add_subdirectory(${networkalgo_SOURCE_DIR}/googletest)

include_directories(soci/include/soci)
include_directories(soci/include)
include_directories(include/)
include_directories(src/)
include_directories(${networkalgo_BINARY_DIR}/soci/include)
include_directories(NanoSQL)
include_directories(cereal/include)
include_directories(spdlog/include)

set(DEBUG_SRC src/debug.cpp)

if (${WITH_TESTS})

    include_directories(googletest/googletest/include)
    set(GTEST_SRC googletest/googletest/src/gtest_main.cc)
    set(GTEST_LIB gtest gtest_main)

    add_executable(graphtest test/graphtest.cpp ${GTEST_SRC} ${DEBUG_SRC} include/graph.hpp)
    target_link_libraries(graphtest ${GTEST_LIB} soci_core soci_mysql)
    use_pch(graphtest)

    #add_executable(lazycachetest test/lazycachetest.cpp ${GTEST_SRC})
    #target_link_libraries(lazycachetest ${GTEST_LIB} soci_core soci_sqlite3)

    add_executable(graph_interfacetest test/graph_interfacetest.cpp ${GTEST_SRC} ${DEBUG_SRC} include/graph_interface.hpp)
    target_link_libraries(graph_interfacetest ${GTEST_LIB})
    use_pch(graph_interfacetest)

    protobuf_generate_cpp(LDGTESTPROTO_SRCS LDGTESTPROTO_HDRS test/leveldbgraphtest.proto)
    add_executable(leveldbgraphtest test/leveldbgraphtest.cpp src/graphdsl.cpp ${GTEST_SRC} ${LDGTESTPROTO_SRCS} ${LDGTESTPROTO_HDRS} ${DEBUG_SRC} include/backend/leveldbgraph.hpp include/graphdsl.hpp)
    target_link_libraries(leveldbgraphtest ${PROTOBUF_LIBRARIES} ${GTEST_LIB} ${LEVELDB_LIBS} ${snappy_LIBRARIES})
    use_pch(leveldbgraphtest)

    protobuf_generate_cpp(REFLECTIONTESTPROTO_SRCS REFLECTIONTESTPROTO_HDRS test/reflectiontest.proto)
    add_executable(reflectiontest test/reflectiontest.cpp ${GTEST_SRC} ${REFLECTIONTESTPROTO_SRCS} ${REFLECTIONTESTPROTO_HDRS} ${DEBUG_SRC} include/reflection.hpp include/debug.hpp include/graphdsl.hpp)
    target_link_libraries(reflectiontest ${PROTOBUF_LIBRARIES} ${GTEST_LIB})
    use_pch(reflectiontest)

    protobuf_generate_cpp(TFLABELPROTO_SRCS TFLABELPROTO_HDRS include/algorithm/tflabel.proto)
    add_executable(tflabeltest test/tflabeltest.cpp src/graphdsl.cpp ${GTEST_SRC} ${LDGTESTPROTO_SRCS} ${TFLABELPROTO_SRCS} ${TFLABELPROTO_HDRS} ${DEBUG_SRC} include/backend/leveldbgraph.hpp include/algorithm/tflabel.hpp)
    target_link_libraries(tflabeltest ${PROTOBUF_LIBRARIES} ${GTEST_LIB} ${LEVELDB_LIBS} ${snappy_LIBRARIES})
    use_pch(tflabeltest)
    file(COPY test/data.txt DESTINATION ${networkalgo_BINARY_DIR})

    add_executable(typedmrumaptest test/typedmrumaptest.cpp ${GTEST_SRC} ${DEBUG_SRC} include/typedmrumap.hpp)
    target_link_libraries(typedmrumaptest ${GTEST_LIB})
    use_pch(typedmrumaptest)

    add_executable(disksettest test/disksettest.cpp ${GTEST_SRC} ${DEBUG_SRC})
    target_link_libraries(disksettest ${GTEST_LIB} ${LEVELDB_LIBS} ${snappy_LIBRARIES} ${PROTOBUF_LIBRARIES} include/diskset.hpp)
    use_pch(disksettest)

    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
        add_executable(graphdsltest test/graphdsltest.cpp ${GTEST_SRC} ${DEBUG_SRC} include/graphdsl.hpp)
        target_link_libraries(graphdsltest ${GTEST_LIB})
        use_pch(graphdsltest)
    endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")

# ----------------
    enable_testing()
    add_test(NAME graphtest COMMAND graphtest)
    #add_test(NAME lazycachetest COMMAND lazycachetest)
    add_test(NAME graph_interfacetest COMMAND graph_interfacetest)
    add_test(NAME leveldbgraphtest COMMAND leveldbgraphtest)
    add_test(NAME reflectiontest COMMAND reflectiontest)
    add_test(NAME typedmrumaptest COMMAND typedmrumaptest)
    add_test(NAME disksettest COMMAND disksettest)
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
        add_test(NAME graphdsltest COMMAND graphdsltest)
    endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")

endif(${WITH_TESTS})
