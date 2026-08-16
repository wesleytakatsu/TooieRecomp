#!/usr/bin/env python3
"""
tools/demo_sync_analysis.py

Analyzes the full Wooded Hollow gameplay demo diagnostic trace (TOOIE_DIAG_DEMO_SYNC=1)
and computes kinematics, landmarks, cadence, and segmentation.
"""

import re
import math
import sys

def parse_demo_sync_log(filepath):
    pattern = re.compile(
        r'\[demo-sync\] glidx=(?P<glidx>\d+)(?: post=(?P<post>\d+))? tick=(?P<tick>\d+) wall_ms=(?P<wall_ms>\d+) gfx=(?P<gfx>\d+) adv=(?P<adv>\d+) '
        r'pad=\((?P<padx>-?\d+),(?P<pady>-?\d+),(?P<padbtn>[0-9a-fA-F]+)\) remap=\((?P<rmx>[-\d\.]+),(?P<rmy>[-\d\.]+)\) '
        r'bstick=\((?P<bsx>[-\d\.]+),(?P<bsy>[-\d\.]+),(?P<bsang>[-\d\.]+),(?P<bsdist>[-\d\.]+)\) '
        r'bs=(?P<bs>-?\d+) pos=\((?P<px>[-\d\.]+),(?P<py>[-\d\.]+),(?P<pz>[-\d\.]+)\) '
        r'ppos=\((?P<ppx>[-\d\.]+),(?P<ppy>[-\d\.]+),(?P<ppz>[-\d\.]+)\) '
        r'vel=\((?P<vx>[-\d\.]+),(?P<vy>[-\d\.]+),(?P<vz>[-\d\.]+)\) velxz=(?P<velxz>[-\d\.]+) tspd=(?P<tspd>[-\d\.]+) '
        r'yaw=(?P<yaw>[-\d\.]+) ideal=(?P<yaw_ideal>[-\d\.]+) phys_yaw=(?P<phys_yaw>[-\d\.]+) mult=(?P<mult>[-\d\.]+) accel=(?P<accel>[-\d\.]+) '
        r'cam=\((?P<cx>[-\d\.]+),(?P<cy>[-\d\.]+),(?P<cz>[-\d\.]+),(?P<cyaw>[-\d\.]+)\) '
        r'digger=\((?P<dgx>[-\d\.]+),(?P<dgy>[-\d\.]+),(?P<dgz>[-\d\.]+)\) '
        r'act2=\((?P<a2mid>\d+),(?P<a2sys>[0-9a-fA-F]+),(?P<a2x>[-\d\.]+),(?P<a2y>[-\d\.]+),(?P<a2z>[-\d\.]+)\) '
        r'gs=(?P<gs>[-\d\.]+) fc=(?P<fc>\d+) cnt=(?P<cnt>\d+)'
    )

    samples = []
    with open(filepath, 'r') as f:
        for line in f:
            if '[demo-sync]' in line and 'cnt=766' in line:
                m = pattern.search(line)
                if m:
                    d = {k: (int(v) if k in ['glidx', 'tick', 'wall_ms', 'gfx', 'adv', 'padx', 'pady', 'bs', 'a2mid', 'fc', 'cnt']
                             else (float(v) if k not in ['padbtn', 'a2sys'] else v))
                         for k, v in m.groupdict().items()}
                    samples.append(d)
    return samples

def main():
    log_path = 'wooded_hollow_demo_sync.log'
    if len(sys.argv) > 1:
        log_path = sys.argv[1]

    samples = parse_demo_sync_log(log_path)
    if not samples:
        print(f"No samples found in {log_path}")
        return

    print(f"Loaded {len(samples)} samples from {log_path} (glidx {samples[0]['glidx']}..{samples[-1]['glidx']})")

    # Landmarks
    landmarks = [
        ("L0: First sample", 2),
        ("L1: First player move", 21),
        ("L2: First straight settle", 35),
        ("L3: Analog turn entry", 74),
        ("L4: Analog turn mid", 76),
        ("L5: Analog turn end", 91),
        ("L6: Second straight settle", 120),
        ("L7: Fire egg input", 322),
        ("L8: Projectile spawn", 334),
        ("L9: Closest approach", 365),
        ("L10: Turn to uphill", 440),
        ("L11: Uphill climb", 550),
        ("L12: Demo end", 765),
    ]

    print("\n" + "="*80)
    print("DEMO LANDMARKS")
    print("="*80)
    for name, target_gl in landmarks:
        s = next((x for x in samples if x['glidx'] == target_gl), None)
        if s:
            print(f"{name:26s} | gl={s['glidx']:3d} | tick={s['tick']:4d} | wall={s['wall_ms']:5d}ms | "
                  f"pos=({s['px']:7.1f},{s['py']:6.1f},{s['pz']:7.1f}) | velxz={s['velxz']:5.1f} | yaw={s['yaw']:5.1f} | "
                  f"cam=({s['cx']:7.1f},{s['cy']:6.1f},{s['cz']:7.1f}, {s['cyaw']:5.1f}) | bs={s['bs']:2d}")

    # Kinematic segmentation
    segments = []
    cur_seg = {'id': 0, 'start': samples[0]['glidx'], 'state': samples[0]['bs'], 'type': 'IDLE', 'start_idx': 0}
    for i in range(1, len(samples)):
        prev = samples[i-1]
        cur = samples[i]
        is_moving = cur['velxz'] > 10.0
        st_change = cur['bs'] != prev['bs']
        stick_mag = math.sqrt(cur['rmx']**2 + cur['rmy']**2)
        turn_change = abs(cur['bsang'] - prev['bsang']) > 15.0 and stick_mag > 0.3

        if st_change or turn_change or (is_moving != (prev['velxz'] > 10.0)):
            cur_seg['end'] = prev['glidx']
            cur_seg['end_idx'] = i - 1
            segments.append(cur_seg)
            cur_seg = {'id': len(segments), 'start': cur['glidx'], 'state': cur['bs'],
                       'type': 'MOVE' if is_moving else 'IDLE', 'start_idx': i}
    cur_seg['end'] = samples[-1]['glidx']
    cur_seg['end_idx'] = len(samples) - 1
    segments.append(cur_seg)

    print("\n" + "="*80)
    print("KINEMATIC SEGMENTS")
    print("="*80)
    for seg in segments:
        sub = samples[seg['start_idx']:seg['end_idx']+1]
        if not sub: continue
        dticks = sub[-1]['tick'] - sub[0]['tick']
        dwall = sub[-1]['wall_ms'] - sub[0]['wall_ms']
        path_len = sum(math.sqrt((sub[j]['px'] - sub[j-1]['px'])**2 + (sub[j]['pz'] - sub[j-1]['pz'])**2) for j in range(1, len(sub)))
        avg_speed = path_len / (len(sub) * 0.033333) if len(sub) > 0 else 0
        peak_speed = max(x['velxz'] for x in sub)
        print(f"Seg {seg['id']:2d} (gl {seg['start']:3d}..{seg['end']:3d}) | {seg['type']:4s} bs={seg['state']:2d} | "
              f"len={path_len:6.1f} | ticks={dticks:3d} | wall={dwall:4d}ms | avg_spd={avg_speed:5.1f} | peak={peak_speed:5.1f} | "
              f"pos=({sub[0]['px']:6.0f},{sub[0]['pz']:6.0f}) -> ({sub[-1]['px']:6.0f},{sub[-1]['pz']:6.0f})")

if __name__ == '__main__':
    main()
