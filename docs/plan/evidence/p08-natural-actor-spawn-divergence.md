---
title: "P08 — divergência no spawn natural de actors (IPL3 integrity gate)"
description: "Diagnóstico fechado: gate anti-tamper func_8001DE64 / D_8007DB79 bloqueia pipeline func_800A5C28 → func_800BDCDC → gccubeDll:5 → gccollectDll:8 → actor creation. Stub address +0x10000 + missing guest words. Discriminator matrix. Fix A decidido, NÃO implementado."
status: evidence
phase: P08
updated: 2026-08-11
tags: [banjo-tooie-recomp, p08, actors, anti-tamper, ipl3, integrity-gate, gccollect, gccube, prop-spawn]
---

# P08 — divergência no spawn natural de actors (IPL3 integrity gate)

**HEAD base:** `7c444e46c1b431bb70311cdbb851b1c2dcf3afc7`  
**Status:** diagnóstico FECHADO e versionado. **Código funcional NÃO alterado.**  
**Fix preferido A:** decidido em review; **implementação em commit separado.**

## 0. Resumo executivo

| item | status |
|---|---|
| Actor manager (`D_80136EE0` / `func_801065E0`) | **CONFIRMED HEALTHY** |
| Natural actors baseline (intro flow) | **0** |
| Root gate | `func_8001DE64` → `D_8007DB79` |
| Baseline flag | `D_8007DB79 = 0x14` → `func_800BDCDC` skipped |
| ADDRESS_TRANSLATION_BUG | **YES** (stub +0x10000) |
| MISSING_EXPECTED_GUEST_WORDS | **YES** (correct addrs also zero) |
| GATE → prop pipeline | **YES** |
| GATE → actor creation | **YES** |
| `func_800D6E54` as population pass | **FALSE LEAD** (resource/reclaim queue) |
| Exact N64 writer of the two guest words | **UNKNOWN** (não overclaim) |
| Post-gate SIGSEGV `do_dma` | **POST_FIX_BLOCKER** (fora de escopo deste doc) |

---

## 1. FRONT-DOORS

Três front-doors de actor creation (call graph estático):

| Front-door | role | direct callers |
|---|---|---|
| `func_80106668` (`funcs_145.c`) | slot allocator → `vector_append` em `D_80136EE0` | somente `func_80107C2C` |
| `func_80107C2C` (`funcs_192.c`) | generic constructor | callback `ActorInfo[0x38]` (tabelas `.data.s`) |
| `func_80108C90` (`funcs_126.c`) | marker→Actor spawn | `func_80800890_gccollectDll`, outros |

Cadeia: `func_80108C90` → `jalr ActorInfo[0x38]` (= `func_80107C2C`) → `func_80106668`.

---

## 2. EXPECTED PIPELINE

```
gsworldDll:2
  -> func_800A5C28
       -> func_8001E204 gate (D_8007DB79)
       -> func_800BDCDC
            -> gccubeDll:5
                 -> gccollectDll:8
                      -> func_80108C90
                           -> func_80107C2C
                                -> func_80106668
```

---

## 3. ACTUAL PIPELINE (baseline natural)

```
gsworldDll:2 -> func_800A5C28  [HIT]
  -> func_8001E204 → D_8007DB79 = 0x14  (nonzero)
  -> SKIP func_800BDCDC
  -> BDCDC=0, cube5=0, collect8=0, c90=0, c2c=0, alloc=0
  -> natural actors = 0
```

### Baseline counters

| counter | value |
|---|---|
| `D_8007DB79` | `0x14` |
| `func_800BDCDC` | 0 |
| `gccubeDll:5` | 0 |
| `gccollectDll:8` | 0 |
| `func_80108C90` | 0 |
| `func_80107C2C` | 0 |
| `func_80106668` | 0 |

---

## 4. ROOT GATE — `func_8001DE64`

Anti-tamper / IPL3 integrity probe (original KSEG1 RDRAM; stub em `boot_hw_stubs.cpp`).

### 4.1 Original MIPS effective addresses

Recomp `funcs_238.c` (espelho do original):

```
lui  $t6, 0xA030          # base = 0xA0300000
lw   $t7, -0x4E0C($t6)    # base + sign_extend(-0x4E0C)
lui  $t8, 0xA030
lw   $t9, -0x1E40($t8)    # base + sign_extend(-0x1E40)
...
xor / or → sb low byte → D_8007DB79
```

Equivalente:

| immediate | sign-extend math | KSEG1 | guest KSEG0 (MEM_*) | phys |
|---|---|---|---|---|
| `-0x4E0C` (= imm16 `0xB1F4`) | `0xA0300000 + sign_extend(0xB1F4)` | `0xA02FB1F4` | `0x802FB1F4` | `0x2FB1F4` |
| `-0x1E40` (= imm16 `0xE1C0`) | `0xA0300000 + sign_extend(0xE1C0)` | `0xA02FE1C0` | `0x802FE1C0` | `0x2FE1C0` |

Delta RDRAM entre words: `0x2FCC` (não contíguo com delta ROM).

### 4.2 Expected words

| word | value | note |
|---|---|---|
| word0 | `0xAD090010` | baked no probe (`lui/ori`); = IPL3 `sw $t1, 0x10($t0)` |
| word1 | `0xAD170014` | baked no probe; = IPL3 `sw $s7, 0x14($t0)` |

Gate:

```
combined = (w0 ^ 0xAD090010) | (w1 ^ 0xAD170014)
D_8007DB79 = combined & 0xFF
```

- zeros guest → `combined` low byte **`0x14`** → gate FECHADO  
- words corretas → `combined == 0` → gate ABERTO → `func_800A5C28` chama `func_800BDCDC`

### 4.3 ROM retail (BE) — `Banjo-Tooie (USA).z64`

| ROM offset | bytes BE | word |
|---|---|---|
| `0x73C` | `AD 09 00 10` | `0xAD090010` |
| `0x750` | `AD 17 00 14` | `0xAD170014` |

Hits únicos no ROM; delta ROM `0x14` ≠ delta RDRAM `0x2FCC` → **não** é cópia contígua de IPL3 para esses addrs.

---

## 5. STUB BUG PROVADO — ADDRESS_TRANSLATION_BUG

Stub atual (`boot_hw_stubs.cpp::func_8001DE64`, HEAD):

```cpp
MEM_W(0, kseg0_gpr(0x8030B1F4u)); // was A030:-0x4E0C
MEM_W(0, kseg0_gpr(0x8030E1C0u)); // was A030:-0x1E40
```

| | stub lê | correto |
|---|---|---|
| word0 KSEG0 | `0x8030B1F4` | `0x802FB1F4` |
| word1 KSEG0 | `0x8030E1C0` | `0x802FE1C0` |
| delta | **+0x10000** | — |

**Causa:** immediate negativo MIPS tratado como **unsigned** (OR / add de `0xB1F4`/`0xE1C0` sem sign-extend) em vez de `base + (int16_t)imm`.

**Classificação:** **REAL BUG.**

**Insuficiência isolada:** address fix sozinho **não** abre o gate — words nos addrs **corretos** também estão **zero** no RDRAM guest (ver matrix §6).

---

## 6. DISCRIMINATOR MATRIX

Env: `TOOIE_PROBE_VARIANT` + GDB auto-start/diag.  
Nota: “crash” GDB inicial em algumas runs = false-positive de hook-stop; matrix abaixo é a fechada.

| variante | flag `D_8007DB79` | BDCDC | cube5 | collect8 | c90 | c2c | alloc | notes |
|---|---|---|---|---|---|---|---|---|
| **baseline** (stub HEAD, no seed) | `0x14` | 0 | 0 | 0 | 0 | 0 | 0 | gate closed |
| **address fix only** | `0x14` | 0 | 0 | 0 | 0 | 0 | 0 | correct addrs still zero |
| **correct seed + wrong stub** | `0x14` | 0 | — | — | — | — | — | seed nos addrs corretos; stub lê +0x10000 |
| **wrong seed + wrong stub** | `0` | 1 | 1 | 1274 | 11 | 10 | 10 | seed nos addrs que o stub lê |
| **correct seed + fixed stub** | `0` | 1 | 1 | 1274 | 11 | 10 | 10 | gate opens; pipeline runs |

**Depois** de gate open (seed_wrong ou seed_correct+fixed): novo **SIGSEGV em `do_dma`**  
(`osPiStartDma_recomp` ← … ← gsmaker ← gsworld ← chintrochar).  
Classificação: **POST_FIX_BLOCKER** — **não** investigar neste commit; sem acoplamento direto ao mecanismo do integrity gate.

### Leitura da matrix

1. Address bug real (+0x10000).  
2. Guest words esperadas **ausentes** nos addrs corretos.  
3. Gate controla prop pipeline **e** criação de actors (c90/c2c/alloc sobem só com flag 0).  
4. OTHER_READERS do flag/path: só `func_8001DE64` grava o padrão medido; consumers via `func_8001E204` / `func_800A5C28`.

---

## 7. CONCLUSÕES PROVEN

| claim | verdict |
|---|---|
| `ADDRESS_TRANSLATION_BUG` | **YES** |
| `MISSING_EXPECTED_GUEST_WORDS` | **YES** |
| `GATE_TO_PROP_PIPELINE` | **YES** (`BDCDC`/`cube5`/`collect8`) |
| `GATE_TO_ACTOR_CREATION` | **YES** (`c90`/`c2c`/`alloc`) |
| `func_800D6E54` actor population | **FALSE LEAD** — fila de setup/resource/reclaim deferido; sem edge causal provado para `func_80108C90` / `func_80107C2C` / `func_80106668` (ver também `p08-intro-actor-spawn-diagnosis.md` §3.5) |

### Exact N64 writer — NÃO overclaim

Redação correta:

> Os valores `0xAD090010` / `0xAD170014` são **necessários como estado guest** no momento do probe e são **consistentes com residual de boot/IPL3** (opcodes IPL3 em ROM `0x73C`/`0x750`). O **mecanismo/writer exato** que os materializa no N64 original **ainda não foi identificado**.

Writer class (hipótese de trabalho, não fato): boot environment / sparse IPL3 residual — não reescrito como “IPL3 DMA contíguo” (deltas ROM vs RDRAM diferem).

---

## 8. ROOT CLASS

**B — actor/prop enumerator nunca é chamado**  
Causa: gate `D_8007DB79=0x14` bloqueia `func_800BDCDC` → `gccubeDll:5` → `gccollectDll:8` antes da enumeração de props / spawn.

---

## 9. DECISÃO DE DESIGN (review) — NÃO IMPLEMENTADA

| opção | shape | algo original | guest state | risk |
|---|---|---|---|---|
| **A (preferida)** | sign-extend addr + seed one-shot dos 2 words RDRAM; DE64 mantém lw/xor/or/sb | YES | YES | LOW |
| B | ROM `0x73C`/`0x750` no override do probe | NO | NO | MED |
| C | force `combined=0` | NO | NO | HIGH |

**Fix A (decidido, commit futuro):**

1. Corrigir address/sign-extension no stub (`base + (int16_t)imm` → KSEG0).  
2. Restaurar **uma vez** os dois guest RDRAM words (`0xAD090010` @ `0x802FB1F4`, `0xAD170014` @ `0x802FE1C0`).  
3. Manter `func_8001DE64` lendo RDRAM e executando XOR/OR original.  
4. Locus de seed: `func_80000450` **após** `boot_osInitialize`, **antes** de entrar em core1.  
5. **Não** seed dentro de DE64 per-call; **não** force flag; **não** alterar actors.

Constantes do seed podem ser literais iguais ao probe **ou** lidas de ROM `0x73C`/`0x750` (mesmos bytes no retail USA) — opcional.

**Este commit:** zero código. Só documentação.

---

## 10. CONFIDENCE

**HIGH** — matrix fechada; MIPS/ROM estáticos fechados; writer exato residual UNKNOWN sem mudar preferência A.

---

## 11. EVIDENCE NOTES

- Logs GDB: `/tmp/opencode/p08_spawnchain*.log`, `p08_gate*.log`, `p08_de64.log`, `p08_addr.log`, `p08_probe_*.log`, `p08_probe_crash_wrong.log`, `p08_probe_crash_fixed.log`.  
- Scripts: `/tmp/opencode/gdb_probe_disc.gdb`, `gdb_probe_crash2.gdb`.  
- Env: `TOOIE_PROBE_VARIANT`, `TOOIE_AUTO_START`, `TOOIE_DIAG`, `TOOIE_DISABLE_LAUNCHER_MUSIC`.  
- ROM check: `Banjo-Tooie (USA).z64` / `build/bt.n64.us.1.0.z64` @ `0x73C`/`0x750`.  
- Stub HEAD: `src/game/boot_hw_stubs.cpp::func_8001DE64` (restaurado ao original do commit base).  
- Recomp mirror: `RecompiledFuncs/funcs_238.c::func_8001DE64`.  
- Metadata: `D_A02FB1F4` / `D_A02FE1C0` / `D_8007DB79` em `metadata/tooie.us.*.toml`.

---

## 12. NEXT

1. Implementar **fix A** em commit de código separado.  
2. Só então retomar SIGSEGV `do_dma` (post-gate blocker).  
3. Não reabrir `func_800D6E54` como population pass.
