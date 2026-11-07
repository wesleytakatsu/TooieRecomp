"""For loops whose seam is weak, move the loop start later (keeping the loop length) and refine the end.

    restart.py <mapping.json> <out.json> <threshold>
"""
import json, os, subprocess, sys
import numpy as np

ALBUM = os.environ["TOOIE_ALBUM"].rstrip("/\\") + "/"
WIN = 24000


def decode(path):
    cmd = ["ffmpeg", "-v", "error", "-i", path, "-ac", "1", "-ar", "48000", "-f", "f32le", "-"]
    return np.frombuffer(subprocess.run(cmd, stdout=subprocess.PIPE, check=True).stdout, dtype=np.float32)


def seam(x, a, b):
    vals = []
    for off in (0, 48000, 96000, 144000, 192000, 240000):
        sa, sb = x[a + off:a + off + WIN], x[b + off:b + off + WIN]
        if len(sa) < WIN or len(sb) < WIN:
            continue
        vals.append(float(np.dot(sa, sb) / (np.linalg.norm(sa) * np.linalg.norm(sb) + 1e-9)))
    return float(np.mean(vals)) if len(vals) >= 3 else -1.0


def fine_end(x, a, b, search=2400):
    total = None
    for off in (0, 48000, 96000, 144000, 192000, 240000):
        seg = x[a + off:a + off + WIN]
        ref = x[b + off - search:b + off + search + WIN]
        if len(seg) < WIN or len(ref) < 2 * search + WIN:
            continue
        c = np.correlate(ref, seg, mode="valid")
        e = np.sqrt(np.convolve(ref * ref, np.ones(WIN), mode="valid")) * np.linalg.norm(seg) + 1e-9
        total = c / e if total is None else total + c / e
    return b + (int(np.argmax(total)) - search if total is not None else 0)


if __name__ == "__main__":
    m = json.load(open(sys.argv[1], encoding="utf-8"))
    threshold = float(sys.argv[3])
    for t in m["tracks"]:
        if "loop_end_frame" not in t:
            continue
        x = decode(ALBUM + t["file"])
        a, b = t["loop_start_frame"], t["loop_end_frame"]
        base = seam(x, a, b)
        if base >= threshold:
            continue
        length = b - a
        best = (base, a, b)
        for step in range(1, 41):
            na = a + int(step * length / 80)  # up to half a loop later
            nb = fine_end(x, na, na + length)
            if nb + 240000 + WIN > len(x):
                break
            s = seam(x, na, nb)
            if s > best[0]:
                best = (s, na, nb)
        s, na, nb = best
        if na != a:
            t["loop_start_frame"], t["loop_end_frame"] = na, nb
            t["end_frame"] = max(t.get("end_frame", nb), nb)
        print(f"{base:.3f} -> {s:.3f}\tstart {a} -> {na}\t{t['file']}", flush=True)
    json.dump(m, open(sys.argv[2], "w", encoding="utf-8"), indent=1)
