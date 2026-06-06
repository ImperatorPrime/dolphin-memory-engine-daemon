cmake_minimum_required(VERSION 3.15)
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

include(GNUInstallDirs)

set(PACKAGE_VERSION "0.0.0" CACHE STRING "Package version")

install(TARGETS dolphin_memory_engine_daemon RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES dist/dolphin-memory-engine-daemon.service DESTINATION lib/systemd/system)

set(CPACK_PACKAGE_NAME "dolphin-memory-engine-daemon")
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Dolphin Memory Engine Daemon")
set(CPACK_PACKAGE_DESCRIPTION "A daemon for running dolphin in randovania as a nintendont connection")
set(CPACK_PACKAGE_CONTACT "ndbrandes@gmail.com")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
set(CPACK_STRIP_FILES TRUE)

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcap2")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/dist/deb/postinst")

set(CPACK_RPM_PACKAGE_REQUIRES "libcap")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dist/rpm/postinst.sh")

include(CPack)

