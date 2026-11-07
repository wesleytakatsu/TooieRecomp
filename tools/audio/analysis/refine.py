"""Refine each mapped loop end on the waveform (+-20 ms, several windows) and report the seam quality.

    refine.py <mapping.json> <out.json> <report.tsv>
"""
import json, os, subprocess, sys
import numpy as np

ALBUM = os.environ["TOOIE_ALBUM"].rstrip("/\\") + "/"
SEARCH = 2400  # 50 ms
WIN = 24000


def decode(path):
    cmd = ["ffmpeg", "-v", "error", "-i", path, "-ac", "1", "-ar", "48000", "-f", "f32le", "-"]
    return np.frombuffer(subprocess.run(cmd, stdout=subprocess.PIPE, check=True).stdout, dtype=np.float32)


def seam(x, a, b):
    vals = []
    for off in (0, 48000, 96000, 144000, 192000, 240000):
        sa, sb = x[a + off:a + off + WIN], x[b + off:b + off + WIN]
        if a + off < 0 or len(sa) < WIN or len(sb) < WIN:
            continue
        vals.append(float(np.dot(sa, sb) / (np.linalg.norm(sa) * np.linalg.norm(sb) + 1e-9)))
    return (float(np.mean(vals)), float(min(vals))) if vals else (0.0, 0.0)


m = json.load(open(sys.argv[1], encoding="utf-8"))
rows = []
for t in m["tracks"]:
    if "loop_end_frame" not in t:
        continue
    x = decode(ALBUM + t["file"])
    a, b = t["loop_start_frame"], t["loop_end_frame"]
    before = seam(x, a, b)
    total = None
    for off in (0, 48000, 96000, 144000, 192000, 240000):
        seg = x[a + off:a + off + WIN]
        ref = x[b + off - SEARCH:b + off + SEARCH + WIN]
        if len(seg) < WIN or len(ref) < 2 * SEARCH + WIN:
            continue
        c = np.correlate(ref, seg, mode="valid")
        e = np.sqrt(np.convolve(ref * ref, np.ones(WIN), mode="valid")) * np.linalg.norm(seg) + 1e-9
        total = c / e if total is None else total + c / e
    shift = int(np.argmax(total)) - SEARCH if total is not None else 0
    nb = b + shift
    if nb > len(x):
        nb = b
        shift = 0
    after = seam(x, a, nb)
    if after[0] > before[0]:
        t["loop_end_frame"] = nb
        t["end_frame"] = max(nb, t.get("end_frame", 0)) if t.get("end_frame", 0) <= b else t["end_frame"]
        if t.get("end_frame", 0) < nb:
            t["end_frame"] = nb
        if t["end_frame"] == b:
            t["end_frame"] = nb
    rows.append((after[0] if after[0] > before[0] else before[0], before[0], shift, t["file"]))
    print(f"{rows[-1][0]:.3f}\t{before[0]:.3f}\t{shift:+d}\t{t['file']}", flush=True)
json.dump(m, open(sys.argv[2], "w", encoding="utf-8"), indent=1)
with open(sys.argv[3], "w", encoding="utf-8") as f:
    for r in sorted(rows):
        f.write(f"{r[0]:.3f}\t{r[1]:.3f}\t{r[2]:+d}\t{r[3]}\n")
