---
title: "Animation"
description: "Primary clip selection vs bone compose. Walk clip is not the body model."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-08
tags: [reverse-engineering, animation]
---

# Animation

## Clip selection (`PROVEN`)

`func_80800000_badata(PlayerState*)` → appearance index.

| Transform | Land appearance | Walk/move clip (table) |
| --- | --- | --- |
| `TRANSFORM_1_BK` | `0x6` | `0x56` |
| `TRANSFORM_A_BANJO` | `0xF` | `0x56` (same table slot pattern; confirm per row if needed) |
| `TRANSFORM_B_KAZOOIE` | `0x10` | `0x13A` |
| `TRANSFORM_D_MUMBO` | `0x11` | Mumbo clip `0xEC` is a **typed** animation, not proven as this row's id without the table dump |

`badata_entrypoint_27` returns `D_80801448[appearance]`. That table is
**animation IDs** (`CORRECTED` vs old model-table claim).

## Bone compose (`PROVEN`)

The local 4x4 writer is `func_800AE820` (Tooie analog of BK
`animMtxList_setBoned`):

```text
func_800AE160 / func_800AE1E8
  a2 = AnimMtxList + 0x50
  func_800AE820
    func_800AE8A8   // per-bone quat compose + scale multiply
    func_800AEAA0   // quat + scale -> 3x3
    func_800AEBA8   // translation into 4x4
```

Skeleton bone: stride `0x10` at `skel+8`; `+0xC` = boneAnimID;
`+0xE` = parent index. Animation record: stride `0x28` at
`*animList + boneAnimID*0x28` with quat **XYZW** at `+0x00..+0x0C`
(X at +0, W at +0xC) and **scale** `+0x10/+0x14/+0x18`.
`func_800AE8A8` sets `q_slot = q_parent ⊗ q_record` (Hamilton) and
`s_slot = parent_scale * local_scale`. Offline 0x62B check: product
error ≤ 1.6e-7 rad. Clip playback is `baanim_update` (`0x8008C894`). `func_8008ADE4`
advances the AnimCtrl timer only (`PROVEN` `STATIC_MIPS`): sample
time is `Animation+0x10` (`func_8008C0C8`), clip id is
`Animation+0x18` (`func_8008C0C0`), duration/rate is `anctrl+0xC`,
wrap is `anctrl+0x1A`. It does **not** write 0x28 bone records.
`__baanim_update_scaleToHorizontalVelocity` is clip **rate** vs
`|v_xz|` from `*(player+0xC8)+0x10`, not bone-107 TRS scale.
P09 sample-time witness: `p09-feather-anim-sample-time-v1`.
Pose apply is `func_800E0170` (`PROVEN`): cart flag-`0x16` header
`s16 start,end,count`; dirent `(boneAnimID<<4)|channel`; keys
`u16 time_flags + s16 value`; `func_800E04C0` linear or cubic;
`func_800E084C` Euler degrees → XYZW. Clip id `Animation+0x18` is
the D674C resource id. Normalized sample time `Animation+0x10`.
`baanim_playForDuration_once` / `_loop` / `*StartingAt` change clips.
P09 clip22 107/108 raw 0x28 matches ROM keys
(`PROVEN`): `p09-animation-clip-transition-and-authored-key-forensic`.
ADE2C reads the same slot: `list+0x50+(index<<6)` (bone 30 = `+0x7D0`).

`func_800AF04C` is **not** the 4x4 writer. Inside `func_800AE8A8`, if
`a3+9` is nonzero, `AF04C` → `func_800E0E94` and AE8A8 multiplies
`f11/f12/f13` as **scale** (`CORRECTED/SUPERSEDED` vs “type-3 Euler
consume” on this path). `func_800AEDA0` is the optional quat additive
when `a3+8` is nonzero. `func_800E0B8C` still writes type-3 0x14-byte
records; presence on a given bone is conditional.

Primary walk clip and secondary `bawobble` are different layers. See
[[secondary-animation]]. `bawobble` is CONDITIONAL (`unkC==2`), not a
per-frame walk transform. Bone local matrices **may legally contain
animated scale**.

P09 V2 runtime confirmation (`PROVEN_FOR_CAPTURE`): in the live combined
`0x62B` Kazooie feather path, boneAnimID 103's raw record scale is unit while
parent index 25 / boneAnimID 107 carries a uniform scale ramp. AE8A8 applies
that inherited scale before AE820 builds the local 4x4, and no AF04C/E0E94
additive record is reached when the additive context byte 9 is zero. The low
determinant samples are therefore a legal uniform-scale state in this
capture, not evidence of a singular matrix. See
`p09-kazooie-feather-v2-producer-witness`.

## Diagnostic wrapper ABI rule (`PROVEN`, 2026-09-04)

Recomp-context guest pointers in GPRs can be sign-extended KSEG0 values such
as `0xFFFFFFFF80xxxxxx`. Diagnostic wrappers that save and restore those GPRs
must preserve the full `gpr` width; converting them through `uint32_t` and
restoring zero-extended values can corrupt the guest pointer before the
original function's matrix work completes. This is a diagnostic integration
rule, not an original-game behavior claim.

## Modding relevance

**PROVEN ROLE:** appearance → clip ID; bones composed by anim ID.

**POTENTIAL MODDING USE:** retarget `D_80801448` rows; do not treat clip
IDs as meshes.
