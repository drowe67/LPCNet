include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    ${lpcnet_SOURCE_DIR})

set(lpcnet_freedv_srcs
    freq.c
    kiss_fft.c
    celt_lpc.c
    pitch.c
    codec2_pitch.c
    mbest.c
    lpcnet_quant.c
    lpcnet_dump.c
    4stage_pred_vq.c
    4stage_direct_split_vq.c
    lpcnet.c
    nnet.c
    nnet_data.c
)

add_library(lpcnetfreedv STATIC ${lpcnet_freedv_srcs})

add_executable(dump_data dump_data.c)
target_link_libraries(dump_data lpcnetfreedv m codec2)

add_executable(test_lpcnet test_lpcnet.c)
target_link_libraries(test_lpcnet lpcnetfreedv m)

add_executable(test_vec test_vec.c)
target_link_libraries(test_vec m)

add_executable(quant_feat quant_feat.c)
target_link_libraries(quant_feat lpcnetfreedv m)

add_executable(tcodec2_pitch tcodec2_pitch.c codec2_pitch.c)
target_link_libraries(tcodec2_pitch m codec2)

add_executable(weight weight.c)
target_link_libraries(weight m)

add_executable(tdump tdump.c)
target_link_libraries(tdump lpcnetfreedv m codec2)

add_executable(quant_test quant_test.c)
target_link_libraries(quant_test lpcnetfreedv m)

add_executable(quant2c quant2c.c)
target_link_libraries(quant2c m)

add_executable(diff32 diff32.c)
target_link_libraries(diff32 m)

add_executable(quant_enc quant_enc.c)
target_link_libraries(quant_enc lpcnetfreedv m)

add_executable(quant_dec quant_dec.c)
target_link_libraries(quant_dec lpcnetfreedv m)

add_executable(lpcnet_enc lpcnet_enc.c)
target_link_libraries(lpcnet_enc lpcnetfreedv m codec2)

add_executable(lpcnet_dec lpcnet_dec.c)
target_link_libraries(lpcnet_dec lpcnetfreedv m codec2)

add_executable(idct idct.c)
target_link_libraries(idct lpcnetfreedv m)
