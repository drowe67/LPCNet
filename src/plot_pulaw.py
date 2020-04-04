#!/usr/bin/python3
# Utility to inspect packed ulaw samples from sw2packedulaw.c (or dump_data.c) before training 

import numpy as np
import matplotlib.pyplot as plt
import sys
import ulaw
import argparse

parser = argparse.ArgumentParser(description='Plot LPCNet training packed ulaw samples')
parser.add_argument('file1', help='pulaw file of packed ulaw samples')
parser.add_argument('--file2', help='optional second packed ulaw file to compare')
parser.add_argument('--nb_samples', type=int, default=-1, help='Optional number of samples to plot')
args = parser.parse_args()

data = np.fromfile(args.file1, dtype='uint8')
nb_samples = args.nb_samples
data = data[:nb_samples]

sig = np.array(data[0::4], dtype='float')
pred = np.array(data[1::4], dtype='float')
in_exc = np.array(data[2::4], dtype='float')
out_exc = np.array(data[3::4], dtype='float')
   
print("exc var: %4.3e" % (np.var(ulaw.ulaw2lin(in_exc))))

plt.figure(1)
plt.subplot(211)
plt.plot(ulaw.ulaw2lin(sig), label='sig')
plt.ylim((-30000,30000))
plt.legend()
plt.subplot(212)
plt.plot(ulaw.ulaw2lin(pred), label='pred')
plt.ylim((-30000,30000))
plt.legend()
plt.show(block=False)

plt.figure(2)
plt.subplot(211)
plt.plot(ulaw.ulaw2lin(in_exc), label='in_exc')
if args.file2:
    data2 = np.fromfile(args.file2, dtype='uint8')
    data2 = data2[:nb_samples]
    in_exc2 = np.array(data2[2::4], dtype='float')
    plt.plot(ulaw.ulaw2lin(in_exc2), label='in_exc2')
plt.ylim((-30000,30000))
plt.legend()
plt.subplot(212)
plt.plot(ulaw.ulaw2lin(out_exc), label='out_exc')
plt.ylim((-30000,30000))
plt.legend()
plt.show()
