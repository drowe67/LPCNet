#!/usr/bin/python3
# trunc_dct.py
#
# David Rowe April 2019
#
# Keras model for estimating the high frequency bands of a mel sampled
# magnitude vector from the low freq bands of the same vector 

# to generate features:
# $ ./src/dump_data --train --c2pitch -z 0 --mag -n 1E6 ~/Downloads/all_speech.sw all_speech.f32 all_speech.pcm

import numpy as np
import sys
from keras.layers import Dense
from keras import models,layers
from keras import initializers

# constants

nb_features = 55
nb_bands = 18
nb_low_bands = 12
nb_high_bands = nb_bands - nb_low_bands
nb_epochs=10

# load training data

feature_file = sys.argv[1]
features = np.fromfile(feature_file, dtype='float32')
nb_frames = int(len(features)/nb_features)
print("nb_frames: %d" % (nb_frames))

# 0..17 mags, 36 = pitch, 37 = pitch gain, 38 = lpc-gain
features = np.reshape(features, (nb_frames, nb_features))

# remove mean from each feature vector, we calculate mean over nb_low
# as that's all we will have in a decoder
m = np.mean(features[:,:nb_low_bands],axis=1)
features[:,:nb_bands] -= m[:,None]
#train_dct -= m[None,:]
print("mean:")
print(m.shape)

#features = features[:100000,:]
# convert log10 or band energy to dB, so loss measures in meaningful units
print("features:")
print(features.shape)
train = np.concatenate( (features[:,:nb_low_bands], features[:,37:38]), axis=1)
print("train:")
print(train.shape)
target = features[:,nb_low_bands:nb_bands]
print("target:")
print(target.shape)

var=np.var(target)
print("target var: %f" % (var))

model = models.Sequential()
model.add(layers.Dense(256, activation='relu', input_dim=nb_low_bands+1))
model.add(layers.Dense(256, activation='relu'))
model.add(layers.Dense(256, activation='relu'))
model.add(layers.Dense(nb_high_bands, activation='linear'))

# Compile our model using the method of least squares (mse) loss function 
# and a stochastic gradient descent (sgd) optimizer

from keras import optimizers
model.compile(loss='mse', optimizer='adam')

# fit model, using 20% of our data for vaildation

history = model.fit(train, target, validation_split=0.2, batch_size=32, epochs=10)
model.save("trunc_mag.h5")

import matplotlib.pyplot as plt

# loss is MSE, or variance - plot std instead for each epoch

plt.figure(1)
plt.plot(np.sqrt(history.history['loss']))
plt.plot(np.sqrt(history.history['val_loss']))
plt.title('model loss')
plt.ylabel('rms error (dB)')
plt.xlabel('epoch')
plt.legend(['train', 'valid'], loc='upper right')

# run model on training data and compare output to sanity check loss

train_out = model.predict(train)
err = (train_out - target)
var = np.var(err)
print(var.shape)
std = np.std(err)
print("var: %f std: %f" % (var,std))

# generate ideal and NN regeneration of high bands on training data

nb_test_frames=600
test_in = train[:nb_test_frames,:]
print("test_in:")
print(test_in.shape)
test_out = np.concatenate((features[:nb_test_frames,:nb_low_bands], model.predict(test_in)), axis=1)
print("test_out:")
print(test_out.shape)

import scipy.io as sio
sio.savemat("orig.mat", {"orig":features[:nb_test_frames,:nb_bands]})
sio.savemat("test_out.mat", {"test_out":test_out})

plt.show()

