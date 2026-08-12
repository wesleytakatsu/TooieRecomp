---
title: "P08 — slot7 table[2] object lifecycle arbitration (Grok)"
description: "Arbitragem independente do objeto table[2].word0=0x8005C168, pool 0x48, clearer/driver e ordering vs C2AB8 busy-wait."
status: evidence
phase: P08
updated: 2026-08-12
tags: [tooie-recomp, p08, slot7, lifecycle, arbitration, audio-pool, C2AB8]
---

# P08 — SLOT7 OBJECT LIFECYCLE ARBITRATION

## Meta

```text
BASE_HEAD:   28634288c1c17ac0cb2a75371d911e1889f33908
GROK_MODEL:  4.5 HIGH
MODE:        REVIEW / ROOT-CAUSE SEMANTICS ONLY (no code, no fix)
METHOD:      static reconstruction of recompiled C (MIPS-faithful) +
             treat prior clean-freeze dynamics as PROVEN
```

## MODEL_DISAGREEMENT

### Gemini 3.6 Flash HIGH — static lifecycle

```text
SUBSTATE2:              pool handle 2
TABLE2_OBJECT_CREATOR:  func_80020790
OBJECT_TYPE:            SoundVoiceInstance / audio SFX event
OBJECT_OWNER:           audio subsystem
OBJECT_ORIGIN:          STALE_INTRO
CLEARER:                func_800205D0
CLEARER_DRIVER:         func_800200B8 / audio lifecycle
CLEARER_BEFORE_C2AB8:   NO
PRIMARY_HYPOTHESIS:     Jiggy/intro SFX still active when teardown starts
ROOT_CAUSE_PROVEN:      YES
```

### DeepSeek V4 Flash HIGH — clean dynamic snapshot

```text
table[2].word0 = 0x8005C168
object: VALID, pool stride 0x48
active list: head == tail == object, next=prev=NULL
free list: intact; object not on free list
object+0x30 = 0x80128B18  (exact backref to table[2])
countdown +0x34 = 2 (still initial)
object state: LIVE
func_800205D0: NEVER executed
manager membership: VALID
corruption: NO
OBJECT_ORIGIN: ATTRACT
STALE_OBJECT: NO
ROOT_CAUSE_PROVEN: NO
driver claim: func_800A907C -> func_8001EE34 (MAIN stuck in C2AB8 so driver never ticks)
```

### ARBITRATION (Grok)

| claim | verdict | level |
| --- | --- | --- |
| creator is `func_80020790` | **ACCEPT** | PROVEN static |
| clearer is `func_800205D0` via `func_800200B8` | **ACCEPT** (only static callers) | PROVEN static |
| object is audio-related pool event | **QUALIFY** (subsystem audio; name SoundVoiceInstance unproven) | PROVEN subsystem / UNKNOWN exact type name |
| STALE_INTRO origin | **REJECT as proven** | INFERENCE only |
| ATTRACT origin | **REJECT as proven** | INFERENCE only (countdown=2 alone insufficient) |
| `CLEARER_BEFORE_C2AB8: NO` on transition path | **ACCEPT** | PROVEN static order |
| Gemini `ROOT_CAUSE_PROVEN: YES` | **REJECT** | overclaim |
| DeepSeek integrity/live/backref/no 205D0 | **ACCEPT** | PROVEN dynamic (prior) |
| DeepSeek `A907C -> 1EE34` | **REFUTED** | static: A907C does not call 1EE34 |
| DeepSeek ROOT_CAUSE_PROVEN: NO | **ACCEPT** | still open |

```text
ARBITRATION_CLASS: C — BOTH PARTIAL
  (structure/clearer/order mostly correct; origin/type naming wrong or unproven;
   Gemini overclaims root cause; DeepSeek driver parent wrong)
```

---

## 1. PROVEN freeze mechanism (unchanged)

```text
PROVEN_ROOT_MECHANISM:
  slot7 pending
    -> C269C(7)==0  (substate=2 && table[2].word0 != 0)
    -> pending != 0
    -> C2AB8 outer loop repeats
    -> MAIN does not return from mode-5 init path
```

`func_80026500` as hang PC: **REFUTED** (prior forensics).

---

## 2. Pool object struct (stride 0x48)

Allocator: `func_8002039C` (from free list `D_800411D8` / `0x8004+0x11D8`).
Creator wrapper: `func_80020790`.
Clearer: `func_800205D0`.

Manager globals (lui 0x8004 + offsets):

| addr | role |
| --- | --- |
| `0x800411D0` | active list head |
| `0x800411D4` | active list tail |
| `0x800411D8` | free list head |
| `0x800411DC` | manager / schedule root (`+0x14` = delta list for `func_80026500`) |
| `0x800411E0` | short counter (flag-0x4 objects) |

### Field table

| OFFSET | WIDTH | READERS | WRITERS | SEMANTIC | CONFIDENCE |
| --- | --- | --- | --- | --- | --- |
| +0x00 | u32 | list walk | 2039C/205D0/DLL helpers | next (active/free) | HIGH |
| +0x04 | u32 | list walk | 2039C/205D0 | prev | HIGH |
| +0x08 | u32* | 1EE34, 205D0→E90A4, 20790 spawn | 2039C (`sw config`) | **config pointer** (from `func_800E9320`/`E90D8`) | HIGH |
| +0x0C..+0x1B | 16B | 26BE0/audio helpers | payload copy via 26500 path | timed-list payload / voice state | MEDIUM |
| +0x28 | f32 | 1EE34 duration math | 2039C via `func_80027170` | rate/duration base | MEDIUM |
| +0x2C | f32 | 20790 scale, 1EE34 | 2039C=1.0f; 20790 *= a3 | scale / intensity | HIGH |
| +0x30 | u32* | 205D0 clearer | 2039C=0; **20790 writes out_param** | **backreference to table entry** (`&table[i].word0`) | HIGH |
| +0x34 | s32 | 1EE34 | **2039C := 2**; 1EE34 dec/reset | **tick countdown / phase delay** | HIGH |
| +0x38 | s16 | 20790, 1EE34 | 2039C=0x7FFF; 20790 scale | volume-like scalar | MEDIUM |
| +0x3A | s16 | 1EE34 | 1EE34 from config | secondary volume/param | MEDIUM |
| +0x3C | u32 | 1EE34 | 1EE34 schedule stamp | schedule key / time stamp | MEDIUM |
| +0x40 | u8 | 1EE34 | 2039C from config | config-derived param | MEDIUM |
| +0x41 | u8 | 20790, 1EE34 | 2039C=0x40; 20790 clamp | priority/level | MEDIUM |
| +0x42 | u8 | 1EE34 | 2039C=0; 20790 from stack | channel/param | MEDIUM |
| +0x43 | u8 | 1EE34 | 20790 from stack | channel/param | MEDIUM |
| +0x44 | u8 flags | 1EE34, 200B8, 20790, 20AD0 | 2039C from config nibble; 20790 clear 0x10 | **lifecycle flags** (0x1,0x2,0x4,0x10,0x12,0x20…) | HIGH |
| +0x45 | u8 | 1EE34 phase gate | 2039C=**5**; 1EE34 sets 4/3/… | **phase / state machine id** | HIGH |

### +0x34 is NOT only “frames remaining forever”

```text
FIELD_34_SEMANTIC:
  integer countdown used by func_8001EE34 phase-0 path:
    if (flags & 0x12)==0 && +0x34 > 0:
      +0x45 = 4
      +0x34 -= 1
      re-schedule via func_80026500(..., key=0x8235)
    if +0x34 <= 0:
      completion paths may call func_800200B8 -> func_800205D0
  ALSO: one path RESETS +0x34 := 2 (0x8001F194)

COUNTDOWN_2_PROVES_FRESH_CREATION: NO
  - 2 is the creation default (2039C)
  - 2 is also a valid reset value during update
  - alone does not prove "just created this frame" nor "attract-only"
```

DeepSeek’s observation “still 2” + “205D0 never ran” jointly implies **the object was never successfully driven to completion**, not necessarily “created one tick ago”.

---

## 3. Config `object+0x08` / DeepSeek `0x801301E0`

```text
CONFIG_TYPE:     audio/config pool entry resolved by func_800E9320 -> func_800E90D8
WHO CREATES:     E90D8 indexes tables at 0x80132560 / 0x80132650 and instance
                 slab near 0x8012D690 (+0x3A8 base)
WHO REFERENCES:  object+0x08; release via func_800E90A4 on clear
SEMANTIC NAME:   UNKNOWN formal name (not labeled in syms as SoundVoice*)
```

Static: `func_80020790` does `config = func_800E9320(0, id-1)` then `obj = func_8002039C(config)`.

Dynamic `0x801301E0` is **compatible** with that slab; exact field dump of that config was not re-captured in this arbitration (static-only pass). Treat pointer identity as PROVEN by DeepSeek; field semantics of the config blob: PARTIAL/UNKNOWN.

---

## 4. `0x8004E5A8`

```text
WHAT_IS_8004E5A8:   UNKNOWN (no static literal / no proven global at that addr
                    in the audited call graph)
READERS/WRITERS:    not established in this pass
RELATION_TO_OBJECT: UNKNOWN
RELATION_TO_SLOT7:  UNKNOWN
```

Nearby **proven** manager cluster is `0x800411D0..0x800411E0`, not `0x8004E5A8`. Do not bind diagnosis to this address without a fresh post-freeze dump of that word.

---

## 5. `func_80020790` — create / attach

### Pseudocode (static)

```c
// a0 = sound/config id (s16 at sp+0x62 after save)
// a1.. stack: volume scale factors, flags, **out_param at sp+0x78**
void func_80020790(id, params..., void **out_param) {
  result = NULL;
  while (id != 0) {
    config = func_800E9320(0, id - 1);
    obj = func_8002039C(config);          // alloc from free list, init +0x34=2
    if (obj) {
      manager->unk3C = obj;               // 0x800411DC+0x3C
      // scale volume +0x38, scale +0x2C by float a3
      // set +0x42/+0x43 from stack; may clear flag 0x10 and insert delayed list
      func_80026500(manager+0x14, ..., key);  // schedule
      result = obj;
      // chain next id from config bytes
    }
    id = next_from_config_or_0;
  }
  if (result) {
    result->backref = out_param;          // +0x30
    // may set flags / secondary schedule
  }
  if (out_param) *out_param = result;     // table[i].word0 = object
  return result;
}
```

```text
OBJECT_CREATION_IS_AUDIO_SPECIFIC:   YES (config via E9320/E90D8 audio tables;
                                         flags drive 26FF0/27090 SFX helpers)
OBJECT_CREATION_IS_GENERIC_MANAGER:  YES (generic 0x48 pool + DLL list + delta schedule)
OBJECT_CREATION_CONTEXT:             both — generic pool object specialized by audio config
```

### Callers of `func_80020790` (static)

1. `func_800DC3D4` — **kinematics attach path** (used by `func_800C16F8`)
2. `func_8001EE34` — chained spawn from an existing object (flag 0x10)

### Kinematics attach chain (table entry)

```text
func_800C16F8(slot, substate)
  -> slot+0x79 = substate
  -> ... resource setup ...
  -> func_800C1430(substate)           // table entry helper
  -> func_800DC3D4(id, ..., out=table_entry_ptr)
       -> func_80020790(..., out_param=table_entry_ptr)
            -> alloc object
            -> *table_entry = object
            -> object+0x30 = table_entry
```

`func_800C16F8` callers: `func_800C2000`, `func_800C3BDC` (many world/teardown callers).

`func_800C15BC` allocates free table indices (word1 flag); index **2** is a free-list handle, not a semantic “intro id”.

```text
SUBSTATE2_MEANING:
  kinematics slot substate index into table[substate] at base 0x80128B08.
  substate==2 means "depends on table entry 2's word0 being zero to be ready".
  It is a resource handle, NOT proof of object type.
```

---

## 6. `func_800205D0` — clear / detach

### Pseudocode

```c
void func_800205D0(obj) {
  osSetIntMask(1);
  // unlink from active head/tail if needed
  func_80020E74(obj);                   // DLL unlink
  // push onto free list 0x800411D8
  if (obj->flags & 4) D_800411E0--;
  obj->phase = 0;                       // +0x45
  if (obj->backref) {                   // +0x30
    if (*obj->backref == obj)
      *obj->backref = 0;                // **clears table[2].word0**
    obj->backref = 0;
  }
  func_800E90A4(obj->config);           // release config ref
  osSetIntMask(prev);
}
```

### Callers

```text
STATIC ONLY CALLER OF 205D0:  func_800200B8
STATIC ONLY CALLER OF 200B8:  func_8001EE34

CAN_800205D0_RUN_FROM_ANY_CONTEXT_OTHER_THAN_MAIN_FRAME_DRIVER:
  YES (potential) / UNKNOWN (observed)
  - 1EE34 has no static direct callers except func_8001ED68
  - 1ED68 has NO static LOOKUP callers (must be reached via jalr / scheduled callback)
  - audio thread func_800128C0 -> func_800129FC -> func_80021574 uses jalr $t9
  - func_8002241C also jalr-dispatches and uses 2645C event pop
  Therefore clearer is NOT hard-wired to MAIN-only in the static graph.
  Dynamic: 205D0 never hit in freeze observations => whatever context should
  drive this object is not completing it before/during C2AB8.
```

---

## 7. Real driver graph

```text
PROVEN static graph:

  [scheduled/jalr path]
       |
       v
  func_8001ED68          // event drain wrapper (no static callers)
       |
       +--> func_8001EE34   // object state machine / tick
              |
              +--> func_800200B8  // complete
              |      +--> (opt) func_80026FF0 / func_80027090 if flag 0x4
              |      +--> func_800205D0   // free + clear backref
              |      +--> func_800201A4
              +--> func_80020790  // chained create
              +--> func_80026500  // re-schedule
              +--> func_80026BE0  // voice/param update
              +--> func_80020128

REFUTED:
  func_800A907C -> func_8001EE34
  (A907C calls: A9010, CA334, 8010FB34, A88C4 only)

Gemini driver name func_800200B8:
  QUALIFY — 200B8 is the completion wrapper, not the tick driver.
  Real tick driver is func_8001EE34; 200B8 only completes.
```

```text
WHO_TICKS:              func_8001EE34
WHO_DECREMENTS_+0x34:   func_8001EE34
WHO_TESTS_COMPLETION:   func_8001EE34
WHO_CALLS_CLEARER:      func_800200B8 -> func_800205D0
WHO_REMOVES_ACTIVE:     func_800205D0 (free list push)
```

---

## 8. Game loop / transition order

### Normal frame (`func_800A7D84`)

Does **not** call `func_800C2AB8` or `func_8001EE34` directly. Calls warp/mode helpers, map/actor updates, transitions, etc. Audio object ticks are **not** on this direct call list.

### Mode transition path (PROVEN clean freeze stack)

```text
func_800A7D84
  -> (warp=3 consumed) func_800A9924
  -> func_800A76F4(5)
       ... map/mode teardown prep ...
       -> func_800A7380(1)
            -> func_800EA0A8
            -> func_800A8F68
            -> func_800C2AB8      // << BUSY-WAIT (slot7 / table[2])
            -> func_800FB968      // not reached while stuck
            -> func_8001A2B0
            -> ...
```

```text
OBJECT_DRIVER_BEFORE_C2AB8_ON_TRANSITION_PATH:  NO
OBJECT_DRIVER_AFTER_C2AB8_ON_TRANSITION_PATH:   NO (not reached while stuck)
OBJECT_DRIVER_NOT_ON_TRANSITION_PATH:           YES
```

---

## 9. When is the object created? (intro vs attract)

```text
STATIC creation call chain for table[2]:
  C16F8 / C3BDC|C2000 -> DC3D4 -> 20790 -> 2039C

MODE/MAP at creation:   NOT PROVEN in this static-only pass
ACTIVE OVERLAY:         NOT PROVEN for this object
CONFIG id:              NOT re-dumped (would require post-freeze read of +0x08 blob)

CLASSIFICATION:
  A INTRO          — NOT PROVEN
  B ATTRACT        — NOT PROVEN
  C HANDOFF/TEARDOWN — POSSIBLE (C3BDC has many teardown callers; A76F4 path)
  D GLOBAL         — NO (pool object with backref, not immortal global)
  E UNKNOWN        — **SELECTED**

OBJECT_ORIGIN: UNKNOWN
```

Evidence against pure Gemini STALE_INTRO:

- countdown=2 is creation default; also resettable
- free/active lists valid → not freed-then-corrupt
- 205D0 never ran → “should have finished earlier” is **EXPECTED≠OBSERVED**, not proof of intro age

Evidence against pure DeepSeek ATTRACT:

- countdown=2 does not prove attract epoch
- no creation call-stack capture at alloc time in clean run

---

## 10. STALE_INTRO hypothesis test

```text
EXPECTED_LIFETIME:              UNKNOWN (depends on config duration / +0x28 math)
EXPECTED_TICKS_BEFORE_TEARDOWN: UNKNOWN
IS_COUNTDOWN=2_COMPATIBLE_WITH_INTRO_STALE: YES (possible if never ticked / reset)
IS_OBJECT_CONFIG_SPECIFIC_TO_JIGGY:         UNKNOWN (no config id proof)
```

Gemini’s “object should have finished before C2AB8” remains **EXPECTED≠OBSERVED**, not a closed proof.

---

## 11. NEW_ATTRACT hypothesis test

```text
WHERE +0x34 gets 2:     func_8002039C (create) and one 1EE34 reset path
IS COUNTDOWN:           YES (integer tick countdown / phase delay)
RESET TO 2 DURING UPDATE: YES (0x8001F194)
COUNTDOWN_2_PROVES_FRESH_CREATION: NO
```

---

## 12. Original expected contract of C2AB8

```text
func_800C2AB8:
  for mode==3 enabled slots:
    if ready (C269C): C16A4 clear slot
    else pending++
  while pending: repeat

Inside loop: NO call to 1EE34 / 200B8 / 205D0 / object tick.
```

```text
C2AB8_EXPECTED_PROGRESS_SOURCE:
  A and/or C — NOT B

  A — table object word0 should already be 0 when convergence runs
      (object completed earlier on a prior tick / teardown drain)
  C — progress may arrive asynchronously from the audio/schedule path
      (1EE34/205D0 on another thread or after yielding) while MAIN spins

  B — REFUTED as sole mechanism: nothing in the C2AB8 loop advances table[2]
```

On N64, a busy-wait can still converge if another runnable thread/interrupt advances the object. On recomp, if MAIN spins without host scheduling the audio/event path that would clear the object, the wait is fatal.

Which of A vs C is the **intended** original contract for this specific mode-5 path: **UNKNOWN / DISPUTED**.

---

## 13. Original vs recomp match (audited funcs)

| func | match notes |
| --- | --- |
| 20790 / 205D0 / 200B8 / 1EE34 / C2AB8 / C269C / C16F8 | recompiled straight-line MIPS; no missing jal found in these bodies |
| A907C | not on this critical path for object clear |

```text
ORIGINAL_RECOMP_MATCH: PARTIAL
  - local bodies look faithful
  - no proven missing call inside the audited functions themselves
  - residual risk is host scheduling / async semantics of the event path
    that should clear the object (outside these bodies), not a visible
    wrong branch inside C2AB8/C269C
```

---

## 14. Dynamic facts treated as PROVEN

From prior clean post-freeze forensics (DeepSeek + Gemini agreement on freeze PC):

```text
PROVEN:
  object live at 0x8005C168 (run-dependent nearby 0x8005Cxxx)
  active list membership valid
  free list valid
  backref +0x30 -> table[2] valid
  table[2].word0 == object
  slot7 pending (substate=2)
  func_800205D0 has not executed
  MAIN in C2AB8 / C269C / get_function
  func_80026500 not hang PC
```

```text
EXPECTED != OBSERVED:
  table[2].word0 should be 0 for C269C(7)==1, but remains nonzero
  205D0 should have run for a completed object, but hit count 0
```

---

## 15. First / earliest lifecycle divergence

```text
EARLIEST_PROVEN_DIVERGENCE:
  On the mode-5 transition path, MAIN enters func_800C2AB8 while
  table[2].word0 still references a live pool object that has never been
  cleared (205D0 never ran). C2AB8 cannot itself complete that object.
  Therefore the convergence loop never terminates.

NOT YET PROVEN:
  - whether the object should have been cleared before A7380/C2AB8 (A)
  - whether an async driver should clear it during the busy-wait (C)
  - intro vs attract vs handoff creation epoch
  - host audio-thread scheduling failure vs game-state gate
```

```text
ROOT_MECHANISM_PROVEN: YES
ROOT_CAUSE_PROVEN:     NO
```

---

## 16. Classification answers (prompt §20–21)

```text
ARBITRATION_CLASS: C
  (optional nuance: leans D if forced — audio object, creation epoch open)

ROOT_MECHANISM_PROVEN: YES
ROOT_CAUSE_PROVEN:     NO
```

---

## 17. NEXT single action (discriminator only — do not implement fix)

One clean post-freeze + optional single instrumented *creation-time* attach is allowed only if static cannot close origin. Prefer **one clean run, attach only after freeze**, non-mutating:

```text
DISCRIMINATOR (minimal):
  After freeze, dump for object at table[2].word0:
    +0x08 config ptr and first 0x20 bytes of config
    +0x34 countdown
    +0x45 phase
    +0x44 flags
    +0x30 backref
  And counters (if one future lightly-instrumented run is authorized):
    hits(func_80020790) with RA / caller
    hits(func_8001EE34) on this object
    hits(func_800205D0)
    which host thread ran them (MAIN vs audio/thread-128C0)

Expected outcomes:
  1) 20790 called only during intro, 1EE34 never after attach
     => STALE / missing async drain
  2) 20790 called during A76F4/C16F8 handoff, countdown still 2, 1EE34=0
     => created on transition then immediately trapped by C2AB8
  3) 1EE34 hits >0 but 205D0=0
     => gate inside 1EE34 / flags / schedule key, not "never scheduled"
```

No patch. No force clear of table[2]. No BaKey.

---

## 18. Specs review (stable contracts)

| SPEC | NEEDS_UPDATE | WHY |
| --- | --- | --- |
| `docs/specs/BT/recomp-readiness.md` | NO | no new readiness criterion closed |
| `docs/specs/BT/rsp-and-audio.md` | NO | A-vs-C drain contract still DISPUTED; do not formalize yet |
| `docs/specs/BT/overlays.md` | NO | not overlay-binding issue |
| `docs/specs/BT/matching.md` | NO | no matching proof change |

---

## 19. Proof summary table

| item | status |
| --- | --- |
| freeze PC = C2AB8 busy-wait on slot7 | PROVEN |
| gate = table[2].word0 != 0 | PROVEN |
| object pool stride 0x48 | PROVEN |
| create 20790 / clear 205D0 | PROVEN |
| +0x30 backref clears table entry | PROVEN |
| +0x34 countdown init 2 | PROVEN |
| 205D0 never ran (clean freeze) | PROVEN (prior dynamic) |
| A907C drives 1EE34 | REFUTED |
| STALE_INTRO root cause | REFUTED as proven / remains INFERENCE |
| ATTRACT origin | INFERENCE / not proven |
| C2AB8 self-progress | REFUTED |
| root cause closed | NO |

=== END EVIDENCE ===
