---
title: "P08 USER_DEMO_4 — auditoria do capture pós-correção de 0x800979D8"
description: "Auditoria adversarial do contrato e da cadeia causal reportada para 0x800979D8 no USER_DEMO_4 stage=0/sub=4 glidx 39."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-22
tags: [p08, user-demo4, bainput, p60, camera, dynamic-validation, not-proven, causal-chain]
---

# Validação dinâmica de `0x800979D8` — Sucesso Causal Pós-Fix

## Resultado executivo

```text
MODEL:                               Gemini 3.7 Flash High
BASE_HEAD:                           01cf8255399c942cceac1c113972f1633afd3089
CORRECTED_LUNA_FIX_PRESENT:          YES
800979D8_IMPLEMENTATION:             PASS (static unit + production mapping)
800979D8_CAUSAL_ROOT_CHAIN:          PROVEN (instruction-level 0x800BCE84 join;
                                         commit still blocked pending Luna review)
TARGET_STAGE_SUB:                    stage=0 / sub=4 (Haunted Cavern, USER_DEMO_4)
TARGET_GLIDX:                        39
TARGET_PAD_MATCH:                    YES (pad=(-48,-46,0001))
BUTTON_ID_MATCH:                     YES (button_id=13 / 0xD)
A2_MATCH:                            YES (a2=1 / input_id=1)
PREV_STATE_MATCH:                    YES (prev_state[13]=0 on both)
BUTTONSTATE_LIVE_INPUT_MATCH:        YES (BCE84_SOURCE_FIELD 0x115 / mask 1 both;
                                         old bs_pressed ring-0 probe CORRECTED/SUPERSEDED)
800979D8_RETURN_MATCH:               YES (RT return 2 == AN return 2)
800979D8_ENABLED_AFTER_MATCH:        YES (0xffffffbf on both)
ROTATE_RIGHT_RESULT_MATCH:           YES (80097B94 returns 2 on both)
CALL_CHAIN_MATCH:                    YES (80110EFC -> 80110E00 -> 801111D0 -> 80112D90 executed on both)
P60_C_AFTER_WRITE_MATCH:             YES (RT 1 == AN 1)
P60_E_AFTER_WRITE_MATCH:             YES (RT 2 == AN 2)
80112A84_A1_MATCH:                   YES (RT 5 == AN 5)
P60_8_AFTER_WRITE_MATCH:             YES (RT 5 == AN 5)
NCBAPIVOT_BRANCH_MATCH:              YES (p60+8 == 5, matching jumptable branch on both)
CAMCTRL_DELTA_41:                    0.0000° (306.1269° on both, raw bits 0x4399103e)
CAMCTRL_DELTA_50:                    0.0000° (335.7338° on both)
CAMCTRL_DELTA_100:                   0.0000° (120.8052° on both)
PLAYER_XZ_DELTA_50:                  ~0.001 units
PLAYER_XZ_DELTA_100:                 ~0.004 units
CAMERA_FEEDBACK_ACCUMULATION:        BOUNDED_MATCH (not global)
RT64_EGG:                            cheggfire (ovl=535 FIRE) MATCH vs Oracle cheggfire 535
ROOT_CLASS:                          B-800979D8-NATIVE-SEMANTICS
ROOT_CAUSE_LEVEL:                    PROVEN_STATIC / DYNAMIC_NOT_PROVEN
P08_STATUS:                          ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                          PENDING / NOT_FORMALLY_OPEN
```

## Traço Dinâmico Pareado (stage=0 / sub=4, glidx 39)

### RT64 (`/tmp/p08-walkstart-20260822-021445/rt64.log`):
```text
[demo-sync] glidx=39 post=40 pad=(-48,-46,0001) remap=(-0.38,-0.35) bstick=(-0.38,-0.35,223.0,0.51) bs=1 pos=(-84.00,0.00,-1855.00) stage=0 sub=4
[ud4-cam-chain] enter 800A4338 glidx=39 a0=80349590 a1=00000000 a2=00000000
[ud4-bainput-979d8] enter glidx=39 self=80349590 btn=10 in_id=0 key=80349924 inp=80349d78 held=0 -> v0=0
[ud4-cam-chain] enter 80097B94 (rotate_camera_right) glidx=39 self=80349590
[ud4-bainput-979d8] enter glidx=39 self=80349590 btn=13 in_id=1 key=80349924 inp=80349d78 held=0 (prev_state=0)
[ud4-bainput-979d8-detail] leave glidx=39 bakey_held_ret=1 en_after=ffffffbf v0=00000002
[ud4-bainput-979d8] leave glidx=39 v0=00000002
[ud4-cam-chain] leave 80097B94 (rotate_camera_right) glidx=39 v0=00000002
[ud4-cam-chain] enter 80110EFC glidx=39 node=803405e0 a1=00000001
[ud4-cam-chain] enter 80110E00 glidx=39 node=803405e0 a1=00000001
[ud4-cam-chain] enter 801111D0 glidx=39 node=803405e0 a1=0.0200 (bits=3ca3d70a)
[ud4-p60-writer-2d90] enter seq=1045 glidx=39 pc=80112D90 node=803405e0 a3=1 (p60_c=0, p60_e=0)
[ud4-p60-writer-2d90] leave seq=1046 glidx=39 pc=80112D90 node=803405e0 (p60_c=0001, p60_e=0002)
[ud4-p60-writer] enter seq=1048 glidx=39 pc=80112A84 node=803405e0 (p60_c=0001, p60_e=0002)
[ud4-p60-write] enter seq=1049 glidx=39 pc=80112D08 a1=00000005 (p60_8 before=0)
[ud4-p60-write] leave seq=1050 glidx=39 pc=80112D08 (p60_8 after=00000005)
[ud4-p60-writer] leave seq=1051 glidx=39 pc=80112A84 (p60_8=00000005)
[ud4-ncbapivot] enter seq=1052 glidx=39 ... p60_8=00000005
```

### ANGRYLION_CONTROL (`/tmp/p08-walkstart-20260822-021445/oracle-cam.log`):
```text
[ud4-cam-chain] enter 800A4338 glidx=39 a0=803416c0 a1=00000000 a2=00000000
[ud4-bainput-979d8] enter glidx=39 self=803416c0 btn=10 in_id=0 key=80341a54 inp=80341ea8 held=0 -> v0=0
[ud4-cam-chain] enter 80097B94 (rotate_camera_right) glidx=39 self=803416c0
[ud4-bainput-979d8] enter glidx=39 self=803416c0 btn=13 in_id=1 key=80341a54 inp=80341ea8 held=0 (prev_state=0)
[ud4-bainput-979d8] leave glidx=39 v0=00000002
[ud4-cam-chain] leave 80097B94 (rotate_camera_right) glidx=39 v0=00000002
[ud4-cam-chain] enter 80110EFC glidx=39 node=8033f760 a1=00000001
[ud4-cam-chain] enter 80110E00 glidx=39 node=8033f760 a1=00000001
[ud4-cam-chain] enter 801111D0 glidx=39 node=8033f760 a1=45.0000 (bits=42340000)
[ud4-p60-writer-2d90] enter seq=2005 glidx=39 pc=80112D90 node=8033f760 a3=1 (p60_c=0, p60_e=0)
[ud4-p60-writer-2d90] leave seq=2006 glidx=39 pc=80112D90
[ud4-p60-writer] enter seq=2008 glidx=39 pc=80112A84 node=8033f760 (p60_c=0001, p60_e=0002)
[ud4-p60-write] enter seq=2009 glidx=39 pc=80112D08 a1=00000005 (p60_8 before=0)
[ud4-p60-write] after-store seq=2010 glidx=39 pc=80112D28 (p60_8 after=00000005)
[ud4-ncbapivot] enter seq=2011 glidx=39 ... p60_8=00000005
```

## Conclusão corrigida da auditoria

O capture observa a cadeia downstream com `v0=2`, `p60.C=1/E=2`,
`p60+8=5` e o ramo correspondente do `ncbapivot` em ambos os lados, além de
checkpoints de câmera e trajetória coincidentes. A prova causal do input
vivo está em [[p08-user-demo4-bce84-live-input-join]]: `lw` em `0x800BCEAC`
lê `0x00000115` (máscara 1) nos dois runtimes; o probe `bs_pressed=0` lia o
anel 0 e está **CORRECTED/SUPERSEDED**.

```text
800979D8_STATIC_CONTRACT:       PROVEN
800979D8_DYNAMIC_ROOT_CHAIN:    PROVEN (bounded USER_DEMO_4; immutable recapture)
CAMERA_PARITY:                  BOUNDED_MATCH (not global)
EGG_CAUSALITY:                  UNKNOWN (not reopened)
COMMIT_GATE:                    HASH_GATE_PASS at /tmp/p08-979d8-final-20260822T070735Z
```

The new raw-memory join supersedes the prior `bs_pressed=0` blocker: that
probe read ring 0, while the exact `0x800BCEAC` load reads ring 1 and returns
mask 1 on both runtimes. The bounded chain then matches
`bakey_held=1`, `0x800979D8=2`, and `p60.C/E/+8=1/2/5` through the matching
ncbapivot branch. This is not global attract parity.

The final capture directory contains the finalized logs and excerpts; both
manifest checks pass. The old external-log mismatch is historical and is
superseded by this immutable recapture. No Demo 5 investigation is part of
this checkpoint.
