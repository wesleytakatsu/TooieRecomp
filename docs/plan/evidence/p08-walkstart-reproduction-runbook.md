---
title: "P08 — USER_DEMO_4 walkstart reproduction runbook"
description: "Operational checklist for reproducing the real-display RT64/Angrylion glidx 39 ncbapivot capture."
status: current
type: procedure
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, walkstart, ncbapivot, rt64, angrylion, reproduction, runbook]
---

# USER_DEMO_4 walkstart reproduction runbook

This is a diagnosis-only procedure. TooieRecomp stays on RT64; Angrylion RDP
Plus is the oracle only. Do not use this runbook to change camera math,
generated recompiler output, `round.w.s`, the egg path, or shared RT64.

## 1. Required runtime contract

- Run from the `TooieRecomp` repository root.
- Use a real X11 display, not Xvfb, headless SDL, or Lavapipe. The reference
  capture used `DISPLAY=:0` and an AMD RX 580 through RADV.
- Use the NTSC-U Rev. 0 cartridge accepted by the oracle launcher:
  `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`. Do not pass the decompressed
  `build/bt.n64.us.1.0.z64` to Angrylion.
- For this walkstart run, set `TOOIE_UD4_WALKSTART=1` and explicitly unset
  `TOOIE_UD4_FIRST_TICK`. The two gates select different diagnostic windows;
  do not leave a stale first-tick variable in the shell.
- Use a timeout of at least 600 seconds. The known-good capture used
  `timeout -k 5s 720s`.

## 2. Preflight

```sh
cd /path/to/TooieRecomp

# X11 presentation must work before starting either emulator.
test -n "${DISPLAY:-}" && xdpyinfo >/dev/null

# Pin the Vulkan loader to RADV when multiple ICDs are installed.
export DISPLAY=:0
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json
test -f "$VK_ICD_FILENAMES"
VK_ICD_FILENAMES="$VK_ICD_FILENAMES" vulkaninfo --summary \
  | rg -i 'deviceName|driverName|radv|radeon'

# Confirm no previous capture is still holding the display or fixed /tmp logs.
ps -eo pid=,args= | rg '[T]ooieRecompiled|[m]upen64plus' || true
```

If the ICD path differs on another machine, select that machine's RADV
`radeon*.json` explicitly and record the path in the evidence. Never silently
fall back to Lavapipe.

## 3. Build once, then rebuild the oracle after hook changes

```sh
# RT64 port.
cmake --build build --target TooieRecompiled -j"$(nproc)"

# First oracle setup only; fetch.sh uses the pinned commits in pins.env.
./tools/oracle/angrylion-control/scripts/fetch.sh

# Required after changing tools/p08_attract_egg_hook.h or the oracle patch.
./tools/oracle/angrylion-control/scripts/build.sh --force
./tools/oracle/angrylion-control/scripts/status.sh
```

`fetch.sh` is not needed when the pinned `work/` trees already exist. The
oracle launcher checks the ROM SHA-1 and refuses a missing or wrong cartridge.
Set `TOOIE_CART_Z64=/absolute/path/to/Banjo-Tooie-USA-Rev0.z64` when the ROM is
not in the location documented by `run.sh`.

## 4. Start the paired capture

Use two terminals (or two independently monitored sessions). Do not join by
wall-clock time: each run is joined later by stage/sub, consumed glrecord, and
pad bytes.

Terminal A — TooieRecomp / RT64:

```sh
env -u TOOIE_UD4_FIRST_TICK \
  DISPLAY=:0 \
  VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
  TOOIE_UD4_WALKSTART=1 \
  TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
  timeout -k 5s 720s ./run-tooie.sh -A \
  > /tmp/p08-rt64-display-glidx39-rerun.log 2>&1
```

Terminal B — Mupen Pure Interpreter + Angrylion RDP Plus + CXD4 LLE:

```sh
env -u TOOIE_UD4_FIRST_TICK \
  DISPLAY=:0 \
  TOOIE_UD4_WALKSTART=1 \
  timeout -k 5s 720s ./run-tooie-angrylion-rdp.sh --measure \
  > /tmp/p08-angrylion-display-glidx39-rerun.log 2>&1
```

`--measure` selects dummy audio and avoids SDL audio blocking the oracle. It
does not change the video/RSP oracle. The RT64 wrapper's `-A` sets
`TOOIE_AUTO_START=1` and `TOOIE_DIAG_ATTRACT=1`; do not replace it with a
launcher-only run.

## 5. Success markers and safe stopping

The target is reached when all of the following are true:

```text
RT64:    stage=0/sub=4 and ncbapivot enter+leave for glidx 36..42
Oracle:  stage=0/sub=4, TRACE_ON and leave for target glidx 39
Join:    at least glidx 39..42 have matching consumed glrecord + pad bytes
```

Inspect without assuming a line-count or time match:

```sh
rg -n '\[ud4-ncbapivot\] (enter|leave)' \
  /tmp/p08-rt64-display-glidx39-rerun.log | tail -20
rg -n 'TRACE_ON|\[ud4-ncbapivot\] leave' \
  /tmp/p08-angrylion-ud4-cam.log | tail -20
rg -n '(^|,)39(,|$)|(^|,)40(,|$)|(^|,)41(,|$)|(^|,)42(,|$)' \
  /tmp/p08-angrylion-user_demo4.csv | head
```

After the markers appear, stop each exact emulator PID with `TERM` rather
than waiting for the 720-second timeout. Verify that no `TooieRecompiled` or
`mupen64plus` process remains. Do not use a broad recursive kill command.

The oracle hook writes fixed paths (`/tmp/p08-angrylion-ud4-cam.log`,
`/tmp/p08-angrylion-user_demo4.csv`, and `/tmp/p08-angrylion-attract-eggs.log`).
Move or copy them immediately after the run so a later attempt cannot replace
the evidence:

```sh
TAG="$(date +%Y%m%d-%H%M%S)"
OUT="/tmp/p08-walkstart-${TAG}"
mkdir -p "$OUT"
cp /tmp/p08-rt64-display-glidx39-rerun.log "$OUT/rt64.log"
cp /tmp/p08-angrylion-display-glidx39-rerun.log "$OUT/oracle-startup.log"
cp /tmp/p08-angrylion-ud4-cam.log "$OUT/oracle-cam.log"
cp /tmp/p08-angrylion-user_demo4.csv "$OUT/oracle-motion.csv"
sha256sum "$OUT"/* > "$OUT/SHA256SUMS"
```

## 6. Join and report

Join the two sides using, in order:

1. `stage` and `sub` (`stage=0/sub=4` is Haunted Cavern);
2. consumed glrecord/glidx;
3. pad x, pad y, and pad buttons.

Do not use wall-clock timestamps, `cammode`, or raw line numbers. The child
call trace can have different nesting; compare it by target glidx and call
address, then report the first numeric input/output difference. Record the
join coverage explicitly when the oracle CSV window does not contain glidx
36/37.

The complete evidence record should include the git HEAD, build exit codes,
display/ICD preflight output, exact commands, timeout/termination result,
artifact paths and hashes, join coverage, ncbapivot entry/leave fields, FCSR
availability, and the first executed child-input difference. The current
known-good run is summarized in
[[p08-user-demo4-walkstart-ncbapivot-discriminator]].

## 7. Failure interpretation

| Symptom | Interpretation / next action |
|---|---|
| `SDL could not initialize Vulkan window` | Display/ICD preflight failed. Fix X11/RADV; do not interpret as game behavior. |
| `No threads left to run!` before sub=4 | RT64 runtime/environment failure. Recheck physical RADV presentation and rerun; not a semantic result. |
| Oracle says binaries are missing | Run `fetch.sh`, `build.sh --force`, then `status.sh`. |
| Oracle rejects ROM SHA-1 | Set `TOOIE_CART_Z64` to the NTSC-U Rev. 0 cartridge; never use the decompressed recomp ROM. |
| Run stops at sub=1/sub=2 | Check `-A`, `TOOIE_UD4_WALKSTART=1`, `env -u TOOIE_UD4_FIRST_TICK`, and the 720-second bound. Preserve the failed logs as environment evidence. |
| Old `TRACE_ON`/CSV data appears immediately | Move the fixed `/tmp` artifacts before retrying and record fresh hashes. |

Any failed preflight or incomplete window is a capture blocker, not evidence
for a camera, FPU, physics, animation, speed, collision, or egg fix.
