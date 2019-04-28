#!/usr/bin/python3
# train_regen.py
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

if len(sys.argv) < 2:
    print("usage: train_regen.py train_input.f32 [remove_mean 1|0]")
    sys.exit(0)

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
#features=features[:10000,:]
print("features shape:")
print(features.shape)

if len(sys.argv) > 2:
    if sys.argv[2] == "1":
        m = np.mean(features[:,:nb_bands],axis=1)
    if sys.argv[2] == "2":
        m = np.mean(features[:,:nb_low_bands],axis=1)
    features[:,:nb_bands] -= m[:,None]

train = np.concatenate( (features[:,:nb_low_bands], features[:,37:38]), axis=1)
print("train shape:")
print(train.shape)
target = features[:,nb_low_bands:nb_bands]
print("target shape:")
print(target.shape)

var=np.var(target)
print("target variance (we aim to improve on this)! %f" % (var))

model = models.Sequential()
model.add(layers.Dense(256, activation='relu', input_dim=nb_low_bands+1))
model.add(layers.Dense(256, activation='relu'))
model.add(layers.Dense(256, activation='relu'))
model.add(layers.Dense(nb_high_bands, activation='linear'))

# Compile our model using the method of least squares (mse) loss function 
# and a stochastic gradient descent (sgd) optimizer

from keras import optimizers
model.compile(loss='mse', optimizer='adam')

# fit model, using 20% of our data for validation

history = model.fit(train, target, validation_split=0.2, batch_size=32, epochs=nb_epochs)
model.save("regen_model.h5")

import matplotlib.pyplot as plt

# "loss" == MSE == variance
# Plot std dev instead for each epoch, this
# is equivalent to rms error is modelling of HF bands, a common measure we use for
# quantisation

plot_en = 0;
if plot_en:
    plt.figure(1)
    plt.plot(10*np.sqrt(history.history['loss']))
    plt.plot(10*np.sqrt(history.history['val_loss']))
    plt.title('model loss')
    plt.ylabel('rms error (dB)')
    plt.xlabel('epoch')
    plt.legend(['train', 'valid'], loc='upper right')
    plt.show()

# run model on training data and measure variance, should be similar to training "loss"

train_out = model.predict(train)
err = (train_out - target)
var = np.var(err)
std = np.std(err)
print("var: %f std: %f" % (var,std))


