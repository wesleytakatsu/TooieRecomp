---
title: "BT P06 — Overlays e syscalls"
description: "Implementa o ciclo completo dos módulos dinâmicos de Banjo-Tooie no runtime nativo."
status: completed
type: implementation-stage
project: TooieRecomp
phase: P06
order: 6
depends_on: P05
updated: 2026-08-09
tags: [banjo-tooie-Recomp, overlays, syscalls, relocations]
---

# BT P06 — Overlays e syscalls

## Resultado esperado

Overlays carregam por índice/ROM, aplicam relocations, expõem entrypoints e
syscalls e descarregam sem referência inválida. Movimentação do heap permanece
neutralizada com segurança enquanto o runtime não oferece remapeamento atômico.

## Status (2026-08-09)

**Concluído no escopo P06.** O boot nativo usa a fila real do scheduler e recebe
ACK do evento 3, atravessa mais de cem syscalls aninhados e não executa mais
LOOKUP nulo. `ovl_load` e `ovl_unload` mantêm o mapa nativo sincronizado.

A imagem suportada nesta fase é exclusivamente `decompressed.us.z64`: pacote
com flag de compressão é recusado, pois tratá-lo como DMA cru corromperia o
overlay. A compactação móvel do heap é um no-op host-safe até o runtime oferecer
uma operação atômica para mover os endereços nativos; unload explícito funciona.

## Como foi implementado

### Problema do recompilador

N64Recomp emite cada stub de `overlay_syscalls` assim:

```c
// 0x800854C0: syscall 164
recomp_syscall_handler(rdram, ctx, 0x800854C0);
return;
// dead: addi/xori $t0 nunca executa
ctx->r8 = ...;
```

Portanto o handler **não pode** confiar em `$t0` no `recomp_context`. Tem de
reconstruir `(overlay_index, entry_index)` a partir do ROM descomprimido.

### Semântica do stub MIPS (verificada no ROM)

| Campo | Origem | Valor |
| --- | --- | --- |
| `syscall_code` | bits `[25:6]` da word em `instruction_vram` | **índice do overlay** (1-based da tabela do jogo; ex. gsmaker=`2`, bainvisible=`164`) |
| `entry_index` | `imm/4` da word seguinte (`addi` ou `xori $t0,$zero,imm`) | índice na lista `entrypoints` do overlay — **não** é o offset de função em `.text` |
| Core2 VRAM→ROM | `rom_off = 0x01E5AEB0 + (vram - 0x800815C0)` | seção `section_16_core2` |

**Não** usar `overlay_sections_by_index` do N64Recomp como mapa por índice de
jogo: a lista `relocatable_sections` está em ordem **alfabética**.

### Tabela de dispatch gerada

| Artefato | Papel |
| --- | --- |
| `tools/generate_overlay_dispatch.py` | Lê `metadata/overlays.table.toml` + `RecompiledFuncs/recomp_overlays.inl` |
| `src/game/generated/overlay_dispatch.inc.h` | `TooieOverlayDispatchEntry`, `kTooieOverlayCount` |
| `src/game/generated/overlay_dispatch.gen.cpp` | Arrays `ovl_N_entrypoints[]` + `g_tooie_overlay_dispatch[885]` |

Cada slot (índice = overlay do jogo) guarda:

- `rom_addr`, `size` (da `section_table` recomp);
- `num_entrypoints` e ponteiros para as funções nativas **sem** prefixo `_`
  (`gsmaker_entrypoint_0`, não `_gsmaker_entrypoint_0`).

Regenerar após mudar overlays/recomp:

```bash
python3 TooieRecomp/tools/generate_overlay_dispatch.py
```

### `recomp_syscall_handler` (`src/game/syscall_handler.cpp`)

Fluxo em runtime:

1. Lê 8 bytes do stub em `recomp::get_rom()` (core2).
2. Decodifica `syscall` + `addi`/`xori`.
3. Preenche `ctx->r8 = entry_index * 4` (compatibilidade com quem leia `$t0`).
4. Valida limites / overlay vazio / entry out of range (loga e retorna).
5. Carrega o pacote pela implementação host de `ovl_load`, preservando a cópia
   dinâmica no heap para relocations e dados do overlay.
6. Registra temporariamente a mesma seção no banco canônico `0x80800000`, onde
   os operandos de `LOOKUP_FUNC` permanecem ligados no C recompilado.
7. **Nested restore:** uma pilha thread-local estaciona o alias externo, ativa o
   callee e restaura os dois mapeamentos ao retornar.
8. Reutiliza um pacote já ativo ou residente. Isso reproduz os stubs que o MIPS
   original autopatcheia; as instruções modificadas não podem alterar o C já
   gerado pelo N64Recomp.
9. Log de dispatch/return (cap configurável).

CMake: `syscall_handler.cpp` + `overlay_dispatch.gen.cpp` no target
`TooieRecompiled`; include `src/game` e `RecompiledFuncs`.

### Boot nativo + overlays dinâmicos (`src/game/boot_hw_stubs.cpp`)

O runtime usa a **imagem descomprimida** (`decompressed.us.z64`). Bridges host
(símbolos **strong** sobre `RECOMP_FUNC` weak):

| Override | Comportamento |
| --- | --- |
| `boot___osSi*` / `boot_osPiGetStatus` / PI access | MMIO host-safe / not busy |
| `boot___osPiRawStartDma` | `recomp::do_rom_read` |
| `boot_osInitialize` | no-op (ultramodern assume hardware) |
| `func_80000450` | carrega **core1** + `load_overlays` + entry; re-instala ultra LOOKUP hooks |
| `func_80019EC0` | carrega bancos fixos (ex. **core2**) sem inflate |
| `rom_read_word` / `rom_dma_read` | PIO/DMA direto no image ROM |
| `func_8001DE64` | anti-tamper Rare: KSEG1 → KSEG0 |
| `func_800815CC` | `D_80126730 = 0x01F00020` (tabela descomprimida; não `table_addr@0x50E0`) |
| `ovl_load` | heap_alloc_sided + DMA pacote + `ovl_clear_bss` + lista `D_80126738` + `load_overlays` no text real; rejeita pacote comprimido |
| `ovl_unload` | restaura syscalls, chama `unload_overlays`, atualiza a lista e libera o heap |
| `defragment_overlays` | no-op host-safe; impede mover blocos sem atualizar atomicamente o mapa nativo |
| `func_80082088` | reloca entrypoints / text_start **sem** PI MMIO `0xA460` nem cart KSEG1 |
| `func_800137F4` | preserva no host o idiom MIPS `move a1, ra; jal; jr a1`, eliminando LOOKUP `0x00000000` |
| `func_80013D10` | handshake real `osSendMesg(BLOCK)` → scheduler → `osRecvMesg(BLOCK)` em `D_80078438` |

#### LOOKUP: ultra hooks vs gap stubs

`banjo::register_boot_lookup_stubs()` **não** pode no-op-ar libultra real:

| VRAM | Antes (errado) | Agora |
| --- | --- | --- |
| `0x8001DCA0` | leitura CP0 recompilada como zero | `__osGetSR_2` host com `SR_IBIT5`; evita o laço fatal `func_800149BC` |
| `0x8002EF70` | no-op | `osDpSetStatus_recomp` |
| `0x8002F240` | no-op | `osDpGetStatus_recomp` |
| `0x8002FE70` | no-op | host no-op yield (`osYieldThread_recomp` é `assert(false)`) |
| `0x80031B60` / `0x80031BA4` | no-op | `__osPiGetAccess/Rel_recomp` |
| `0x80030A10` | no-op | `osPfsInit_recomp` |
| gaps verdadeiros | no-op | no-op (lista reduzida) |

Hooks são reinstalados após o load de core1.

### Correções posteriores encontradas durante P08

Estas correções pertencem ao ciclo de overlays, embora tenham sido descobertas
ao diagnosticar a câmera da abertura:

- `ovl_load` usa `rom_addr` e `size` da seção recompilada para registrar código.
  O `text_size` interno do pacote pode ser menor que a seção (`0x230` contra
  `0x240` foi observado), caso em que o runtime antes não selecionava seção.
- `func_80082088` cobre relocations curtas e longas e não soma `text_start` duas
  vezes.
- O stream curto no ROM descomprimido pode já estar sem XOR. O host valida o
  stream cru e o stream com a chave derivada da tabela; só aplica a chave se o
  cru for inválido.
- O alias canônico e o mapeamento do heap têm papéis diferentes e ambos são
  necessários: o primeiro resolve `LOOKUP_FUNC`, o segundo mantém correta a base
  usada pelos `RELOC_*`.
- Reentrada no mesmo overlay ativo reutiliza o pacote em vez de repetir todo o
  load/unload; overlays residentes (`flag 0x04`) permanecem em cache.

O smoke de 28 s após essas mudanças chegou a `39075` dispatches e `39075`
retornos, sem chamada em voo, crash ou lookup ausente. As mudanças ainda não
estavam commitadas no momento do handoff; ver
[[../../plan/evidence/p08-session-handoff]].

`TOOIE_AUTO_START=1`: após ~30 frames do launcher, `check_all_stored_roms` +
`start_game` (útil para smoke sem UI).

### Nomes de thread Tooie

`get_game_thread_name` em `src/main/main.cpp` foi alinhado a
`banjo-tooie-Recomp/include/threads.h` (id 5 = **SCHEDULER**, não “RESET” do BK).

## Tarefas

- [x] `P06-T01` Gerar e registrar tabelas de seções e overlays por índice.
- [x] `P06-T02` Implementar lookup por índice (dispatch table + ROM decode).
- [x] `P06-T03` Aplicar relocations via `load_overlays` + image em RDRAM antes do call.
- [x] `P06-T04` Registrar entrypoints e dispatcher de syscalls com checagem de limites.
- [x] `P06-T05` Host `ovl_load` + `func_80082088` para pacotes descomprimidos
      (heap + BSS + `load_overlays`); pacotes comprimidos falham de forma explícita.
- [x] `P06-T06` Unload sincroniza o mapa nativo; movimentação/defrag neutralizada
      com segurança enquanto não houver API host para remapear uma seção carregada.
- [x] `P06-T07` Instrumentar load/dispatch (stderr: índice, ROM, VRAM, return).
- [x] `P06-T08` Testes unitários de decode + sanity da tabela gerada.
- [x] `P06-T09` Detectar índice inválido / entry out of range / overlay vazio antes de call.
- [x] Integração: boot → core2 → handshake event 3 → syscalls aninhados com restore.

## Gate de saída

- [x] Boot atravessa as primeiras trocas de overlay (`gcstatusDll`, `chbaddieDll`,
      `cosection`) e continua em globals (`glglobalsettings`, `glgamestore`, …).
- Diferido para P07/P09: uma área jogável carrega, executa e descarrega módulos repetidamente.
- [x] Decode + dispatch sem entrypoint desconhecido nos cenários unitários.
- Diferido para P09: sanitizers sem uso após unload ou acesso fora dos limites.
- [x] Scheduler processa o evento 3 pela fila real e envia ACK em `D_80078438`.
- Diferido para P07: VI/RSP avançam continuamente para frame.

## Verificação

```bash
: "Decode unitário (não precisa do binário do jogo)"
python3 TooieRecomp/tools/test_syscall_decode.py

: "Regenerar tabela (após recomp/metadata)"
python3 TooieRecomp/tools/generate_overlay_dispatch.py

: "Smoke runtime (ROM descomprimida em build/ ou config path como bt.n64.us.1.0.z64)"
cd TooieRecomp/build
TOOIE_AUTO_START=1 ./TooieRecompiled
: "stderr esperado: core2/815CC, dispatch/return e event 3 scheduler ack"
: "Evidence: docs/plan/evidence/p06-runtime-scheduler-handshake.log"
```

## Decisões e itens diferidos

1. **Compressão/XOR:** não integra o runtime atual. `decompressed.us.z64` é o
   contrato de entrada; flag `0x80` retorna erro antes de alocar ou registrar código.
2. **Defrag móvel:** diferido até existir remapeamento atômico no librecomp.
   Executá-lo hoje deixaria entrypoints nativos apontando para o endereço antigo.
3. **Área jogável, sanitizers e frame contínuo:** dependem de VI/RSP/audio e são
   gates de [[07-rsp-and-audio]] e [[09-gameplay-parity-and-enhancements]].

## Evidência

- [[../../plan/evidence/p06-syscall-decode.log]]
- [[../../plan/evidence/p06-build-summary.txt]]
- [[../../plan/evidence/p06-runtime-syscall-dispatch.log]] — primeiros 3 syscalls
- [[../../plan/evidence/p06-runtime-scheduler-handshake.log]] — fila real, 3 ACKs,
  110 dispatches em 20 s e nenhum LOOKUP ausente
- [[../../plan/evidence/p08-session-handoff]] — correções pós-P06 de lifecycle,
  relocations, reuse ativo/residente e smoke de 39.075 dispatches

## Arquivos principais

| Caminho | Função |
| --- | --- |
| `TooieRecomp/src/game/syscall_handler.cpp` | `recomp_syscall_handler` + nested restore |
| `TooieRecomp/src/game/boot_hw_stubs.cpp` | bridges boot/DMA/ovl_load/815CC/82088/13D10/ultra LOOKUP |
| `TooieRecomp/src/game/generated/overlay_dispatch.*` | tabela gerada |
| `TooieRecomp/tools/generate_overlay_dispatch.py` | gerador |
| `TooieRecomp/tools/test_syscall_decode.py` | testes de decode |
| `TooieRecomp/src/main/main.cpp` | auto-start + nomes de thread |
| `TooieRecomp/src/game/rom_decompression.cpp` | `bk_on_init` → LOOKUP stubs |
| `TooieRecomp/CMakeLists.txt` | fontes + include `src/game` |
| `banjo-tooie-Recomp/src/core2/overlays.c` | referência `ovl_load` / unload / defrag |
| `banjo-tooie-Recomp/src/core1/1E2B840.c` | scheduler thread + event 3 / `func_80013D10` |

## Próxima etapa

1. Implementar eventos VI e tasks RSP — ver [[07-rsp-and-audio]].
2. Entrar no loop de frame e carregar uma área jogável.
3. Fechar os gates de unload repetido e sanitizers em P09.
