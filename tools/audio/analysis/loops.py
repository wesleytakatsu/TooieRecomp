"""Find the repeat (loop) of a gamerip MP3: the rip plays the sequence, its loop again, then fades.

    loops.py <mp3...>  -> TSV: file, frames, loop_start, loop_end, length_s, similarity, refine_corr
Frames are 48 kHz PCM frames of the decoded file.
"""
import os, subprocess, sys
import numpy as np
import features as F
import onset as O

MIN_LOOP_S = 6.0


def decode48(path):
    cmd = ["ffmpeg", "-v", "error", "-i", path, "-ac", "1", "-ar", "48000", "-f", "f32le", "-"]
    return np.frombuffer(subprocess.run(cmd, stdout=subprocess.PIPE, check=True).stdout, dtype=np.float32)


def centered(c):
    c = c - c.mean(axis=1, keepdims=True)
    return c / (np.linalg.norm(c, axis=1, keepdims=True) + 1e-9)


def find(path):
    x16 = F.decode(path, 1200)
    c, loud = F.chroma(x16)
    c = centered(c)
    on = O.onset(x16)
    n = min(len(c), len(on))
    c, on, loud = c[:n], on[:n], loud[:n]
    fps = 50
    # Coarse search at 10 frames/s. A loop of length L must hold over a window of 0.8 L; any multiple of the true loop
    # is also a valid loop, so take the longest L that holds (short repeated phrases are not the song's loop).
    k = 5
    m = n // k
    cc = centered(c[:m * k].reshape(m, k, 12).mean(axis=1))
    scored = []
    for Lq in range(int(MIN_LOOP_S * 10), int(m * 0.62)):
        w = int(0.7 * Lq)
        if m - Lq < w:
            break
        sim = (cc[:m - Lq] * cc[Lq:]).sum(axis=1)
        cs = np.concatenate([[0], np.cumsum(sim)])
        win = (cs[w:] - cs[:-w]) / w
        i = int(np.argmax(win))
        scored.append((float(win[i]), Lq, i))
    best = None
    if scored:
        top = max(s for s, _, _ in scored)
        good = [x for x in scored if x[0] >= max(0.9, top - 0.03) or (x[1] >= 400 and x[0] >= 0.78)]
        if good:
            _, Lq, iq = max(good, key=lambda x: x[1])
            fine = None
            for L in range(Lq * k - 8, Lq * k + 9):
                w = int(0.7 * L)
                if n - L < w:
                    continue
                sim = (c[:n - L] * c[L:]).sum(axis=1)
                cs = np.concatenate([[0], np.cumsum(sim)])
                win = (cs[w:] - cs[:-w]) / w
                i = int(np.argmax(win))
                if fine is None or win[i] > fine[0]:
                    fine = (float(win[i]), L, i)
            best = fine
    if best is None:
        return None
    score, L, start = best
    # Earliest frame from which the repeat holds: walk back while similarity stays high.
    a, b = c[:n - L], c[L:]
    sim = (a * b).sum(axis=1)
    smooth = np.convolve(sim, np.ones(25) / 25, "same")
    s = start
    while s > 0 and smooth[s - 1] > 0.8:
        s -= 1
    # Sample-accurate loop length from the waveform: several 2 s windows across the loop vote, so random in-sequence
    # effects in one window do not move the result.
    x = decode48(path)
    a0 = int((s / fps + 1.0) * 48000)
    Lc = int(L / fps * 48000)
    win = 96000
    lo, hi = Lc - 2400, Lc + 2400
    total = None
    count = 0
    for k in range(6):
        p = a0 + int(k * (Lc - win) / 6)
        seg = x[p:p + win]
        ref = x[p + lo:p + hi + win]
        if len(seg) < win or len(ref) < hi - lo + win:
            continue
        corr = np.correlate(ref, seg, mode="valid")
        energy = np.sqrt(np.convolve(ref * ref, np.ones(win), mode="valid")) * np.linalg.norm(seg) + 1e-9
        c = corr / energy
        total = c if total is None else total + c
        count += 1
    if total is None:
        return (len(x), None, None, L / fps, score, 0.0)
    k = int(np.argmax(total))
    Lexact = lo + k
    refine = float(total[k] / count)
    loop_start = a0
    loop_end = a0 + Lexact
    return (len(x), loop_start, loop_end, Lexact / 48000, score, refine)


if __name__ == "__main__":
    for p in sys.argv[1:]:
        try:
            r = find(p)
        except Exception as e:
            print(f"{os.path.basename(p)}	ERROR {e}", flush=True)
            continue
        if r is None:
            print(f"{os.path.basename(p)}\tNONE", flush=True)
        else:
            frames, ls, le, length, score, refine = r
            print(f"{os.path.basename(p)}\t{frames}\t{ls}\t{le}\t{length:.4f}\t{score:.3f}\t{refine:.4f}", flush=True)
