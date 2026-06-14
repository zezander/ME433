import serial
import matplotlib.pyplot as plt
import time

port = "/dev/cu.usbmodem102"
baud = 115200
ser = serial.Serial(port, baud, timeout=2)

time.sleep(2)
ser.write(b'a')

index = []
desired = []
current = []
ui = []

while True:
    line = ser.readline().decode(errors="ignore").strip()
    if line:
        print(line)
        parts = line.split()
        if len(parts) == 4:
            try:
                index.append(int(parts[0]))
                desired.append(int(parts[1]))
                current.append(int(parts[2]))
                ui.append(int(parts[3]))
            except ValueError:
                pass
    if len(index) >= 400:
        break

ser.close()

plt.figure()
plt.plot(index, desired, label="Desired Current")
plt.plot(index, current, label="Measured Current")
plt.xlabel("Sample")
plt.ylabel("Current")
plt.grid(True)
plt.legend()
plt.show()

plt.figure()
plt.plot(index, ui)
plt.xlabel("Sample")
plt.ylabel("Controller Output (ui)")
plt.grid(True)
plt.title("Control Effort")
plt.show()