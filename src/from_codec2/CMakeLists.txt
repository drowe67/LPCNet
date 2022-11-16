# Files brought in from Codec2 that are mandatory for LPCNet.
set(codec2_import_srcs
    sine.c
    nlp.c
    codec2_fft.c
    kiss_fft.c
    kiss_fftr.c
)

# Set visibility of files brought in from Codec2 so that they're not visible to 
# users (such as Codec2 itself).
foreach(CODEC2_FILE IN LISTS codec2_import_srcs)
    set_source_files_properties(${CODEC2_FILE} PROPERTIES COMPILE_FLAGS "-fPIC -fvisibility=hidden -include ${CMAKE_CURRENT_SOURCE_DIR}/codec2_renames.h")
endforeach()

add_library(lpcnet_from_codec2 STATIC ${codec2_import_srcs})