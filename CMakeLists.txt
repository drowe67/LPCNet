# TODO: 1/ Will this work when cross compiling for Windows?  Another approach is to supply
#          flags manually on cmd line
#       2/ Should we standardise on just AVX?  As machine we run on
#          may be different to machine we build on
cmake_minimum_required(VERSION 3.0)
project(LPCNet C)

set(CMAKE_C_FLAGS "-Wall -W -Wextra -Wno-unused-function -O3 -g -I. -MD ${CMAKE_C_FLAGS}")

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
set(LPCNET_ROOT http://rowetel.com/downloads/deep/)
set(LPCNET_FILE lpcnet_190215.tgz)
set(LPCNET_URL ${LPCNET_ROOT}${LPCNET_FILE})

# Work around not having the FetchContent module.
if(CMAKE_VERSION VERSION_LESS 3.11.4)
    set(lpcnet_SOURCE_DIR ${CMAKE_BINARY_DIR}/src)
    if(NOT EXISTS ${lpcnet_SOURCE_DIR})
        file(DOWNLOAD ${LPCNET_URL}
            ${CMAKE_BINARY_DIR}/${LPCNET_FILE}
            SHOW_PROGRESS
        )
        file(MAKE_DIRECTORY ${lpcnet_SOURCE_DIR})
        execute_process(COMMAND tar -xzf ${CMAKE_BINARY_DIR}/${LPCNET_FILE} -C ${CMAKE_BINARY_DIR}/src)
    endif()
else()
    include(FetchContent)
    FetchContent_Declare(
        lpcnet
        URL ${LPCNET_URL})
    FetchContent_GetProperties(lpcnet)
    if(NOT lpcnet_POPULATED)
        FetchContent_Populate(lpcnet)
    endif()
endif()

add_subdirectory(src)

