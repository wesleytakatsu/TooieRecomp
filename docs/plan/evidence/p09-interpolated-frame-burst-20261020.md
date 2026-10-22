---
title: "P09 — Burst F8 de 20 PNGs de frames interpolados"
description: "Atalho F8 implementado e comprovado em jogo com 20 PNGs intermediários renderizados, manifesto e status."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-20
tags: [p09, interpolation, screenshot, f8, rt64, patches]
---
# Evidência P09: burst F8 (20 PNGs interpolados)

> **Atualização 2026-10-21:** o atalho F8 foi **removido** a pedido do usuário (ver [[p09-menu-interpolation-toggle-20261021]]). Esta evidência permanece historicamente verdadeira para o período em que o hotkey existia; a infra RT64 de captura continua dormente e nenhuma tecla a aciona.

**Escopo:** ferramenta para o usuário achar glitches visualmente. Não há diagnóstico visual automático. Estado: **`IMPLEMENTED / RUNTIME_SHORTCUT_20_PNG_PASS`** na execução isolada abaixo. Isso não prova correção de glitch nem captura de um glitch específico.

## Prova real do atalho — 2026-10-20

Sonnet implementou via Cursor `claude-sonnet-5-5-medium`, sessão `e7701bdc-9552-4ae4-8203-e59a9f252b5c`, exit 0 / JSON success, 916.127 s. Prompt/saída/exit `sonnet-render-burst-implement*` na cache `/home/wesley/.cache/tooie-interpolation-next-20261020/`. Executor declarou navegação obrigatória incompleta e jCodeMunch/audit indisponíveis; root/executor Codex complementaram leitura relevante, revisão localizada e checks. Não confundir reporte do executor com conformidade integral da sessão dele.

- Gate final serial `rtk cmake --build build --target p09_interp_burst_test TooieRecompiled -j2`: exit **0**, `ninja: no work to do`; build de implementação reportado Sonnet exit 0. Gate unitário final `rtk ./build/p09_interp_burst_test`: exit **0**, PASS. Builds de verificação intermediários sobrepostos foram interrompidos/timeout e **não** contam como PASS.
- Binário testado SHA256 `7fb2d73e1bb883e96510a84d5e7acbb76374df99cd2f4c089eef883d7d9de0cc`; Tooie HEAD `73bfd4303b319ddb98fd42ddb6f6ccf8025794c2`, RT64 `6f1c2d99a4ea571c139f449c326fd176ba8f3496`, ambos dirty protegido. Graft final atualizado e root atualizou índices jCodeMunch dos oito arquivos de fonte relevantes.
- Instância isolada PID **`1431958`**, start_ticks **`10790476`**, janela própria X11 `130023426`; metadata/log em `/home/wesley/.cache/tooie-interpolation-next-20261020/render-burst-runtime-20261020T204919Z/`. Apenas `TOOIE_TEST_INTERPOLATION_FPS=60`, `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0`, auto-start/music-off e `APP_FOLDER_PATH` isolado; sem snapshot/SWAY/V7/heavy. Hashes da configuração/saves pessoais antes/depois iguais; nenhuma preferência pessoal alterada. Instância de teste anterior já estava encerrada ao lançar (nenhum sinal necessário nessa troca).
- F8 enviado à janela com PID/ticks verificados. Diretório real **`/home/wesley/Imagens/TooieRecomp/interpolated-burst-20261020-174942/`**: 20 arquivos `image_001.png`…`image_020.png`, 20 linhas em `manifest.csv`, `status.txt` `DONE saved=20/20`. Todos decodificados, 1622×912, 20 hashes distintos; `kind=INTERPOLATED`, `curW=0.333333` ou `0.666667`, matched=1, mapped=1/1, estágio `PRE_VI_RESOLVED_COLOR_TARGET`. Taxas registradas para essa cena: target 60 / `viOriginalRate=20` (não inferir taxa universal). Endpoints/nativos foram rejeitados e contados.
- Root examinou independentemente `image_010.png`: imagem do title upright, cores legíveis e sem overlay FPS. Não prova geometria de todas as cenas ou portabilidade do readback. Capture é pré-VI, não corresponde pixel a pixel ao desktop/VI/UI.
- Segunda solicitação já enviada ao receber orientação de encerrar verificação: pasta `interpolated-burst-20261020-175030/`, log `DONE 20/20`, segundo F8 rápido **ignorado por burst ativo**. Nenhum terceiro burst/teste. Isso confirma rearmamento e exclusão de solicitação concorrente nessa execução; os 20 arquivos dessa segunda pasta não receberam a mesma inspeção visual independente da primeira.
- Gameplay/attract continua disponível para o usuário apertar **F8** durante o movimento, e indicar **pasta + números das imagens** que mostram glitch. No caminho normal CURRENT/30, ausência de interpolação foi validada no host; run normal separado em jogo **NOT_TESTED** nesta etapa. Nenhum glitch específico foi alegado como capturado ou resolvido.

**`NEXT_SINGLE_ACTION`: usuário apertar F8 na cena desejada e informar a pasta e os números dos PNGs com glitch.** Sem commit/push; árvore dirty preexistente preservada.

## O que foi feito

- F8 (`SDL_AddEventWatch`, `!repeat`) só chama `RT64::interpBurst::request()`; não consome evento nem altera binding (`src/game/tooie_interp_burst.cpp`). Se o usuário religou F8 como botão, ambos agem.
- Pasta base: `XDG_PICTURES_DIR/TooieRecomp` (parse simples de `user-dirs.dirs`, só `$HOME` expandido; fallback `~/Pictures`; sem home: `<app folder>/captures`). Não existia base nativa de screenshot. Cada burst usa `interpolated-burst-<hora>[-n]/` criado com `create_directory` exclusivo, `image_001.png`…`image_020.png`, `manifest.csv`, `status.txt`. Sem preferência persistida.
- RT64 (`patches/rt64/rt64-p09-interp-burst-capture.patch`): `hle/rt64_interp_burst.h` (novo, inline; controlador + writer), `FrameMeta` coindexada com o slot de textura em `InterpolatedFrameCounters` (escrita sob `interpolatedMutex` antes de `available++`/`threadAdvanceWorkloadId`), captura/readback na thread de apresentação.
- Estágio salvo: `PRE_VI_RESOLVED_COLOR_TARGET` (cena do jogo; sem filtro VI/UI/desktop). Sem mudanças no Plume nem segundo passe VI.
- Aceita só: `generate && !paused && 0<curW<1 && prevFrame.matched && mappedWorkloads>0 && !forceCurrent && !forceEndpoint`, `meta.workloadId == present.workloadId`, sem duplicata `(workloadId,frameIndex)`. Classificação é de **frame**, não afirma que toda transformação foi interpolada (transformações não mapeadas voltam a CURRENT).
- Frame base não-MSAA (índice 0 no loop de apresentação) também é capturável; slots seguem o índice real do loop (frames pulados não deslocam a metadata).
- Readback: 1 buffer por captura (vive só na iteração), `rowWidth` alinhado a 256 B em texels, barreiras `COPY_SOURCE`→`SHADER_READ`, mapeado só após `presentGraphicsWorker->wait()`, repacote para linhas justas, **enfileirado só após `swapChain->present()` com sucesso**. Fila do writer limitada a 2; um writer persistente (sem detach, join no destrutor). Formato ≠ `R8G8B8A8_UNORM` (HDR): erro `UNSUPPORTED_FORMAT`, sem mis-encode. PNG RGB8 via miniz já compilado.
- Sem interpolação (`targetRate<=viOriginalRate`, CURRENT/30): termina no primeiro present com `NO_INTERPOLATION_ACTIVE` e as duas taxas, 0 PNG, sem pasta. Watchdog 10 s (`steady_clock`) roda na thread do writer, termina mesmo sem presents (pausa) e relata salvos/candidatos/motivos de rejeição.
- Logs `[tooie-interp-burst]` em stdout (início, pasta, cada PNG, fim/erro). Overlay de FPS **não** alterado (opcional, não feito).
- Burst perturba o tempo (readback síncrono): **não** é prova de cadência/FPS.

## Validação real

| Item | Resultado |
|---|---|
| `cmake --build build --target p09_interp_burst_test TooieRecompiled -j2` | exit 0 |
| `build/p09_interp_burst_test` | exit 0, `PASS` (classificação de todos os ramos, endpoint/forced/native/unmatched/mismatch, pitch/PNG round trip, XDG, debounce, 20 PNGs numerados + manifest + segunda pasta, timeout sem presents, timeout com rejeições, HDR, erro de escrita, destrutor com burst ativo) |
| Replay em clone limpo do pin `6f1c2d99` + 4 patches RT64 anteriores | `git apply --check`+apply exit 0; reverse-check ok; compilação isolada com as flags do build de `rt64_present_queue.cpp` e `rt64_workload_queue.cpp` ok (contrib/plume lido da árvore suja por symlink) |
| Reverse-check na árvore RT64 suja real | ok (patch representa o delta aplicado) |
| SHA256 do patch | `af9cd9d9f94718f8ca76f9606f50093317d9ac676ddaca07e44c5bc3bfd4767b` |

Contexto do workload: o bloco de metadata foi colocado após `threadRenderFrame` e usa só `curFrameWeight`/`prevFrameWeight` e `forceCurrentEnv()` (mesma semântica de `TOOIE_DIAG_FORCE_CURRENT_FRAME`), para não depender de `p09ForceCurrentNote*` (estado sujo não versionado). `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` é relido por getenv no header.

## Riscos e limites

- `present.workloadId == meta.workloadId` pode divergir em runs rápidos; vira `META_MISMATCH` (contado) e pode estourar o timeout com poucos PNGs.
- Coerência de memória do readback depende do VMA/Plume (mesmo padrão de `rt64_native_target.cpp`); `VulkanBuffer::map` só chama `vmaMapMemory`, READBACK usa HOST_ACCESS_RANDOM, sem garantia explícita HOST_COHERENT/invalidate. Prova local mostrou PNGs corretos na RX580/RADV; **portabilidade para memória não coerente não foi comprovada**. Nenhuma mudança global Plume foi feita.
- Plume/RT64 sujos continuam fora do patch; build real usa a árvore suja.
- Overlay de status não implementado.

## Critérios históricos planejados antes da prova

Instância isolada com `TOOIE_TEST_INTERPOLATION_FPS=60`, F8 na janela própria: esperado 1 pasta, 20 PNGs válidos, manifest `INTERPOLATED` com `0<curW<1`, `status.txt` `DONE`; F8 duplo = 1 pasta; sem opt-in (CURRENT/30) = `NO_INTERPOLATION_ACTIVE` imediato; pausar durante o burst = timeout/cancelamento.

Resultado atual é a prova limitada registrada acima; este parágrafo conserva os critérios planejados, não classifica os casos de run normal/pausa como executados em jogo.
