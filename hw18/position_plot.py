import numpy as np
import matplotlib.pyplot as plt

center_deg = 173
range_deg = 45
K = 0.25

angle_deg = np.linspace(center_deg - 60, center_deg + 60, 300)

x = (angle_deg - center_deg) / range_deg
x_clamped = np.clip(x, -1, 1)

u = -K * x_clamped

plt.figure()
plt.plot(angle_deg, x_clamped, label="Normalized position x")
plt.plot(angle_deg, u, label="Motor command u")
plt.axvline(center_deg, linestyle="--", label="Center = 173 deg")
plt.axvline(center_deg - range_deg, linestyle=":", label="-45 deg limit")
plt.axvline(center_deg + range_deg, linestyle=":", label="+45 deg limit")
plt.xlabel("Encoder angle (deg)")
plt.ylabel("Normalized value")
plt.title("Normalized Position and Motor Command")
plt.grid(True)
plt.legend()
plt.show()