# plot packed ulaw samples

import numpy as np
import matplotlib.pyplot as plt
import sys
import ulaw

data = np.fromfile(sys.argv[1], dtype='uint8')
nb_samples = 8000

data = data[:nb_samples*4]

sig = np.array(data[0::4], dtype='float')
pred = np.array(data[1::4], dtype='float')
in_exc = np.array(data[2::4], dtype='float')
out_exc = np.array(data[3::4], dtype='float')

plt.plot(ulaw.ulaw2lin(sig))
plt.plot(ulaw.ulaw2lin(pred))
plt.plot(ulaw.ulaw2lin(in_exc))
plt.plot(ulaw.ulaw2lin(out_exc))
plt.show()
