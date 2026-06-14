import serial
import numpy as np
import matplotlib.pyplot as plt
from time import sleep

PORT = '/dev/tty.usbmodem1101'
BAUD = 115200
NUM_SAMPLES = 500

ser = serial.Serial(PORT, BAUD, timeout=10)
sleep(2)  # wait for Pico to boot and print "Send number of samples:"

# Send the sample count
ser.write(f"{NUM_SAMPLES}\n".encode())
print(f"Sent: {NUM_SAMPLES} samples")

print("Collecting data...")

time_ms = []
raw = []
filtered = []

while True:
    line = ser.readline().decode('utf-8').strip()
    if not line:
        continue
    print(line)

    if "," not in line:
        continue
    if line.startswith("time_ms"):  # skip header
        continue

    try:
        t, r, f = line.split(",")
        time_ms.append(float(t))
        raw.append(float(r))
        filtered.append(float(f))
    except:
        pass

    if len(raw) >= NUM_SAMPLES:
        break

ser.close()

time_s = np.array(time_ms) / 1000.0
raw = np.array(raw)
filtered = np.array(filtered)

# sampling frequency
dt = np.mean(np.diff(time_s))
fs = 1.0 / dt

print("Sampling frequency:", fs)

# FFT
raw_fft = np.abs(np.fft.rfft(raw - np.mean(raw)))
filtered_fft = np.abs(np.fft.rfft(filtered - np.mean(filtered)))

freqs = np.fft.rfftfreq(len(raw), d=dt)

# time domain plots
plt.figure()
plt.plot(time_s, raw)
plt.xlabel("Time (s)")
plt.ylabel("Raw Signal")
plt.title("HX711 Raw Data")
plt.grid()

plt.figure()
plt.plot(time_s, filtered)
plt.xlabel("Time (s)")
plt.ylabel("Filtered Signal")
plt.title("HX711 Filtered Data")
plt.grid()

# FFT plots
plt.figure()
plt.plot(freqs, raw_fft)
plt.xlim([0, 40])
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude")
plt.title("FFT of Raw Signal")
plt.grid()

plt.figure()
plt.plot(freqs, filtered_fft)
plt.xlim([0, 40])
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude")
plt.title("FFT of Filtered Signal")
plt.grid()

plt.show()