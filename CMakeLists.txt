cmake_minimum_required(VERSION 4.3)
project(dolphin_memory_engine_daemon)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED)

add_executable(dolphin_memory_engine_daemon
        src/daemon/main.cpp
        src/daemon/Daemon.cpp
        src/daemon/Daemon.h
        src/daemon/Session.cpp
        src/daemon/Session.h
)

target_include_directories(dolphin_memory_engine_daemon PRIVATE Boost::headers)
target_link_libraries(dolphin_memory_engine_daemon PRIVATE Boost::headers)

include(CheckCXXSourceCompiles)

# CMAKE_REQUIRED_INCLUDES is used by CheckCXXSourceCompiles
set(CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS})

message(STATUS "Checking if boost supports local sockets")

check_cxx_source_compiles("
    #include <boost/asio/detail/config.hpp>

    int main() {
        #ifndef BOOST_ASIO_HAS_LOCAL_SOCKETS
        #error \"BOOST_ASIO_HAS_LOCAL_SOCKETS not defined\"
        #endif
        return 0;
    }
" BOOST_ASIO_HAS_LOCAL_SOCKETS)

if(BOOST_ASIO_HAS_LOCAL_SOCKETS)
    message(STATUS "Checking if boost supports local sockets - success")
else()
    message(FATAL_ERROR "This projects requires boost to support local sockets")
endif()

add_subdirectory(external/dolphin-memory-engine/Source)

target_link_libraries(dolphin_memory_engine_daemon PRIVATE dolphin-memory-engine cap)

