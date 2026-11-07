"""Write a TOOIE_TEST_AUDIO_RENDER plan: every area preset (mask A at bpm A, mask B at bpm B) of each layered sequence,
then every other sequence whole.

    TOOIE_ROM_IMAGE=<decompressed USA image> make_plan.py <plan.tsv> [--presets-only]
"""
import os, struct, sys

d = open(os.environ["TOOIE_ROM_IMAGE"], "rb").read()  # decompressed USA image (35098224 bytes)
PRESET0 = 0x1EFD774  # core2 data, 12-byte {mask_a, mask_b, bpm_a, bpm_b}
# Preset index ranges per layered sequence (neighbours share a sequence; see the jukebox in audio-sequence-volume).
groups = {
    0x32: range(0x01, 0x03), 0x39: range(0x03, 0x0B), 0x3E: range(0x0B, 0x13), 0x40: range(0x13, 0x1D),
    0x45: range(0x1D, 0x20), 0x4B: range(0x20, 0x26), 0x52: range(0x26, 0x29), 0x3F: range(0x29, 0x2C),
    0x41: range(0x2C, 0x31), 0x57: range(0x32, 0x37), 0x59: [0x37, 0x38, 0x46], 0x75: range(0x39, 0x3B),
    0x58: range(0x3B, 0x45),
}
# Empty sequence IDs crash the original player.
EMPTY = {0x13, 0x21, 0x22, 0x23, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2C, 0x2D, 0x35, 0x36, 0x3C, 0x51, 0xA0}
plan, seen = [], set()
for seq, presets in groups.items():
    for p in presets:
        a, b, ta, tb = struct.unpack_from(">IIHH", d, PRESET0 + 12 * p)
        for tag, mask, bpm in (("A", a, ta), ("B", b, tb)):
            if mask in (0, 0xFFFFFFFF) or bpm == 0xFFFF or (seq, mask, bpm) in seen:
                continue
            seen.add((seq, mask, bpm))
            plan.append((f"s{seq:02X}_p{p:02X}{tag}_{mask:08X}_{bpm}", seq, f"{mask:08X}", 25, bpm))
if "--presets-only" not in sys.argv:
    for seq in range(0x00, 0xA1):
        if seq not in groups and seq not in EMPTY:
            plan.append((f"s{seq:02X}_whole", seq, "-", 20, None))
with open(sys.argv[1], "w", encoding="utf-8", newline="\n") as f:
    for name, seq, mask, sec, bpm in plan:
        f.write(f"{name}\t{seq:X}\t{mask}\t{sec}" + (f"\t{bpm}" if bpm else "") + "\n")
print(len(plan), "entries", round(sum(p[3] + 1.6 for p in plan) / 60, 1), "minutes")
