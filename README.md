# LPCNet

Low complexity implementation of the WaveRNN-based LPCNet algorithm, as described in:

J.-M. Valin, J. Skoglund, [LPCNet: Improving Neural Speech Synthesis Through Linear Prediction](https://jmvalin.ca/papers/lpcnet_icassp2019.pdf), *Submitted for ICASSP 2019*, arXiv:1810.11846.

# Introduction

Work in progress software for researching low CPU complexity algorithms for speech synthesis and compression by applying Linear Prediction techniques to WaveRNN. High quality speech can be synthesised on regular CPUs (around 3 GFLOP) with SIMD support (AVX, AVX2/FMA, NEON currently supported).

The BSD licensed software is written in C and Python/Keras. For training, a GTX 1080 Ti or better is recommended.

This software is an open source starting point for WaveRNN-based speech synthesis and coding.

# Quickstart

1. Set up a Keras system with GPU.

1. Generate training data:
   ```
   make dump_data
   ./dump_data --train input.s16 features.f32 data.u8
   ```
   where the first file contains 16 kHz 16-bit raw PCM audio (no header) and the other files are output files. This program makes several passes over the data with different filters to generate a large amount of training data.

1. Now that you have your files, train with:
   ```
   ./train_lpcnet.py features.f32 data.u8
   ```
   and it will generate a wavenet*.h5 file for each iteration. If it stops with a 
   "Failed to allocate RNN reserve space" message try reducing the *batch\_size* variable in train_wavenet_audio.py.

1. You can synthesise speech with Python and your GPU card:
   ```
   ./dump_data --test test_input.s16 test_features.f32
   ./test_lpcnet.py test_features.f32 test.s16
   ```
   Note the .h5 is hard coded in test_lpcnet.py, modify for your .h file.

1. Or with C on a CPU:
   First extract the model files nnet_data.h and nnet_data.c
   ```
   ./dump_lpcnet.py lpcnet15_384_10_G16_64.h5
   ```
   Then you can make the C synthesiser and try synthesising from a test feature file:
   ```
   make test_lpcnet
   ./dump_data --test test_input.s16 test_features.f32
   ./test_lpcnet test_features.f32 test.s16
   ```
 
# Speech Material for Training 

Suitable training material can be obtained from the [McGill University Telecommunications & Signal Processing Laboratory](http://www-mmsp.ece.mcgill.ca/Documents/Data/).  Download the ISO and extract the 16k-LP7 directory, the src/concat.sh script can be used to generate a headerless file of training samples.
```
cd 16k-LP7
sh /path/to/concat.sh
```

# Reading Further

1. [LPCNet: DSP-Boosted Neural Speech Synthesis](https://people.xiph.org/~jm/demo/lpcnet/)
1. Sample model files:
https://jmvalin.ca/misc_stuff/lpcnet_models/

# David's Quantiser Experiments

First build a C lpcnet_test C decoder with the [latest .h5](https://jmvalin.ca/misc_stuff/lpcnet_models/) file.

Binary files for these experiments [here](http://rowetel.com/downloads/deep/lpcnet_quant)

## Exploring Features

Install GNU Octave (if thats your thing).

Extract a feature file, fire up Octave, and mesh plot the 18 cepstrals for the first 100 frame (1 second):

```
$ ./dump_data --test speech_orig_16k.s16 speech_orig_16k_features.f32
$ cd src
$ octave --no-gui
octave:3> f=load_f32("../speech_orig_16k_features.f32",55);
nrows: 1080
octave:4> mesh(f(1:100,1:18))
```

## Uniform Quantisation

Listen to the effects of 4dB step uniform quantisation on cepstrals:

```
$ cat ~/Downloads/wia.wav | ./dump_data --test - - | ./quant_feat -u 4 | ./test_lpcnet - - | play -q -r 16000 -s -2 -t raw -
```

This lets us listen to the effect of quantisation error.  Once we think it sounds OK, we can compute the variance (average squared quantiser error). A 4dB step size means the error PDF is uniform in the range of -2 to +2 dB.  A uniform PDF has variance of (b-a)^2/12, so (2--2)^2/12 = 1.33 dB^2.  We can then try to design a quantiser (e.g. multi-stage VQ) to achieve that variance.

## Training a Predictive VQ

Checkout and build [codec2-dev from SVN](http://rowetel.com/codec2.html):

```
$ svn co https://svn.code.sf.net/p/freetel/code/codec2-dev
$ cd codec2-dev && mkdir build_linux && cd build_linux && cmake ../ && make
```

In train_pred2.sh, adjust PATH for the location of codec2-dev on your machine.

Generate 5E6 vectors using the -train option on dump_data to apply a bunch of different filters, then run the predictive VQ training script
```
$ cd LPCNet
$ ./dump_data --train all_speech.s16 all_speech_features_5e6.f32 /dev/null
$ ./train_pred2.sh
```

## Mbest VQ search

Keeps M best candidates after each stage:

```cat ~/Downloads/speech_orig_16k.s16 | ./dump_data --test - - | ./quant_feat --mbest 5 -q pred2_stage1.f32,pred2_stage2.f32,pred2_stage3.f32 > /dev/null```

In this example, the VQ error variance was reduced from 2.68 to 2.28 dB^2 (I think equivalent to 3 bits), and the number of outliers >2dB reduced from 15% to 10%.


