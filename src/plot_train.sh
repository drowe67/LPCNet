#!/bin/sh
# plot_train.sh
# David Rowe Jan 2019
#

# plot graphs of loss and spares categorical accuracy to get a feel
# for progress while training

grep loss $1 | sed -n 's/.*===\].*step - loss: \(.*\)/\1/p' > loss.txt
python3 ~/LPCNet/src/plot_train.py loss.txt
