# TODO: 1/ Will this work when cross compiling for Windows?  Another approach is to supply
#          flags manually on cmd line
#       2/ Should we standardise on just AVX?  As machine we run on
#          may be different to machine we build on
cmake_minimum_required(VERSION 3.0)
project(LPCNet C)

option(DISABLE_CPU_OPTIMIZATION "Disable CPU optimization discovery." OFF)
option(AVX2 "Enable AVX2 CPU optimizations." OFF)
option(AVX "Enable AVX CPU optimizations." OFF)
option(NEON "Enable NEON CPU optimizations for RPi." OFF)

include(GNUInstallDirs)
mark_as_advanced(CLEAR
    CMAKE_INSTALL_BINDIR
    CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR
)

#
# Prevent in-source builds
# If an in-source build is attempted, you will still need to clean up a few
# files manually.
#
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds in ${CMAKE_BINARY_DIR} are not "
   "allowed, please remove ./CMakeCache.txt and ./CMakeFiles/, create a "
   "separate build directory and run cmake from there.")
endif("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")


# Set project version information. This should probably be done via external
# file at some point.
#
set(LPCNET_VERSION_MAJOR 0)
set(LPCNET_VERSION_MINOR 1)
# Set to patch level if needed, otherwise leave FALSE.
# Must be positive (non-zero) if set, since 0 == FALSE in CMake.
set(LPCNET_VERSION_PATCH FALSE)
set(LPCNET_VERSION "${LPCNET_VERSION_MAJOR}.${LPCNET_VERSION_MINOR}")
# Patch level version bumps should not change API/ABI.
set(SOVERSION "${LPCNET_VERSION_MAJOR}.${LPCNET_VERSION_MINOR}")
if(LPCNET_VERSION_PATCH)
    set(LPCNET_VERSION "${LPCNET_VERSION}.${LPCNET_VERSION_PATCH}")
endif()
message(STATUS "LPCNet version: ${LPCNET_VERSION}")

# Set default flags
set(CMAKE_C_FLAGS "-Wall -W -Wextra -Wno-unused-function -O3 -g -I. -MD ${CMAKE_C_FLAGS} -DENABLE_ASSERTIONS")

# Detection of available CPU optimizations
if(NOT DISABLE_CPU_OPTIMIZATION)
    if(UNIX AND NOT APPLE)
        message(STATUS "Looking for available CPU optimizations on Linux/BSD system...")
        execute_process(COMMAND grep -c "avx2" /proc/cpuinfo
            OUTPUT_VARIABLE AVX2)
        execute_process(COMMAND grep -c "avx " /proc/cpuinfo
            OUTPUT_VARIABLE AVX)
        execute_process(COMMAND grep -c "neon" /proc/cpuinfo
            OUTPUT_VARIABLE NEON)
    elseif(APPLE)
        # Under OSX we need to look through a few sysctl entries to determine what our CPU supports.
        message(STATUS "Looking for available CPU optimizations on an OSX system...")
        execute_process(COMMAND sysctl -a COMMAND grep machdep.cpu.leaf7_features COMMAND grep -c AVX2
            OUTPUT_VARIABLE AVX2)
        execute_process(COMMAND sysctl -a COMMAND grep machdep.cpu.features COMMAND grep -c AVX
            OUTPUT_VARIABLE AVX)
    elseif(WIN32)
        message(STATUS "No detection capability on Windows, assuming AVX is available.")
        set(AVX TRUE)
    else()
        message(STATUS "System is not *nix, processor specific optimizations cannot be determined.")
        message("   You can try setting them manually, e.g.: -DAVX2=1 or -DAVX=1 or -DNEON=1")
    endif()
endif()

if(${AVX2} OR ${AVX2} GREATER 0)
    message(STATUS "avx2 processor flags found or enabled.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2 -mfma")
elseif(${AVX} OR ${AVX} GREATER 0)
# AVX2 machines will also match on AVX
    message(STATUS "avx processor flags found or enabled.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx")
endif()

# RPi
if(${NEON} OR ${NEON} GREATER 0)
    message(STATUS "neon processor flags found or enabled.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon -march=armv8-a -mtune=cortex-a53")
endif()

# grab latest NN model (or substitute your own)
set(LPCNET_ROOT http://rowetel.com/downloads/deep/)
set(LPCNET_FILE lpcnet_191005_v1.0.tgz)
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

# Find codec2
if(CODEC2_BUILD_DIR)
    find_package(codec2 REQUIRED
        PATHS ${CODEC2_BUILD_DIR}
        NO_DEFAULT_PATH
        CONFIGS codec2.cmake
    )
    if(codec2_FOUND)
        message(STATUS "Codec2 library found in build tree.")
    endif()
else()
    find_package(codec2 REQUIRED)
endif()

add_subdirectory(src)

# Ctests ----------------------------------------------------------------------

include(CTest)
enable_testing()

add_test(NAME core_synthesis_default
         COMMAND sh -c "cd ${CMAKE_CURRENT_SOURCE_DIR}/unittest; SYNTH=1 ./test_core_nn.sh")
add_test(NAME core_synthesis_load_20h
         COMMAND sh -c "cd ${CMAKE_CURRENT_SOURCE_DIR}/unittest; SYNTH_20h=1 ./test_core_nn.sh")
add_test(NAME core_synthesis_mag
         COMMAND sh -c "cd ${CMAKE_CURRENT_SOURCE_DIR}/unittest; SYNTH_mag=1 ./test_core_nn.sh")
add_test(NAME nnet2f32
         COMMAND sh -c "cd ${CMAKE_CURRENT_BINARY_DIR}; ./src/nnet2f32 t.f32")
add_test(NAME SIMD_functions
         COMMAND sh -c "cd ${CMAKE_CURRENT_BINARY_DIR}; ./src/test_vec")
