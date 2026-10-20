---
title: "P07 — Investigação do áudio tic tic"
status: active
type: evidence
phase: BT-07
updated: 2026-10-19
tags: [audio, investigation, p07]
---

# Evidência Técnica: Investigação e Correção do Glitch de Áudio ("Tic Tic") — TooieRecomp

- **Data**: 2026-10-19 | **Commit HEAD**: `5af4510744ddc90d25f9223ab48e9cff76f5baca` (código/docs não commitados; sem push; dirty tree preservado).
- **Sintoma Reportado**: Artefato auditivo intermitente ("tic tic") audível na introdução e demos (`USER_OBSERVED`).
- **Estado Geral**: FIX IMPLEMENTADO NO BUILD LOCAL PADRÃO e `USER_CONFIRMED_FOR_TESTED_INTRO_RECORDING`. O usuário confirmou explicitamente que o ruído "Desapareceu" na gravação com a correção da fila. NENHUMA alegação de correção global ou garantia para toda a campanha/hotplug/SFX.
- **Rastreabilidade**: REQ-AUD-05 parcialmente validado para a introdução; P07-T06 parcial inalterado; P07-T07 `UNTESTED/PENDING`. Sem `FULL PASS`.
- **Sessões e Ferramentas**: Modelos via Cursor CLI (`claude-sonnet-5-5-medium` e `grok-4.7-medium`) operados sob supervisão direta do root após limite de subagentes internos; Cursor sem erro de cota. Grok implementou diagnóstico delimitado (`3e86c773-6083-4e0e-8ed8-bd236383948c`) e corrigiu concorrência; root adicionou captura de retorno SDL/segurança de ciclo de vida e fix de uma linha no cálculo de fila. Sonnet revisou ciclo de vida (`82806a7b-ab7a-4e9d-ab48-d1b8d0a64e22`), validando eliminação de corridas via SC handshake, slots imutáveis e unit tests (`tooie_audio_capture_test` exit 0). Gemini 3.8 Flash high via agy restrito a documentação.

---

## 1. Fatos Estáticos e Correção de Produção no Host

1. **Pipeline de Áudio do Host**:
   - `src/main/main.cpp (queue_samples/reset_audio)` implementa streaming via `SDL_QueueAudio` (linha 438) com `callback = nullptr` (linha 517). Conversão de PCM guest 16-bit estéreo para float32 estéreo 48.000 Hz.
   - Guest opera com taxa dinâmica via `osAiSetFrequency` (`0x5622` = 22.050 Hz em `func_8001268C.s:48`; `0x17D0` é estritamente `text_size` do microcódigo `n_aspMain`).
2. **Fix de Produção Permanente (Default Ativo)**:
   - Em `src/main/main.cpp`, o cálculo do tempo de fila dividia incorretamente os bytes float32 host enfileirados pela taxa de amostragem de entrada do guest:
     - *Fórmula Corrigida*: `cur_queued_microseconds = uint64_t(sdl_queued_bytes_before) / (output_channels * sizeof(float)) * 1000000 / output_sample_rate;`
     - Elimina a superestimação de 2.17x (46–59 ms reais eram calculados como 100–128 ms, ultrapassando o limiar de 100 ms e acionando decimação espúria).
     - Fix sempre ativo por padrão; sem flag de ativação; sem alteração de FPS, velocidade, RSP ou lógica de jogo.
3. **Diagnóstico Integrado (Default OFF)**:
   - Ativação estrita via `TOOIE_AUDIO_CAPTURE_DIR` (caminho absoluto obrigatório fora do repositório) e opcional `TOOIE_AUDIO_CAPTURE_SECONDS` (10..20s, default 12s; limites de 2400 chunks / 24 MiB / 256 KiB por região).
   - Hot path com apenas uma verificação atômica quando desligado; cópias pré-alocadas e flush via worker assíncrono após o término da janela. Dados capturados: guest s16 (ordem de memória), F32 pré-skip, F32 pós-skip enviados ao SDL e JSONL de chunks com `sdl_queue_result`.

---

## 2. Histórico de Achados Offline e Comparativos (Preservados)

- **Comparativos BK e DK64**: Concluídos em `sonnet-bk-addendum.txt` e `grok-dk64-audio-comparison.txt`. BK compartilha o algoritmo de fila do host sem correção direta portável; DK64 trata temporizadores e mensagens (`timing_fixes.c`), sem transferibilidade demonstrada para o Tooie.
- **Harness Offline (`resample_boundary_probe.py`)**: Script SHA256 `4f4e75f24504d51c515850caaff8d7be9b6bacebd71766976ae5d061b4c89d1f`, saída `5e74c254bc8aa51ce2f4caa97a6ba4e176d7a483f1ab8907bad5578ad19b7f7a`. Demonstrou descontinuidade sintética (ratio 3.57x) na borda de chunks SDL_AudioCVT. Registrado como artefato sintético mensurável, sem necessidade de alterar esse caminho para resolver o trecho testado; contribuição perceptível adicional UNKNOWN.
- **Vieses Intencionais de Fila**: Subtração de 1 VI em `get_frames_remaining` e desconto de bytes em `ultramodern::get_remaining_audio_bytes` mantidos como colchões intencionais do host.

---

## 3. Matriz de Hipóteses Atualizada e Arbitragens

| ID | Hipótese | Fundamentação / Análise Técnica | Status Atual |
| :--- | :--- | :--- | :--- |
| **H1** | Preempção / Thread 4 | Sem mudanças nesta rodada; não foi isolada como causa adicional. | `UNKNOWN` |
| **H2** | Descarte espúrio por bug de unidades | Divisão por taxa guest induzia skips espúrios quando fila atingia ~46 ms. | `CONFIRMED ROOT CAUSE FOR INTRO TIC` |
| **H3** | Vieses AI / Colchão | Vieses subtrativos intencionais do host; preservados. | `INTENTIONAL / PRESERVED` |
| **H4** | Resampler Boundary / Split | Descontinuidade sintética no caminho segmentado que usa SDL; contribuição perceptível adicional UNKNOWN. | `PROVEN (OFFLINE); AUDIBILITY UNKNOWN` |
| **H5** | Roteamento Host / Bluetooth | PipeWire/Pulse 48k A2DP; monitoramento de sink input validou integridade do stream. | `CONTEXTUAL` |

- **Arbitragens Técnicas**:
  - *Grok Review Final (`5a32d97e-5388-424b-b589-471435e6a552`)*: Dividiu incorretamente a fila real de 46..59 ms novamente para 21..27 ms e descreveu o fix como flag de 20s. REJEITADO/SUPERSEDED pela matemática e código reais.
  - *Tese de Inaudibilidade do Sonnet*: Rejeitada; a confirmação auditiva direta do usuário sobrepõe suposições teóricas.

---

## 4. Resultados Runtime Práticos (`~/.cache/tooie-audio-investigation/practical/`)

1. `baseline-20261019T195956Z`: Falha antes da gravação por campo JSON do Pulse monitor; 0 bytes gravados.
2. `baseline-20261019T200024Z` (Binário limpo original `ffe3d7a52e2e4b97d159e79fb156ab27e3a909dfa73a44633f7ff15b8bfb46d4`):
   - Execução de 35.09s; stream capturado de 32.9387s F32 stereo 48 kHz (`intro-reference.wav`). Usuário confirmou presença do ruído típico ("tic tic"). Pulse ERR 0 em 35 observações. Encerramento exit 0 pós-harness.
3. `baseline-20261019T233829Z` (Smoke OFF binário `a7e0c35114668ce1dc649c83afa13a1d74837aadb2f1c7df54c6b7df859e4bde`):
   - Inicialização concorrente com refresh do Graft perturbou timing; encerrado com SIGKILL após timeout de SIGTERM (5s).
4. `diagnostic-20261019T234211Z` (Pré-fix, binário `66459ae289a54ae6ebd86d2fa2713c5148e6587257ffc8db59ef87191d878f8f`):
   - Janela de 20s, 581 chunks (input 22.050 Hz / output 48.000 Hz). Skips espúrios: `skip=1` em 47 chunks; `skip=0` em 534. Todos retornos SDL foram 0.
   - Fila real nos chunks com skip: 46.2917..59.2083 ms (falsamente estimada pela fórmula antiga em 100.77..128.89 ms).
   - Descartes: 40.454 frames removidos (0.8428s); pré 7.629.432 bytes vs pós 7.305.800 bytes. 71 leituras de fila zero antes do enqueue; 14 chunks pós não alinhados a frames estéreo aceitos pelo SDL.
5. `diagnostic-20261019T234439Z` (Pós-fix, binário `1ab4d2cea2c70d028529e82ecb01892871fe196f292b62ad15060cc049f9963c`):
   - Janela de 20s, 579 chunks. `skip=0` em TODOS os 579 chunks; zero descartes; todos retornos SDL foram 0; zero desalinhamentos.
   - SHA256 pré e pós idênticos: `36110c17de770f3dec29892d18ca0d9ace8f222589fd060b7b221701c8c33b33`.
   - 19 snapshots de fila zero (não underruns); áudio pré/pós 19.284333s vs guest 19.284535s. Lacunas de até 358 ms nos primeiros 3s explicam diferença para wall clock (19.998662s).
   - **Validação do Usuário**: Usuário escutou `intro-queue-time-fix.wav` (gravação Pulse de 33s) e atestou: "Desapareceu".
6. `baseline-20261019T234751Z` (Final OFF com mesmo binário final):
   - Execução de 35.125s, stream de 32.9387s, sem criação de diretório de captura, peak F32 finito 0.3440, Pulse ERR 0.

---

## 5. Residuais Técnicos e Limitações Conhecidas

- **Decimação em Overflows Legítimos**: Em sobrecargas reais da fila (>100 ms), o caminho de decimação por subamostragem e potenciais desalinhamentos de frame permanecem inalterados no código; não foram exercitados após a correção da fórmula.
- **Descontinuidade Sintética SDL**: Pequenos saltos de segunda diferença na junção de buffers convertidos pelo SDL continuam mensuráveis, mas não constituem a causa do artefato resolvido.
- **Encerramento de Processo**: As três últimas execuções de teste foram paralisadas pelo harness via SIGTERM e SIGKILL após 5s (o worker de diagnóstico já havia efetuado o flush aos 20s). Não foram crashes espontâneos, mas encerramento gracioso não foi validado.

---

## 6. Próxima Ação Singular Concreta (`NEXT_SINGLE_ACTION`)

- **Ação**: Realizar sessão focada de QA auditivo em gameplay ativo (músicas, efeitos de passos, diálogos/vozes, pausas e transições de tela) executando o binário padrão `./build/TooieRecompiled`, sem diagnósticos ativos e sem assumir a conclusão do soak P07-T07.
- **Status**: Pronto para execução. Nenhuma verificação adicional é requerida para a gravação da introdução já confirmada pelo usuário.

---

## 7. Validação Documental e Build

- `cmake --build build --target tooie_audio_capture_test -j2` exit 0; `./build/tooie_audio_capture_test` exit 0 (PASS `/tmp/tooie-audio-cap-46cQjl`).
- `cmake --build build --target TooieRecompiled -j2` exit 0 (`audio-queue-time-fix-build.log`).
- `git diff --check`: exit 0 (árvore de trabalho limpa de erros de formatação; arquivos de log/dumps mantidos fora do git).
- Graft atualizado e jCodeMunch reindexado. `markdown_vault_audit`: 292 arquivos, 0 erros e 61 avisos históricos; `git diff --check` exit 0.
- Snapshot local dos cinco arquivos de código/build/teste: `/home/wesley/.cache/tooie-audio-investigation/audio-capture-and-queue-fix-source.tar`, SHA256 `2a96d7a7ed45dd37b8923ecf536ada32c862d01f0925412d22567c9df3ca2450`.
- Diff rastreado local: `/home/wesley/.cache/tooie-audio-investigation/audio-capture-tracked.diff`, SHA256 `868d8cc0d40730ee36ed88c6688d75378580e40f89c0b4e3c2cf0ff453209619`. Arquivos gerados e dependências não foram alterados; sem commit/push nesta rodada.
