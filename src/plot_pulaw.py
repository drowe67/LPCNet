#!/usr/bin/python3
# Utility to inspect packed ulaw samples before training from sw2packedulaw.c

import numpy as np
import matplotlib.pyplot as plt
import sys
import ulaw

data = np.fromfile(sys.argv[1], dtype='uint8')
nb_samples = 8000

#data = data[:nb_samples*20]

sig = np.array(data[0::4], dtype='float')
pred = np.array(data[1::4], dtype='float')
in_exc = np.array(data[2::4], dtype='float')
out_exc = np.array(data[3::4], dtype='float')
   
print("exc var: %4.3e" % (np.var(ulaw.ulaw2lin(in_exc))))

"""
s_in = 32000*np.sin(np.arange(0,8000)*np.pi*2/100)
s_out = ulaw.ulaw2lin(ulaw.lin2ulaw(s_in))
plt.figure(1)
plt.plot(s_in)
plt.plot(s_out)
plt.show(block=False)
"""

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
if len(sys.argv) == 3:
    data2 = np.fromfile(sys.argv[2], dtype='uint8')
    in_exc2 = np.array(data2[2::4], dtype='float')
    plt.plot(ulaw.ulaw2lin(in_exc2), label='in_exc2')
plt.ylim((-30000,30000))
plt.legend()
plt.subplot(212)
plt.plot(ulaw.ulaw2lin(out_exc), label='out_exc')
plt.ylim((-30000,30000))
plt.legend()
plt.show()
