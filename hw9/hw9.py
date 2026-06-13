import csv
import numpy as np
import matplotlib.pyplot as plt


# -----------------------------
# CSV LOADER
# -----------------------------
def load_csv(filename):
    time = []
    signal = []

    with open(filename, newline='') as f:
        reader = csv.reader(f)

        for row in reader:
            time.append(float(row[0]))
            signal.append(float(row[1]))

    return np.array(time), np.array(signal)


# -----------------------------
# SAMPLE RATE
# -----------------------------
def sample_rate(time):
    return len(time) / (time[-1] - time[0])


# -----------------------------
# FFT
# -----------------------------
def compute_fft(signal, fs):

    fft_vals = np.fft.fft(signal)

    freqs = np.fft.fftfreq(len(signal), d=1/fs)

    half = len(freqs) // 2

    return freqs[:half], np.abs(fft_vals[:half])


# -----------------------------
# MOVING AVERAGE FILTER
# -----------------------------
def moving_average(signal, window):

    filtered = []

    for i in range(len(signal)):

        start = max(0, i - window + 1)

        avg = np.mean(signal[start:i+1])

        filtered.append(avg)

    return np.array(filtered)


# -----------------------------
# IIR FILTER
# -----------------------------
def iir_filter(signal, A, B):

    output = np.zeros(len(signal))

    output[0] = signal[0]

    for i in range(1, len(signal)):
        output[i] = A * output[i-1] + B * signal[i]

    return output


# -----------------------------
# FIR FILTER
# -----------------------------
def fir_filter(signal, taps):

    return np.convolve(signal, taps, mode='same')


# -----------------------------
# LOWPASS FIR WEIGHTS
# -----------------------------
def lowpass_fir(cutoff_hz, fs, taps):

    fc = cutoff_hz / fs

    n = np.arange(taps)

    center = (taps - 1) / 2

    h = np.sinc(2 * fc * (n - center))

    h *= np.hamming(taps)

    h /= np.sum(h)

    return h


# -----------------------------
# PLOT FFT ANALYSIS
# -----------------------------
def save_fft_plot(time, signal, fs, name):

    freqs, mag = compute_fft(signal, fs)

    plt.figure(figsize=(10,8))

    plt.subplot(2,1,1)
    plt.plot(time, signal)
    plt.title(f"{name} Signal")
    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude")

    plt.subplot(2,1,2)
    plt.plot(freqs, mag)
    plt.title("FFT")
    plt.xlabel("Frequency (Hz)")
    plt.ylabel("Magnitude")

    plt.tight_layout()
    plt.savefig(f"{name}_fft.png")
    plt.close()


# -----------------------------
# COMPARE SIGNALS
# -----------------------------
def save_signal_compare(time,
                        original,
                        filtered,
                        title,
                        filename):

    plt.figure(figsize=(10,5))

    plt.plot(time, original, 'k', label='Original')
    plt.plot(time, filtered, 'r', label='Filtered')

    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude")
    plt.title(title)

    plt.legend()
    plt.grid()

    plt.savefig(filename)
    plt.close()


# -----------------------------
# COMPARE FFTS
# -----------------------------
def save_fft_compare(original,
                     filtered,
                     fs,
                     title,
                     filename):

    f1, m1 = compute_fft(original, fs)
    f2, m2 = compute_fft(filtered, fs)

    plt.figure(figsize=(10,5))

    plt.plot(f1, m1, 'k', label='Original FFT')
    plt.plot(f2, m2, 'r', label='Filtered FFT')

    plt.xlabel("Frequency (Hz)")
    plt.ylabel("Magnitude")
    plt.title(title)

    plt.legend()
    plt.grid()

    plt.savefig(filename)
    plt.close()


# -----------------------------
# MAIN
# -----------------------------
files = [
    "sigA.csv",
    "sigB.csv",
    "sigC.csv",
    "sigD.csv"
]

for file in files:

    name = file.replace(".csv", "")

    time, signal = load_csv(file)

    fs = sample_rate(time)

    print(name)
    print("Sample Rate:", fs)

    save_fft_plot(time, signal, fs, name)

    # -------------------------
    # MOVING AVERAGE
    # -------------------------
    maf_window = 20

    maf = moving_average(signal, maf_window)

    save_signal_compare(
        time,
        signal,
        maf,
        f"{name} Moving Average X={maf_window}",
        f"{name}_maf_signal.png"
    )

    save_fft_compare(
        signal,
        maf,
        fs,
        f"{name} FFT MAF X={maf_window}",
        f"{name}_maf_fft.png"
    )

    # -------------------------
    # IIR
    # -------------------------
    A = 0.98
    B = 0.02

    iir = iir_filter(signal, A, B)

    save_signal_compare(
        time,
        signal,
        iir,
        f"{name} IIR A={A} B={B}",
        f"{name}_iir_signal.png"
    )

    save_fft_compare(
        signal,
        iir,
        fs,
        f"{name} FFT IIR A={A} B={B}",
        f"{name}_iir_fft.png"
    )

    # -------------------------
    # FIR
    # -------------------------
    taps = 51
    cutoff = 200

    weights = lowpass_fir(cutoff, fs, taps)

    fir = fir_filter(signal, weights)

    save_signal_compare(
        time,
        signal,
        fir,
        f"{name} FIR {taps} taps cutoff={cutoff}Hz",
        f"{name}_fir_signal.png"
    )

    save_fft_compare(
        signal,
        fir,
        fs,
        f"{name} FFT FIR {taps} taps cutoff={cutoff}Hz",
        f"{name}_fir_fft.png"
    )

print("Done")