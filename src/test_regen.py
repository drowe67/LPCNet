#!/usr/bin/python3
# regen_high.py
#
# David Rowe April 2019
#
# Regnerate high freq bands from the low freq bands of the same vector, uses model trained by trunc_mag.py

import numpy as np
import sys
from keras.layers import Dense
from keras import models,layers
from keras import initializers
from keras.models import load_model

model = load_model('trunc_mag.h5')

nb_features = 55
nb_bands = 18
nb_low_bands = 12

# load inputdata

feature_file_in = sys.argv[1]
feature_file_out = sys.argv[2]
features_in = np.fromfile(feature_file_in, dtype='float32')
nb_frames = int(len(features_in)/nb_features)
print("nb_frames: %d" % (nb_frames))

# 0..17 mags, 36 = pitch, 37 = pitch gain, 38 = lpc-gain
features_in = np.reshape(features_in, (nb_frames, nb_features))

# remove mean from each feature vector
m = np.mean(features_in[:,:nb_bands],axis=1)
features_in[:,:nb_bands] -= m[:,None]

lowbands_voicing = np.concatenate( (features_in[:,:nb_low_bands], features_in[:,37:38]), axis=1)
#hibands = model.predict(lowbands_voicing)
#print(hibands.shape)
hibands = np.zeros((nb_frames, nb_bands-nb_low_bands), dtype='float32')
print(hibands.shape)
features_out = np.concatenate((features_in[:,:nb_low_bands], hibands, features_in[:, nb_bands:]), axis=1)
features_out[:,:nb_bands] += m[:,None]
features_out.tofile(feature_file_out)
print("features_out:")
print(features_out.shape)
