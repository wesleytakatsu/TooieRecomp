"""Check the in-game recordings made with the pack enabled against the mapping.

    verify_e2e.py <mapping.json> <render-e2e dir> <mp3 cache>
For each entry: the file the mapping selects for (sequence, mask, bpm) must be the best match of the recording.
"""
import glob, json, os, re, sys
import numpy as np
import match as M

mapping = json.load(open(sys.argv[1], encoding="utf-8"))
rdir = sys.argv[2]
cache = M.load_cache(sys.argv[3])
by_name = {n: c for n, c, _ in cache}
rules = []
for t in mapping["tracks"]:
    for r in t["rules"]:
        rules.append((r["id"], r.get("channel_value"), t.get("bpm"), t["file"]))


def expected(seq, mask, bpm):
    for s, value, b, f in rules:
        if s != seq:
            continue
        if value is None and mask is None:
            return f
        if value is not None and mask == value and (b is None or bpm is None or abs(b - bpm) < 0.5):
            return f
    return None


plan = [l.rstrip("\n").split("\t") for l in open(os.path.join(rdir, "plan.tsv"), encoding="utf-8") if l.strip()]
ok = bad = native = 0
for i, (name, seq, mask, sec, *rest) in enumerate(plan):
    if name == "warmup":
        continue
    wavs = glob.glob(os.path.join(rdir, f"{i:03d}_*.wav"))
    if not wavs:
        print("MISSING", name); bad += 1; continue
    seqv = int(seq, 16)
    maskv = None if mask == "-" else int(mask, 16)
    bpm = float(rest[0]) if rest else None
    want = expected(seqv, maskv, bpm)
    r, w = M.render_features(wavs[0])
    if w.sum() < 10:
        print("SILENT", name, want); bad += 1; continue
    res = sorted((M.best_lag(r, w, c) + (n,) for n, c, _ in cache), reverse=True)
    top_s, top_lag, top = res[0]
    if want is None:
        native += 1
        print(f"NATIVE\t{name}\tbest={top} {top_s:.3f}")
        continue
    ws, wl = M.best_lag(r, w, by_name[want])
    good = (top.startswith(want) or abs(ws - top_s) < 0.005) and ws >= 0.9 and abs(wl) <= 25
    ok += good; bad += not good
    print(f"{'OK' if good else 'BAD'}\t{name}\twant={want} {ws:.3f} lag={wl / 50:+.2f}\tbest={top} {top_s:.3f}")
print(f"ok={ok} bad={bad} native={native}")
