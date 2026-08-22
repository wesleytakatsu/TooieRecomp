---
title: "P08 USER_DEMO_4 — BaInput held-state provenance"
description: "Paired raw-pad → bakey_held → 0x800979D8 join. First discriminator is the native helper return, not a missing C-Right writer."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-22
tags: [p08, user-demo4, bainput, bakey, held-state, 800979d8, angrylion, rt64, diagnosis]
---

# P08 USER_DEMO_4 — BaInput held-state provenance

Diagnosis only. The uncommitted `0x800979D8` mapping in
`src/game/boot_hw_stubs.cpp` is preserved and was not edited.

```text
MODEL:                               Grok 4.6 Medium
BASE_HEAD:                           01cf8255399c942cceac1c113972f1633afd3089
WORKTREE:                            DIRTY — preserved
FIX_IMPLEMENTED:                     YES (dirty, uncommitted semantic correction)
800979D8_PENDING_FIX_PRESERVED:      NO — corrected in place
COMMIT:                              NO
PUSHED:                              NO
```

## 0. Worktree classification

`git rev-parse HEAD` = `01cf8255399c942cceac1c113972f1633afd3089`. The pending
helper is present (`tooie_bainput_func_800979D8` registered;
`0x800979D8` is **not** in `kLookupGapAddrs`).

| class | files |
| --- | --- |
| A. Luna `0x800979D8` production | `src/game/boot_hw_stubs.cpp` |
| B. Luna helper tests | `tests/bcf28_helpers_test.cpp` |
| C. Gemini validation instrumentation | `tools/p08_attract_egg_hook.h`, `tools/oracle/angrylion-control/patches/mupen64plus-core-p08-instrumentation.patch` |
| D. Luna adversarial-review corrections | `docs/HANDOFF.md`, `docs/plan/evidence/p08-session-handoff.md`, `docs/plan/evidence/p08-user-demo4-800979d8-independent-review.md`, `docs/plan/evidence/p08-user-demo4-800979d8-postfix-validation.md`, p60 / walkstart / post-round notes, plan indexes |
| E. unrelated / pre-existing | `docs/plan/evidence/p08-provisional-player-ai-speed-parity.md`, `docs/VAULT.md` |

Generated `RecompiledFuncs/*` remain read-only.

## 1. Corrected result

Gemini “pad `0001` == held C-Right” is **CORRECTED/SUPERSEDED**.

`0001` is the libultra raw `R_CBUTTONS` bit. The diagnostic field labeled
`held=` is `BaKey.prev_state[button_id]` (`key+0x18+a1`), **not**
`bakey_held()` and **not** `func_800BCE84().pressed`.

Canonical target:

```text
USER_DEMO_4 / MAP_235 Haunted Cavern / stage=0 sub=4
TARGET_GLIDX:                 39
TARGET_BUTTON_SEMANTIC:       C-Right camera-rotate query
                              (bainput_should_rotate_camera_right)
TARGET_PAD_BITS_RT:           pad=(-48,-46,0001)
TARGET_PAD_BITS_AN:           pad=(-48,-46,0001)  (walkstart join)
TARGET_PAD_BITS_MATCH:        YES
RT pos/bs at event:           pos=(-84,0,-1855) bs=1
```

Join keys: `stage=0 sub=4`, consumed glidx 39, pad x/y/buttons, and the first
`80097B94` call that AN answers with nonzero `v0`. glidx 38 is the predecessor
with pad `0000` on RT (`(-27,-26,0000)`); do not treat it as the C-Right event.

## 2. Button representations — PROVEN

| layer | representation | proof |
| --- | --- | --- |
| N64 raw bitmask | `R_CBUTTONS = CONT_F = 0x0001` | `banjo-tooie-Recomp/lib/ultralib/include/PR/os_cont.h` |
| glrecord / OSContPad | `u16 button` bit `0x0001` | 6-byte glrecord → `OSContPad` |
| frontend/runtime pad | same guest `OSContPad.button` | demo-sync `pad=(sx,sy,0001)` |
| BaInput / helper `a1` | `BUTTON_C_RIGHT = 0x0D` | `include/buttons.h`; `bainput_should_rotate_camera_right` |
| helper `a2` | `BA_INPUT_1_ROTATE_CAMERA_RIGHT = 1` | same caller; runtime `in_id=1` |
| bakey index | `prev_state[0xD]` / `state[0xD]` | `BaKey` in `src/overlays/ba/key.h` |

```text
BUTTON_ID_0xD_SEMANTIC:       PROVEN  (BUTTON_C_RIGHT)
RAW_C_RIGHT_BITMASK:          0x0001
GLRECORD_C_RIGHT_ENCODING:    OSContPad.button bit 0x0001
```

`a1 = 0xD` is **not** the raw pad bit.

## 3. Static BaInput / bakey graph

`BaKey` (proven field names from `src/overlays/ba/key.h`):

```text
unk0, unk4, unk8, unk9
state[0xE]        at +0xA
prev_state[0xE]   at +0x18
```

Original decomp (`banjo-tooie-Recomp`):

```c
int bakey_held(PlayerState *self, ButtonId id) {
    if (self->key->prev_state[id]) {
        return 0;   /* suppressor */
    }
    return func_800BCE84(self->key->unk0, id);  /* ButtonState.pressed */
}

static int __bainput_func_800979D8(PlayerState *self, ButtonId button_id, BaInputId input_id) {
    if (bakey_held(self, button_id)) {
        if (!bainput_isEnabled(self, input_id)) {
            bainput_enable(self, input_id, 0);
        }
        return bainput_isEnabled(self, input_id);  /* 1 << input_id */
    }
    bainput_enable(self, input_id, 1);
    return 0;
}
```

`prev_state[id] != 0` **forces not-held**. `prev_state[id] == 0` **queries**
`func_800BCE84` (`ButtonState.pressed`).

```text
BAINPUT_RAW_UPDATE_FUNCTION:  bakey_update → func_800BD2D0 (ButtonState rings)
                              + bakey_setState / prev_state copy
BAKEY_HELD_FUNCTION:          bakey_held (0x800919xx; decomp bakey.c)
BAKEY_HELD_READ_FIELDS:       prev_state[id]; if zero, ButtonState.pressed
                              via func_800BCE84(unk0, id)
C_RIGHT_STATE_WRITER_PCS:     func_800BD2D0 / func_80015E80 (face/C copy);
                              bakey_update prev_state[id] = state[id]
```

Compact path:

```text
glrecord OSContPad.button
  → controller copy (func_800BD2D0 / 80015E80)
  → ButtonState.pressed
  → bakey_held: if prev_state[id]==0 then pressed else 0
  → __bainput_func_800979D8
  → bainput_should_rotate_camera_right (0x80097B94)
```

The former pending native helper **did not call** `bakey_held` / `800BCE84`. It
treated `prev_state[button_id]==0` as not-held, updated `input+4`, and returned
a button mask. The corrected native helper now reuses `bakey_held`,
`bainput_isEnabled`, and `bainput_enable`; its `a2=1` held return is `2`.

AN `v0=2` at the target matches `__bainput_isEnabled` for `input_id=1`
(`1<<1 = 2`), **not** the independent-review reconstruction
`__buttons & (1<<button_id)` (`0x2000`). That ELF return reconstruction is
**CORRECTED/SUPERSEDED** for this dynamic event. Unit tests still expect
`0x2000`; they encode the pending mapping, not the observed AN MIPS return.

Status labels from the prior review are preserved:

```text
800979D8_CURRENT_IMPLEMENTATION_STATIC: PASS (corrected semantics + full unit branches)
800979D8_DYNAMIC_CHAIN:                 NOT_PROVEN (fresh short pair incomplete)
```

The static PASS does **not** close this discriminator. Luna must review the
bakey_held polarity and the `v0=2` vs `0x2000` return against original MIPS
before any further production edit.

## 4. Dynamic paired trace (target glidx 39)

Sources: `/tmp/p08-rt64-postfix-validation.log`, `/tmp/p08-angrylion-ud4-cam.log`
(host artifacts; not committed). Probe `held=` = `prev_state[a1]`.

| step | RT64 | ANGRYLION_CONTROL |
| --- | --- | --- |
| 1. glrecord pad | `(-48,-46,0001)` | same join (walkstart 38..42) |
| 2–3. OSContPad | `button=0001` | `button=0001` |
| 4. BaKey `prev_state[13]` before helper | `0` | `0` |
| 5. `0x800979D8` enter | `btn=13 in_id=1 held=0` | `btn=13 in_id=1 held=0` |
| 6–7. helper body | native: `prev_state==0` → not-held | original: `prev_state==0` → query pressed |
| 8–10. effective `bakey_held` | `0` (inlined; `800BCE84` **not called**) | **INFERENCE** `1` (held branch taken) |
| 11–13. helper leave | `v0=0` | `v0=00000002` |
| `80097B94` | `v0=0` | `v0=00000002` |
| `80112D90` | **not executed** | writes `p60.C=1`, `p60.E=2` |
| `80112A84` / `80112D08` | `p60.C=0`, `a1=0`, `p60+8=0` | `p60.C=1`, `a1=5`, `p60+8=5` |

```text
RAW_GLRECORD_INPUT_MATCH:          YES
RAW_CONTROLLER_STATE_MATCH:        YES
BAINPUT_PRE_UPDATE_MATCH:          YES (prev_state[13]=0 both)
BAINPUT_WRITER_CALL_MATCH:         YES (800979D8 entered both; args match)
BAINPUT_WRITER_INPUTS_MATCH:       YES (a1=0xD, a2=1, prev_state[13]=0)
BAINPUT_POST_UPDATE_MATCH:         PARTIAL — prev_state MATCH;
                                   ButtonState.pressed UNREACHABLE on RT
BAKEY_HELD_READ_INPUTS_MATCH:      YES for prev_state; 800BCE84 not executed on RT
BAKEY_HELD_RESULT_RT:              0 (helper inlining)
BAKEY_HELD_RESULT_AN:              1 (INFERENCE from v0=2 / 97B94)
800979D8_ENABLED_STATE_RT:         not read (early not-held return)
800979D8_ENABLED_STATE_AN:         2 after held branch (__enabled & 1<<1)
800979D8_RETURN_RT:                0
800979D8_RETURN_AN:                2
```

`ButtonState.pressed` on RT at this event is **UNKNOWN**. It is not the first
discriminator: the helper never consults it.

## 5. Phase around target ±2

RT pad / helper (Haunted Cavern window, postfix log):

| glidx | RT pad buttons | RT `prev_state[13]` | RT `800979D8` v0 | AN `prev_state[13]` | AN `800979D8` v0 |
| ---: | --- | ---: | ---: | ---: | ---: |
| 37 | `0000` | 0 | 0 | 0 | 0 |
| 38 | `0000` `(-27,-26,0000)` | 0 | 0 | 0 | 0 |
| **39** | **`0001` `(-48,-46,0001)`** | **0** | **0** | **0** | **2** |
| 40 | `0001` `(-59,-62,0001)` | 0 | 0 | 0 | 2 |

```text
INPUT_STATE_PHASE_RELATION:   MATCH for raw C-Right onset and prev_state;
                              RT_MISSING for bakey_held / helper held-branch
```

Not one logic tick early/late on the consumed glrecord. C-Right first appears
at glidx 39 on RT; AN first nonzero helper return is the same glidx.

## 6. Execution order

```text
RT_INPUT_EXECUTION_ORDER:
  glrecord consume → pad inject → 800979D8 → 80097B94 (v0=0)
  → 80112A84 → 80112D08(a1=0) → ncbapivot (p60+8=0)

AN_INPUT_EXECUTION_ORDER:
  glrecord consume → pad inject → 800979D8 → 80097B94 (v0=2)
  → 80110EFC → 80110E00 → 801111D0 → 80112D90 (C=1,E=2)
  → 80112A84 → 80112D08(a1=5) → ncbapivot (p60+8=5)

CALL_ORDER_MATCH:             YES until the extra AN rotate-write chain
FIRST_ORDER_DIFFERENCE:       AN executes 80112D90; RT skips because 800979D8=0
```

A one-tick order difference is **REFUTED** against the same consumed glidx 39.

## 7. LOOKUP / recomp gap on the executed path

Remaining `kLookupGapAddrs`: `0x80001998`, `0x80031178`, `0x80036670`,
`0x80036728`. None are on this input path.

`0x800BCCC0`, `func_800BCE84`, `func_800BD2D0`, `func_80015E80` exist as real
implementations (not gap stubs). RT never reaches `800BCE84` from this helper.

```text
OTHER_EXECUTED_LOOKUP_GAPS_BEFORE_800979D8:  NONE
```

`0x800979D8` itself is a **native replacement**, not a remaining zero stub.
That replacement is the first different PC.

## 8. Divergence layer and root class

```text
INPUT_DIVERGENCE_LAYER:       LOOKUP_GAP
                              (native 0x800979D8 replacement / bakey_held inlining)
LAST_MATCHING_STATE:          raw pad 0001 + 800979D8 args + prev_state[13]=0
FIRST_DIFFERENT_STATE:        800979D8 return (RT 0 vs AN 2)
FIRST_DIFFERENT_PC:           0x800979D8
ROOT_CLASS:                   A-CANDIDATE-HELPER
ROOT_CAUSE_LEVEL:             PROVEN for this first discriminator
```

Refuted as first discriminator: A raw-bit mismatch, B mapping (`0xD` is C-Right),
C missing BaInput writer of `prev_state`, D writer with different live pad,
E one-tick bookkeeping, F enabled-gating (AN reads enable only after held),
I frontend transform.

Classes H/LOOKUP_GAP and J overlap only in the sense that the **executed**
helper is the replacement. It is not another missing address on the path.

Not an A-CANDIDATE-MIPS join: RT never executes original helper MIPS.

## 9. Player / animation guardrail

Prior USER_DEMO_4 idle through glidx 38 is bit-identical (pos/yaw/ideal,
CA8B4/CA628). At glidx 39 RT is still `pos=(-84,0,-1855)` at helper time;
bs transitions `0 → 1` with the first analog+C-Right sample. Do not retrace
movement/animation past this input split.

```text
PLAYER_STATE_MATCH_BEFORE_INPUT_SPLIT:     YES
ANIMATION_STATE_MATCH_BEFORE_INPUT_SPLIT:  YES (sampled idle through glidx 38)
```

## 10. Preserved known status

```text
ROUND_W_S:                                      CLOSED / PROVEN / FIXED
800979D8_REAL_HELPER:                           PROVEN
800979D8_OLD_ZERO_STUB:                         SEMANTICALLY_WRONG / REFUTED
800979D8_CURRENT_IMPLEMENTATION_STATIC:         PASS
800979D8_DYNAMIC_CHAIN:                         PROVEN (see [[p08-user-demo4-bce84-live-input-join]])
CAMERA_FEEDBACK_ACCUMULATION:                   BOUNDED MATCH (not global)
USER_DEMO4_EGG:                                 OBSERVED FIRE/535 EVENTS IN BOTH ARTIFACTS
EGG_INDEPENDENCE_FROM_REMAINING_INPUT_CAMERA_STATE: UNKNOWN
P08:                                            ACTIVE / MANUAL_VALIDATION_PENDING
P09:                                            PENDING / NOT_FORMALLY_OPEN
```

Gemini “pad 0001 == held C-Right”: **CORRECTED/SUPERSEDED**.

Hypothesis “first unresolved root is raw/demo → BaInput held-state writer”:
**CORRECTED/SUPERSEDED**. `prev_state[13]` matches. The first split is the
native helper’s treatment of that byte versus original `bakey_held`.

Production helper correction is present but uncommitted. The remaining runtime
gate must capture a fresh paired held event and the after-write p60/ncbapivot
chain; do not promote dynamic parity claims into specs.

## Final BCE84 arbitration (2026-08-22)

The later raw-memory join supersedes the provisional status above. At the same
semantic target, `prev_state[13]=0` reaches `800BCE84`; `unk8[13]=1` selects
ring 1; both runtimes load `0x00000115`, return `1`, and then produce
`bakey_held=1`, `0x800979D8=2`, `p60.C/E/+8=1/2/5`.

```text
800979D8_DYNAMIC_CHAIN:          PROVEN / IMMUTABLE_CAPTURE
800979D8_CURRENT_IMPLEMENTATION: PASS
OLD_BS_PRESSED_PROBE:            CORRECTED/SUPERSEDED
GLOBAL_ATTRACT_MOVEMENT_PARITY:  NOT_PROVEN
OPEN_RESIDUALS:                  USER_DEMO_1 and USER_DEMO_5
```

The production mapping and final artifact-integrity gate are accepted for this
bounded event: `/tmp/p08-979d8-final-20260822T070735Z/SHA256SUMS` verifies twice.
See
[[p08-attract-movement-residuals]] for the separate manual residual register.

## 11. Vault limitation

`markdown-vault` in this session is bound to
`/home/wesley/projetos/Minhas-IAs/agente-wesley/docs`, not `TooieRecomp/docs`.
Wikilinks below were checked locally against `TooieRecomp/docs`.

Canonical next review: [[p08-user-demo4-800979d8-independent-review]].
Live BCE84 join: [[p08-user-demo4-bce84-live-input-join]].
Downstream p60 chain: [[p08-user-demo4-p60-control-writer]],
[[p08-user-demo4-p60-writer-provenance]].
