---
title: "P08 — render-submission matrix diagnosis"
description: "Rare view-in-projection vs RT64 G_MTX_PROJECTION LOAD decompose. Banjo incremental MUL avoids the path Tooie hits. Offline numeric probe of matrixDecomposeViewProj."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, render-submission, rt64, matrix, viewproj, banjo, transform-tagging]
---

# P08 — RENDER SUBMISSION MATRIX DIAGNOSIS

```text
BASE_HEAD:                           fe2683ebc70309191ef339efd4881d61f8471a76
MODEL:                               Grok 4.6 HIGH
MODE:                                NARROW RENDER / RT64 INTERCEPTION DIAGNOSIS ONLY
SCENE:                               map 335 / MAP_14F_IOH_WOODED_HOLLOW (mode 5)
ROOT_CAUSE_LEVEL:                    B
FINAL_VERTEX_TRANSFORM_WRONG:        NO
TRANSFORM_CLASSIFICATION_WRONG:      YES
CAMERA_EXTRACTION_WRONG:             YES
ACTOR_CAMERA_CAUSAL_CLASS:           B
BANJO_TRANSFORM_TAGGING_FOUND:       YES
BANJO_TOOIE_CONVENTION_EQUIVALENT:   PARTIAL
FIX_IMPLEMENTED:                     YES (see p08-render-submission-matrix-fix.md)
```

> [!NOTE]
> Fix implemented and validated in [[p08-render-submission-matrix-fix]].

---

## 1. Executive summary

The first Wooded Hollow visual error is **not** ordinary clip-space multiplication.

Tooie builds a combined view-projection on the CPU (`func_800E44FC` /
`func_800E443C`) and **LOADs it once** onto `G_MTX_PROJECTION`. RT64's
`RSP::matrixCommon` treats that LOAD as a view-projection candidate
(`isMatrixViewProj`) and runs `matrixDecomposeViewProj`. The host then
**always overwrites** the submitted viewProj with `view * proj`
(`rt64_projection_processor.cpp`).

An offline numeric reconstruction of that decomposer (Wooded Hollow camera
from [[p08-actor-camera-transform-discriminator]]) shows:

- reconstructed `V * P` equals the submitted VP (max abs error ~1e-13);
- extracted view **Z axis and Z translation are wrong** by the
  `guPerspective`/`func_800CB124` scale of `0.5`, because the decomposer
  hardcodes `P[2][3] = -1` instead of the scaled `-0.5`.

Banjo-Kazooie uses the same Rare "view lives in the projection stack" idea,
but **never LOADs a translated viewproj**. It LOADs pure `P`, then MULs
rotation-only view. That path never enters `matrixDecomposeViewProj`.
BanjoRecomp then adds `gEXSetViewMatrixFloat` + matrix-group IDs for
interpolation. Tooie has `patches/transform_ids.h` only; no `.c` patches,
no `gEXEnable`, empty `register_tooie_patches()`.

```text
FIRST_EXACT_SEMANTIC_MISMATCH:
  Tooie G_MTX_PROJECTION LOAD of combined (T_S * V_rot * P)
  vs RT64 LOAD-path matrixDecomposeViewProj, which assumes
  unscaled P[2][3] = -1.

INTENDED_CLIP:     submitted_VP * M * v     (F3DEX2)
RT64_HOST_CLIP:    M * V_extracted * P_extracted
PRODUCT:           numerically equal to submitted VP
EXTRACTED_VIEW:    Z column and Z translation scaled by 0.5
```

Do **not** treat this as a shared RT64 bug until a second game hits the
same LOAD-of-scaled-VP path. BanjoRecomp already solved the Rare convention
game-locally.

---

## 2. Tooie display-list convention (PROVEN)

Primary source: `banjo-tooie-Recomp/src/core2/1EBCFC0.c` (`func_800E44FC`,
`func_800E443C`, `func_800E42F0`, `func_800E4640`).

Comments in `func_800E44FC` state Rare puts the view in `G_MTX_PROJECTION`
so lighting stays world-space (unlike SM64).

Exact multiply order is **not** `P * V`. `func_80018C50` is left-multiply
(`incoming * stack`):

```text
mlMtxSet(P)                         // func_800CB124 → standard persp, *0.5
func_80018C50(func_800CA7AC(vp))    // viewport+0xA4 = V_rot
func_80018C50(T_S)                  // inv-scale + -translation*inv_scale
mlMtxApply → gSPMatrix LOAD PROJECTION NOPUSH
guMtxIdent → gSPMatrix LOAD MODELVIEW NOPUSH
```

```text
TOOIE_PROJECTION_LOAD:
  func_800E44FC
  flags = G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH
  value = T_S * V_rot * P
  cached at D_8012D558

TOOIE_MODELVIEW_LOAD:
  func_800E44FC
  flags = G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH
  value = identity

TOOIE_MODEL_MULTIPLY:
  actor/model path (func_800E28A4 / model draw)
  world T(pos) * S * R(PYR) loaded/multiplied into MODELVIEW
  (CPU 3x3 Y-up already proven in the discriminator)

TOOIE_ORTHO_LOAD:
  func_800E42F0
  G_MTX_PROJECTION LOAD ortho + MODELVIEW identity

TOOIE_PROJECTION_RELOAD:
  func_800E4640 re-LOADs D_8012D558 (combined VP) if the cache is live

MATRIX_FLAGS:
  gameplay viewproj = LOAD | NOPUSH | PROJECTION
  identity model    = LOAD | NOPUSH | MODELVIEW

MULTIPLICATION_ORDER (CPU mlMtx, PROVEN):
  VP = T_S * V_rot * P
  clip_hw = VP * M * v

func_800CB124 (PROVEN from asm):
  near = viewport+0xEC (clamped >= 1)
  far  = viewport+0xF0 (clamped >= near+100)
  P[0][0] = fov/aspect term
  P[1][1] = viewport+0xF4
  P[2][2] = near/far term
  P[2][3] = -1, then entire P *= 0.5
  P[3][2] = near/far term
  P[3][3] = 0

func_800CA7AC (PROVEN): return viewport+0xA4 (V_rot)
func_800CA6E8 (PROVEN): return viewport+0x164 (world scale)
func_800CA7E4 (PROVEN): copies eye translation via func_800EE7F8
```

Correction to the previous discriminator prose: it said Rare submits
`Proj * View`. The **mlMtx product is `View * Proj`** (`T_S * V_rot * P`),
same factorisation Banjo builds incrementally. The earlier `P * V` wording
is **CORRECTED/SUPERSEDED**.

---

## 3. Banjo-Kazooie comparison (PROVEN)

### 3.1 What BanjoRecomp's transform tagging actually solves

High-priority files under `BanjoRecomp/patches/`:

| File | Role |
| --- | --- |
| `graphics_patches.c` | `gEXEnable` + RDRAM extended + refresh rate + near clip |
| `camera_transform_tagging.c` | **viewport projection path** |
| `projection_transform_tagging.c` | projection IDs (gameplay/HUD/title/…) |
| `transform_tagging.c` | model/geo/bone/billboard matrix groups |
| `marker_transform_tagging.c` | per-marker model IDs |
| `map_ / sky_ / hud_ / particle_ / prop_ / text_*` | other IDs |

`WHAT_PROBLEM_THE_BANJO_PATCH_SOLVES` (do **not** copy blindly):

1. **Interpolation IDs** (`gEXMatrixGroup*`) so RT64 can match transforms
   across frames at high host refresh.
2. **`gEXSetViewMatrixFloat`** on
   `viewport_setRenderPerspectiveMatrix`: Banjo leaves **camera
   translation in MODELVIEW** (`func_80252AF0(camera, object, …)` =
   `T(object - camera)`). The extra view matrix tells RT64 the camera
   translation so interpolation does not treat camera motion as object
   motion.
3. **Incremental projection submission** (original BK, preserved by the
   patch): `LOAD P`, then `MUL` roll/pitch/yaw. Rotation-only view never
   trips `isMatrixViewProj`.

Banjo original (`bk-decomp/src/core1/viewport.c`):

```text
guPerspective(..., scale=0.5) → LOAD PROJECTION
guRotate(-roll)               → MUL  PROJECTION
guRotate(-pitch)              → MUL  PROJECTION
guRotate(-yaw)                → MUL  PROJECTION
guTranslate(0,0,0)            → LOAD MODELVIEW   // identity
```

Then `modelRender_draw` replaces MODELVIEW with a **camera-relative** model.

```text
IS_THE_RARE_MATRIX_CONVENTION_SAME:     PARTIAL
  same: view lives in the projection stack; lighting world-space
  same: product V * P (not P * V)
  different: Banjo translation is in MODELVIEW (camera-relative models)
  different: Tooie translation+scale are in the combined PROJECTION LOAD
  different: Tooie MODELVIEW is world-space M

IS_THE_PATCH_ACTIVE_IN_BANJORECOMP:     YES
IS_EQUIVALENT_LOGIC_PRESENT_IN_TOOIE:   NO
  Tooie has patches/transform_ids.h (header only)
  register_tooie_patches() is empty
  RecompiledPatches/patches.c is a P05 placeholder
  no gEXEnable anywhere in TooieRecomp
```

---

## 4. Wooded Hollow matrix command sequence (PROVEN guest / INFERENCE host)

For the first mode-5 Wooded Hollow frame, before a green Jinjo draw:

```text
gSPPerspNormalize(persp_norm from func_800CB124)
gSPMatrix(D_8012D558, PROJECTION | LOAD | NOPUSH)   // T_S * V_rot * P
gSPMatrix(identity,   MODELVIEW  | LOAD | NOPUSH)
… later …
gSPMatrix(actor_M,    MODELVIEW  | LOAD or MUL | …) // world T*S*R
draw model
```

Guest addresses (from prior discriminator, still valid):

```text
viewport:     D_8012D500
Jinjo actor:  0x801c6d60
eye:          ~(2091.67, 313.14, 560.81)
V_rot 3x3:    Y-up, ~26° pitch down (dumped previously)
M 3x3:        yaw 41°, Y = (0,1,0)
```

This session did **not** capture a live RDRAM dump of the fixed-point mtx
(no gameplay process was launched). The offline probe used those dumped
CPU numbers plus `func_800CB124`'s proven P form.

---

## 5. RT64 interpretation (PROVEN)

`BanjoRecomp/lib/rt64/src/hle/rt64_rsp.cpp` `RSP::matrixCommon`:

```text
if (G_MTX_PROJECTION && LOAD):
    viewProj = submitted
    if isMatrixViewProj(submitted):          // |m[3][3]|>=1e-6 && |1-m[3][3]|>=1e-6
        matrixDecomposeViewProj(submitted, view, proj)
    else:
        proj = submitted
        view = I

if (G_MTX_PROJECTION && MUL):
    viewProj = submitted * viewProj
    if isMatrixAffine(submitted) && !identity:
        view = submitted * view              // Banjo rotations take this path
    else:
        proj = submitted * proj

if (G_MTX_MODELVIEW):
    LOAD or MUL onto the model stack as usual
```

`matrixDecomposeViewProj` (`rt64_math.cpp`) assumes a **standard unscaled**
perspective (`P[2][3] = -1`, `P[3][3] = 0`) and pulls the view Z axis from
column 3 of VP. Both Banjo and Tooie scale P by `0.5`, so real `P[2][3] = -0.5`.

After decompose, `ProjectionProcessor` **always** does:

```text
viewProjMatrix = mul(viewMatrix, projMatrix);
```

Default `TransformGroup.matrixId = G_EX_ID_AUTO`, `decompose = true`.
Projection interpolation defaults to **simple** (not decomposed) unless a
non-AUTO id is set. Tooie never calls `gEXEnable`, so Banjo's
`gEXSetViewMatrixFloat` / group IDs cannot run.

```text
RT64_EXPECTATION:
  LOAD of pure P  → proj=P, view=I
  MUL of affine V → view accumulates; proj stays P
  optional gEXSetViewMatrixFloat to publish camera translation
  host clip uses reconstructed view * proj

TOOIE_SUBMISSION:
  single LOAD of T_S * V_rot * P  (translation makes [3][3] != 0,1)
  MODELVIEW = world M
  no extended GBI
```

Answers:

- Does RT64 assume projection stack = projection only? **On LOAD of a
  detected viewproj, no — it splits.** On LOAD of pure P, yes.
- Does it assume MODELVIEW = view * model? **It treats MODELVIEW as the
  model stack.** World transform recorded as `M * extended.viewProj`.
  `extended.viewProj` stays identity unless `gEXSetViewMatrixFloat` /
  `gEXSetProjMatrixFloat` run.
- Does it derive camera/view from MODELVIEW? **No. From the projection
  LOAD/MUL split (and optional gEX view).**
- Does it classify transforms for interpolation using stack type? **Yes
  (projection groups vs model groups).**
- Rare view-in-projection consequence for Tooie: **camera/view extraction
  wrong** (Z axis / Z translation). Not a wrong final `V*P` product.

---

## 6. CPU vs GPU matrix comparison

Using Wooded Hollow eye / `V_rot` from the discriminator and a
`func_800CB124`-shaped P (`fovy=50`, `aspect=4/3`, `near=10`, `far=8000`,
`scale=0.5`). Command:

```sh
python3 tools/p08_rt64_decompose_probe.py
```

### Banjo incremental `V_rot * P` (no translation in projection)

```text
isMatrixViewProj: FALSE
path:             FALLBACK_VIEW=I, proj = entire V_rot*P
max |V*P - VP|:   0
extracted view:   I
```

RT64 then receives Banjo's later `MUL` rotations on the affine path, so
view becomes `V_rot` and proj stays `P`. Translation stays in MODELVIEW.

### Tooie combined `T(-eye) * V_rot * P`, scale=1

```text
isMatrixViewProj: TRUE
path:             DECOMPOSE
VP[3][3]:         280.405
max |V*P - VP|:   2.27e-13          ← product preserved

INTENDED_VIEW_3x3:
  [  0.5843  -0.3564   0.7291 ]
  [  0.0000   0.8984   0.4392 ]
  [ -0.8115  -0.2566   0.5249 ]
EXTRACTED_VIEW_3x3:
  [  0.5843  -0.3564   0.3645 ]     ← Z column * 0.5
  [  0.0000   0.8984   0.2196 ]
  [ -0.8115  -0.2566   0.2625 ]
INTENDED_VIEW_TRANS:  (-2091.67, -313.14, -560.81)
EXTRACTED_VIEW_TRANS: (-2091.73, -313.14, -280.41)   ← Z * 0.5
```

```text
INTENDED_MATRIX:           T(-eye) * V_rot * P   then * M
SUBMITTED_PROJECTION:      T(-eye) * V_rot * P
SUBMITTED_MODELVIEW:       M (world)
RT64_EFFECTIVE_MATRIX:     M * V_extracted * P_extracted
                           == M * submitted_VP     (product)
FIRST_NUMERIC_DIFFERENCE:  extracted view Z axis and Z translation
                           (scale-0.5 vs hardcoded P[2][3]=-1)
```

Non-unit `func_800CA6E8` world scale makes the view 3x3 / translation
error worse. Counterfactual `P * V` is **not** what Tooie submits
(`[3][3]=0` → fallback view=I).

Truth labels:

| Claim | Label |
| --- | --- |
| Tooie product is `T_S * V_rot * P` | PROVEN |
| RT64 LOAD decompose runs on that matrix | PROVEN (source) + INFERENCE (live frame not dumped) |
| Product `V_extracted * P_extracted == VP` | PROVEN (numeric, reconstructed P) |
| Extracted view Z is wrong by 0.5 | PROVEN (numeric, reconstructed P) |
| Live Wooded Hollow P is exactly fovy=50/near=10 | INFERENCE (form proven; parameters assumed) |

---

## 7. Discriminator (do not conflate)

```text
FINAL_VERTEX_TRANSFORM_WRONG:   NO
  Host reconstructs V*P; that product matches the submitted VP.
  Ordinary clip-space math is not the bug.

TRANSFORM_CLASSIFICATION_WRONG: YES
  Combined scaled VP is classified as viewproj and split with the
  unscaled-P decomposer. Banjo incremental MUL never takes this path.

CAMERA_EXTRACTION_WRONG:        YES
  Extracted view is not orthonormal. Z translation is half the eye
  offset. Matches camera-at-turf / camera-inside-mesh.
  The "supine Jinjo" is consistent with a wrong camera/view basis
  (or viewing a standing model from a crushed-forward camera), not
  with a wrong Actor.rotation / CPU model matrix.
```

---

## 8. Banjo hook → Tooie equivalent

| BANJO_FUNCTION | BANJO_PURPOSE | TOOIE_EQUIVALENT | PROVEN_EQUIVALENT? | NEEDED? |
| --- | --- | --- | --- | --- |
| `viewport_setRenderPerspectiveMatrix` | LOAD P + MUL V_rot; `gEXSetViewMatrixFloat` | `func_800E44FC` + `func_800E443C` + `func_800CB124` | PARTIAL (same V*P idea; Tooie folds T_S into one LOAD) | **YES** (split or retag this path) |
| `viewport_setRenderViewportAndOrthoMatrix` | ortho + identity view | `func_800E42F0` | PARTIAL | later (HUD/text) |
| `func_802E39D0` | gameplay/HUD projection IDs | unknown Tooie frame composer | UNKNOWN | not for first Wooded Hollow shot |
| `modelRender_draw` / `func_80338904` / bones / billboard | model matrix groups | Tooie model draw / `func_800E28A4` | PARTIAL (world M vs Banjo camera-relative M) | later (interpolation) |
| `player_draw` / `__marker_draw` | Banjo/marker IDs | Tooie player/marker draw | UNKNOWN (different overlays) | later |
| `ncCamera_update` | skip interp on cuts | Tooie camera controller | UNKNOWN | later |
| `gEXEnable` in `graphics_patches.c` | enable extended GBI | **none** | no equivalent | only if using gEX* |
| sky/hud/particle/prop/text/map tagging | interpolation IDs | unknown | UNKNOWN | **NO** for this shot |

Do **not** port skinning, zoombox, pillarbox, or HUD score IDs.

---

## 9. Game-specific differences (verified)

| Topic | Banjo | Tooie | Label |
| --- | --- | --- | --- |
| View in projection stack | yes | yes | PROVEN |
| How view is submitted | LOAD P + MUL R | one LOAD of T_S*V*P | PROVEN |
| Camera translation | MODELVIEW (`func_80252AF0`) | inside PROJECTION (`func_800E443C`) | PROVEN |
| Model space | camera-relative | world | PROVEN |
| P scale 0.5 | `guPerspective(..., 0.5)` | `func_800CB124` multiplies P by 0.5 | PROVEN |
| Extended GBI | enabled every frame | not enabled | PROVEN |
| Patch compiler | full `RECOMP_PATCH` suite | empty scaffold | PROVEN |
| `func_800E4640` reload of combined VP | n/a | can re-LOAD cached VP | PROVEN definition; call-graph UNKNOWN |

---

## 10. Minimal fix owner

```text
FIX_OWNER_CANDIDATE:  A  (Tooie-local transform / viewport hook)
                      not C (shared RT64) unless a second game proves
                      the decomposer bug is generic
```

Prefer a Tooie-local solution: Banjo already handled this convention
game-specifically.

---

## 11. Disposable experiment

```text
DISPOSABLE_EXPERIMENT:
  Offline reimplementation of isMatrixViewProj + matrixDecomposeViewProj
  against Banjo incremental VP vs Tooie combined VP.
  Script: tools/p08_rt64_decompose_probe.py
  No game process, no generated-C, no RT64/runtime edits.

EXPERIMENT_RESULT:
  Banjo V_rot*P:          isMatrixViewProj=FALSE (expected)
  Tooie T(-eye)*V_rot*P:  isMatrixViewProj=TRUE
                          V*P product error ~1e-13
                          extracted view Z axis and Z trans * 0.5
  Causal class from this experiment: B
  (mechanism proven; no live visual correction this session)
```

A live `TOOIE_DIAG_MATRIX` dump of `func_800E44FC`'s stack was **not**
added. The offline probe plus primary-source GBI/RT64 is enough for
Level B. A later implementer may dump live `P[2][3]` / `VP[3][3]` to
confirm the assumed fovy/near/scale on the first Wooded Hollow frame.

---

## 12. Causal classification

```text
ACTOR_CAMERA_CAUSAL_CLASS: B
  Shared actor+camera visual error is the RT64 LOAD-path split of
  Tooie's combined scaled viewproj. Exact permanent hook
  (func_800E44FC split and any func_800E4640 reload) is designed,
  not implemented. Simulation values remain out of scope / unchanged.
```

Not A: no reversible in-game experiment yet flipped the Jinjo/camera
while leaving simulation untouched.

---

## 13. Out of scope (unchanged)

Later Y=-2800, underwater camera, other attract maps, probe map=0,
animation fidelity, collision, timing, title/file-select. PATCH4 /
BCF28 / core1 RA / Actor.rotation / camera simulation / model-matrix
construction remain closed.

---

## 14. Root-cause level

```text
ROOT_CAUSE_LEVEL: B

ROOT_CAUSE:
  Tooie submits T_S * V_rot * P as a single G_MTX_PROJECTION LOAD.
  RT64 classifies it as viewproj and decomposes with P[2][3]=-1.
  Tooie P is scaled by 0.5, so extracted view Z / camera Z are wrong
  while the reconstructed clip product stays correct.
  Banjo avoids this LOAD path; Tooie has no equivalent hook.
```

---

## 15. Fix design (NOT implemented)

```text
FIX_OWNER:     TOOIE_NATIVE (viewport / projection submission)
FILES:
  src/game/boot_hw_stubs.cpp   OR new src/game/transform_hooks.cpp
  (optional later) patches/ + register_patches.cpp once a real
  RECOMP_PATCH pipeline exists
  do NOT touch shared RT64 / N64ModernRuntime / N64Recomp /
  RecompiledFuncs / Actor.rotation / camera sim
FUNCTIONS:
  func_800E44FC   — split LOAD P + MUL V_rot + MUL T_S
  func_800E4640   — must not re-LOAD the combined VP
                    (rebuild incremental or cache P and V separately)
BANJO_CODE_REUSED: NO (reuse the *strategy*, not the BK functions)
SEMANTIC_CHANGE:
  Guest clip product unchanged (still T_S * V_rot * P).
  RT64 then takes the MUL-affine path: view = T_S*V_rot, proj = P.
  Extracted camera/view become orthonormal / correctly translated.
WHY_GAME_LOCAL_OR_SHARED:
  Rare Tooie submission shape. Banjo already solved it locally.
  Do not change RT64's decomposer without a generic second case.
EXPECTED_VISUAL_EFFECT:
  green Jinjo upright; camera at the dumped elevated attract pose;
  no camera inside the Jinjo mesh in this shot.
REGRESSION_RISK:
  func_800E4640 reload; ortho path (func_800E42F0) must stay identity
  view; any code that assumes D_8012D558 is a single combined mtx.
  Do not enable gEX* until gEXEnable exists.
  Keep BCF28 + core1 RA tests passing.
```

Smallest first patch: **only** change how `func_800E44FC` (and the
`D_8012D558` reload) emit GBI. Do not port Banjo model/marker/sky
tagging in the same change.

`gEXSetViewMatrixFloat` is **not** required for the first visual fix if
the incremental MUL path puts translation in `view`. It becomes useful
later for high-FPS interpolation, together with `gEXEnable`.

---

## 16. Future acceptance test

Same deterministic scene: map 335, first visible mode-5 Wooded Hollow.

After the later fix:

1. Green Jinjo upright (not supine).
2. Banjo remains upright; Wooded Hollow path still grounded.
3. Camera at the expected elevated attract position
   (eye Y ≈ +313..+354, not turf, not inside the Jinjo).
4. Visible demo still progresses (gsattract ep1).
5. `bcf28_helpers_test` PASS.
6. `core1_ra_wrappers_test` PASS.

“Looks better” is not enough. Simulation dumps from
`TOOIE_DIAG_MOVEMENT=1` must remain in the same band as the
discriminator (Jinjo pitch/roll 0, Banjo grounded walk).

---

## 17. Documentation / checkpoint

```text
EVIDENCE_CREATED:           docs/plan/evidence/p08-render-submission-matrix-diagnosis.md
PROBE_RETAINED:             tools/p08_rt64_decompose_probe.py
SESSION_HANDOFF_UPDATED:    YES
GLOBAL_HANDOFF_UPDATED:     YES
PHASE_P08_UPDATED:          YES
FIX_IMPLEMENTED:            NO
```
