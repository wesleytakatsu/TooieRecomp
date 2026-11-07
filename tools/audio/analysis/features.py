"""Chroma features (50 frames/s) for audio files, cached as .npy next to a cache dir.

    features.py <cache-dir> <audio files...>
"""
import os, subprocess, sys
import numpy as np

RATE = 16000
HOP = RATE // 50
NFFT = 4096
SECONDS = float(os.environ.get("FEATURE_SECONDS", "150"))


def decode(path, seconds=SECONDS):
    cmd = ["ffmpeg", "-v", "error", "-i", path, "-t", str(seconds), "-ac", "1", "-ar", str(RATE), "-f", "f32le", "-"]
    raw = subprocess.run(cmd, stdout=subprocess.PIPE, check=True).stdout
    return np.frombuffer(raw, dtype=np.float32)


_window = np.hanning(NFFT).astype(np.float32)
_freqs = np.fft.rfftfreq(NFFT, 1.0 / RATE)
_valid = (_freqs > 55) & (_freqs < 4200)
_pitch = np.zeros(len(_freqs), dtype=int)
_pitch[_valid] = np.round(12 * np.log2(_freqs[_valid] / 440.0)).astype(int) % 12
_matrix = np.zeros((12, len(_freqs)), dtype=np.float32)
_matrix[_pitch[_valid], np.nonzero(_valid)[0]] = 1.0


def chroma(x):
    if len(x) < NFFT:
        x = np.pad(x, (0, NFFT - len(x)))
    frames = 1 + (len(x) - NFFT) // HOP
    idx = np.arange(NFFT)[None, :] + HOP * np.arange(frames)[:, None]
    spec = np.abs(np.fft.rfft(x[idx] * _window, axis=1)).astype(np.float32)
    c = (spec ** 2) @ _matrix.T          # frames x 12
    energy = c.sum(axis=1, keepdims=True)
    c = np.sqrt(c)
    c /= np.linalg.norm(c, axis=1, keepdims=True) + 1e-9
    loud = np.log10(energy[:, 0] + 1e-9)
    return c.astype(np.float32), loud.astype(np.float32)


def cached(cache, path):
    key = os.path.join(cache, os.path.basename(path) + ".npz")
    if os.path.exists(key) and os.path.getmtime(key) >= os.path.getmtime(path):
        z = np.load(key)
        return z["c"], z["l"]
    c, l = chroma(decode(path))
    np.savez(key, c=c, l=l)
    return c, l


if __name__ == "__main__":
    cache = sys.argv[1]
    os.makedirs(cache, exist_ok=True)
    for p in sys.argv[2:]:
        cached(cache, p)
        print("ok", os.path.basename(p), flush=True)
