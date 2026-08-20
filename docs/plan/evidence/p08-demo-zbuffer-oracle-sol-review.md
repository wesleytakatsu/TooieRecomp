---
title: "P08 — independent Z-buffer oracle-validity review"
description: "Adversarial arbitration of the Rice-zero versus RT64-live Z-buffer differential. Rice is not a valid oracle for CPU-visible rendered depth; RT64 has valid writeback ownership/order but is not bit-exact. A new Angrylion Plus + LLE RSP run is required."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-20
tags: [p08, camera, zbuffer, oracle, rice, rt64, angrylion, rdp, depth, evidence]
---

# P08 — independent Z-buffer oracle-validity review

## 1. Verdict

The previous “Original” side must be renamed **ORIGINAL_RICE_CONTROL** for
this mechanism. It is not a hardware oracle for func_8001A0C0: Rice tracks
SetDepthImage and renders depth in its host OpenGL target, but does not encode
that rendered depth into the guest RDRAM Z image at FullSync.

The opposite conclusion — that every RT64 value is therefore hardware-correct
— is also rejected. RT64 owns the correct guest range and completes native
depth readback before raising the DP interrupt, but its current conversion is
not a bit-exact N64 Z writer. In the reviewed checkout it derives visible D16
from host float depth, supplies dz = 0 at a documented TODO, and the observed
clear value 0xFFFE does not equal the 0xFFFC fill word emitted by the game.

Therefore the historical camera differential is **MIXED**:

- Rice's persistent zero is a proven reference-renderer artifact.
- RT64's live-depth existence, destination, size, and FullSync ordering are
  strong/correctly motivated.
- RT64's exact depth bits are not a proven real-N64 oracle.
- A new accurate Original renderer run is the remaining camera discriminator.

No production fix is authorized or implemented by this review.

## 2. Terminology correction

This evidence uses three deliberately separate subjects:

- **ORIGINAL_GAME**: Banjo-Tooie's guest MIPS and display-list behavior.
- **ORIGINAL_RICE_CONTROL**: Mupen64Plus Pure Interpreter + Rice Video HLE,
  the control used by the prior differential.
- **REAL_N64_EXPECTATION**: RSP/RDP/RDRAM and CPU-cache semantics on hardware.

“The Original never writes the target” is superseded. The measured statement
is: **the Rice-based control never materializes rendered Z at the target**.

## 3. Object identity

### Independent evidence

1. D_8007BCC4 holds guest address 0x80137840 and the allocation is 0x21D80
   bytes, exactly 304 × 228 × 2.
2. ORIGINAL_GAME func_800E7BC8 emits G_SETZIMG using the physical form of the
   pointer returned by func_8001A0A8, which is D_8007BCC4.
3. func_800E7C28 rebinds that same physical address as an RGBA16 color image,
   width 304, solely for the fill-mode clear path.
4. func_800E7E70 invokes the depth-image bind and clear setup in the frame
   display-list path; func_800E7EF4 terminates the list with FullSync.
5. RT64 identifies the resulting framebuffer pair as a depth target and the
   captured copy covers 0x137840..0x1595C0, matching the allocation.

The dimensions are corroboration, not the sole argument. The display-list
identity and render-path ownership independently bind the allocation to the
RDP Z image.

~~~text
D_8007BCC4_IDENTITY: PROVEN
D_8007BCC4_SEMANTIC: active 304×228, 16-bit ORIGINAL_GAME RDP Z image;
                     temporarily rebound as RGBA16 for fill-mode clearing
~~~

Generated RecompiledFuncs/funcs_191.c and funcs_228.c were inspected read-only.
The decisive guest semantics come from the original MIPS, not generated C.

## 4. func_8001A0C0 role

The original leaf computes 608 × y + 2 × x and returns the unsigned halfword
at D_8007BCC4 plus that offset.

Its minimum necessary caller, func_800E81E0, does more than test a nonzero
flag. It splits the returned word into the N64 compressed-Z exponent
(bits 15..13) and mantissa (bits 12..2), reconstructs depth using
D_801234C0, compares that depth with a projected point, and returns a ratio
used by the subsequent visibility/classifier path. The visible dz bits
(bits 1..0) are intentionally irrelevant to this decoder.

~~~text
FUNC_8001A0C0_ROLE: PROVEN
ROLE: screen-space CPU Z sample consumed by a projection-depth /
      occlusion-visibility query in func_800E81E0
~~~

## 5. Real N64 RDP/RDRAM/CPU semantics

SetDepthImage selects the RDRAM address used by the RDP as its Z image.
Depth-tested triangles can update that RDRAM image. FullSync is the ordering
primitive at the end of the command stream: the DP interrupt is raised only
after preceding RDP work reaches completion.

The CPU can consequently read rendered Z from RDRAM. That statement is
conditional at the CPU interface because the R4300 data cache is not coherent
with RDP/device writes. A KSEG0 reader must not retain a stale line: software
must invalidate the relevant D-cache line/range, use an uncached KSEG1 alias,
or otherwise establish that no stale cache line exists. D_8007BCC4 is observed
as a KSEG0 pointer (0x80137840), and this review did not prove an explicit
cache invalidation paired with the exact query.

~~~text
REAL_N64_Z_WRITES_RDRAM: YES
CPU_CAN_READ_RENDERED_Z: CONDITIONAL
REQUIRED_SYNC_CONDITIONS:
  1. the RDP processes all relevant triangles and the FullSync;
  2. the CPU observes DP task completion / interrupt;
  3. the CPU reads through an uncached alias or has no stale D-cache line
     (normally enforced with osInvalDCache for a cached alias)
DEPTH_READ_AFTER_REQUIRED_SYNC: UNKNOWN
~~~

Primary architectural references:

- [Nintendo gDPSync manual](https://ultra64.ca/files/documentation/online-manuals/man/n64man/gdp/gDPSync.html)
- [Nintendo osInvalDCache manual](https://ultra64.ca/files/documentation/online-manuals/man/n64man/os/osInvalDCache.html)
- [Nintendo hardware glossary](https://ultra64.ca/files/documentation/online-manuals/man/n64man/misc/glossaryHardware.html)

## 6. N64 depth encoding and RT64 conversion

The CPU-visible N64 Z word is not linear normalized-depth u16. The RDP works
with an 18-bit Z value and stores a compressed visible representation:

- bits 15..13: exponent;
- bits 12..2: mantissa;
- bits 1..0: upper two bits of compressed dz;
- the remaining two compressed-dz bits live in the hidden RDRAM coverage
  storage used by the RDP, not in the CPU-visible halfword.

The guest byte order is big-endian. The SDK definition G_MAXFBZ = 0x3FFF and
GPACK_ZDZ(z, dz) = (z << 2) | dz are consistent with this layout.

Reviewed RT64 commit 6f1c2d99a4ea571c139f449c326fd176ba8f3496
uses FbWriteDepthCS/FloatToDepth16 to turn native floating-point depth into a
visible 16-bit N64-like word and endian-swaps words when copying the readback
buffer to guest RDRAM. However, the shader explicitly uses dz = 0 with a TODO.
It therefore cannot preserve the per-pixel dz field or hidden dz state and is
not bit-exact N64 depth memory. Native GPU depth/rasterization equivalence for
the observed high 14 bits is also not independently established here.

~~~text
EXPECTED_N64_DEPTH_MEMORY_FORMAT:
  compressed Z exponent+mantissa in visible bits 15..2, upper compressed-dz
  bits in 1..0, lower compressed-dz bits in hidden RDRAM storage; big-endian
RT64_DEPTH_WRITEBACK_FORMAT:
  normalized native float depth -> FloatToDepth16 visible word, dz forced to
  zero (TODO), host/guest endian conversion, no bit-exact hidden-dz recreation
FORMAT_MATCH: NO
~~~

Relevant source references:

- [N64 SDK gbi.h definitions](https://mountainflaw.github.io/doxygen/d3/daf/gbi_8h_source.html)
- [RT64 FbWriteDepthCS at the reviewed commit](https://github.com/rt64/rt64/blob/6f1c2d99a4ea571c139f449c326fd176ba8f3496/src/shaders/FbWriteDepthCS.hlsl)
- [RT64 Depth.hlsli at the reviewed commit](https://github.com/rt64/rt64/blob/6f1c2d99a4ea571c139f449c326fd176ba8f3496/src/shaders/Depth.hlsli)

## 7. Clear value arbitration

ORIGINAL_GAME func_800BC5B0 emits G_SETFILLCOLOR 0xFFFCFFFC, followed by a
fill rectangle while the Z allocation is bound as RGBA16 color memory.
Fill mode writes the selected 16-bit fill word. Thus an untouched pixel
immediately after this clear must contain 0xFFFC in the CPU-visible halfword.

The measured RT64 control instead first exposes 0xFFFE at the target after
the setup/clear. Resemblance to “far depth” is insufficient: the low bits are
part of the stored representation, and the guest command supplies the exact
word.

~~~text
EXPECTED_CLEAR_Z_BITS: 0xFFFC
RT64_CLEAR_Z_BITS: 0xFFFE
CLEAR_VALUE_MATCH: NO
~~~

This is an exact RT64 format/writeback discrepancy. It is not, by itself, a
causal explanation of func_800E81E0 because that function discards bits 1..0.

## 8. Rice control validity

At local Rice commit 038882dd, DLParser_SetZImg only records the Z-image
metadata. Rasterization/depth compare lives in the host OpenGL depth target.
DLParser_RDPFullSync raises the DP interrupt but performs no depth-to-RDRAM
conversion. The CPU framebuffer-read path recognizes that an address lies in
the Z range only for diagnostics and has no N64 depth encoder/copyback path;
the implemented copies are color-image conversions.

Therefore the measured persistent zero says something precise about Rice,
not about ORIGINAL_GAME or REAL_N64_EXPECTATION.

~~~text
RICE_DEPTH_WRITEBACK_IMPLEMENTED: NO
RICE_ZERO_BASELINE_HARDWARE_VALID: UNKNOWN
RICE_VALID_ORACLE_FOR_FUNC_8001A0C0: NO
~~~

A coincidental zero at this exact CPU read could result from unsynchronized or
stale-cache hardware state, and the static evidence does not exclude that.
Rice cannot establish such a result because its renderer omits the required
RDRAM update altogether. Thus the observed zero is invalid as a baseline even
though the isolated bit value is not categorically hardware-impossible.

## 9. RT64 writeback validity

RT64's writeback is invoked because the active framebuffer pair owns an RDP
depth image that has native render work pending. State::fullSync flushes the
render target, schedules native target conversion, submits and waits for GPU
completion, then Framebuffer::copyNativeToRAM copies both eligible color and
depth targets. Only after state->fullSync returns does the GBI layer call
dpInterrupt.

The dynamic capture independently fixes the destination and extent:

- guest physical base: 0x00137840;
- guest virtual base: 0x80137840;
- size: 0x21D80;
- sampled offset: 0x17FC0;
- sampled address: 0x8014F800.

~~~text
RT64_WRITEBACK_REASON:
  FullSync flush of an active RDP framebuffer pair whose depth image has
  native writes and must become CPU-visible in guest RDRAM
RT64_WRITEBACK_TIMING_VALID: YES (for the RDRAM completion/DP-interrupt boundary)
RT64_WRITEBACK_ADDRESS_VALID: YES
RT64_WRITEBACK_SIZE_VALID: YES
RT64_READBACK_ORDER_VALID: YES
RT64_HARDWARE_FIDELITY_LEVEL: REFUTED
~~~

“REFUTED” applies to exact hardware/bit fidelity, not to the existence,
ownership, address, extent, or FullSync ordering of the copyback. Exact CPU
visibility for the ORIGINAL_GAME path remains conditional on the unresolved
guest cache state described above.

## 10. Most important oracle answer

~~~text
IS_THE_ORIGINAL_MUPEN_RICE_TRACE_A_VALID_ORACLE_FOR_THE
FUNC_8001A0C0_Z_BUFFER_CAMERA_PATH: NO
~~~

Pure Interpreter improves CPU instruction fidelity; it does not repair the
missing RDP depth-to-RDRAM semantics of the selected Rice video plugin.
Further RT64 camera comparisons against Rice's zero-depth state are invalid.

## 11. Replacement Original oracle and RSP

The first practical replacement should preserve the already instrumentable
Mupen64Plus Pure Interpreter core and replace both graphics and, for the
strongest independent run, the RSP plugin:

~~~text
RECOMMENDED_ORIGINAL_ORACLE:
  pinned Mupen64Plus Pure Interpreter
  + pinned Angrylion RDP Plus in reference/native-resolution mode
  + pinned mupen64plus-rsp-cxd4 LLE

WHY:
  Angrylion is a low-level software RDP implementation that performs N64
  depth stores against RDRAM and is deterministic/instrumentable without
  adding Vulkan integration ownership as a second variable. Its Mupen64Plus
  plugin is explicitly built/tested with rsp-cxd4.

RSP_HLE_ACCEPTABLE_FOR_THIS_ORACLE: CONDITIONAL
RECOMMENDED_RSP: mupen64plus-rsp-cxd4 (LLE), pinned commit
~~~

rsp-hle is sufficient for this narrow mechanism only if a command-stream
capture proves that it emits the same SetDepthImage, triangle/Z parameters,
FullSync, and ordering as the LLE task. Accurate RDP execution is the component
that actually performs the depth stores, but using CXD4 LLE removes HLE
microcode translation as an avoidable oracle variable.

Practical references:

- [Angrylion RDP Plus](https://github.com/ata4/angrylion-rdp-plus)
- [ParaLLEl-RDP](https://github.com/Themaister/parallel-rdp)

ParaLLEl-RDP is a useful second independent backend, but its Vulkan
integration must itself own/import RDRAM and wait at SyncFull. For the first
bounded run, Angrylion's software path is easier to audit and instrument in
the already-built Mupen environment.

## 12. Existing camera differential

~~~text
OLD_CAMERA_DIFFERENTIAL_STATUS: MIXED
ROOT_CAUSE_LEVEL: C-ORACLE
ROOT_CAUSE:
  ORIGINAL_RICE_CONTROL is invalid for rendered-Z CPU reads. RT64's writeback
  ownership/order is plausible and its address/size are proven, but its exact
  N64 depth format is refuted. A hardware-faithful Original run is the only
  remaining discriminator for the camera result.
SUBSIDIARY_RT64_FINDING: B-FORMAT (proven clear/dz fidelity defect;
                          causal relevance to the camera is not proven)
FIX_IMPLEMENTED: NO
~~~

The old terms “Original = 0” and “Recomp bug” are superseded. The preserved
measured chain is:

- ORIGINAL_RICE_CONTROL: record+0x34 = 0;
- RT64_CONTROL: record+0x34 = 1;
- REAL_N64_EXPECTATION: not yet dynamically measured for this scene/query.

## 13. One bounded replacement-oracle experiment

### Exact next-agent task

1. In /tmp/p08-m64p, leave the repository worktree untouched. Record exact
   commits/configuration for Mupen64Plus core, Angrylion RDP Plus, and
   mupen64plus-rsp-cxd4. Build/install the two plugins into the existing
   temporary Mupen prefix. Select Pure Interpreter, native/reference
   Angrylion settings, deterministic single-run settings, and CXD4 LLE.
2. Verify the ROM SHA-1 is
   af1a89e12b638b8d82cc4c085c8e01d4cba03fb3.
3. Reproduce the same attract-demo/control path and identify the same Q1 stream
   using the existing E82EC/Q1 ordinal alignment, not host presentation index
   alone.
4. With temporary core instrumentation, capture:
   D_8007BCC4 base; target coordinate (176,161); target address 0x8014F800;
   target value over time; func_8001A0C0 return; func_800E81E0 result;
   record+0x34; and, if practical, the same 25 aligned classifier samples.
5. Add DP FullSync/interrupt timestamps or ordinals and the CPU read ordinal.
   If practical, also log cache invalidation/alias evidence and the post-clear
   target before geometry.
6. Revert every temporary emulator/core change and rebuild/reinstall the clean
   pinned core. Store captures under /tmp; write only a new evidence note in
   the repository.
7. Classify the outcome exactly:
   A = Rice-like zero;
   B = RT64-like live depth with matching bits;
   C = live depth with different bits;
   D = another behavior.

Primary question: does this software-LLE Original control populate the Q1 Z
sample, and do its decoded high 14 Z bits and downstream classifier agree with
RT64?

~~~text
NEXT_ORACLE_EXPERIMENT:
  run the bounded Pure Interpreter + Angrylion RDP Plus + CXD4 LLE capture
  above with the same ROM, demo, Q1 alignment, and requested state fields
NEXT_SINGLE_ACTION:
  run the bounded hardware-faithful Original oracle experiment specified above
~~~

## 14. Review limits and validation

- No production/runtime/generated file was edited.
- No replacement renderer was installed or run; it was not already present in
  /tmp/p08-m64p.
- The exact ORIGINAL_GAME D-cache state at the query is unresolved.
- RT64 high-14-bit live depth equivalence to hardware is unresolved.
- The existing P08 runtime/build suite was not rerun because this checkpoint is
  documentation/arbitration only.

## 15. Handoff

~~~text
=== P08 Z-BUFFER ORACLE SOL REVIEW ===

BASE_HEAD:
e48d88cb9f4eef3727f38f6540270c6e1b810850

MODEL:
GPT-5.6 Sol X-HIGH

D_8007BCC4_IDENTITY:
PROVEN

FUNC_8001A0C0_ROLE:
PROVEN

REAL_N64_Z_WRITES_RDRAM:
YES

CPU_CAN_READ_RENDERED_Z:
CONDITIONAL

REQUIRED_SYNC_CONDITIONS:
FullSync/DP completion plus non-stale CPU access (KSEG1 or D-cache invalidation/
absence of a cached stale line).

EXPECTED_N64_DEPTH_MEMORY_FORMAT:
Compressed Z exponent+mantissa in visible bits 15..2, upper compressed-dz in
bits 1..0, remaining dz in hidden RDRAM storage, big-endian.

RT64_DEPTH_WRITEBACK_FORMAT:
Native float depth converted to a visible N64-like D16 word; dz forced to zero
at a TODO; endian-corrected copyback; not bit-exact hidden-dz storage.

FORMAT_MATCH:
NO

EXPECTED_CLEAR_Z_BITS:
0xFFFC

RT64_CLEAR_Z_BITS:
0xFFFE

CLEAR_VALUE_MATCH:
NO

RICE_DEPTH_WRITEBACK_IMPLEMENTED:
NO

RICE_ZERO_BASELINE_HARDWARE_VALID:
UNKNOWN

RICE_VALID_ORACLE_FOR_FUNC_8001A0C0:
NO

RT64_WRITEBACK_TIMING_VALID:
YES (RDRAM completion before DP interrupt)

RT64_WRITEBACK_ADDRESS_VALID:
YES

RT64_WRITEBACK_SIZE_VALID:
YES

RT64_HARDWARE_FIDELITY_LEVEL:
REFUTED (exact bit fidelity; ownership/address/size/order remain valid)

RSP_HLE_ACCEPTABLE_FOR_THIS_ORACLE:
CONDITIONAL

RECOMMENDED_ORIGINAL_ORACLE:
Mupen64Plus Pure Interpreter + Angrylion RDP Plus reference/native mode +
mupen64plus-rsp-cxd4 LLE, all pinned.

RECOMMENDED_RSP:
mupen64plus-rsp-cxd4 LLE

DEPTH_READ_AFTER_REQUIRED_SYNC:
UNKNOWN

RT64_READBACK_ORDER_VALID:
YES

OLD_CAMERA_DIFFERENTIAL_STATUS:
MIXED

ROOT_CAUSE_LEVEL:
C-ORACLE

ROOT_CAUSE:
Rice-zero is an invalid renderer oracle; RT64 exact format is also not
hardware-exact. The replacement Original run is the remaining discriminator.

FIX_IMPLEMENTED:
NO

NEXT_ORACLE_EXPERIMENT:
The bounded Angrylion Plus + CXD4 LLE run in section 13.

TESTS:
Docs-only checkpoint; markdown_vault_audit and git diff --check required.
Runtime/build suite not required because no tracked diagnostic/runtime file
changed.

MARKDOWN_VAULT_AUDIT:
PASS for errors: 140 files scanned, 0 errors. The audit retained 59 repository
warnings; it reported no issue for this new evidence note.

COMMIT:
diagnose(p08): review zbuffer oracle validity

PUSHED:
YES (checkpoint pushed to origin/main after commit validation).

NEW_HEAD:
This checkpoint; resolve with git rev-parse HEAD.

WORKTREE:
Expected clean after checkpoint.

NEXT_SINGLE_ACTION:
run the bounded hardware-faithful Original oracle experiment specified above.

=== END HANDOFF ===
~~~
