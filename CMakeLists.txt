# Minimum required version
cmake_minimum_required (VERSION 3.12)

# Project definition
project(main LANGUAGES CXX)

# Set variables before including
set(ENABLE_ALL_WARNINGS "off")
set(ENABLE_EFFECTIVE_CXX "off")

# Set llvm paths
set(LLVM_COV_PATH "/usr/bin/llvm-cov-12")
set(LLVM_PROFDATA_PATH "/usr/bin/llvm-profdata-12")

# Include various cmake stuff for c++
include(cmake/compiler-options.cmake)
include(cmake/c++-standards.cmake)
include(cmake/code-coverage.cmake)
include(cmake/sanitizers.cmake)
include(cmake/tools.cmake)

# Require c++17
cxx_20()

message(STATUS "Using ${CMAKE_CXX_COMPILER} (compiler id: ${CMAKE_CXX_COMPILER_ID})")

# Variables
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -march=native -fcoroutines-ts")
endif()

# TODO: Optimization flags for the compiler
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")

if(DEFINED ENV{Boost_DIR})
	set(BOOST_ROOT $ENV{Boost_DIR})
else()
	set(BOOST_ROOT "/home/matthijs/boost/boost_1_77_0")
endif()

# Find required libraries
find_package(Boost 1.77.0 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Add compile definitons
add_compile_definitions(BOOST_ALL_DYN_LINK)
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)
add_compile_definitions(FMT_HEADER_ONLY)

# Include directories
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# Executable
add_executable(main
	websocket_client_sync_ssl.cpp)

# Link
target_link_libraries(main PUBLIC ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES})
