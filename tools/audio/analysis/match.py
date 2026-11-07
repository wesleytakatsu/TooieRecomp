"""Rank album MP3s against native renders by chroma correlation over time lag.

    match.py <mp3-cache-dir> <render.wav...>   -> TSV on stdout: render, rank, score, lag_s, file
"""
import glob, os, sys
import numpy as np
from features import chroma, decode

MAX_LAG_S = float(os.environ.get("MATCH_MAX_LAG", "20"))
TOP = int(os.environ.get("MATCH_TOP", "5"))


def center(c):
    c = c - c.mean(axis=1, keepdims=True)
    return c / (np.linalg.norm(c, axis=1, keepdims=True) + 1e-9)


def load_cache(cache):
    items = []
    for p in sorted(glob.glob(os.path.join(cache, "*.mp3.npz"))):
        z = np.load(p)
        items.append((os.path.basename(p)[:-4], center(z["c"]), z["l"]))
    return items


def best_lag(r, w, m):
    """Weighted mean frame cosine for each lag of m relative to r (m index = r index + lag). Returns (score, lag)."""
    n = len(r)
    max_lag = int(MAX_LAG_S * 50)
    min_lag = -int(2 * 50)
    size = 1
    while size < len(m) + n:
        size *= 2
    total = np.zeros(size)
    rw = r * w[:, None]
    for k in range(12):
        a = np.fft.rfft(rw[:, k], size)
        b = np.fft.rfft(m[:, k], size)
        total += np.fft.irfft(np.conj(a) * b, size)
    # total[lag] = sum_t rw[t] . m[t + lag]; negative lags wrap to the end.
    lags = np.arange(min_lag, max_lag + 1)
    vals = total[lags % size]
    # Normalise by the render weight that overlaps the candidate at that lag.
    cw = np.concatenate([[0], np.cumsum(w)])
    norm = []
    for lag in lags:
        lo = max(0, -lag)
        hi = min(n, len(m) - lag)
        norm.append(cw[hi] - cw[lo] if hi > lo else 0)
    norm = np.array(norm)
    full = cw[-1]
    score = np.where(norm > 0.5 * full, vals / np.maximum(norm, 1e-9), 0)
    i = int(np.argmax(score))
    return float(score[i]), int(lags[i])


def render_features(path):
    c, l = chroma(decode(path, 120))
    c = center(c)
    w = (l > (l.max() - 3.5)).astype(np.float64)  # ignore silence and near-silence
    return c, w


if __name__ == "__main__":
    cache = load_cache(sys.argv[1])
    for wav in sys.argv[2:]:
        r, w = render_features(wav)
        if w.sum() < 10:
            print(f"{os.path.basename(wav)}\t0\t0\t0\tSILENT")
            continue
        results = sorted(((best_lag(r, w, c) + (name,)) for name, c, _ in cache), reverse=True)
        for rank, (score, lag, name) in enumerate(results[:TOP]):
            print(f"{os.path.basename(wav)}\t{rank}\t{score:.3f}\t{lag / 50:.2f}\t{name}", flush=True)
