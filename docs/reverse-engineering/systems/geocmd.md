---
title: "GeoCmd"
description: "Sibling walker DE2A4, compact type-2 DCF48, type-0x11/0x18 DLs; plume GeoCmd parent is 107; left/crest DLs enumerated."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-22
tags: [reverse-engineering, geocmd]
---

# GeoCmd

## V6C feather evidence correction (2026-09-05)

**PROVEN for the supplied V6C capture:** `G_DL 0x03000040` resolved to
`0x0011C870` and produced zero tracked triangles. The hypothetical segment-3
mapping/leftover-triangle mechanism described in the older static discussion
below is **REFUTED for this captured path**; do not revive it as current evidence.

The three right-plume own DL provenance results are accepted. V6C classifies
only `0x592` as JOIN; the known `0x54C` and `0x564` distal join DLs are not
covered by that class. Root join unresolved count `3252/3720` blocks a complete
CPU geometry claim. A join can alter its connecting surface but cannot move
the separately submitted own-mesh vertices; exact visual artifact ownership
remains UNKNOWN. See `p09-post-triangle-architectural-review`
for source/CSV limits and the next witness.

## Dispatch (`PROVEN` generated C)

`func_800DE2A4` is a **sibling** walker:

- command type = `lw(node+0)` (full word; table index)
- handler = `*( *(D_8012CF8C) + type*4 )` then `LOOKUP_FUNC`
- next sibling = `node + lw(node+4)`; `lw(+4)==0` ends that list

`D_8012CF8C` is selected from `D_80123198[mode]` via `func_800DF4B0`.
Init copies index 0. Other callers select 1 or 2. **Multiple tables exist.**

Children are **not** walked by DE2A4 itself. A parent handler must call
DE2A4 on a child pointer. Compact type-2 does
`DE2A4(node + lbu(node+8))` in `func_800DCF48` (`lbu(+8)==0x10`).

## GeoCmd `0x02` compact bone (`PROVEN` `0x62B` layout)

On live `0x62B`, type 2 is a **16-byte header** (`byte(+8)==0x10`), not a
fat dual-mtx record:

```text
word(+0)     type 2
word(+4)     sibling continue (0 = no sibling; children still exist)
byte(+8)     header size 0x10
byte(+9)     bone INDEX
hword(+0xA)  boneAnimID
then children at node+0x10
```

GeoCmd children are the display list. Skeletal parent is
`skeleton[index].parent` (`+0xE`), used by `func_800AE8A8`. On `0x62B`
the plume GeoCmd nodes (100–105) are children of parent 107
(`+0x12320`), siblings of head 108. Crest 79 is a child of head 108.
Do not treat a node's sibling continue as its child list.

Root `@0x119F0` is type 2 continue 0, INDEX 0, anim 43. DE2A4 therefore
fires **one** type-2 handler then returns. The rest of the skeleton is
reached only if that handler recurses.

Snout `@0x11CB8` lives in the **neck `@0x11C40` child list**, but it is
**not** the list head. Compact walk (`PROVEN` `STATIC_ROM`):

```text
neck 0x11C40
  DE2A4(neck+0x10) list head 0x11C50 type2 idx10 anim74
  sibling 0x11CA0 type 0xA continue 0x18
  sibling 0x11CB8 type2 idx11 anim42          ← snout
    child list 0x11CC8 type 0x18, 0x11CF8 type 0x11
```

A wrapped `func_800DE2A4` only observes `a1` (the list head). Live census
`DE2A4_REL_0X11CB8=0` with live `0x11CC8` is therefore expected and does
**not** prove the snout handler was skipped. The sibling-aware 2026-09-03
capture records `DCF48_REL_0X11CB8=2243` and
`DCF48_SNOUT_ACCEPT_COUNT=2243`; exact DCF48 execution of `+0x11CB8` is
`PROVEN_RUNTIME`.

`func_800DCF48` `a0` is the **Gfx packet writer**. It composes
`func_80019AA0(0x8012CE58, ADE2C)` onto the `D_8007BCB0` float stack,
`mlMtxApply`/`guMtxF2L` into `lw(gfx+4)`, then emits `G_MTX` with that
same pointer. Compact `word(+4)` is sibling continue, not a matrix
pointer.

Live type-2 handler for this encoding is `func_800DCF48`
(`PROVEN_RUNTIME` by the 2026-09-03 DE2A4 census; 30 bounded type-2 tuples
all carried handler `0x800DCF48`):

```text
lb INDEX at +9
func_800ADE2C(anim_mtx_list, index)   // current bone matrix
G_MTX PUSH DA380002                  // SINGLE matrix, not dual E2180
DE2A4(node + lbu(+8))
G_MTX POP D8380002
```

`func_800E2AA4` → `func_800E2180` dual-mtx is a **different** type-2
implementation (fat node, previous/current getters). Fresh capture
`E2AA4_RAW_ENTRY_COUNT=0` with live `0x62B` registered: runtime
`0x62B` snout **does not** execute E2AA4. Treat E2AA4 as
`STATIC_PATH_CANDIDATE` / alternate table or command shape.

The earlier census had `DE2A4_TYPE2_COUNT=65426` and
`DE2A4_LAST_TYPE2_HANDLER=0x800DCF48`; its `DE2A4_REL_0X11CB8=0` was a
list-head observation limit. The corrected capture also records
`ADE2C_SNOUT_SAMPLES=2243`. The live edge
`0x62B+0x11CB8 → DCF48 → ADE2C` is therefore `PROVEN_RUNTIME`. The current
diagnostic stores only the first 8 of the 16 32-bit words of the N64 Mtx, so
matrix semantic validity and RT64 interpolation identity remain `UNKNOWN`.

`func_800DF4B0(mode)` selects `D_80123198[mode]` into `D_8012CF8C`. Init
copies index 0. Other DE448 sites pass 1 or 2. Player `80092444` does not
jal DF4B0 immediately before DE448; the live mode during player draw is
`UNKNOWN` even though type-2 slot 2 is DCF48.

## GeoCmd `0x03` / `0x0A` / `0x11` / `0x0C`

`0x03`: display lists (shared on ExtraSegments models). `0x0A`: attach /
refpoint **tag**. Tag is not boneAnimID and not an RT64 matrix group.
`part 3 == tag 5`: `REFUTED`. On `0x62A`, tag 5 is right hand.

## GeoCmd `0x11` / `0x18` visible DLs (`PROVEN` `STATIC_MIPS` + `STATIC_ROM`)

Type `0x11` handler `func_800DD65C` emits `G_DL` `0xDE00` (call). It does
**not** emit vertices on the CPU. The Gfx pointer is:

```text
runtime G_DL = osVirtualToPhysical(*(D_8012C960) + (lh(node+8)<<3) + 8)
static Gfx  = model_bin + 0x2670 + dl_index*8   // 0x62B DL table
```

On `0x62B` the DL header is at `+0x2668` (count `0xB48`); `Gfx[0]` is at
`+0x2670`. `lh(node+8)` is the display-list index.

Type `0x18` handler `func_800DD998` emits the same `G_DL`, then a trailing
`G_MTX DA380002` restore. The type-0x18 **display list itself** contains
`G_VTX` under the current bone, `G_POPMTX`, then `G_VTX` in parent space
(join triangles). The trailing `G_MTX` is a same-depth restore of the bone
that just popped inside that DL. This compact join is Tooie-specific.

`0x11` / `0x0C` geometry DLs are **children of the bone node**, not the
bone command itself. Do not treat the next GBI word after DCF48's `G_MTX`
as the vertex load.

## Live `0x62B` primary feather subtree (`PROVEN` `STATIC_ROM`)

Compact type-2 `+0x124A0` (idx 30, boneAnimID 103) is **not** a direct
type-0x11 parent of `dl=0x534`. That previous statement is
`CORRECTED/SUPERSEDED`: `dl=0x534` is the child of bone 105 (`+0x124D0`).
Bone 103's own type-0x11 is `+0x12580` `dl=0x5AA`.

```text
+0x124A0 type=0x02 DCF48 idx=30 anim=103
  child +0x124B0 type=0x02 idx=31 anim=104
    child +0x124C0 type=0x02 idx=32 anim=105
      child +0x124D0 type=0x11 DD65C dl=0x534
      sibling +0x124E0 type=0x18 DD998 dl=0x54C
    sibling +0x12510 type=0x18 DD998 dl=0x564
    sibling +0x12540 type=0x11 DD65C dl=0x57C
  sibling +0x12550 type=0x18 DD998 dl=0x592
  sibling +0x12580 type=0x11 DD65C dl=0x5AA
```

All four type-0x11/0x18 DLs are F3DEX2. Command order inside each
(`PROVEN` `STATIC_ROM`): two `G_GEOMETRYMODE` (`and=0xC0FFFB` then
`set G_SHADE|G_SHADING_SMOOTH`; **preserves** `G_CULL_*`),
setup/texture, then `G_DL dest=0x03000040`
(`w0=0xDE000000` call), **then** that DL's own `G_VTX` / triangles /
`G_ENDDL`. Join `0x592` is `G_VTX` child slots → `G_POPMTX` → `G_VTX`
parent slots 28–31 → mixed `G_TRI2`. No `G_CULLDL`. F3DEX2 `G_CULLDL`
is a no-op in this RT64 (`TODO`).

`0x03000040` is a segmented address, not a Gfx index. Runtime segment-3
base is `RUNTIME_REQUIRED`. If it were `Gfx[0]` (`model+0x2670`), dest
is `+0x26B0` and that chunk starts with `G_TRI2` on slots
`4,5,6,7,8,2,1,9` before `G_VTX 0x010000A0` (`LEFTOVER_TRI_BEFORE_VTX`
`POSSIBLE`). The same mapping would make root `G_DL 0x03000010` at
`Gfx[5]` call `Gfx+0x10` (`G_TEXTURE`) and then re-hit that `G_DL`, so
do **not** treat `segment 3 == Gfx[0]` as proven. Snout uses
`0x03000010`.

Static own/join vertex tables (`PROVEN` `STATIC_ROM`; F3DEX2
`v0=p0(1,7)-n`; `0x5AA` v0=0 not 8):

```text
0x5AA: n=8 v0=0 slots 0-7   addr=0x010037D0
0x57C: n=8 v0=0 slots 0-7   addr=0x010036D0
0x534: n=12 v0=0 slots 0-11 addr=0x01003510
0x592: n=4 v0=0 slots 0-3   addr=0x01003750; POP; n=4 v0=28 slots 28-31 addr=0x01003790
```

Join triangles mix child 0–3 with parent 28–31. Own meshes reuse slots 0–N
across nested bones. RT64 reload writes a new `worldIndices=curTransformIndex`;
`G_POPMTX` does not clear the RSP vertex cache.

RSP order for bone 103: `G_MTX destA` → nested `G_MTX` 104/105 → tip
`dl=0x534` (current = 105) → type-0x18 join/restore → `dl=0x5AA` own
mesh under destD restore → DCF48 `G_POPMTX`. Command order is `MIXED`.
Snout `+0x11CB8` differs: its **first** child is type `0x18` (`dl=0xF2`),
so the first `G_VTX` is immediately after destA.

Banjo `0x62A` snout: GeoCmd `0x02` @ `0x14960`, idx 10, boneAnimID 42.
Banjo `0x62B` snout: compact GeoCmd `0x02` @ `0x11CB8`, idx 11, boneAnimID 42.

Kazooie-on-back on the same `0x62B` tree (`PROVEN` `STATIC_ROM`): parent
`+0x12320` idx 25 anim 107; head `+0x12330` idx 26 anim 108; crest
`+0x12368` idx 27 anim 79 (child of 108); right plume `+0x124A0` idx 30
anim 103 and left plume `+0x12590` idx 33 anim 100 are **siblings of
head 108**, not GeoCmd children of 108 (`CORRECTED/SUPERSEDED`). Left
own/join DLs: `0x636/0x61E/0x608/0x5F0/0x5C0/0x5D8`. Crest:
`0x4A6/0x4B0` (nested `G_DL 0x03000010` **before** `G_VTX`; zero
`G_MODIFYVTX` on both DLs). Solo `0x61C` crest `+0xF558`
idx 11 anim 69. Compact `hdr=0x10` — same DCF48 single-G_MTX class as
the snout (`STRONG_INFERENCE` for handler; snout handler
`PROVEN_RUNTIME`).

## ExtraSegments models

Header `gtype` bit `0x40`. `0x61F` has **zero** GeoCmd `0x02`. Snout is
bone INDEX 4. If live body is `0x61F`, snout **does not** use DCF48/E2AA4.

## GeoCmd tipo 14: gates de esfera e efeitos colaterais (`CORRECTED/SUPERSEDED` / `STRONG_INFERENCE`)

Os comandos GeoCmd tipo 14 (`0x800DDDC4`, `0x800DDF14`, `0x800DDFA0`) realizam gates de visibilidade por esfera delimitadora.

- **Despacho interceptável via `jalr` (`CORRECTED/SUPERSEDED`)**:
  São despachados pelo walker `func_800DE2A4` com `jalr` obtido através de `get_function`. Dessa forma, wrappers registrados **são executados**. O comentário histórico anterior no repositório que afirmava o oposto foi categorizado como `CORRECTED/SUPERSEDED`.
- **Efeitos colaterais em tipos 10 e 15**:
  As subárvores subordinadas aos comandos tipo 14 podem conter nós do tipo 10 (posição de tag de anexo consumida por aproximadamente 61 sítios da lógica de jogo) e do tipo 15 (flags de zona). Expandir o culling em widescreen simplesmente forçando a inversão do retorno $v_0$ cria o risco real de executar esses nós e contaminar a simulação. No censo do attract mode, os nós de tipo 14 registraram 0 entradas.
- **Risco latente em gates tipo 13 (`STRONG_INFERENCE`)**:
  A expansão de AABB `0x800CAA24` anteriormente empregada em produção invertia $v_0$ globalmente. O mesmo canal de efeitos colaterais de tipos 10 e 15 existe sob os gates de GeoCmd tipo 13 (`0x800DDC28` / `0x800DDD14`).

Ver [[view-volume]] e [[model-renderer]].

## Modding relevance

**PROVEN ROLE:** hierarchy / DL / attach commands on GeoCmd models.

**POTENTIAL MODDING USE:** add/remove/move parts on the compact type-2
tree (`byte(+8)` header, children at `+header`). Dual-mtx E2AA4 edits do
not apply to the live `0x62B` snout until a capture shows E2AA4 entries.
