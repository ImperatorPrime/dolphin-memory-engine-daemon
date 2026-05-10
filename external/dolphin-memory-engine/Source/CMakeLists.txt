cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GCC_min_version 10)
project(dolphin-memory-engine)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



if(WIN32)
    set(DolphinProcessSrc DolphinProcess/Windows/WindowsDolphinProcess.cpp)
endif(WIN32)

if(UNIX AND NOT APPLE)
   set(DolphinProcessSrc DolphinProcess/Linux/LinuxDolphinProcess.cpp)
endif(UNIX AND NOT APPLE)

if(APPLE)
   set(DolphinProcessSrc DolphinProcess/Mac/MacDolphinProcess.cpp)
endif(APPLE)

set(SRCS ${DolphinProcessSrc}
         DolphinProcess/DolphinAccessor.cpp
         Common/MemoryCommon.cpp
         Common/PPC/PowerPCAssembler.cpp
         Common/PPC/PowerPCDisassembler.cpp
         MemoryWatch/MemWatchEntry.cpp
        MemoryScanner/MemoryScanner.cpp)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(dolphin-memory-engine ${SRCS})