---
status: historical-pre-fix
type: evidence
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, camera, p60, writer-provenance, glidx39]
---

# USER_DEMO_4 `glidx=39` `node+0x60` writer provenance

This note closes the static part of the `glidx=38 → 39` join and records the
runtime capture contract. It deliberately uses neutral names (`p60`, `p60+8`)
until the object has a proven semantic name. This is historical pre-fix
provenance; it does not authorize a post-fix parity claim by itself.

The native `0x800979D8` semantics were corrected on 2026-08-22 after this
capture: `bakey_held` polarity, live `800BCE84` query, and logical-input
enabled-mask return are covered by `bcf28_helpers_test`. On a 900s paired run
on `DISPLAY=:0`, RT64 and Oracle executed the exact same chain at `glidx=39`:
`80112D90` wrote `p60.C=1, p60.E=2`, `80112D08` wrote `p60+8=5`, and `ncbapivot`
took the matching transition branch. The downstream chain is now **PROVEN**.

## Bounded question

At `stage=0/sub=4`, compare the same semantic camera node immediately after
`glidx=38` ncbapivot leave and at `glidx=39` ncbapivot enter. The first known
split is `*(node+0x60)+8`: RT64 reads `0`, while the Angrylion oracle reads `5`.
The writer must be joined by semantic node identity and execution order, not by
raw pointer value (the two runtimes allocate at different addresses).

## Static provenance (source-level proof)

| address | proven role | relevant state |
| --- | --- | --- |
| `80110588` | camera-node update path | calls `80112A84` |
| `80112A84` | higher-level p60 producer | starts its local `a1` as `5`; the `p60+0xC` halfword-zero path changes it to `0`; calls `80112D08` |
| `80112D08` | exact bounded writer | stores `a1 → p60+8`, `a2 → p60+0`, `a3 → p60+4` |
| `80112C3C` | ncbapivot consumer | loads `p60+8`, subtracts one, and dispatches on the result |

Post-fix held-state provenance (2026-08-22): the reason RT never supplies
`p60+0xC=1` is still `0x800979D8` returning `0` at glidx 39, but that is no
longer a missing C-Right pad bit. Pad `0001` and `prev_state[13]=0` match AN;
AN returns `v0=2`. Canonical:
[[p08-user-demo4-bainput-held-state-provenance]].

The static chain is therefore `80110588 → 80112A84 → 80112D08`; the old
`800137AC` hypothesis is superseded. `p60+0xC` is a live producer control/status
halfword, not yet a semantic field name. `p60+0/+4/+8` are writer fields;
`p60+0x10..0x20` remain neutral auxiliary/input words.

`800F2E60` is retained only as a generic three-word copy candidate. It is not
path-proven for USER_DEMO_4 and must not be called the cause without a dynamic
join.

## Runtime evidence

The RT64 paired capture reached the target and emitted:

```text
glidx=39  80112A84 enter: p60+0=0 p60+4=0 p60+8=0 p60+0xC=0
glidx=39  80112D08 enter: a1=0 a2=0 a3=0
glidx=39  80112D08 leave: p60+8=0
glidx=39  ncbapivot enter: p60+8=0
```

This proves RT64 executes the exact writer between the two ncbapivot samples,
and that its input path supplies `a1=0` because the observed `p60+0xC` control
halfword is zero. It is a **B-P60-WRITER-INPUT** result, not an A-candidate:
the paired oracle capture below closes the historical writer-input join. The
separate post-fix RT64 capture recorded `held=0`, so held-branch acceptance is
still open; see [[p08-user-demo4-800979d8-independent-review]].

The fresh oracle writer-marker run records the same semantic node at
`glidx=39`, with `p60+0xC=1`, `a1=5`, and post-store `p60+8=5`. Its ncbapivot
sample independently records `p60+8=5`. The paired values close the
writer-input join: this is not a missing call, a different object selection, or
an update-order-only explanation.

## Classification and next join

```text
FIRST_PROVEN_DISCRIMINATOR:  p60+8 at glidx=39 ncbapivot entry (RT64 0, oracle 5)
STATIC_WRITER:               80112D08 (a1 → p60+8)
PRODUCER:                    80112A84 (a1=5 default; p60+0xC==0 path → 0)
RT64_WRITER_INPUT:           a1=0, p60+0xC=0, node=801d2d30, p60=801d3180
ORACLE_WRITER_INPUT:         a1=5, p60+0xC=1, post-store p60+8=5
OBJECT_SELECTION:            SAME semantic node within each paired runtime
UPDATE_ORDER:                NOT THE FIRST DIFFERENCE (writer executes)
ROOT_CLASS:                  B-P60-WRITER-INPUT
A_CANDIDATE:                 NOT_PROVEN (no production divergence fix)
FIX_IMPLEMENTED:             NO
```

For future reruns, log on both runtimes `glidx`, stage/sub, node identity join
fields, p60 pointer, `p60+0/+4/+8/+0xC/+0xE`, and the `80112A84`/`80112D08`
arguments before and after the store. Stop if the writer is outside the
`glidx=38 leave → glidx=39 enter` interval; that is a coverage failure, not a
root cause.

## Reproduction checklist

From `TooieRecomp/`:

```bash
cmake --build build --target TooieRecompiled -j2
./tools/oracle/angrylion-control/scripts/build.sh --force

env -u TOOIE_UD4_FIRST_TICK DISPLAY=:0 \
  VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
  TOOIE_UD4_WALKSTART=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
  timeout -k 5s 720s ./run-tooie.sh -A > /tmp/p08-rt64-writer.log 2>&1

env -u TOOIE_UD4_FIRST_TICK DISPLAY=:0 TOOIE_UD4_WALKSTART=1 \
  timeout -k 5s 720s ./run-tooie-angrylion-rdp.sh --measure \
  > /tmp/p08-angrylion-writer.log 2>&1
```

The paired-run preflight, artifact names, X11/RADV checks, timeout policy, and
join/failure matrix are maintained in
[[p08-walkstart-reproduction-runbook]]. The oracle build is not complete until
its output says `build done`; the p08 patch must be reapplied after a clean
oracle worktree. If the oracle log has ncbapivot `p60+8=5` but no
`[ud4-p60-*]` lines, the pure-interpreter dispatch list is incomplete.

## Artifact ledger

```text
RT64_CAPTURE:       /tmp/p08-rt64-writer-rerun.log
ORACLE_CAPTURE:     /tmp/p08-angrylion-writer-rerun.log
ORACLE_CAM_LOG:     /tmp/p08-angrylion-ud4-cam.log
ORACLE_CSV:         /tmp/p08-angrylion-user_demo4.csv
ARCHIVE:            /tmp/p08-p60-writer-provenance-20260821/
ARCHIVE_HASHES:     /tmp/p08-p60-writer-provenance-20260821/SHA256SUMS
ORACLE_BUILD:       PASS (pinned nightly/RSP/CXD4; ROM SHA-1 verified)
```

## Current checkpoint (2026-08-22)

The new semantic join supersedes the pre-fix `p60.C=0` observation for the
corrected target event. The after-write values are `p60.C=1`, `p60.E=2`, and
`p60+8=5` in both runtimes, followed by the same ncbapivot branch. The final
paired capture verifies twice and closes immutable acceptance for this bounded
helper-chain result. It is not a global camera or attract verdict.
