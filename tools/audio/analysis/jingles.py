"""Match short renders (jingles) against short album files with onset-aligned spectral features.

    jingles.py <out.tsv> <render-dir>...   (renders named NNN_sXX_whole.wav)
"""
import glob, os, subprocess, sys
import numpy as np
import features as F

ALBUM = os.environ["TOOIE_ALBUM"].rstrip("/\\") + ""
RATE = 16000
HOP = 160          # 100 frames/s
NFFT = 1024
_win = np.hanning(NFFT).astype(np.float32)
_freqs = np.fft.rfftfreq(NFFT, 1 / RATE)
# 24 log-spaced bands 80 Hz - 7.5 kHz
edges = np.geomspace(80, 7500, 25)
_bands = np.zeros((24, len(_freqs)), np.float32)
for b in range(24):
    _bands[b, (_freqs >= edges[b]) & (_freqs < edges[b + 1])] = 1


def spec(x):
    if len(x) < NFFT:
        x = np.pad(x, (0, NFFT - len(x)))
    n = 1 + (len(x) - NFFT) // HOP
    idx = np.arange(NFFT)[None, :] + HOP * np.arange(n)[:, None]
    p = np.abs(np.fft.rfft(x[idx] * _win, axis=1)) ** 2
    return np.log10(p @ _bands.T + 1e-10)


def active(s):
    # First contiguous loud region (gaps up to 0.3 s), so the quiet Game Select ambience after a jingle is excluded.
    e = s.max(axis=1)
    thr = e.max() - 2.0
    on = np.nonzero(e > thr)[0]
    if not len(on):
        return 0, 0
    a = b = on[0]
    for i in on[1:]:
        if i - b > 30:
            break
        b = i
    return a, b + 1


def norm_rows(s):
    s = s - s.mean(axis=1, keepdims=True)
    return s / (np.linalg.norm(s, axis=1, keepdims=True) + 1e-9)


def decode(path, seconds=None):
    cmd = ["ffmpeg", "-v", "error", "-i", path] + (["-t", str(seconds)] if seconds else []) + ["-ac", "1", "-ar", str(RATE), "-f", "f32le", "-"]
    return np.frombuffer(subprocess.run(cmd, stdout=subprocess.PIPE, check=True).stdout, dtype=np.float32)


def prep(x):
    s = spec(x)
    a, b = active(s)
    seg = s[a:b]
    env = seg.max(axis=1)
    return norm_rows(seg), (env - env.mean()) / (env.std() + 1e-9), b - a


def score(r, m):
    (rs, re, rn), (ms, me, mn) = r, m
    if rn < 5 or mn < 5:
        return 0.0, 0
    # Durations must be close (a jingle is a fixed length).
    ratio = min(rn, mn) / max(rn, mn)
    best = (-9.0, 0)
    for lag in range(-15, 16):
        a0, b0 = max(0, -lag), max(0, lag)
        n = min(rn - a0, mn - b0)
        if n < 5:
            continue
        sp = float((rs[a0:a0 + n] * ms[b0:b0 + n]).sum(axis=1).mean())
        en = float(np.dot(re[a0:a0 + n], me[b0:b0 + n]) / n)
        v = 0.6 * sp + 0.4 * en
        if v > best[0]:
            best = (v, lag)
    return best[0] * ratio ** 0.5, best[1]


if __name__ == "__main__":
    out = sys.argv[1]
    shorts = []
    for p in sorted(glob.glob(os.path.join(ALBUM, "*.mp3"))):
        x = decode(p, 25)
        if len(x) < 25 * RATE - RATE:  # under ~24 s
            shorts.append((os.path.basename(p), prep(x)))
    print(len(shorts), "short album files", flush=True)
    with open(out, "w", encoding="utf-8") as f:
        for d in sys.argv[2:]:
            for w in sorted(glob.glob(os.path.join(d, "*_whole.wav"))):
                raw = np.frombuffer(open(w, "rb").read()[44:], dtype=np.float32).reshape(-1, 2).mean(1)
                x = raw[:len(raw) // 3 * 3].reshape(-1, 3).mean(1).astype(np.float32)
                if np.sqrt((x ** 2).mean()) < 1e-4:
                    f.write(f"{os.path.basename(w)}\tSILENT\n"); continue
                r = prep(x)
                res = sorted(((score(r, m) + (n,)) for n, m in shorts), reverse=True)[:3]
                f.write(f"{os.path.basename(w)}\t{r[2] / 100:.2f}s\t" + "\t".join(f"{s:.3f} {lag / 100:+.2f} {n}" for s, lag, n in res) + "\n")
                f.flush()
