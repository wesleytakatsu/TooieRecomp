"""Score matrix renders x album files (chroma, best lag), saved as matrix.npz, plus best render per file.

    matrix.py <render-dir> <mp3-cache> <out-prefix>
"""
import glob, os, sys
import numpy as np
import match as M

render_dir, cache_dir, out = sys.argv[1:4]
cache = M.load_cache(cache_dir)
names = [n for n, _, _ in cache]
plan = [l.rstrip("\n").split("\t") for l in open(os.path.join(render_dir, "plan.tsv"), encoding="utf-8") if l.strip()]
plan = plan[:int(os.environ.get("MATRIX_LIMIT", len(plan)))]
scores = np.zeros((len(plan), len(cache)), dtype=np.float32)
lags = np.zeros((len(plan), len(cache)), dtype=np.float32)
rms = np.zeros(len(plan), dtype=np.float32)
for i in range(len(plan)):
    wavs = glob.glob(os.path.join(render_dir, f"{i:03d}_*.wav"))
    if not wavs:
        continue
    x = np.frombuffer(open(wavs[0], "rb").read()[44:], dtype=np.float32)
    rms[i] = float(np.sqrt((x ** 2).mean())) if len(x) else 0
    if rms[i] < 1e-4:
        continue
    r, w = M.render_features(wavs[0])
    if w.sum() < 10:
        continue
    for j, (_, c, _) in enumerate(cache):
        s, lag = M.best_lag(r, w, c)
        scores[i, j] = s
        lags[i, j] = lag / 50
    print(i, plan[i][0], names[int(np.argmax(scores[i]))], f"{scores[i].max():.3f}", flush=True)
np.savez(out + ".npz", scores=scores, lags=lags, rms=rms, names=np.array(names), entries=np.array([p[0] for p in plan]))
with open(out + "_byfile.tsv", "w", encoding="utf-8") as f:
    f.write("file\tbest\tscore\tlag\tsecond\tscore2\n")
    for j, n in enumerate(names):
        order = np.argsort(-scores[:, j])
        a, b = order[0], order[1]
        f.write(f"{n}\t{plan[a][0]}\t{scores[a, j]:.3f}\t{lags[a, j]:.2f}\t{plan[b][0]}\t{scores[b, j]:.3f}\n")
