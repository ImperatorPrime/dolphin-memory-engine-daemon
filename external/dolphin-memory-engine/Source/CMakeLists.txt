cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GCC_min_version 10)
project(dolphin-memory-engine)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



if(WIN32)
    set(DolphinProcessSrc DolphinProcess/Windows/WindowsDolphinProcess.cpp)
    set(DolphinProcessHeader DolphinProcess/Windows/WindowsDolphinProcess.h)
endif(WIN32)

if(UNIX AND NOT APPLE)
   set(DolphinProcessSrc DolphinProcess/Linux/LinuxDolphinProcess.cpp)
   set(DolphinProcessHeader DolphinProcess/Linux/LinuxDolphinProcess.h)
endif(UNIX AND NOT APPLE)

if(APPLE)
   set(DolphinProcessSrc DolphinProcess/Mac/MacDolphinProcess.cpp)
   set(DolphinProcessHeader DolphinProcess/Mac/LinuxDolphinProcess.h)
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


target_sources(dolphin-memory-engine
        PUBLIC
        FILE_SET HEADERS
        FILES
        ${DolphinProcessHeader}
        DolphinProcess/DolphinAccessor.h
        DolphinProcess/IDolphinProcess.h
        Common/PPC/PowerPCAssembler.h
        Common/PPC/PowerPCDisassembler.h
        MemoryWatch/MemWatchEntry.h
        MemoryScanner/MemoryScanner.h
)