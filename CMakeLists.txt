# TODO: 1/ Will this work when cross compiling for Windows?  Another approach is to supply
#          flags manually on cmd line
#       2/ Should we standardise on just AVX?  As machine we run on
#          may be different to machine we build on
cmake_minimum_required(VERSION 3.0)
project(LPCNet C)

set(CMAKE_C_FLAGS "-Wall -W -Wextra -Wno-unused-function -O3 -g -I../include -MD ${CMAKE_C_FLAGS}")

execute_process(COMMAND grep -c "avx2" /proc/cpuinfo
    OUTPUT_VARIABLE AVX2)
execute_process(COMMAND grep -c "avx " /proc/cpuinfo
    OUTPUT_VARIABLE AVX)
execute_process(COMMAND grep -c "neon" /proc/cpuinfo
    OUTPUT_VARIABLE NEON)
message("AVX2: ${AVX2} AVX: ${AVX} NEON: ${NEON}")

if(${AVX2} GREATER 0)
    message(STATUS "avx2 processor flags found.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2 -mfma")
elseif(${AVX} GREATER 0)
# AVX2 machines will also match on AVX
    message(STATUS "avx processor flags found.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx")
endif()

# RPi
if(${NEON} GREATER 0)
    message(STATUS "neon processor flags found.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon -march=armv8-a -mtune=cortex-a53")
endif()

# grab latest NN model (or substitute your own)
include(FetchContent)
FetchContent_Declare(
    lpcnet
    URL http://rowetel.com/downloads/deep/lpcnet_190215.tgz)
FetchContent_GetProperties(lpcnet)
if(NOT lpcnet_POPULATED)
    FetchContent_Populate(lpcnet)
endif()

add_subdirectory(src)
