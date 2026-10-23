---
title: "RE correction log"
description: "High-value identity mistakes that someone could plausibly repeat."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-09
tags: [reverse-engineering, corrections]
---

# RE correction log

Not a diary. Only mistakes that are easy to repeat.

## Resource `0x56` is not Banjo geometry

| Field | Old | Current |
| --- | --- | --- |
| Type | model geometry | **ANIMATION** |
| Role | Banjo/BK body | Banjo/BK walk animation |
| Status | `REFUTED` as model | `PROVEN` as animation |
| Header | — | magic `000000c8` (not `0000000B`) |
| Provenance | P09 misread of `D_80801448` / `badata_entrypoint_27` | `STATIC_ROM` + decomp: that table is animation IDs |

`D_80801448[appearance 0x6] = 0x56`. Appearance 0x6 is default
`TRANSFORM_1_BK` land walk (`func_80800000_badata`). That selects the
**walk clip**, not the body model.

## Resource `0xEC` is not the Mumbo model

`0xEC` decompresses as animation (`00000028`, flag `0x16`). Mumbo
**geometry** is `0x665` (GeoCmd) / `0x854` (ExtraSegments). `CORRECTED`.

## V5B did not freeze every world transform used by visible feather triangles

`TOOIE_DIAG_FEATHER_NO_INTERP=1` on bones `79, 100–105` produced masks
`0x7F` and `RESULT==CURRENT` 211/211, visually `SAME`. That set omitted
JOIN parents 107 (`0x592`/`0x61E`) and 108 (`0x4B0`). Mixed interpolated
parent / NO_INTERP child vertices were possible. Label
`TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS:
REFUTED_FOR_VISIBLE_FEATHER_CHAIN` was incomplete coverage. V5C froze
all nine **logical bones** (`79,100–105,107,108`) at the GeoCmd2
token / first `G_VTX` only and is visually `SAME`. That is **not**
every concrete `worldIndex` used by visible triangles (JOIN `G_POPMTX`
parent slots are extra occurrences). Label
`REFUTED_FOR_ALL_TRANSFORMS_DIRECTLY_USED_BY_VISIBLE_FEATHER_TRIANGLES`
is `CORRECTED/SUPERSEDED`. Live untagged worlds interpolate with
`G_EX_INTERPOLATE_DECOMPOSE` (`decompose=true`). VERSION=2 SIMPLE
matches live explosions (`REFUTED` as the captured root). Use
`CURRENT_ROOT_CLASS: INDEPENDENT_VIEW_AND_WORLD_INTERPOLATION_OF_COUPLED_RELATIVE_MOTION`.
Provenance:
`p09-relative-view-world-interpolation-root-cause`,
`p09-feather-no-interp-v5c`,
`p09-world-occurrence-identity-root-cause-audit`,
`p09-boned-interpolation-counterfactual`,
`p09-kazooie-feather-v6-post-matrix-vertex-path`.

## Global current-frame FIXED is not a production interpolation disable

`TOOIE_DIAG_FORCE_CURRENT_FRAME=1` is `USER_OBSERVED` `FIXED` with extra
high-rate renders still on. That proves **some** frame-weight consumer
is causal. It does **not** license permanently forcing all processors
to CURRENT. V5C did **not** refute interpolation of every concrete
world slot used by visible feather vertices. Provenance:
`p09-high-rate-force-current-frame`,
`p09-temporal-coherency-root-cause`.

## Projection-only CURRENT is not a causal isolate

`TOOIE_DIAG_FORCE_CURRENT_PROJECTION` mixed Projection=`CURRENT` with
interpolated world/RSP/Vertex. User visual was the original glitch
plus severe ghosting. That is `INVALID_TEMPORALLY_INCOHERENT`, not
proof that ProjectionProcessor is the original root. The diagnostic
was removed. Do not add another per-component CURRENT override.
Default high-rate after removal is still `GLITCH` (no leak).
Provenance: `p09-temporal-coherency-root-cause`.

## V7 current-frame handoff CORRECT does not cover intermediate high-rate math

V7 `FULL_VISIBLE_FEATHER_CPU_HANDOFF_STATUS=CORRECT` is the
endpoint/current GameFrame path. It does not prove that interpolated
`viewProj(w)*world(w)*(pos-vel*(1-w))` is geometrically sane at
intermediate `w`. Do not treat a V7 PASS as a high-rate interpolation
PASS. Provenance:
`p09-passive-high-rate-temporal-coherency-witness`.

## Vel=0 does not make RSP/Vertex a temporal no-op

F3DEX2 empty velocity and default `vertexInterpolation=SKIP` make the
`pos - vel*(1-w)` term zero. `RSPProcessCS` still multiplies
interpolated `viewProjMat` and `worldMat`. `prevWorldMats` is not the
ordinary raster POSITION source. Provenance:
`p09-temporal-coherency-root-cause`.

## Prior Q-witness small deltas do not outrank global-current FIXED

The CPU Q' witness reported small interpolation deltas. It did not
include the later high-rate global-current `FIXED`. It also does not
justify a mixed-time projection-only A/B (that experiment is
`INVALID`). Provenance:
`p09-feather-postprocess-q-witness-v1`,
`p09-temporal-coherency-root-cause`.

## V5C SAME is not original-framerate FIXED

V5C only neutralized selected TransformProcessor world matrices.
Original-framerate graphics (`targetRate=0`) also removes extra
`threadRenderFrame` calls, `GameFrame::match`, and interpolated
color-target presents. Those are not the same experiment. Do not treat
V5C as a global current-frame test. Provenance:
`p09-high-rate-force-current-frame`.

## BanjoRecomp PR #22 tagging is not a ready P09 fix

Tooie has **no** `transform_tagging.c`, no writer of
`cur_drawn_model_transform_id`, and no `gEXMatrixGroup` on
`func_800DCF48` / type `0x11`/`0x18`. That architectural gap is
`PROVEN`. Live 0x62B feathers/snout use RT64 AUTO. Porting PR #22/#23
or CPU-skinning PR #26/#46/#117 is **not** a ready tagging/skinning
solution: AUTO identity `CORRECT`, DLs are rigid-bone, no GeoCmd1 sort
node on that tree. V5B `SAME` does **not** close interpolation of JOIN
parents 107/108 (see V5C). `CORRECTED` vs tagging-as-`ROOT_CANDIDATE`.
Provenance:
`p09-banjorecomp-transform-tagging-portability-audit`.

## V6 zero-triangle capture is not missing feather geometry

A real V6 run wrote schema
`P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6` with
`TRACKED_BONES_WITH_TRI_MASK=0x00` and no
`/tmp/tooie-kazooie-feather-v6-tri.csv`. V4/V5B already had GMTX and
G_VTX masks `0x7F`. F3DEX2 `G_TRI1`/`G_TRI2` do call the hooked
`RSP::drawIndexedTri`. The empty CSV is a diagnostic key/filter failure:
hits were stored under matrix/match workload, the G_MTX token is cleared
after the first world-transform G_VTX, and `FEATHER_TRI_ROWS` only
incremented after that lookup. `CORRECTED`. Do not treat TRI=0 as
renderer evidence. Provenance:
`p09-kazooie-feather-v6b-triangle-witness-repair`.

## Feather `0x5AA` G_VTX destination is v0=0, not v0=8

F3DEX2 `G_VTX` `w0=0x01008010` is `n=8`, `v0=p0(1,7)-n=0`, slots 0–7.
The earlier “v0=8” decode is `CORRECTED`. Source `0x010037D0`.

## Bone 103 does not have a direct type-0x11 child `dl=0x534`

The previously cited “type-0x11 child of `0x62B+0x124A0`” with
`dl=0x534` is the child of bone 105 (`+0x124D0`). Bone 103's own
type-0x11 is `+0x12580` `dl=0x5AA`; its type-0x18 join is `+0x12550`
`dl=0x592`. `CORRECTED/SUPERSEDED`. Provenance:
`p09-kazooie-feather-gmtx-to-dl-trace`.

## Kazooie plume GeoCmd parent is 107, not head 108

On `0x62B`, type-2 nodes for boneAnimIDs 100–105 are GeoCmd **siblings
of head 108** under parent `+0x12320` INDEX 25 / boneAnimID 107. Listings
that placed 100–105 in the head-108 child list followed 108's sibling
continue (`next=0x160`) and are `CORRECTED/SUPERSEDED`. Crest 79 remains
a GeoCmd child of head 108. Skeletal compose parent of 103 and 100 is
the same index 25 / 107. Provenance:
`p09-visual-geometry-identity-and-scope-forensic`.

## Kazooie V1 low determinant is not a singular matrix

Frames 1420–1424 of the carry-Banjo feather capture are uniformly scaled
orthonormal rotations (det 0.0725→1). `ROTATION_CONTINUITY:
DISCONTINUOUS/DEGENERATE` and `TRUE_GUEST_MATRIX_ANOMALY_COUNT: 3` are
`TOO_STRONG`. Mechanism is `UNIFORM_SCALE`; bug-vs-intent is
`INSUFFICIENT_EVIDENCE`.

## `func_800AF04C` / `func_800E0E94` inside `func_800AE8A8` is a scale multiply

When `a3+9` is nonzero, AE8A8 calls AF04C and multiplies `f11/f12/f13`
into parent scale. That consume is **not** proven as type-3 Euler on
this path. `CORRECTED/SUPERSEDED`.

## Resource `0x13A` is not the Kazooie model

`0x13A` is `ASSET_13A_ANIM_BSKAZPADDLE_UNKNOWN` in decomp `ba/assets.h`
and decompresses as animation (`0000003f`). Kazooie **geometry** is
`0x61C` (solo / `TRANSFORM_B_KAZOOIE` store at `8080C014`). Combined
BK GeoCmd `0x62B` also contains a Kazooie-on-back type-2 subtree
(head `anim 108`, plumes `100–105`, crest `79`). ExtraSegments `0x61F`
bones 15–24 are the same on-back skeleton **without** GeoCmd `0x02`.
`0x13A` is never geometry. `CORRECTED`.

## `0x83A` is reduced Kazooie, not a Banjo-on-back combined mesh

Overlay 118 `8080BFAC` stores `0x83A` when `a1==0xB` and `8009EA2C!=0`.
The asset is gtype 0, 47 bones, crest INDEX 8 anim 69. It has **no**
Banjo sub-skeleton. `CORRECTED` vs treating every 47-bone body as `0x61F`.

## `part 3 == tag 5` is not an identity

Wobble `func_800E0B8C(part=3)` writes boneAnimID **3 (hip)**.
On `0x62A`, GeoCmd `0x0A` tag 5 is the **right hand** (bone 5 / anim 16).
`REFUTED` as identity.

## Overlay 118 `badata` table `D_80801448` is not a model table

It is an animation-id table. Body candidates for BK sit in a separate
overlay-118 data region around `0x8080C160` (`0x61F`). `CORRECTED`.

## `ctx->r31` is not a generated LOOKUP_FUNC callsite identity

Generated `LOOKUP_FUNC(addr)(rdram, ctx)` for a guest `jal` does **not**
assign `ctx->r31`. Treating wrapper `ctx->r31` as the guest return
address (`0x800926E0` for `0x800926D8`, or overlay RAs
`0x808172A0/0x808172F4/0x8081A4D4`) is `REFUTED` as a generic host
filter. Same class as P08 `func_80013A7C` LOOKUP_FUNC(0). Prefer an
explicit `--wrap` scope. `CORRECTED`.

## `func_800E2180` is not nested under `func_80092444`

Player DE448 submit is immediate `DE498`/`DE2A4`, but wrapped `800E2180`
ran 3491 times with `NO_SCOPE` on a proven player-scope capture. Do not
filter snout matrices on `80092444` TLS. Durable identity is `0x62B`
`model_bin`. `CORRECTED`.

## `0x62B` snout is not at `0x14960`

`0x14960` is the `0x62A` GeoCmd `0x02` snout (INDEX 10). Independently
parsed `0x62B` is 78200 bytes (`0x13178`); `0x14960` is past the end.
`0x62B` snout is GeoCmd `0x02` at `0x11CB8`, INDEX 11, boneAnimID 42,
parent neck 18. `CORRECTED`.

## `func_800E0B8C` is not a per-walk-frame transform

It queues type-3 Euler records. BK walk calls it only if bawobble
`unkC==2`. Zero raw entry on a walk with a working E2180 map hook does
not mean the wrapper failed. `CORRECTED`.

## `func_80092444` is not the babackpack lookup path

`LOOKUP_FUNC(0x80084500)` inside `func_80092444` executed 0 times on the
old path (1503/1503 early-return). That **does not** refute the later
`jal 0x800DE448` at `0x800926D8`. Assess independently. `CORRECTED`.

## E2180 population is not automatically the live `0x62B` snout

The corrected current-build capture reached `func_800E2180` 1,062 times and
had 216 entries inside the durable `0x62B` model span, but none matched the
independent snout identity (`+0x11CB8`, type 2, boneAnimID 42). The bounded
in-span tuple was `a1=model_bin+0x1BD20`, with raw `WORD0=0x000A0004` and
`HWORD_A=0x0020`. E2180 still receives the saved E2AA4 `a1`; therefore the
remaining issue is live GeoCmd population/field decoding, not proof of a
matrix or interpolation bug. Do not treat every E2180 call as the snout
transform until that structure is resolved. `CORRECTED`.

## Allocator/`0x20000` span is not `0x62B` ownership

The durable-bin witness used `rel < 0x20000` as “inside model”.
Decompressed `0x62B` is `0x13178`. Capture `a1=0x80241E50` with
`MODEL_BIN=0x80226130` has `rel=0x1BD20`, which is past
`0x80226130+0x13178=0x802392A8`. That row is **not** inside the asset.
Do not use heap/container extent as model ownership. `CORRECTED`.

## Live `0x62B` snout is not `func_800E2AA4`

Fresh capture: `DURABLE_0X62B_REGISTER_HITS=4106`, `LAST_RESOURCE=0x62B`,
`E2AA4_RAW_ENTRY_COUNT=0`. Static membership “type 2 → E2AA4” is only a
`STATIC_PATH_CANDIDATE`. Compact `0x62B` type 2 is a 0x10-byte header
(`byte(+8)==0x10`); children start at `+0x10`; sibling continue is
`word(+4)`. Root continue 0 means DE2A4 fires one type-2 handler then
stops. `func_800DCF48` recurses with `DE2A4(node+lbu(+8))` and pushes a
**single** G_MTX from `func_800ADE2C(bone INDEX)`. Do not treat
`word(+4)==0x50` on the snout as a fat dual-mtx node size. Do not claim
live `0x62B` snout uses E2AA4/E2180 until a capture shows E2AA4 entries.
`CORRECTED`.

## Wrapped `func_800DE2A4` is not a full node census

The walker loops `node += lw(node+4)` internally. `--wrap` / LOOKUP of
`func_800DE2A4` only sees the **list head**. Live `0x62B` snout `+0x11CB8`
is a sibling after type `0x0A` `@0x11CA0` in the neck child list; its
child `+0x11CC8` **does** appear as a DE2A4 `a1`. `DE2A4_REL_0X11CB8=0`
does not mean the snout branch was skipped. `CORRECTED`.

## `D_8007BCD0` is not the `func_80019AA0` / `mlMtxApply` cursor

Generated MIPS is `lui 0x8008; addiu/lw -0x4350` = **`D_8007BCB0`**.
`mlMtxSet` resets that word to arena base `0x8007B4F0`. `func_80019AA0`
does `*cursor += 0x40` and writes the affine product there.
`mlMtxApply` calls `guMtxF2L(a0=*cursor, a1=entry a0)`. V2 telemetry that
read `D_8007BCD0` (NaNs, `SOURCE_MATCHES_COMPOSED=NO`) is a **wrong
global**, not game-matrix evidence. `CORRECTED`.

## DCF48 G_MTX operand is `gfx_ctx+4`, not GeoCmd `node+4`

`func_800DCF48` saves `s0=a0` (the Gfx packet writer). `mlMtxApply` and
the `G_MTX` word both use `lw 4(s0)`. Compact type-2 `word(+4)` is the
sibling continue, not a matrix pointer. Rotating G_MTX addresses are the
Gfx Mtx output ring. `CORRECTED`.

## State `bs = 4` during Demo 7 overhead cinematic slide is not current state

| Field | Old Claim | Current Proven Fact |
| --- | --- | --- |
| State field | `MEM_W(player + 0x120)` as `bs_cur = 4` | `MEM_W(player + 0x120)` is `bs_prev = 4`; `bs_cur` is at `+0x4` = 116 (`0x74`) |
| Status | `REFUTED` as current state | `PROVEN` `funcs_350.c` / `0x8009E5C8` |
| Meaning | Banjo sliding in State 4 | Banjo sliding in State 116 (`_bsrest_entrypoint_12`) |
| Provenance | Bounded diagnostic analysis | Primary MIPS disassembly of `bs_setState` |

The struct layout at `*(player + 0x120)` is `+0: prev`, `+4: cur`, `+8: next`.
During the stationary/sliding window (`glidx 38..120`), Banjo is in State 116
(`0x74`, `bsrest` overlay 84), while `bs_prev` holds 4. Calling this state "cinematic 4"
is `CORRECTED/SUPERSEDED`.

## Prior claim "no reset in 116 init helpers" is refuted

| Field | Old Claim | Current Proven Fact |
| --- | --- | --- |
| State 116 INIT | "No init helper resets tspd/vel" | Helper at `0x80800000` (`static_344_80800000`) clears `tspd=0` and vel to 0 |
| Status | `REFUTED` | `PROVEN` via MIPS trace and caller audit |
| Defect Location | Guest state logic bug | Generator bug: `RecompiledFuncs/funcs_416.c` emitted empty body |
| Provenance | Initial disassembly review | Oracle seq10/11 callstack trace and N64Recomp symbol sizing investigation |

The helper at `0x80800000` in overlay 84 (`bsrest`) calls
`baphysics_set_target_horizontal_velocity(ps, 0)` and clears velocity vectors.
It is invoked at `0x80800308` from `0x8080020C`, called by `bsrest` INIT ep9
(`0x808005B8`). The helper was emitted as an empty function by `N64Recomp`
due to zero-size ELF marker symbols at that address. `CORRECTED/SUPERSEDED`.

## `bs_setState` entry point is `0x8009E5C8`, not `0x8009E5D8`

Address `0x8009E5D8` was an interior instruction inside `bs_setState`. The actual
function entry point is `0x8009E5C8` (`funcs_350.c:1434`), which receives `(player, new_state)`
and sets `next` (`+0x8`) or updates `prev` (`+0x0`) and `cur` (`+0x4`). `CORRECTED`.

## `func_800EF04C` is a 3D vector addition utility, not a motion integrator

`func_800EF04C` computes `out[i] = a[i] + b[i]` for 3D float vectors. It does not
perform physical kinematic motion integration ($x_{t+1} = x_t + v \cdot \Delta t$).
Treating `func_800EF04C` as the physics integrator is `CORRECTED/SUPERSEDED`.

## First spatial candidate list entry is not the first qualifying candidate

In `func_800CD820` swept sphere queries over spatial grid lists (entry stride 12),
the first entry in a spatial list is NOT necessarily the first candidate that qualifies
for collision checks. `_subaddiecustomhits_entrypoint_5` (`0x8008A018`) evaluates
candidates individually. Collapsing the first list candidate to the first qualifying
candidate is `CORRECTED/SUPERSEDED`.

## Custom bound center is not proven actor base/world position

`_subaddiecustomhits_entrypoint_5` (`0x8008A018`) checks descriptor word `*(*(*a0) + 8)`.
If bit 1 is SET, it copies actor record `+4/+8/+0xC` floats directly; ELSE it branches to
`func_800EC75C` custom offset calculation. The relationship between the returned bound
center and the actor's base or world position is `UNKNOWN` pending descriptor bit 1
branch proof. Calling the observed 427–431 unit center difference an "enemy position" or
"world position" difference before that proof is `CORRECTED/SUPERSEDED`.

## Diagnostic halfword reader 4-alignment restriction

Diagnostic reader `tooie_d1p_hu` previously required 4-byte aligned addresses, falsely
returning 0 for halfwords at `+2` within an aligned word (including `+0x1A`, `+0x66`, `+0x6A`).
Prior host observation that `lhu(+0x66) == 0` was an artifact of the reader defect, not
guest memory. The native helper now validates the containing 4-byte word. `CORRECTED`.

## Guest VI cadence causality in attract demo timing

Stating that the 100/419 VI cadence difference is proven non-causal was `OVERSTRONG`.
The player controller sequence matches 420/420 entries at `0x800151BC` ENTRY, but player
input parity does NOT refute NPC or mob timing divergence driven by VI or frame counters.
Current status: VI handler cadence difference is observed; causal role in NPC/mob timing is
`UNKNOWN / OPEN HYPOTHESIS`; `--nospeedlimit` is logical execution only, not realtime proof.
`CORRECTED/SUPERSEDED`.
