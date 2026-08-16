#!/usr/bin/env python3
"""Join original + recomp Wooded Hollow traces by consumed_glidx and emit a numeric differential.

Original CSV: tools/mupen64plus_demo_trace.c / original_demo_trace_bizhawk.lua
Recomp log: TOOIE_DIAG_DEMO_SYNC=1 stderr ([demo-sync] lines)
"""
from __future__ import annotations

import csv
import math
import re
import sys
from pathlib import Path

LANDMARKS = (35, 74, 76, 91, 120, 200, 215, 236, 261, 282, 322, 334, 365, 440, 550, 765)
DRIFT_POINTS = (35, 91, 200, 322, 365, 550, 765)

# Exact-bit / numeric gate vs gameplay-meaningful gate.
# Host-FPU / emulator COP1 rounding on N64 floats is typically << 1 unit.
# A gameplay-visible walk error is tens of units (first-straight is ~510 u/s).
EXACT_POS = 1e-3          # 0.001 world units: treat as first numeric difference
MEANINGFUL_POS = 2.0      # 2 units (~4 mm in Tooie world scale) is visible path error
EXACT_YAW = 0.05          # degrees
MEANINGFUL_YAW = 2.0
EXACT_SPD = 0.05
MEANINGFUL_SPD = 2.0


def ang_delta(a: float, b: float) -> float:
    d = (a - b + 180.0) % 360.0 - 180.0
    return abs(d)


def parse_recomp(path: str, max_glidx: int | None = 765) -> dict[int, dict]:
    pat = re.compile(
        r"\[demo-sync\] glidx=(?P<glidx>-?\d+)(?: post=(?P<post>-?\d+))? "
        r"tick=(?P<tick>\d+) wall_ms=(?P<wall_ms>\d+) gfx=(?P<gfx>\d+) adv=(?P<adv>-?\d+) "
        r"pad=\((?P<padx>-?\d+),(?P<pady>-?\d+),(?P<padbtn>[0-9a-fA-F]+)\) "
        r"remap=\((?P<rmx>[-\d.]+),(?P<rmy>[-\d.]+)\) "
        r"bstick=\((?P<bsx>[-\d.]+),(?P<bsy>[-\d.]+),(?P<bsang>[-\d.]+),(?P<bsdist>[-\d.]+)\) "
        r"bs=(?P<bs>-?\d+) pos=\((?P<px>[-\d.]+),(?P<py>[-\d.]+),(?P<pz>[-\d.]+)\) "
        r"ppos=\((?P<ppx>[-\d.]+),(?P<ppy>[-\d.]+),(?P<ppz>[-\d.]+)\) "
        r"vel=\((?P<vx>[-\d.]+),(?P<vy>[-\d.]+),(?P<vz>[-\d.]+)\) velxz=(?P<velxz>[-\d.]+) "
        r"tspd=(?P<tspd>[-\d.]+) yaw=(?P<yaw>[-\d.]+) ideal=(?P<yaw_ideal>[-\d.]+) "
        r"phys_yaw=(?P<phys_yaw>[-\d.]+) mult=(?P<mult>[-\d.]+) accel=(?P<accel>[-\d.]+) "
        r"cam=\((?P<cx>[-\d.]+),(?P<cy>[-\d.]+),(?P<cz>[-\d.]+),(?P<cyaw>[-\d.]+)\) "
        r"digger=\((?P<dgx>[-\d.]+),(?P<dgy>[-\d.]+),(?P<dgz>[-\d.]+)\) "
        r"act2=\((?P<a2mid>\d+),(?P<a2sys>[0-9a-fA-F]+),(?P<a2x>[-\d.]+),(?P<a2y>[-\d.]+),(?P<a2z>[-\d.]+)\) "
        r"gs=(?P<gs>[-\d.]+) fc=(?P<fc>\d+) cnt=(?P<cnt>\d+)"
    )
    out: dict[int, dict] = {}
    prev = -1
    with open(path, "r", errors="replace") as f:
        for line in f:
            if "[demo-sync]" not in line:
                continue
            m = pat.search(line)
            if not m:
                continue
            d = m.groupdict()
            rec = {
                "glidx": int(d["glidx"]),
                "post": int(d["post"]) if d["post"] is not None else int(d["glidx"]) + 1,
                "tick": int(d["tick"]),
                "wall_ms": int(d["wall_ms"]),
                "gfx": int(d["gfx"]),
                "adv": int(d["adv"]),
                "padx": int(d["padx"]),
                "pady": int(d["pady"]),
                "padbtn": int(d["padbtn"], 16),
                "bs": int(d["bs"]),
                "px": float(d["px"]),
                "py": float(d["py"]),
                "pz": float(d["pz"]),
                "ppx": float(d["ppx"]),
                "ppy": float(d["ppy"]),
                "ppz": float(d["ppz"]),
                "vx": float(d["vx"]),
                "vy": float(d["vy"]),
                "vz": float(d["vz"]),
                "velxz": float(d["velxz"]),
                "tspd": float(d["tspd"]),
                "yaw": float(d["yaw"]),
                "ideal": float(d["yaw_ideal"]),
                "phys_yaw": float(d["phys_yaw"]),
                "mult": float(d["mult"]),
                "accel": float(d["accel"]),
                "cx": float(d["cx"]),
                "cy": float(d["cy"]),
                "cz": float(d["cz"]),
                "cyaw": float(d["cyaw"]),
                "dgx": float(d["dgx"]),
                "dgy": float(d["dgy"]),
                "dgz": float(d["dgz"]),
                "a2mid": int(d["a2mid"]),
                "a2sys": d["a2sys"],
                "a2x": float(d["a2x"]),
                "a2y": float(d["a2y"]),
                "a2z": float(d["a2z"]),
                "fc": int(d["fc"]),
                "cnt": int(d["cnt"]),
            }
            g = rec["glidx"]
            if max_glidx is not None and g > max_glidx:
                break
            if g not in out:
                out[g] = rec
            elif g < max(out):
                break
    return out


def parse_original_csv(path: str, max_glidx: int | None = 765) -> dict[int, dict]:
    out: dict[int, dict] = {}
    prev = -1
    with open(path, newline="") as f:
        r = csv.DictReader(f)
        for row in r:
            rec = {
                "glidx": int(row["consumed_glidx"]),
                "post": int(row["post_glidx"]),
                "vi": int(row.get("vi_count") or 0),
                "gfcnt": int(row["gfcnt"]) if row.get("gfcnt") not in (None, "") else None,
                "mode": int(row["mode"]) if row.get("mode") not in (None, "") else None,
                "map": int(row["map"]) if row.get("map") not in (None, "") else None,
                "padx": int(row["pad_x"]),
                "pady": int(row["pad_y"]),
                "padbtn": int(row["pad_btn"], 16) if isinstance(row["pad_btn"], str) else int(row["pad_btn"]),
                "bs": int(row["bs"]) if row.get("bs") not in (None, "") else None,
                "px": float(row["px"]),
                "py": float(row["py"]),
                "pz": float(row["pz"]),
                "ppx": float(row["ppx"]) if row.get("ppx") not in (None, "") else None,
                "ppy": float(row["ppy"]) if row.get("ppy") not in (None, "") else None,
                "ppz": float(row["ppz"]) if row.get("ppz") not in (None, "") else None,
                "vx": float(row["vx"]),
                "vy": float(row["vy"]),
                "vz": float(row["vz"]),
                "velxz": float(row["velxz"]),
                "tspd": float(row["tspd"]),
                "yaw": float(row["yaw"]),
                "ideal": float(row["ideal_yaw"]) if row.get("ideal_yaw") not in (None, "") else None,
                "phys_yaw": float(row["phys_yaw"]) if row.get("phys_yaw") not in (None, "") else None,
                "mult": float(row["mult"]) if row.get("mult") not in (None, "") else None,
                "accel": float(row["accel"]) if row.get("accel") not in (None, "") else None,
                "cx": float(row["cam_x"]),
                "cy": float(row["cam_y"]),
                "cz": float(row["cam_z"]),
                "cyaw": float(row["cam_yaw"]),
                "dgx": float(row["digger_x"]) if row.get("digger_x") not in (None, "") else None,
                "dgy": float(row["digger_y"]) if row.get("digger_y") not in (None, "") else None,
                "dgz": float(row["digger_z"]) if row.get("digger_z") not in (None, "") else None,
                "a2mid": int(row["act2_mid"]) if row.get("act2_mid") not in (None, "") else None,
                "a2sys": row.get("act2_sys"),
                "a2x": float(row["act2_x"]) if row.get("act2_x") not in (None, "") else None,
                "a2y": float(row["act2_y"]) if row.get("act2_y") not in (None, "") else None,
                "a2z": float(row["act2_z"]) if row.get("act2_z") not in (None, "") else None,
            }
            g = rec["glidx"]
            if max_glidx is not None and g > max_glidx:
                break
            if prev >= 0 and g < prev:
                break
            if g not in out:
                out[g] = rec
            prev = g
    return out


def pos_err(a, b, ax="px", ay="py", az="pz"):
    if a.get(ax) is None or b.get(ax) is None:
        return None, None, None, None, None
    dx = b[ax] - a[ax]
    dy = b[ay] - a[ay]
    dz = b[az] - a[az]
    xz = math.hypot(dx, dz)
    d3 = math.sqrt(dx * dx + dy * dy + dz * dz)
    return dx, dy, dz, xz, d3


def classify_drift(errs: list[tuple[int, float]]) -> str:
    if not errs:
        return "UNKNOWN"
    xs = [e[1] for e in errs]
    if max(xs) < MEANINGFUL_POS:
        return "NONE"
    # constant offset if later-early ~ 0
    early = xs[0]
    late = xs[-1]
    mid = xs[len(xs) // 2]
    growth = late - early
    if abs(growth) < MEANINGFUL_POS and early >= MEANINGFUL_POS:
        return "CONSTANT_OFFSET"
    if growth > 10 and (late / max(early, 0.1)) > 3:
        return "LINEAR_SPEED_DRIFT"
    # piecewise if it jumps then plateaus
    jumps = sum(1 for i in range(1, len(xs)) if abs(xs[i] - xs[i - 1]) > 10)
    if jumps >= 2:
        return "PIECEWISE"
    if mid > early + 10 and late < mid - 5:
        return "DECELERATION_DRIFT"
    if late > mid + 10 > early + 5:
        return "ACCELERATION_DRIFT"
    return "MIXED"


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: compare_demo_traces.py ORIGINAL.csv RECOMP.log [summary.md]", file=sys.stderr)
        return 2
    orig_path, recomp_path = sys.argv[1], sys.argv[2]
    out_md = sys.argv[3] if len(sys.argv) > 3 else None

    orig = parse_original_csv(orig_path)
    recp = parse_recomp(recomp_path)
    common = sorted(set(orig) & set(recp))
    pad_match = 0
    pad_fail = []
    for g in common:
        o, r = orig[g], recp[g]
        if o["padx"] == r["padx"] and o["pady"] == r["pady"] and o["padbtn"] == r["padbtn"]:
            pad_match += 1
        else:
            pad_fail.append((g, o["padx"], o["pady"], o["padbtn"], r["padx"], r["pady"], r["padbtn"]))

    print("ORIGINAL_SAMPLE_COUNT:", len(orig))
    print("RECOMP_SAMPLE_COUNT:", len(recp))
    print("COMMON_SAMPLE_COUNT:", len(common))
    print("PAD_SEQUENCE_MATCH_COUNT:", pad_match)
    rate = (pad_match / len(common) * 100.0) if common else 0.0
    print(f"PAD_SEQUENCE_MATCH_RATE: {rate:.4f}%")
    if pad_fail[:8]:
        print("PAD_FAIL_EXAMPLES:", pad_fail[:8])
    if not common:
        print("NO_COMMON_SAMPLES")
        return 1
    if pad_match != len(common):
        print("PAD_IDENTITY_GATE: FAIL — stop before position compare")
        return 1
    print("PAD_IDENTITY_GATE: PASS")

    first_exact = None
    first_mean = None
    first_field = None
    drift_errs = []
    rows = []
    for g in common:
        o, r = orig[g], recp[g]
        dx, dy, dz, xz, d3 = pos_err(o, r)
        cdx, cdy, cdz, cxz, cd3 = pos_err(o, r, "cx", "cy", "cz")
        dspd = r["velxz"] - o["velxz"]
        dtspd = r["tspd"] - o["tspd"]
        dyaw = ang_delta(r["yaw"], o["yaw"])
        dcyaw = ang_delta(r["cyaw"], o["cyaw"])
        state_match = (o.get("bs") is None) or (r.get("bs") is None) or (o["bs"] == r["bs"])
        dg = None
        if o.get("dgx") is not None and r.get("dgx") is not None:
            dg = math.sqrt((r["dgx"] - o["dgx"]) ** 2 + (r["dgy"] - o["dgy"]) ** 2 + (r["dgz"] - o["dgz"]) ** 2)
        a2 = None
        if o.get("a2x") is not None and r.get("a2x") is not None:
            a2 = math.sqrt((r["a2x"] - o["a2x"]) ** 2 + (r["a2y"] - o["a2y"]) ** 2 + (r["a2z"] - o["a2z"]) ** 2)

        rec = dict(g=g, dx=dx, dy=dy, dz=dz, xz=xz, d3=d3, dspd=dspd, dtspd=dtspd,
                   dyaw=dyaw, dcyaw=dcyaw, cxz=cxz, cd3=cd3, state_match=state_match, dg=dg, a2=a2,
                   o=o, r=r)
        rows.append(rec)
        if g in DRIFT_POINTS and xz is not None:
            drift_errs.append((g, xz))

        if first_exact is None:
            reasons = []
            if xz is not None and xz > EXACT_POS:
                reasons.append(f"player_xz={xz:.4f}")
            if abs(dspd) > EXACT_SPD:
                reasons.append(f"speed={dspd:.4f}")
            if dyaw > EXACT_YAW:
                reasons.append(f"yaw={dyaw:.4f}")
            if o.get("bs") is not None and r.get("bs") is not None and o["bs"] != r["bs"]:
                reasons.append(f"state {o['bs']}!={r['bs']}")
            if reasons:
                first_exact = g
                first_field = reasons[0]
        if first_mean is None:
            reasons = []
            if xz is not None and xz > MEANINGFUL_POS:
                reasons.append(f"player_xz={xz:.4f}")
            if abs(dspd) > MEANINGFUL_SPD:
                reasons.append(f"speed={dspd:.4f}")
            if dyaw > MEANINGFUL_YAW:
                reasons.append(f"yaw={dyaw:.4f}")
            if o.get("bs") is not None and r.get("bs") is not None and o["bs"] != r["bs"]:
                reasons.append(f"state {o['bs']}!={r['bs']}")
            if reasons:
                first_mean = g
                if first_field is None:
                    first_field = reasons[0]

    print("FIRST_EXACT_DIFFERENCE_GLIDX:", first_exact)
    print("FIRST_MEANINGFUL_DIVERGENCE_GLIDX:", first_mean)
    print("FIRST_DIVERGENT_FIELD:", first_field)
    print("DRIFT_CLASS:", classify_drift(drift_errs))

    print("\n=== LANDMARK TABLE ===")
    print(f"{'gl':>4} {'o_pos':>32} {'r_pos':>32} {'xz':>8} {'spd_o/r':>15} {'yaw_o/r':>15} {'st':>6} {'cam_xz':>8}")
    for g in LANDMARKS:
        rec = next((x for x in rows if x["g"] == g), None)
        if not rec:
            print(f"{g:4d} MISSING")
            continue
        o, r = rec["o"], rec["r"]
        print(f"{g:4d} ({o['px']:8.1f},{o['py']:7.1f},{o['pz']:8.1f}) "
              f"({r['px']:8.1f},{r['py']:7.1f},{r['pz']:8.1f}) "
              f"{rec['xz']:8.3f} {o['velxz']:6.1f}/{r['velxz']:6.1f} "
              f"{o['yaw']:6.1f}/{r['yaw']:6.1f} "
              f"{str(o.get('bs'))}/{str(r.get('bs')):>3} {rec['cxz'] if rec['cxz'] is not None else float('nan'):8.3f}")

    print("\n=== DRIFT ERRORS ===")
    for g, xz in drift_errs:
        print(f"PLAYER_ERROR_GL{g}: {xz:.4f}")

    # camera lag at 91
    if 91 in orig and 91 in recp:
        o, r = orig[91], recp[91]
        o_lag = ang_delta(o["yaw"], o["cyaw"])
        r_lag = ang_delta(r["yaw"], r["cyaw"])
        print(f"CAMERA_LAG_GL91_ORIGINAL: {o_lag:.2f}")
        print(f"CAMERA_LAG_GL91_RECOMP: {r_lag:.2f}")
        print("CAMERA_LAG_MATCH:", "YES" if abs(o_lag - r_lag) < 2.0 else "NO")

    # clocks
    o_vis = [orig[g]["vi"] for g in common if orig[g].get("vi")]
    if len(o_vis) >= 2:
        dvs = [o_vis[i] - o_vis[i - 1] for i in range(1, len(o_vis))]
        print(f"ORIGINAL_VI_PER_GLRECORD: mean={sum(dvs)/len(dvs):.4f} min={min(dvs)} max={max(dvs)}")
    r_advs = [recp[g]["adv"] for g in common]
    print(f"RECOMP_GL_ADV_FIELD: mean={sum(r_advs)/len(r_advs):.4f}")

    if 322 in orig and 322 in recp:
        o, r = orig[322], recp[322]
        _, _, _, xz, _ = pos_err(o, r)
        print("PLAYER_PHASE_GL322:", "DIFF" if xz is not None and xz > MEANINGFUL_POS else "MATCH")
        if o.get("dgx") is not None:
            dg = math.hypot(r["dgx"] - o["dgx"], r["dgz"] - o["dgz"])
            print("DIGGER_PHASE_GL322:", "DIFF" if dg > MEANINGFUL_POS else "MATCH")
        _, _, _, cxz, _ = pos_err(o, r, "cx", "cy", "cz")
        print("CAMERA_PHASE_GL322:", "DIFF" if cxz is not None and cxz > MEANINGFUL_POS else "MATCH")

    if out_md:
        Path(out_md).write_text("see stdout; this helper prints the required tables\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
