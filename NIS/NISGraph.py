import matplotlib.pyplot as plt
import numpy as np

""" Graphs """
NIS_radar_file = open("NIS_radar.txt", 'r')
NIS_laser_file = open("NIS_laser.txt", 'r')

NIS_radar = []
for line in NIS_radar_file:
    NIS_radar.append(float(line))

NIS_laser = []
for line in NIS_laser_file:
    NIS_laser.append(float(line))

radar_line = []
for sample in NIS_radar:
    radar_line.append(7.8)

laser_line = []
for sample in NIS_laser:
    laser_line.append(5.9)
NIS_radar[0] = 0

plt.subplot(121)
plt.plot(NIS_radar)
plt.plot(radar_line)
plt.title('Radar')
plt.ylabel('NIS')
plt.xlabel('Sample')

plt.subplot(122)
plt.plot(NIS_laser)
plt.plot(laser_line)
plt.title('Laser')
plt.xlabel('Sample')

plt.show()
