import numpy as np
import serial
import matplotlib
import threading
from queue import Queue
matplotlib.use("TkAgg")

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.gridspec as gridspec
from matplotlib.ticker import MaxNLocator

queue_sensor = Queue()
queue_root   = Queue()

temperatures_sensor = []
temperatures_root = []
temperatures_interp = []
differences_disp = []
differences_calc = []
differences_interp = []

def read_rate_once(port, marker):
    while True:
        line = port.readline().decode().strip()
        pos = line.find(marker)
        if pos != -1:
            found_value = line[pos + len(marker):]
            found_value = int(found_value)
            return found_value

def read_serial_bus(port, marker, q):
    while True:
        line = port.readline().decode().strip()
        pos = line.find(marker)
        if pos != -1:
            found_value = line[pos + len(marker):]
            found_value = int(found_value) / 100
            q.put(found_value)

def interpolate(x1, x2, n):
    new_x = []
    base = x1
    step = (x2 - x1) / n
    for i in range(1, n):
        new_x.append(base + i * step)
    new_x.append(x2)
    return new_x

def animate(i):
    sensor_temp = queue_sensor.get()
    temperatures_sensor.append(sensor_temp)
    if i % reduced_sending_rate == 0:
        root_temp = queue_root.get()
        temperatures_root.append(root_temp)

    #debug
    if i % reduced_sending_rate == 0:
        print(temperatures_sensor[-1], temperatures_root[-1])
        print("----------------------------------------")

    if i == 0:
        temperatures_interp.append(temperatures_root[0])
        differences_interp.append(0)
    elif i % reduced_sending_rate == 0:
        interpolated_values = interpolate(temperatures_interp[-1], temperatures_root[-1], reduced_sending_rate)
        temperatures_interp.extend(interpolated_values)
        for i in range(reduced_sending_rate):
            j = reduced_sending_rate - i
            differences_interp.append(temperatures_sensor[-j] - temperatures_interp[-j])

    new_diff_value = temperatures_sensor[-1] - temperatures_root[-1]
    if len(differences_disp) > 1 and np.isnan(differences_disp[-1]):
        differences_disp[-1] = (new_diff_value + differences_disp[-2]) / 2
    if i % reduced_sending_rate == 0:
        differences_disp.append(np.nan)
    else:
        differences_disp.append(new_diff_value)
    differences_calc.append(new_diff_value)

    x_sensor = np.arange(len(temperatures_sensor))
    x_root = np.arange(len(temperatures_root)) * reduced_sending_rate
    line_sensor.set_data(x_sensor, temperatures_sensor)
    line_root.set_data(x_root, temperatures_root)
    ax1.relim()
    ax1.autoscale_view()

    x_diff = np.arange(len(differences_disp))
    x_interp = np.arange(len(differences_interp))
    line_diff.set_data(x_diff, differences_disp)
    line_interp.set_data(x_interp, differences_interp)
    ax2.relim()
    ax2.autoscale_view()

    mse_basic = np.nanmean(np.square(differences_calc))
    mse_basic = round(mse_basic, 5)
    mse_text_basic.set_text(f'mse: {mse_basic}')

    mse_interp = np.nanmean(np.square(differences_interp))
    mse_interp = round(mse_interp, 5)
    mse_text_interp.set_text(f'mse: {mse_interp}')

    return line_sensor, line_root, line_diff, line_interp, mse_text_basic, mse_text_interp

def init():
    return []

print("Starting...")
s = serial.Serial('/dev/ttyACM0')
operating_rate = read_rate_once(s, "Operating rate is: ")
reduced_sending_rate = read_rate_once(s, "Reduced sending rate is: ")
s.close()

fig = plt.figure()
gs = gridspec.GridSpec(2, 3, width_ratios=[2, 1, 1], height_ratios=[6, 1])
ax1 = fig.add_subplot(gs[:, 0])
ax2 = fig.add_subplot(gs[0, 1:3])
ax3 = fig.add_subplot(gs[1, 1])
ax4 = fig.add_subplot(gs[1, 2])

line_sensor, = ax1.plot([], [], label='temperature sensor')
line_root, = ax1.plot([], [], label='temperature root')
ax1.set_title('Temperatures')
ax1.set_xlabel('Iteration')
ax1.xaxis.set_major_locator(MaxNLocator(integer=True))
ax1.set_ylabel('Temperature in Celsius')
ax1.legend()
ax1.grid()

line_diff, = ax2.plot([], [], label='temperature difference to last value')
line_interp, = ax2.plot([], [], label='temperature difference interpolated')
ax2.set_title('Temperature error')
ax2.set_xlabel('Iteration')
ax2.xaxis.set_major_locator(MaxNLocator(integer=True))
ax2.set_ylabel('Temperature difference in Celsius')
ax2.legend()
ax2.grid()

mse_text_basic = ax3.text(0.5, 0.5, "", fontsize=24, ha='center', va='center')
ax3.axis('off')

mse_text_interp = ax4.text(0.5, 0.5, "", fontsize=24, ha='center', va='center')
ax4.axis('off')

s0 = serial.Serial('/dev/ttyACM0')
threading.Thread(target=read_serial_bus, args=(s0, "Temperature: ", queue_sensor), daemon=True).start()
s1 = serial.Serial('/dev/ttyACM1')
threading.Thread(target=read_serial_bus, args=(s1, "payload: ", queue_root), daemon=True).start()

ani = animation.FuncAnimation(fig, animate, init_func=init, interval=operating_rate, blit=True)
plt.show()