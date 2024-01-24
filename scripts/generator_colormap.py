import numpy as np
import matplotlib.pyplot as plt

N = 256
colormaps = ['cividis', 'jet', 'coolwarm', 'viridis', 'plasma']

intensities = np.linspace(0., 1., N)

for cmap_name in colormaps:
  cmap = plt.cm.get_cmap(cmap_name)
  
  print(N)
  print(cmap_name)
  print('[')
  for intensity in intensities:
    rgba = cmap(intensity)
    print(f'{rgba[0]:.8f}, {rgba[1]:.8f}, {rgba[2]:.8f},')
  print(']')
  print()
