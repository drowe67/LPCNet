/*---------------------------------------------------------------------------*\

  FILE........: codec2_renames.h
  AUTHOR......: Mooneer Salem
  DATE CREATED: August 20, 2022

  Applies renames for Codec2 functions brought into LPCNet to avoid multiple
  symbol errors when linking Codec2 and anything that uses it.

  NOTE: this file needs to be included near the top of each Codec2 .c file
  prior to including any Codec2 related .h files.

\*---------------------------------------------------------------------------*/

#ifndef CODEC2_RENAMES_H
#define CODEC2_RENAMES_H

#define codec2_fftr __codec2__codec2_fftr
#define codec2_fftri __codec2__codec2_fftri
#define codec2_fft_alloc __codec2__codec2_fft_alloc 
#define codec2_fftr_alloc __codec2__codec2_fftr_alloc
#define codec2_fft_free __codec2__codec2_fft_free
#define codec2_fftr_free __codec2__codec2_fftr_free
#define codec2_fft __codec2__codec2_fft
#define codec2_fft_inplace __codec2__codec2_fft_inplace

#define kiss_fft_alloc __codec2__kiss_fft_alloc
#define kiss_fft __codec2__kiss_fft
#define kiss_fft_stride __codec2__kiss_fft_stride
#define kiss_fft_cleanup __codec2__kiss_fft_cleanup
#define kiss_fft_next_fast_size __codec2__kiss_fft_next_fast_size

#define kiss_fftr_alloc __codec2__kiss_fftr_alloc
#define kiss_fftr __codec2__kiss_fftr
#define kiss_fftri __codec2__kiss_fftri

#define nlp_create __codec2__nlp_create
#define nlp_destroy __codec2__nlp_destroy
#define nlp __codec2__nlp

#define c2const_create __codec2__c2const_create
#define make_analysis_window __codec2__make_analysis_window
#define hpf __codec2__hpf
#define dft_speech __codec2__dft_speech
#define two_stage_pitch_refinement __codec2__two_stage_pitch_refinement
#define estimate_amplitudes __codec2__estimate_amplitudes
#define est_voicing_mbe __codec2__est_voicing_mbe
#define make_synthesis_window __codec2__make_synthesis_window
#define synthesise __codec2__synthesise
#define codec2_rand __codec2__codec2_rand

#endif /* CODEC2_RENAMES_H */
