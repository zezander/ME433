import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# ── CONFIG ──────────────────────────────────────────────────
PORT = None          # set to e.g. '/dev/tty.usbmodem101', or leave None to auto-detect
BAUD = 115200
WINDOW_SECS = 4
SAMPLE_RATE = 100
MAX_POINTS = WINDOW_SECS * SAMPLE_RATE
# ────────────────────────────────────────────────────────────

def find_pico_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        if 'USB' in p.description or 'ACM' in p.device or 'Pico' in p.description:
            return p.device
    return None

if PORT is None:
    PORT = find_pico_port()
    if PORT is None:
        print("Could not auto-detect Pico. Set PORT manually at the top of this file.")
        exit(1)
    print(f"Found Pico on {PORT}")

ser = serial.Serial(PORT, BAUD, timeout=0.1)

tData   = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
refA    = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
refB    = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
measA   = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
measB   = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)

# ── PLOT SETUP ───────────────────────────────────────────────
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(11, 7), facecolor='#111')
fig.suptitle('Pico DAC — reference vs measured', color='white', fontsize=13)

for ax in (ax1, ax2):
    ax.set_facecolor('#111')
    ax.tick_params(colors='#888')
    for spine in ax.spines.values():
        spine.set_color('#333')
    ax.set_ylim(-0.15, 3.45)
    ax.set_ylabel('Voltage (V)', color='#888')
    for v in [0, 0.825, 1.65, 2.475, 3.3]:
        ax.axhline(v, color='#2a2a2a', linewidth=0.7, zorder=0)
    ax.axhline(0,   color='#444', linewidth=0.7)
    ax.axhline(3.3, color='#444', linewidth=0.7)

ax1.set_title('Channel A — 2 Hz sine', color='white', fontsize=10)
ax2.set_title('Channel B — 1 Hz triangle', color='white', fontsize=10)
ax2.set_xlabel('Time (s)', color='#888')

# reference = dashed, measured = solid
lineRefA, = ax1.plot([], [], color='#5b9bd5', linewidth=1.0, linestyle='--', label='reference')
lineMeasA,= ax1.plot([], [], color='#00e5ff', linewidth=1.2, linestyle='-',  label='measured')

lineRefB, = ax2.plot([], [], color='#f08060', linewidth=1.0, linestyle='--', label='reference')
lineMeasB,= ax2.plot([], [], color='#ffcc00', linewidth=1.2, linestyle='-',  label='measured')

ax1.legend(loc='upper right', facecolor='#222', labelcolor='white', fontsize=8, framealpha=0.7)
ax2.legend(loc='upper right', facecolor='#222', labelcolor='white', fontsize=8, framealpha=0.7)

readA = ax1.text(0.01, 0.88, '', transform=ax1.transAxes, color='#00e5ff', fontsize=9, family='monospace')
readB = ax2.text(0.01, 0.88, '', transform=ax2.transAxes, color='#ffcc00', fontsize=9, family='monospace')

plt.tight_layout()

# ── ANIMATION ────────────────────────────────────────────────
def update(frame):
    while ser.in_waiting:
        try:
            raw = ser.readline().decode('utf-8').strip()
            t_val, ra, rb, ma, mb = map(float, raw.split(','))
            tData.append(t_val)
            refA.append(ra)
            refB.append(rb)
            measA.append(ma)
            measB.append(mb)
        except Exception:
            pass

    t_list = list(tData)

    lineRefA.set_data(t_list, list(refA))
    lineMeasA.set_data(t_list, list(measA))
    lineRefB.set_data(t_list, list(refB))
    lineMeasB.set_data(t_list, list(measB))

    if t_list[-1] > WINDOW_SECS:
        ax1.set_xlim(t_list[-1] - WINDOW_SECS, t_list[-1])
        ax2.set_xlim(t_list[-1] - WINDOW_SECS, t_list[-1])
    else:
        ax1.set_xlim(0, WINDOW_SECS)
        ax2.set_xlim(0, WINDOW_SECS)

    readA.set_text(f'measured = {list(measA)[-1]:.3f} V')
    readB.set_text(f'measured = {list(measB)[-1]:.3f} V')

    return lineRefA, lineMeasA, lineRefB, lineMeasB, readA, readB

ani = animation.FuncAnimation(fig, update, interval=30, blit=False)
plt.show()
ser.close()