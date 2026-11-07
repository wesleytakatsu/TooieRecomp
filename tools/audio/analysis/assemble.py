"""Assemble the album -> game mapping from the render matches, jingle matches and loops.

    assemble.py <out-mapping.json> <report.tsv>
Inputs in this folder: m1.npz (render-full 0..91), m2.npz (render-full2), mt.npz (render-tempo2), jingles2.tsv,
loops_all.tsv; album manifest from the album folder.
"""
import json, os, re, struct, sys
import numpy as np

S = os.path.dirname(os.path.abspath(__file__))
ALBUM = os.environ["TOOIE_ALBUM"].rstrip("/\\") + ""
ROM = os.environ["TOOIE_ROM_IMAGE"]  # decompressed USA image (35098224 bytes)
RATE = 48000
SONG_MIN, SONG_REVIEW = 0.80, 0.70
LAYER_MIN = 0.80
JINGLE_MIN = 0.65
# Decided by elimination where the jingle matcher is unsure (see the report).
OVERRIDES = {0x6F: "112. Equip Ice Eggs.mp3"}
LONG = {0x7B: "04. Hag Overture.mp3", 0x7C: "187. Hag 1 Destroyed.mp3", 0x88: "18. An Evil Plan.mp3", 0x8C: "22. An Evil Plan Revealed.mp3"}  # 70 s and 45 s renders
SKIP = {0x46}  # unnamed, ambiguous between two short effects: keep the original

manifest = json.load(open(os.path.join(ALBUM, "download-manifest.json"), encoding="utf-8-sig"))
sha = {f["filename"]: f["sha256"] for f in manifest["files"]}
frames_of = {}
loops = {}
for line in open(os.path.join(S, "loops_best.tsv"), encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 7 and p[2] != "None":
        loops[p[0]] = dict(frames=int(p[1]), start=int(p[2]), end=int(p[3]), seconds=float(p[4]), score=float(p[5]), refine=float(p[6]))
        frames_of[p[0]] = int(p[1])
    elif len(p) >= 2 and p[1].isdigit():
        frames_of[p[0]] = int(p[1])

rom = open(ROM, "rb").read()
def preset(i):
    return struct.unpack_from(">IIHH", rom, 0x1EFD774 + 12 * i)

report = []
rules = {}      # file -> list of rule dicts
contract = {}   # file -> bounds dict

def add(file, rule, bounds, why):
    if file in contract and contract[file] != bounds:
        report.append(("CONFLICT", file, json.dumps(rule), why))
        return False
    contract[file] = bounds
    rules.setdefault(file, []).append(rule)
    report.append(("MAP", file, json.dumps(rule), why))
    return True

def song_bounds(file, bpm=None):
    lp = loops.get(file)
    b = {}
    if lp and lp["score"] >= 0.9 and lp["refine"] >= 0.7 and lp["seconds"] >= 8:
        b = {"loop_start_frame": lp["start"], "loop_end_frame": lp["end"], "end_frame": lp["end"]}
    if bpm:
        b["bpm"] = bpm
    return b

# 1. Area variants of layered sequences (masks and tempos from the presets), from the tempo renders.
mt = np.load(os.path.join(S, "mt.npz"), allow_pickle=True)
names = list(mt["names"])
families = {}
for i, entry in enumerate(mt["entries"]):
    m = re.match(r"s([0-9A-F]{2})_p([0-9A-F]{2})([AB])_([0-9A-F]{8})_(\d+)", str(entry))
    seq, p, tag, mask, bpm = int(m[1], 16), int(m[2], 16), m[3], int(m[4], 16), int(m[5])
    order = np.argsort(-mt["scores"][i])
    j, j2 = int(order[0]), int(order[1])
    score = float(mt["scores"][i, j])
    margin = score - float(mt["scores"][i, j2])
    file = LONG.get((seq, mask), names[j])
    # Accepted below LAYER_MIN only when no other file comes close.
    if score < LAYER_MIN and score >= 0.70 and margin >= 0.30:
        score = LAYER_MIN
    families.setdefault(seq, []).append((score, file, mask, bpm, str(entry)))
# Preset 0x46 (Jinjo Village drained, 0xB000 at 75 BPM): separate 30 s render, 0.963 against track 24.
families.setdefault(0x59, []).append((0.963, "24. Jinjo Village (Drained Jingaling's Palace).mp3", 0xB000, 75, "s59_p46A_0000B000_75"))
for seq, members in sorted(families.items()):
    good = [x for x in members if x[0] >= LAYER_MIN]
    for score, file, mask, bpm, entry in members:
        if score < LAYER_MIN:
            report.append(("NATIVE", entry, f"{score:.3f}", file))
    if not good:
        continue
    # Family loop in beats: the length most members agree on (each converted from its own tempo), then the best
    # refined member's exact value; ties go to the longer loop.
    cands = []
    for score, file, mask, bpm, entry in good:
        lp = loops.get(file)
        if lp and lp["score"] >= 0.78 and lp["refine"] >= 0.6:
            cands.append((lp["seconds"] * bpm / 60, lp["refine"], file))
    if not cands:
        report.append(("NOLOOP", f"seq {seq:#x}", "", ""))
        continue
    def support(c):
        return sum(1 for o in cands if abs(o[0] - c[0]) < 0.1)
    top = max(support(c) for c in cands)
    agreeing = [c for c in cands if support(c) == top]
    length = max(c[0] for c in agreeing)
    chosen = max((c for c in agreeing if abs(c[0] - length) < 0.1), key=lambda c: c[1])
    beats_len, _, ref = chosen[0], None, chosen[2]
    report.append(("FAMILY", f"seq {seq:#x}", f"loop={beats_len:.4f} beats support={top}/{len(cands)}", ref))
    beats_start = max(loops[f]["start"] / RATE * b / 60 for _, f, _, b, _ in good if f in loops) + 0.5
    for score, file, mask, bpm, entry in sorted(good, reverse=True):
        f = lambda beats: int(round(beats * 60 / bpm * RATE))
        bounds = {"bpm": bpm, "loop_start_frame": f(beats_start), "loop_end_frame": f(beats_start + beats_len),
                  "end_frame": f(beats_start + beats_len)}
        # Channels loop on their own: a reliable, long loop measured in this file wins over the family's.
        lp = loops.get(file)
        if lp and lp["score"] >= 0.9 and lp["refine"] >= 0.8 and lp["seconds"] * bpm / 60 >= 0.5 * beats_len:
            bounds.update({"loop_start_frame": lp["start"], "loop_end_frame": lp["end"], "end_frame": lp["end"]})
        if bounds["end_frame"] > frames_of.get(file, 0):
            report.append(("TOO_SHORT", entry, file, json.dumps(bounds)))
            continue
        lp = loops.get(file)
        own = lp["seconds"] * bpm / 60 if lp else 0
        agree = lp and abs(own - beats_len) < 0.05 or (lp and own > 1 and abs(beats_len / own - round(beats_len / own)) < 0.01)
        add(file, {"id": seq, "channel_mask": 0xFFFFFFFF, "channel_value": mask, "native_channel_mask": mask}, bounds,
            f"{entry} score={score:.3f} family_loop={beats_len:.3f} beats ref={ref} own_loop_agrees={bool(agree)}")

# 2. Whole sequences (songs) from the chroma matrices; 3. jingles from the onset-aligned matcher.
jingle = {}
for line in open(os.path.join(S, "jingles2.tsv"), encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    m = re.match(r"\d+_s([0-9A-F]{2})_whole", p[0])
    if m and len(p) > 2:
        s, lag, f = p[2].split(" ", 2)
        jingle[int(m[1], 16)] = (float(s), f, float(p[1][:-1]))
for npz in ("m1.npz", "m2.npz"):
    z = np.load(os.path.join(S, npz), allow_pickle=True)
    for i, entry in enumerate(z["entries"]):
        m = re.match(r"s([0-9A-F]{2})_whole", str(entry))
        if not m:
            continue
        seq = int(m[1], 16)
        if seq in families or seq in SKIP:
            continue
        j = int(np.argmax(z["scores"][i])); score = float(z["scores"][i, j]); file = str(z["names"][j])
        js = jingle.get(seq)
        if seq in OVERRIDES:
            add(OVERRIDES[seq], {"id": seq}, {}, "override by elimination (equip-egg series)")
        elif seq in LONG:
            add(LONG[seq], {"id": seq}, song_bounds(LONG[seq]), "decided by a long render (files that start alike)")
        elif js and js[2] < 15 and js[0] >= JINGLE_MIN:
            add(js[1], {"id": seq}, {}, f"jingle score={js[0]:.3f} active={js[2]}s")
        elif score >= SONG_MIN:
            add(file, {"id": seq}, song_bounds(file), f"song score={score:.3f}")
        elif score >= SONG_REVIEW:
            report.append(("REVIEW", f"seq {seq:#x}", f"{score:.3f}", file))
        else:
            report.append(("NATIVE", f"seq {seq:#x}", f"{score:.3f}", file))

tracks = []
for file in sorted(rules, key=lambda f: int(f.split(".")[0])):
    t = {"file": file, "sha256": sha[file], **contract[file], "rules": rules[file]}
    tracks.append(t)
mapping = {
    "album": manifest["album"], "album_folder": "Banjo-Tooie HD",
    "mod": {"game_id": "bt", "id": "tooie_hd_audio", "display_name": "Banjo-Tooie HD Audio", "version": "1.0.0",
            "minimum_recomp_version": "0.1.6", "authors": ["Grant Kirkhope", "L. Spiro (HD restore)"],
            "description": "HD soundtrack: every area variant, tempo and jingle mapped to the game's own sequences.",
            "short_description": "HD soundtrack"},
    "tracks": tracks,
}
json.dump(mapping, open(sys.argv[1], "w", encoding="utf-8"), indent=1)
with open(sys.argv[2], "w", encoding="utf-8") as f:
    for r in report:
        f.write("\t".join(r) + "\n")
unused = sorted(set(sha) - set(rules), key=lambda f: int(f.split(".")[0]))
print(len(tracks), "files mapped,", sum(len(r) for r in rules.values()), "rules;", len(unused), "album files unused")
print("unused:", "; ".join(unused))
