# P08 — SIGSEGV do_dma: dinamic diagnosis

**HEAD:** `f865c038455da1e5fafb5257ece974e1e15c9399`
**Modo:** diagnóstico somente. Zero código alterado.
**Status:** ROOT HIGH — overlay base-vram collision no func_map.

---

## 1. Objetivo

Descobrir a PRIMEIRA origem de `size = 0xFF3FF03E` no `do_dma` SIGSEGV e
classificar a divergência. Nada foi corrigido.

---

## 2. Confirmação da ABI (resultado: CORRETA)

`0x8002E450` = `osPiStartDma` (libultra clássico, 7 args O32):

| arg | reg/stack | host `osPiStartDma_recomp` | verificação |
|---|---|---|---|
| mb | a0 | `ctx->r4` | `0x80076940` |
| pri | a1 | `ctx->r5` | `0` |
| direction | a2 | `ctx->r6` | `0` (cart→RDRAM) |
| devAddr | a3 | `ctx->r7 \| rom_base` | `0x00F13B24` |
| dramAddr | sp+0x10 | `MEM_W(0x10, r29)` | `0x80011000` |
| size | sp+0x14 | `MEM_W(0x14, r29)` | `0xFF3FF03E` ← bad |
| mq | sp+0x18 | `MEM_W(0x18, r29)` | `0x8007695C` |

Callsite `func_80013224` monta exatamente essa forma (a3=devAddr, sp10=dramAddr,
sp14=size, sp18=mq). `func_80013324` repassa args sem tocar. Host lê exatamente
os mesmos slots.

```
CALLSITE_SIZE:   0xFF3FF03E
HOST_RECEIVED:   0xFF3FF03E
SAME: YES   -> problema NÃO é ABI/host extraction.
```

---

## 3. Origem do size

```
func_800E6950(idx, flag, 0)
  a0 (devAddr) = func_800D5A6C(idx) = D_8012B448 + (romword(D_8012B900 + idx*4) >> 6)
  a1 (size)    = func_800D5950(idx) = (romword(D_8012B900+idx*4+4) >> 6)
                                     - (romword(D_8012B900+idx*4) >> 6)
  a2 (dramAddr base) = D_80124A8C = 0x80011000  (constante)
```

- `func_800D5950` lê 2 words do ROM via `rom_read_word` (0x8001E210) na
  offset `(D_8012B900 + idx*4) & 0x0FFFFFFF` e retorna a diferença `>>6`.
- Para idx válido (positivo pequeno) isso é o tamanho correto do bloco:
  - idx=0x707  → size 0x3D08 (DMA ok)
  - idx=0x8a6  → size 0x17B8 (DMA ok)
  - idx=0x0    → size 0xD0 (DMA ok)
- Para idx = **-0x3AB** → `D_8012B900 - 0xEAC` cai fora da tabela → garbage
  → `size = 0xFF3FF03E`.

```
SIZE_SOURCE:    func_800D5950(idx) sobre tabela D_8012B900 + idx*4 (ROM words)
SIZE_LAST_WRITER: rom_read_word -> D_8012B440 / D_8012B444
SIZE_EXPECTED:  blocos válidos dão 0xD0..0x3D08
SIZE_ACTUAL:    0xFF3FF03E  (idx negativo = fora da tabela)
```

A causa raiz NÃO é o cálculo do size em si: o idx chegou **negativo** porque a
cadeia de chamadas anterior invocou a função errada.

---

## 4. Cadeia do idx negativo (prova dinâmica)

Captura GDB (cadeia completa, várias execuções independentes):

```
E6950_ENTER a0=0xfffffffffffffc55 a1=0x2 a2=0x0
D5A6C#14   idx=0xfffffffffffffc55
D5950#13   idx=0xfffffffffffffc55
DMA_LOOP_ENTER a0=0xf13b24 a1=0xffffffffff3ff03e a2=0xffffffff80223820
DMA#293 mb=0x80076940 pri=0 dir=0 dev=0xf13b24 va=0x80011000 sz=0xff3ff03e
SIGSEGV -> do_dma
```

Montagem:

```
func_80800484_chintrochar   (actor ptr 0x801DF300)
  jal 0x80800000            (RELOC intra-overlay para a PRÓPRIA base)
  -> LOOKUP_FUNC(0x80800000)
  -> resolve para gsworldDll_entrypoint_0  (ERRO!)
  -> chama func_80082558 (gsmaker ep2 stub)
  -> gsmaker ep2: a0 += 0x955   => 0x801DFC55
  -> gsmaker ep1: MEM_H(0x20,s0)=a0 ; a0=lh => (int16_t)0xFC55 = -0x3AB
  -> func_800D674C(-0x3AB) -> func_800D5B34 -> func_800E6950(-0x3AB,...)
  -> devAddr=0xF13B24, size=0xFF3FF03E -> osPiStartDma -> do_dma SIGSEGV
```

Cada passo verificado dinamicamente:
- `GS1#2 a0=0xffffffff801dfc55` (ep1 recebe +0x955 do ep2)
- `D674#22 a0=0xfffffffffffffc55` (ep1 truncou p/ int16)
- `CHBASE_HITS=0 GSEP0_HITS=2` → `func_80800000_chintrochar` **nunca** é chamada;
  `gsworldDll_entrypoint_0` é chamada 2× (1ª correta idx=0x18b, 2ª errada actor ptr).

---

## 5. Por que 0x80800000 resolve para gsworldDll (ROOT)

- **Muitos DLL overlays carregam na base vram `0x80800000`**:
  `.gsworldDll`, `.chintrochar`, `.gsmaker`, `.fxdialog`, `.gspropsDll`,
  `.gsmaker` etc. Todos têm `entrypoint_0`/func na mesma vram `0x80800000`.
- O `func_map` do N64Recomp é **um mapa plano vram→func** (`std::unordered_map`).
- `recomp_syscall_handler` / `activate_canonical_mapping` chamam
  `load_overlays(rom_addr, kOverlayVram=0x80800000, size)` que faz
  `func_map[0x80800000+offset] = func` para TODAS as funcs do overlay ativado.
- Quando `gsworldDll` (ovl=726) é despachado, ele **sobrescreve**
  `func_map[0x80800000] = gsworldDll_entrypoint_0`.
- O overlay `chintrochar` (ovl=440) foi carregado (ovl_load idx=440 para heap),
  mas a base `0x80800000` do func_map ainda aponta para `gsworldDll_entrypoint_0`.
- `func_80800484_chintrochar` (chintrochar, vram 0x80800484) faz o `jal 0x80800000`
  intra-overlay esperando `func_80800000_chintrochar` (que recebe actor ptr e
  lê `0x76($a0)`), mas cai em `gsworldDll_entrypoint_0`.

```
func_80800484_chintrochar (0x80800484, .chintrochar)
  0x808004E8: jal 0x80800000      ; R_MIPS_26 target 0x80800000 (base do próprio overlay)
  func_map[0x80800000] = gsworldDll_entrypoint_0   <-- venceu último overlay carregado
  gsworldDll_entrypoint_0(actor_ptr)   <-- trata actor ptr como world index
```

`func_80800000_chintrochar` NUNCA é invocada (CHBASE_HITS=0) — a chamada
intra-overlay foi sequestrada por outra DLL que ocupa a mesma base vram.

---

## 6. Exame do size 0xFF3FF03E

- u32 = 0xFF3FF03E
- s32 = -0x00C00FC2 (negativo)
- halves: 0xFF3F / 0xF03E
- Não corresponde a: campos adjacentes de header, instruções MIPS válidas,
  endereços de ROM esperados, ou underflow `end-start` de bloco legítimo.
- É o resultado de `(w1>>6) - (w0>>6)` sobre 2 words lidas FORA da tabela
  (idx -0x3AB), i.e. garbage determinístico da indexação negativa — **não**
  é stack stale, nem pointer arithmetic corrompida.

`func_80117248` (loop de DMA chunked) usa `slti $at, a2, 0x800` (comparação
**signed**); size negativo não dispara o clamp 0x800 → repassa `0xFF3FF03E`
inteiro como chunk → `do_dma` tenta ler 4 GB → SIGSEGV em `do_dma+592`
(`movzbl (%rax,%rdx,1)`).

---

## 7. Primeira divergência

```
EXPECTED:
  func_80800484_chintrochar -> func_80800000_chintrochar (base do overlay chintrochar,
  recebe actor ptr 0x801DF300, processa intro)

ACTUAL:
  func_map[0x80800000] = gsworldDll_entrypoint_0  (outro overlay, última ativação)
  gsworldDll_entrypoint_0(actor ptr) -> gsmaker ep2/1 -> idx (int16) = -0x3AB
  -> func_800D5A6C/800D5950 em idx inválido -> devAddr=0xF13B24 size=0xFF3FF03E
  -> osPiStartDma -> do_dma SIGSEGV
```

Não paramos em "size inválido": a origem é o **binding errado de
`LOOKUP_FUNC(0x80800000)`** — colisão de base vram entre overlays.

---

## 8. Classificação

```
ROOT_CLASS: H (other) — overlay base-vram collision no func_map:
            intra-overlay jal 0x80800000 de chintrochar resolveu para
            gsworldDll_entrypoint_0 em vez de func_80800000_chintrochar.
CONFIDENCE: HIGH  (prova dinâmica completa, múltiplas execuções)
CODE_CHANGED: NO
FIX_PROPOSED: NONE (diagnóstico somente; direção possível: resolução de
             intra-overlay calls relativa ao overlay ativo, ou restaurar
             func_map[0x80800000] ao ativar chintrochar/descarregar gsworld)
NEEDS_GROK: YES (mecânica de canonical mapping / load_overlays em overlays
             que compartilham base 0x80800000)
```

---

## 9. Evidências dinâmicas (logs)

- `/tmp/opencode/p08_do_dma_diag.log` — cadeia idx→devAddr→size→DMA#293
- `/tmp/opencode/p08_do_dma_chain.log` — WORLDEP0/GS2/GS1/D674 a0 chain
- `/tmp/opencode/p08_chintro_base.log` — CHBASE_HITS=0, GSEP0_HITS=2, ovl_load idx=440
- `/tmp/opencode/p08_world_bt.log` — backtrace completo do crash

Backtrace do crash (recompilado):

```
do_dma ← osPiStartDma_recomp ← func_80013224 ← func_80013324
  ← func_80117248 ← func_800E6950 ← func_800D5B34 ← func_800D674C
  ← gsmaker_entrypoint_1 ← gsmaker_entrypoint_2 ← recomp_syscall_handler
  ← gsworldDll_entrypoint_0 ← func_80800484_chintrochar
  ← func_80800540_chintrochar ← func_80100120 ← func_801001D8
```

---

## 10. Por que não é (discriminadores)

- **A (caller size corrupto):** o size é consequência do idx negativo, não
  cálculo errado do caller com inputs válidos.
- **B (ABI/stack):** descartado — callsite e host leem os mesmos slots.
- **C (stack stale/uninit):** descartado — sp+0x14 contém exatamente o que o
  caller gravou; o valor vem do ROM via tabela, não de stack.
- **D (asset/header):** descartado — a tabela lida é válida; o ÍNDICE é que
  está inválido.
- **E (decompressed-ROM):** descartado — não é questão de inflate.
- **F (mem corruption):** descartado — não há write corrompendo; é
  dispatch/function-binding.
- **G (host do_dma):** descartado — args são garbage por upstream, não bug host.
- **H (other):** escolhido — colisão de overlays na base vram 0x80800000.
