---
title: "Handoff — TooieRecomp"
description: "P08 correção de bordas em widescreen (16:9) implementada via planos de culling nativos no volume de visão (ws=0 em todos os kinds, attract preservado e gameplay validado pelo usuário). P09 causa raiz dos glitches de interpolação provada no pareamento WORLD do RT64 (-94% de estiramento). P11 build nativo Windows 11 x64, testes e launcher comprovados no host de dev."
status: current
type: handoff
project: TooieRecomp
updated: 2026-10-23
tags: [tooie-recomp, handoff, p08, p09, p10, p11, packaging, release, clean-clone, windows, linux, kitfree, restored-ui, n64recomp, rt64, interpolation, settings, menu, candidate-package]
---
# Handoff — TooieRecomp

## CURRENT — Desempenho: travas na troca de fase e logs sempre ligados (2026-10-23)

Ver [[plan/evidence/perf-overlay-dedupe-and-logs-20261023]].
- **Sintoma (`USER_OBSERVED`)**: Jogo pesado; carregar a Demo 5 (fase de gelo, mapa 296) demora e trava.
- **Causa raiz comprovada (`PROVEN`)**: `activate_canonical_mapping` chama `load_overlays` repetidamente no dispatch de syscalls; `load_overlay` acumulava seções em `loaded_sections` sem deduplicar, e `unload_overlays` apagava linearmente $O(n)$ do meio do vetor, causando travamento quadrático crescente nas trocas de mapa (até 91 s no attract).
- **Correção no runtime**: Patch local `vendor/patches-local/n64modernruntime-overlay-section-dedupe.patch` deduplica registros `(ram, secao)`. VENDOR VERIFY OK (29 patches). Travas caíram de 110,8 s para 2,8 s no attract de 7 min.
- **Saneamento de logs síncronos**: Auditoria estática localizou logs e I/O bloqueante sempre ativos (`USER_DEMO_5 MAP=296` com `fprintf`+`fflush` a cada chamada de câmera, logs por overlay, `getenv` repetitivo em `transform_hooks.cpp`, CAS atômico em `ensure_init`). Confinados sob `TOOIE_DIAG_UD5=1` e `TOOIE_DIAG_OVERLAY_LOG=1` (default OFF), getenv em cache e fast path. Linhas de log no attract caíram de 83.264 para 533; FPS mediano subiu de 56 para 72.
- **Testes**: Suíte required 9/9, extra 17/17 PASS.
- Commit e push autorizados pelo usuário (2026-10-23). Linux `NOT_RUN`.
- `NEXT_SINGLE_ACTION`: validação Linux (usuário) das correções P08/P09/desempenho; resíduos de baixo custo: ~70 wrappers `TOOIE_UD4_WRAP` de investigação encerrada e matching de frames do RT64 com Interpolation OFF.

## CURRENT — P09 glitches raros de interpolação: guardas novas no RT64 (2026-10-23)

Ver [[plan/evidence/p09-interp-glitch-guards-20261023]].

Patch local `vendor/patches-local/rt64-p09-interp-glitch-guards.patch` (VENDOR VERIFY OK, 28 patches). Todas default ON, cada uma com `=0`:
- `TOOIE_TEST_P09_ALIGN_MATCH`: grupos de hash com contagem diferente entre frames são alinhados em ordem com lacunas (antes caíam no matcher guloso).
- `TOOIE_TEST_P09_CAMERA_CUT`: câmera AUTO que gira > 30° ou salta > 250 unidades entre frames do jogo é corte; a cena não é interpolada nesse frame (antes o RT64 sempre interpolava a câmera AUTO).
- `TOOIE_TEST_P09_SEGMENT_COHERENCE`: osso travado pelo gate AUTO com vizinhos (≤ 80 u) interpolados volta a interpolar (membros de modelos segmentados).
- `TOOIE_TEST_P09_JOINT_COHERENCE`: hierarquia inferida (origem do filho fixa no sistema do pai); a origem do filho é presa ao pai interpolado.
- `TOOIE_TEST_P09_SPIKE_GUARD`: objeto (triângulos + juntas) com aresta interpolada > 2× a atual é mostrado em CURRENT nesse frame.
- Guarda de fechamento antiga agora default OFF (`TOOIE_TEST_P09_CLOSURE_GUARD=1` religa): encolhia peças e abria juntas.
- Diagnósticos default-OFF: `TOOIE_DIAG_P09_PAIR_AUDIT=<arquivo>`, `TOOIE_DIAG_P09_CAMERA_CUT=1`, `TOOIE_DIAG_P09_JOINT=1`; o classificador de rods agora emite `JG`/`JGW` (aberturas de junta).
- Medição attract (wl 1000–2400): triângulos esticados 378 → 0; juntas abertas 202 (>3 u) → 0; 16 cortes de câmera tratados (`PROVEN` métrico). Usuário jogando: "Ficou muito bom" (`USER_OBSERVED`, 2026-10-23). Testes: required 9/9, extra 17/17 PASS; `p09_ordered_match_test` 41/41; `p09_world_interpolation_guard_test` 35/35. Linux `NOT_RUN`.
- `NEXT_SINGLE_ACTION`: ver seção de desempenho acima.

## CURRENT — P08 bordas em widescreen: planos de culling nativos (2026-10-22)

Ver [[plan/evidence/p08-widescreen-native-planes-20261022]].
- **Sintoma e relato do usuário (`USER_OBSERVED`)**: Em renderização 16:9, itens, portas, Jinjos e paredes sumiam nas bordas laterais da tela (observado no Gameplay Demo 2 e Terrydactyland).
- **Mapa estático e causa raiz (`PROVEN`)**: Ponteiro global do volume de visão `0x8012D500` (espelho de `0x80127680 + 12 * idx`). Apenas 41 acessores leem esse ponteiro. Os vetores horizontais eram derivados do aspecto 4:3 de `+0xE8` em `0x800CA1B0`/`0x800CA218`/`0x800CA2A8`, e `0x800CAF34` montava os 4 planos $\{n, d\}$ a cada frame sem ler `+0xE8`. O renderizador de modelos `0x800DE498` (kind 13) causava 1323 rejeições indevidas de widescreen (`ws`) em modelos distintos.
- **Tentativa 1 refutada (`REFUTED`)**: Forçar desenho em `0x800DE498` restaurando `0x8012C940`/v0 causava divergência de simulação com callbacks ativos; com callbacks nulos mantinha determinismo mas causava atores congelados na borda, itens/Jinjos visíveis através de paredes e escotilha de Jamjars incorreta (`USER_OBSERVED`). Mantido como experimento default-OFF `TOOIE_TEST_WS_MODEL_DRAW_ASPECT=1`.
- **Solução por planos nativos (`PROVEN` / produção default ON)**: Durante a execução de `0x800CA1B0`/`0x800CA218`/`0x800CA2A8`, o aspecto em `+0xE8` é temporariamente substituído por `aspecto * (alvo/origem) * margem_lateral` (default 1.15) e restaurado logo após; a projeção `0x800CB124` permanece em 4:3 para o RT64. Todos os 41 acessores e testes do jogo operam em 16:9 nativo sem anamorfismo; expansões por teste (`0x800CAA24`/`0x800CACEC`) passam a $k=1$. O censo de rejeições ws caiu para 0 em todos os tipos. Desativável via `TOOIE_TEST_WS_NATIVE_PLANES=0`.
- **Proteção do attract demo (`PROVEN` métrico)**: Sob `0x80127632 == 5`, os planos permanecem em 4:3 para evitar dessincronização da gravação original de N64 (que divergia na posição 1348); caminho de 1603 posições do jogador na sonda de simulação é 100% idêntico ao original.
- **Validação interativa e testes (`USER_OBSERVED` / `PROVEN`)**: Usuário confirmou em gameplay: "Parece tudo correto agora". Suíte completa `Build-Tooie.ps1 -Stage Test -AllTests`: 9/9 obrigatórios + 17/17 extras PASS (5 skipped por desenho; novo teste `p08_ws_frustum_test` 68/68 PASS); executável SHA256 `0D44B01E52223B9579C19852CEA06EEEC7457913F14F33DA1A5DC63292C72B02`.
- `NEXT_SINGLE_ACTION`: investigar o glitch de interpolação em cutscene relatado pelo usuário (close de Banjo/Kazooie, escopo P09), após commit autorizado desta correção.

## CURRENT — P09 causa raiz dos glitches de interpolação e correção ordered match (2026-10-22)

Ver [[plan/evidence/p09-ordered-transform-match-20261022]].
- **Causa raiz dos glitches de interpolação (`PROVEN`)**: Banjo-Tooie não usa gEX (todas WORLD são `G_EX_ID_AUTO`). No RT64 (`GameFrame::matchScene`), o pareamento guloso por custo `computeTransformMatch` (origem + predição `prevPos + linearVelocity`) não distingue ossos com pivô compartilhado (ex.: penas da crista da Kazooie) ou instâncias da mesma malha. A velocidade herdada corrompe os frames seguintes em cascata (em 794 pares errados graves, 355 seriam corretos sem predição). O osso interpolado entre dois ossos diferentes gera "rods" e espigões. Guardas anteriores falharam porque usavam `max(aresta PREV, aresta CURRENT)`, mascarando o defeito quando PREV já vinha de par errado.
- **Ferramenta de diagnóstico**: `TOOIE_DIAG_P09_ROD_CLASSIFY=<arquivo>` (`lib/rt64/src/render/rt64_p09_rod_classify.h`, default-OFF) classifica triângulos com aresta renderizada $> 2\times$ CURRENT e $> 4$ unidades em categorias I, U, P, C, N. Na medição base (attract demo, wl 1000–2400, ~1320 frames), 94% dos triângulos esticados (6268 de 6640) eram P (inconsistência PREV vs CURRENT decorrente de pareamento incorreto).
- **Alternativas refutadas (`REFUTED`)**: Custo por geometria de vértices piorou $4\times$ (24215 esticados; ossos com 3–4 vértices encaixam geometricamente sob matrizes alheias); buffer RDRAM não é único (compartilhado por instâncias sob matrizes distintas em 720 de 1073 casos).
- **Correção no owning layer RT64**: `lib/rt64/src/hle/rt64_p09_ordered_match.h` introduz identidade estrutural: para cada hash de draw call com contagem idêntica nos dois frames, a $k$-ésima chamada atual pareia com a $k$-ésima anterior ("ordered pairs"), priorizado via `std::stable_partition` antes do fallback de custo original. Rejeição de salto de volume $\ge 8\times$ (2× linear, `scaleCompatible`). Padrão ON; `TOOIE_TEST_P09_ORDERED_MATCH=0` restaura pareamento original.
- **Resultados (`PROVEN` métrico / `STRONG_INFERENCE` visual)**: Triângulos esticados caíram de 6640 para 378 (−94%; P=338, U=28, N=12, I=0). Auditoria visual cega (240 capturas anonimizadas, 4 agentes Sonnet 5.5): na base, 1 espigão laranja grande (sev 3, wl 1698) + 3 incertos; na correção, 0 claros e no mesmo instante (wl 1699) o espigão não existe.
- **Guardas mantidas**: Desligar guardas (`TOOIE_TEST_P09_TRANSLATION_COHERENCE=0 TOOIE_TEST_P09_CLOSURE_GUARD=0 TOOIE_TEST_P09_GATE_SPEED_ONLY=0`) eleva esticados para N=7939; o gate AUTO ainda desliga translação em ~17% das WORLD.
- **Testes e integridade**: Testes host `tests/p09_ordered_match_test.cpp` 26/26 PASS, guardas 35/35 PASS; `Build-Tooie.ps1 -Stage Test -AllTests` 9/9 obrigatórios + 16/16 extras PASS; `vendor_verify.sh --reverse-check` VENDOR VERIFY OK (27 patches); exe Windows SHA256 `6F39CC23FA3DBC8969BC77F0917823F765063495197C593ED62DE32132855DE4`.
- **Status do Gate e Resíduos**: Gate técnico formal P09 permanece aberto (não fechado). Restam P=338 triângulos esticados (possível "pop" de osso do próprio jogo como `HYPOTHESIS`); hash com contagem divergente cai no fallback; captura burst no Windows/D3D12 encerrou processo logo após pedido (1×, `UNKNOWN`); validação de gameplay pelo usuário pendente; validação Linux pendente; nada commitado.
- **Teste de gameplay pelo usuário (`USER_OBSERVED`, 2026-10-22)**: jogando com Interpolation ON neste build, o usuário relatou que os rods/espigões "sumiram ou quase sumiram". Gate técnico formal P09 continua não fechado; resíduos acima seguem registrados.
- **Commit/push e Linux (2026-10-22)**: correção commitada e enviada (`6685b48` em `origin/main`). Linux: análise de código apenas (sem build/teste) — a causa e a correção estão em código RT64 comum (`GameFrame::matchScene`, header `rt64_p09_ordered_match.h`), C++ padrão sem `#ifdef` de plataforma e independente da API gráfica; o efeito deve valer no Linux (`STRONG_INFERENCE`). Compilação GCC/Clang, `p09_ordered_match_test` e efeito visual no Linux: `NOT_RUN` (usuário valida no PC Linux).
- `NEXT_SINGLE_ACTION`: ver seção P08 acima.

## CURRENT — P11 build nativo Windows comprovado no host de dev (2026-10-22)

- **Commits e push**: Etapas W0 a W4 integradas e enviadas a `origin/main` (commits `6862e29`..`f22e867`, HEAD `f22e867`).
- **Novo status Windows**: `WINDOWS_NATIVE_BUILD_PROVEN_DEV_HOST`. Pipeline nativo completo comprovado em Windows 11 x64 sem WSL via `tools\windows\Build-Tooie.ps1 -Stage All` (Check → Extract → Recomp → Configure → Build → Test → Package).
- **Geração RecompiledFuncs**: Geração nativa de 420/420 arquivos byte-idêntica ao baseline Linux (`metadata/recomp-output.sha256`) via `N64Recomp.exe` com escrita binária (`n64recomp-binary-output.patch`) e miniz vendorizado para extração (`tools/extract_rom.py`).
- **Executável nativo**: `build-win\TooieRecompiled.exe` (23.944.704 bytes) linkado com `clang-cl` 20.1.8 + `lld-link` (Visual Studio 2026/2022 Community), com DLLs adjacentes (`SDL2.dll`, `dxil.dll`, `dxcompiler.dll`). Resolução de símbolos fracos em COFF via `RECOMP_FUNC` (`__attribute__((weak, noinline))`) e envelopamento `/wrap:` unificado no `CMakeLists.txt` (`TOOIE_WRAPPED_SYMBOLS`).
- **Launcher e ROM de cartucho**: O launcher aceita diretamente a ROM comercial de cartucho (`.z64`/`.n64`) com descompressão transparente em memória via `tooie_cart_decompress` (miniz + fmt 11.1.0) além da imagem `decompressed.us.z64`. Abertura do launcher e boot do jogo com a ROM de cartucho confirmados pelo usuário (`PROVEN`).
- **Testes de host Windows**: 9/9 testes obrigatórios PASS, 15/15 extras PASS (`-AllTests`), 5 alvos de driver com SKIP documentado por desenho. Pilha de 8 MiB configurada para `p09_exact_use_correlation_test` (1355/1355 passed).
- **Pacote e smoke test**: Estágio Package gerou `dist\TooieRecompiled-windows-x64-<HEAD>[-dirty]-private-local.zip` (+ `.sha256`) com paridade ao Linux (106 arquivos no manifesto, `AUDIT.txt` PASS, zero ROM/ELF); smoke test de 15s PASS (`tools/smoke_package.py`).
- **Documentação pública e specs**: README (§1.3, §1.5, §2), `tools/BUILDING.md`, `tools/windows/README.md` (novo), specs e planos atualizados com o status comprovado do build nativo; `ImportGenerated` mantido como fallback legado.
- **Pendências registradas (não aprovadas / pendentes)**:
  - Validação no Linux das alterações compartilhadas de código e CMake (o usuário fará no PC Linux em paralelo);
  - Confirmação auditiva da música do launcher no Windows (`PENDING_USER`);
  - Validação manual interativa completa W5 no Windows (D3D12/Vulkan, áudio WASAPI, controle SDL, persistência de `graphics.json`);
  - CI do GitHub Actions não disparado (`NOT_RUN`);
  - Smoke test do pacote no Windows é não-isolado (configuração persistida em `%LOCALAPPDATA%\TooieRecompiled`).
- `NEXT_SINGLE_ACTION`: ver seção P09 acima.

## ANTERIOR (2026-10-21) — Geração 100% Kit-Free provada e Restauração Integral da UI autorizada pelo usuário (2026-10-21)

Ver [[plan/evidence/p10-kitfree-generation-and-ui-restoration-20261021]].
- **Contrato Kit-Free Validado (`PROVEN`)**:
  - O pipeline de build padrão do Linux é **100% livre do compilador legado**: `tools/build_linux.sh` executa por padrão os estágios `preflight verify extract recomp configure build test package`.
  - Zero IDO 5.3, zero MIPS GCC 2.7.2, zero MIPS binutils, zero binários ELF, zero rede e zero checkouts externos no build normal.
  - Prova executada por Sonnet em sandbox estrita bubblewrap (`tools/offline_sandbox.sh`, sem rede, `/home` vazio, sem irmãos, árvore extraída de tarball fonte limpo sem `.git`, único input = cópia da ROM do usuário):
    - `extract` + `recomp` (90s): gerou os 420 arquivos em `RecompiledFuncs/`, **todos os 420 idênticos byte a byte à linha de base da rota ELF** (`new_gen.sha256 == baseline_gen.sha256`; `diff -rq` vazio). Sem `.elf` e sem `decomp-work` na árvore.
    - `tools/run_rsprecomp.sh`: microcode `rsp/n_aspMain.cpp` idêntico byte a byte ao rastreado no git (`31d946c6...`).
    - `configure` + `build` + `test`: 1159/1159 passos de compilação Clang/Ninja exit 0; 8 testes unitários host executados com PASS.
    - Smoke test em ambiente isolado (20s cada): processo vivo sem ROM; processo vivo com ROM descompactada e `TOOIE_AUTO_START=1` registrando "TOOIE_AUTO_START: starting game".
    - Testes negativos: ausência ou zeração de ROM descompactada falha com exit 1 e deixa `RecompiledFuncs` intocado (420 antes e depois).
    - `tests/n64recomp_zero_size_static_test.sh`: PASS (quirk de `.cosection`/`.dbpalette` contornado sem perda da numeração `static_344`).
  - Causa raiz da falha da rota antiga de símbolos e correção no owning layer: N64Recomp upstream perdia a flag relocatable em seções sem relocação, a numeração de seções ELF, a ordem de emissão de funções, 74.613 de 104.964 entradas de relocação de runtime (seções cruzadas e R_MIPS_32) e o nome da ROM. Corrigido via `vendor/patches-local/n64recomp-symbols-file-fidelity.patch` (19 patches aplicados e verificados com `--reverse-check`). Metadados `metadata/tooie.us.syms.toml` (10,8 MB) regenerados via `generate_metadata.py --dump-context` e manifesto de saída `metadata/recomp-output.sha256` adicionado para verificação automatizada (`tools/verify_generated.sh`).
  - Disciplina epistêmica: A igualdade de saída C prova a fidelidade dos metadados e a equivalência do gerador; ela **não** prova por si só que o payload `.text` do ELF é idêntico a fatias da ROM (já que o gerador lê palavras da ROM em ambas as rotas). A falha do teste inicial de símbolos decorreu de aborto parcial (falta da flag relocatable em `.cosection`), não de defasagem de idade de metadados. Sonnet corrige relatório de investigação privada.
- **Restauração Integral da Interface de Usuário (User Clarification D1 & D2)**:
  - O usuário esclareceu formalmente que a restrição de recursos livres/próprios se aplica **exclusivamente aos dados de jogo da ROM** (texturas, áudio, geometria, scripts). Todos os assets de menu, artes, fontes tipográficas, animações completas e o logo Tooie previamente concluídos devem ser preservados e mantidos localmente no repositório.
  - Root restaurou a partir de `candidate-backup` todos os 44 arquivos originais da interface: artes vetoriais (`Banjo.svg`, `Kazooie.svg`, `Cloud1-4.svg`, `JiggyColor/Hole/Shine.svg`, `Logo.svg`), ícones de menu e controle (`assets/icons/`), fontes tipográficas (`Suplexmentary Comic NC.ttf`, `InterVariable.ttf`, `NotoEmoji-Regular.ttf`, `promptfont.ttf`), stylesheet (`recomp.rcss`), animação (`src/main/launcher_animation.cpp`) e o logo Tooie original (`ui_assets/TooieLogo.svg` viewBox `0 0 7300 2450`).
  - A interface neutra interina (774x272 / 29 assets / Inter-only) e o binário gerado durante o teste neutro (`5c2c8df2...`) foram formalmente **SUPERSEDED**. O rebuild da UI restaurada foi concluído pelo root com sucesso (`root-restored-ui-build-result.json` exit 0): 43/43 arquivos de fonte/recursos e 40 recursos de UI idênticos ao backup de candidatos, binário SHA-256 `0cfcd79206efc74b74e26836ce468c69bcadaf2c3950b40eb31971946026892b` rigorosamente idêntico ao original pré-redesenho, e visual do launcher inspecionado (`launcher-restored-menu.png` / `root-restored-menu-visual.json`, captura do próprio PID com exit 0). O pacote Linux final foi gerado e verificado: `TooieRecompiled-linux-x86_64-73bfd43-dirty-private-local.tar.gz` (SHA-256 `4f702d2ba0a0472345f6f1317bfe75eb0df462e96b2352d6d875edaa5c4ebcf9`, 104 arquivos regulares, `tools/verify_package.sh` exit 0, UI exata, zero ROM). Preflight kit-free do root exit 0. A prova limpa kit-free (420 arquivos idênticos, 1159 passos, 8 testes PASS) utilizou o interim neutro 5c2 em sandbox isolada, separada da prova do rebuild da UI restaurada (sem fusão indevida de escopos).
  - Target gerenciado CMake `TooieUIAssets`: limpa e recopia `assets/` para `<build>/assets` em cada build do aplicativo (com guarda out-of-source), tratando incrementalidade e deleções de assets.
  - Honestidade de licenciamento: `Suplexmentary Comic NC.ttf` mantém proveniência (SMWCNC / Jayvee D. Enaguas) e status de licença desconhecida/não verificada preservados honestamente sem bloquear a restauração; avisos das fontes OFL 1.1 e controller DB zlib retidos. Zero dados de jogo no repositório.
- **Requisitos de ROM e Launcher**:
  - ROM de entrada: `Banjo-Tooie (USA).z64` na raiz (MD5 `40e98faa24ac3ebe1d25cb5e5ddf49e4`, tamanho `33554432` bytes, SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`, estritamente git-ignored).
  - Launcher exige estritamente a imagem descompactada: `build-linux/extract/decompressed.us.z64` (tamanho `35098224` bytes, SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, XXH3 `9F843AD3455EA395`, gerada por `tools/extract_rom.sh`). ROMs brutas de cartucho (XXH3 `00F70DE5F2D70EA2`) são rejeitadas.
- **Estágio Mantenedor (Separado do Build Normal)**:
  - Estágio `decomp` (`tools/build_linux.sh decomp`, `tools/preflight.sh decomp` e `tools/generate_metadata.py`) é estritamente **maintainer-only**, necessário somente para quem altera fontes de decompilação ou regenera `tooie.us.syms.toml`. Exige `TOOIE_TOOLCHAIN_KIT` (34 arquivos verificados contra `tools/toolchain.sha256`) e pacotes MIPS (`gcc-mips-linux-gnu`, `binutils-mips-linux-gnu`, `libtoml11-dev`, `python3-venv`).
- **Windows 11 x64**:
  - Status: `WINDOWS_BUILD_PENDING_NATIVE_HOST`. Nenhum executável Tooie PE/DLL existe nos artefatos. Geração local no WSL2 usa o pipeline kit-free (`tools/build_linux.sh preflight verify extract recomp`, sem IDO), importado no Windows com `.\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated`.
- **Invariantes de Glitch e Gameplay**:
  - Portão técnico P09 permanece **FAILED** (`passed: false`). P09 e P10 permanecem formalmente abertos.
  - Interpolation menu default é CURRENT 30 FPS (OFF). F8 screenshot hotkey permanece desativada.
- **Disciplina de Git e Entrega**:
  - Checkpoint autorizado pelo usuário (2026-10-21): o escopo standalone revisado é commitado e enviado a `origin/main`; o hash do commit e o recibo do push ficam fora do Git (diretório da tarefa), sem autorreferência. Estado dirty anterior (sem commit/push) é histórico. Arquivos locais de agentes, ROM, `RecompiledFuncs/`, `dist/` e `build-*` permanecem não versionados.
  - Arquivo-fonte final será exportado após finalização documental, com checksum gerado em sidecar externo adjacente (sem embedding de hash autorreferencial).
- `NEXT_SINGLE_ACTION`: Transferir o código-fonte final e o bundle privado de código gerado para máquina física Windows 11 x64 para validação do build nativo (Check/ImportGenerated/Build/Package).

## SUPERSEDED (2026-10-21) — projeto standalone: dependências do BanjoRecomp vendorizadas, build offline provado

Ver [[plan/evidence/p10-standalone-vendor-20261021]] (substitui o desenho de bootstrap externo da seção seguinte, mantida como histórico).
- **Estado**: `lib/` (rt64, N64ModernRuntime+N64Recomp, RecompFrontend, SlotMap, SDL2-windows), `assets/` (diretório real de UI; symlink removido; zero dados de jogo), `recompcontrollerdb.txt`, `decomp/` (só fontes) e `tools/wheelhouse/` estão no repositório como arquivos regulares; proveniência/pins/licenças/patches integrados em `vendor/` (`MANIFEST.sha256` cobre os arquivos vendorizados; patches reverse-applied via `tools/vendor_verify.sh --reverse-check`). `tools/deps.lock`, `bootstrap_deps.sh`, `apply_runtime_patches.sh` e `third_party/` foram aposentados. Sem checkouts de outros projetos. Pré-requisitos explícitos: ROM canônica `Banjo-Tooie (USA).z64` (tamanho `33554432` bytes, MD5 `40e98faa24ac3ebe1d25cb5e5ddf49e4`, SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`), imagem ROM descompactada (`35098224` bytes, SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, gerada por `tools/extract_rom.sh` e estritamente exigida pelo launcher), kit IDO/gcc (`TOOIE_TOOLCHAIN_KIT`, 34 arquivos em `tools/toolchain.sha256`), pacotes do SO.
- **Prova Linux offline (`PROVEN`)**: árvore candidata materializada em sandbox (`tools/offline_sandbox.sh`, bwrap, sem rede, HOME vazio, irmãos invisíveis): geração offline (420 arquivos `RecompiledFuncs`: 417 C + `lookup.cpp` + `funcs.h` + `recomp_overlays.inl`, idênticos byte a byte ao bundle `e7bb66d4` e à árvore de prova; RSP `n_aspMain.cpp` separado no git), build `-j2` (1158 passos, exit 0), 8/8 testes host PASS, pacote corrigido (`TooieRecompiled-linux-x86_64-nogit-private-local.tar.gz`, SHA-256 `b6475879582aba0fc8b2d3c5d7b39805f08912fb272c71b17c67a959b8d1fa74`, 101 arquivos regulares no tar (99 na auditoria), 100 no manifesto, F1 ldconfig resolvido para 0 avisos; supersedendo o inicial `64f130aa…` com 90 avisos). Binário `0cfcd79206efc74b74e26836ce468c69bcadaf2c3950b40eb31971946026892b`. Smoke isolado: processo vivo por 20s em X11/RADV sob loader padrão e sob override do sistema (`TOOIE_SMOKE_LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu`, `zlib1g 1:1.3.dfsg-3.1ubuntu2.2`) com envio de SIGTERM ao próprio PID (exit 0). Limitação: teste de liveness apenas; sem avaliação de gameplay ou renderização. Teste limpo de zlib segue pendente (`docs/plans/BT/10-linux-packaging-and-release.md:47`).
- **Arquivo-fonte de entrega:** `dist/standalone-20261021/TooieRecomp-source-73bfd430-dirty.tar.gz`, exportado após a revisão final dos guias, acompanhado de `.sha256`; inclui fontes locais e documentação revisada, sem ROM, `RecompiledFuncs/` ou configs pessoais de agentes. O hash `4aab8d21…` identifica apenas a prova anterior à documentação, preservada como histórico.
- **Windows (`WINDOWS_BUILD_PENDING_NATIVE_HOST`)**: **Nenhum executável Tooie (.exe ou .dll) do jogo existe** nos artefatos (binários de SDK como `dxc.exe` e DLLs SDL2 vendorizados sob `lib/` não contam como build do jogo). O configure WIN32 cross-host passou via alvos locais de SDL2, mas sondas de unidades de tradução (TUs) no Linux falharam em cabeçalhos de terceiros (`plume_d3d12.cpp`, `plume_vulkan.cpp`), de modo que não há alegação de compilação de todas as TUs (bloqueador B1 resolvido). Exige máquina nativa Windows 11 x64 com VS 2022 Build Tools (clang-cl) e Windows 11 SDK. Invocação PowerShell requer argumentos individuais por estágio no ValidateSet (`Check`, `ImportGenerated -GeneratedFrom <dir>`, `Build`, `Package`).
- **B2 arbitrado pela raiz (`CORRECTED/SUPERSEDED`)**: a preocupação inicial da revisão independente (`independent-review.initial.md` / `gate.initial.json`) foi arbitrada em `root-baseline-arbitration.md`: o diff completo de início de tarefa e o diff E0 compartilham o mesmo SHA-256 (`1b8f7b96f9d23a0629c43b8d4446b5a78c407d7e429833ebc08e1ab73ba7c462`); todos os 91 arquivos protegidos de `src/`, `include/`, `tests/` são idênticos byte a byte; todos os 21 corpos de patches antigos estão preservados sob `vendor/`. Não há bloqueador de confirmação de backup pelo dono.
- **P09/P10 inalterados**: gate de glitch `FAILED`; Interpolation OFF/`CURRENT30`; F8 desativado; diagnósticos de burst mantidos.
- **Decisões do dono abertas**: licença do projeto (nenhuma inventada); linhas `UNKNOWN`/`UNVERIFIED` em `vendor/UPSTREAMS.tsv` (RecompFrontend raiz, DXC, decomp/ultralib, fonte Suplexmentary, screenshots) bloqueiam redistribuição pública; compatibilidade com o GPL-3.0 do BanjoRecomp (40 arquivos de linhagem comum em `vendor/DERIVED-FILES.tsv`); commit do delta (120 entradas de status; nada foi staged/commitado/pushado). C3 corrigido: export sem Git informa contagem indisponível, não zero. Permissões POSIX são verificadas no Linux e explicitamente SKIPPED no MSYS; Python 3.8+ é pré-requisito do verificador no Windows.
- `NEXT_SINGLE_ACTION`: transferir o arquivo-fonte final e o bundle privado gerado da pasta `dist/standalone-20261021/` para Windows 11 x64, seguir `LEIA-ME.md` (VS/SDK + Git + Python) e registrar o resultado nativo de Check/ImportGenerated/Build/Package. O clone remoto só conterá este delta depois de publicação; nada foi commitado/enviado.

## SUPERSEDED (histórico; ver seção CURRENT acima) — prontidão para clone limpo Windows 11 x64 + Linux: auditoria, materialização, resolução de gate e verificação final (2026-10-21)

Ver [[plan/evidence/p10-windows-linux-clean-clone-readiness-20261021]]. Investigação forense multi-agente e resolução técnica de gate concluídas sobre por que o clone em outra máquina (Windows 11 x64) falha ou está incompleto. Diagnosticadas 6 causas raiz (dependência implícita dos irmãos `../BanjoRecomp` [superada subsequentemente pelo standalone vendoring em `lib/`, `decomp/`, `assets/`], deltas de features locais não commitados no `HEAD` remoto `73bfd43`, ausência de manifesto/bootstrap de dependências, CMake Linux-only, ausência de shims `--wrap` GNU ld no Windows, e symlink de assets).
- **Solução implementada**: manifesto de pins `tools/deps.lock` + script `tools/bootstrap_deps.sh` (fetch shallow por SHA em `third_party/`), shims `_WIN32` para `__real_*` em `backpack_lookup_diag.cpp`, guards `<unistd.h>`/`getpid()`, CMake branch `WIN32` com fetch SDL2 2.30.3 VC, script PowerShell `tools/windows/Build-Tooie.ps1`.
- **Prova de materialização limpa Linux (`LINUX_CANDIDATE_PACKAGE_VERIFIED`)**: árvore limpa em `/home/wesley/tooie-proof/tooie` (sem substring `.c` no caminho) executou bootstrap, 17 patches idempotentes (incluindo `rt64-p09-local-pair-snapshot-portable-paths.patch`), decompilação, recompilação (420 arquivos `RecompiledFuncs` idênticos byte a byte ao desenvolvedor: 417 `funcs_*.c` + `lookup.cpp` + `funcs.h` + `recomp_overlays.inl`), build `-j2` (1158 passos, exit 0), 8 testes de unidade host PASS (exit 0), e empacotamento (`TooieRecompiled-linux-x86_64-73bfd43-dirty-private-local.tar.gz`, SHA-256 final `a327666f8baff1ce1134803563d5eb4d5ad1c50edf3359e0a419de5a22a6996f`, 105 arquivos auditados, 31 MB; binário `24e47b02fdf2288d4f59f6bff9f6bda43422b4810b6220a9aee0a4f648a9c148` com 0 strings `/home/`; zero ROM/ELF). Verificação de integridade via `tools/verify_package.sh` PASS (106 arquivos no `MANIFEST.sha256`, zero ROM). Smoke tests isolados: processo vivo por 20s em X11/RADV sob loader padrão e sob override de biblioteca do sistema (`TOOIE_SMOKE_LD_LIBRARY_PATH=/lib/x86_64-linux-gnu`, mapeando `zlib1g 1:1.3.dfsg-3.1ubuntu2.2`) com saída limpa e `interpolation: false` (CURRENT 30 FPS).
- **Status Windows 11 x64 (`WINDOWS_BUILD_PENDING_NATIVE_HOST`)**: **Nenhum executável (.exe ou .dll) do jogo existe** em nenhum artefato. Sondas preliminares cross-host de `-fsyntax-only` haviam sido executadas na prova anterior, mas foram superadas: sondas cross-host subsequentes no Linux falharam em cabeçalhos de terceiros (`plume_d3d12.cpp`, `plume_vulkan.cpp`), de modo que não há alegação de compilação de todas as TUs (bloqueador B1 resolvido). O configure WIN32 passou, mas compilar 90 MB de C gerado e linkar requer máquina nativa Windows 11 x64 com VS 2022 Build Tools (clang-cl) e Windows 11 SDK (com aceitação da licença da MS pelo usuário). Todas as instruções para Windows permanecem classificadas como **não verificadas** até execução nativa.
- **Suplemento de entrada gerada (transferência direta privada)**: pacote `TooieRecomp-generated-private-local.tar.gz` (SHA-256 `e7bb66d40d6f6c499f6d5e5567bdd9cbad2bd3fffb9483900a5fc2d11e379680`, 420 arquivos) preparado para transferência direta privada. No Windows, `Build-Tooie.ps1 -Stage ImportGenerated -GeneratedFrom C:\path\RecompiledFuncs` dispensa o uso de WSL2. Arquivo fonte candidato `TooieRecomp-source-candidate-73bfd43-plus-delta.tar.gz` (SHA-256 `ea0606998192b17a4696ba80546436c002948b48e938e16eaf9a85772d617582`, 512 arquivos regulares, sem ROM/binários).
- **Política de Git CRLF**: proibida recomendação de configuração global; usar estritamente por clone `git -c core.autocrlf=false clone ...`.
- **Arbitragem e verificação final do gate técnico independente Haiku 5.5 (`final-gate.json`)**:
  - `linux.accepted: true` (apenas como pacote candidato dirty local privado, testado neste host Linux; não é release pública).
  - `windows.accepted: false` (`WINDOWS_BUILD_PENDING_NATIVE_HOST`).
  - `repositoryCompleteness.accepted: false` (`REPOSITORY_NOT_COMPLETE`, commit do delta é decisão do usuário).
  - B3 (libz): RESOLVIDO no mapeamento (`DEPENDENCIES.txt` mapeia `zlib1g` com nota `HOST-SHADOW`); smoke com zlib do sistema verificado PASS 20s.
  - B4 (paths pessoais): RESOLVIDO no binário (0 strings `/home/` via patch de snapshot portátil); ressalva documentada: configs de agentes no arquivo fonte candidate contêm caminhos locais do HEAD do git.
  - B5 (manifesto de licenças): MECANISMO RESOLVIDO (`THIRD-PARTY-MANIFEST.tsv` gerado automaticamente com 106 arquivos e 41 linhas UNKNOWN/UNVERIFIED; `package_licenses.tsv` falha se houver arquivo sem regra); decisões de licenciamento permanecem com o proprietário.
  - B7 (contagens): RECONCILIADO (46 arquivos no proof tree e BUILD-INFO; 38 no delta de materialização; 85 no working tree raiz).
- **Invariantes preservados**: toggle Graphics **Interpolation** (default OFF / 30 FPS / CURRENT) preservado; F8 screenshot hotkey permanece removida (mecanismos de diagnóstico RT64 subjacentes permanecem disponíveis via flags explícitas); portão técnico de interpolação P09 segue **FAILED**; fases P09 e P10 **NÃO estão fechadas**. Sem commit/push; modificações uncommitted no worktree preservadas.

`NEXT_SINGLE_ACTION`: Em um host Windows 11 x64 nativo com Visual Studio 2022 Build Tools (clang-cl, Windows 11 SDK) e Git for Windows instalados, clonar com CRLF por clone (`git -c core.autocrlf=false clone ...`), importar o bundle de código gerado (`TooieRecomp-generated-private-local.tar.gz`) via `.\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated -GeneratedFrom C:\path\RecompiledFuncs`, executar `.\tools\windows\Build-Tooie.ps1 -Stage Build` e `.\tools\windows\Build-Tooie.ps1 -Stage Package`, e registrar os códigos de saída, o SHA-256 do `TooieRecompiled.exe` e o smoke test de execução.

## Histórico (2026-10-21, anterior) — opção Interpolation no menu ESC + F8 removido (2026-10-21)

Ver [[plan/evidence/p09-menu-interpolation-toggle-20261021]]. Pedido explícito do usuário substitui a trava de 30 FPS: Graphics ganhou o toggle persistido **Interpolation** (`graphics.json` chave `interpolation`; ausente/inválido/legado = OFF = Manual 30 + CURRENT, caminho estável). ON libera Framerate (Manual 60 quando o valor salvo não geraria frames) e passa `endpointCurrent=0` ao RT64 em runtime (Apply, latch por workload, sem restart/setenv); Manual 120 salvo sobrevive ao boot. Novo patch `patches/rt64/rt64-p09-host-endpoint-current-toggle.patch` (SHA256 `b96e2b70…d8f15`, registrado em `tools/apply_runtime_patches.sh`). **F8 desativado**: hotkey, `tooie_interp_burst.cpp` e módulo `tooie_rsp_endpoint_current` removidos; infra de captura RT64 dormente; instruções “apertar F8” abaixo são históricas e `SUPERSEDED`. Provas: build `-j2` exit 0 (binário `3589d734…8ab2`), 9 testes host exit 0, jogo isolado ON→OFF→ON + relaunch persistido nos dois modos + F8 sem artefato (cena de título/attract, não save jogado). **Não** corrige glitch e **não** aprova o gate técnico: ligar ON expõe os glitches articulados abertos; padrão permanece OFF. Sem commit/push; worktree sujo preservado (RT64/N64ModernRuntime/RecompFrontend/plume só com o delta do patch novo).

`NEXT_SINGLE_ACTION`: usuário ligar Interpolation em gameplay real (ESC → Graphics → rolar → Interpolation → Apply) e relatar visualmente; só depois retomar os blockers B1–B7 do gate técnico abaixo.

## Histórico (2026-10-21, anterior) — continuação técnica pós-cota Grok concluída; gate técnico Haiku 5.5 PENDING / FAILED (2026-10-21)

Continuação técnica executada após esgotamento de saldo da API do Grok (`status 402 Payment Required: Grok Build usage balance exhausted`) registrada em `/home/wesley/.cache/tooie-glitch-doc-audit-and-fix/` (`continuation-status.json`, `technical-sonnet-report.md`, `technical-haiku-review.md`, `technical-gate.json`):
- **Registro de transição por cota do Grok e prova de build retida**:
  - Execução interrompida do Grok preservou a árvore limpa `rt64-clean-pin` e build serial `cleanpin-build` (log de 136 KB, exit 0, binário `cleanpin-build/TooieRecompiled` SHA256 `43e070abb80c5a9272d2945354c652578a09366d0ca7fbf9ec5c9b322ee4942f`).
  - Durante o primeiro burst de captura na árvore limpa (`cpon1`), ocorreu SIGSEGV na biblioteca plume (`plume::VulkanCommandList::copyTextureRegion` dereferenciando `dstTexture` nulo). Identificado que o código de `plume` upstream suportava apenas buffer->texture, e uma edição desversionada de 31 linhas existia no submódulo sujo de plume (`plume_vulkan.cpp`, mtime 2026-08-27) adicionando a ramificação texture->buffer (`vkCmdCopyImageToBuffer`).
  - Sonnet extraiu e versionou o patch `patches/rt64/rt64-plume-texture-to-buffer-copy.patch` (SHA256 `49c218efa703a8946d65652474b209934cd8ec96a55c2053b93826cf8d070852`, base `d890ac899e505fb30040e037a4037cdeca68f033`), registrando-o em `tools/apply_runtime_patches.sh`.
  - Recompilação do pin limpo (`cleanpin-build2.log`, exit 0) gerou o binário auditado final SHA256 `965bbabbc3938ac833fd11b0ccb8b198f27e2108f95ebb51c405f4b4924aec05`.
- **Diagnóstico do crash fatal `ong2` (SIGSEGV `rc=-11`)**:
  - Análise do coredump do PID 1659795 (`gdb -batch`): o thread hóspede falhou em `unload_overlays ← ovl_unload ← func_8008160C ← func_800123F4 ← func_800124EC` lendo nó de `unordered_map` já desalocado, enquanto o thread principal executava os destruidores estáticos de `~unordered_map` a partir de `exit()`.
  - Mecanismo (`PROVEN_SOURCE`): em `librecomp/src/recomp.cpp` (`recomp::start`), o loop `while(!exited){sleep(1ms); update_gfx}` aguarda `exited` (definido apenas em `ultramodern::quit()`) e junta apenas threads de início/evento/limpeza/salvamento; threads OSThread do hóspede não são parados antes do retorno de `main()`. É uma condição de corrida no encerramento do processo, independente das guardas de interpolação.
  - Mitigação de harness aplicada em `run_ab.py` e `play.py` (SIGTERM → espera 30s → SIGABRT → SIGKILL com `wait()`). O produto em si mantém a condição de corrida em aberto (correção pertence ao `N64ModernRuntime`, fora de escopo).
- **Métricas GPU e controles contrafactuais reproduzidos**:
  - `tools/vover.py` re-executado sobre os logs de auditoria do binário de pin limpo: na title scene, overshoot de vértice caiu de 38.115 (OFF) para 3.178 (ON), e triângulos esticados cruzando WORLD caíram de 19.374 para 0.
  - No attract loop completo (workloads até 14438), overshoot caiu de ~210k cumulativo para ~20k.
  - Scripted input com `tools/play.py` navegou até o New Game intro cobrindo atores não-jogadores (Klungo wl 2600-3000, Mumbo e Bottles wl 3200-3900).
- **Veredito do gate técnico independente Haiku 5.5 (`technical-gate.json`): `passed: false`**:
  - **B1 (Critério de estiramento circular)**: `crossStretchedTris = 0` avalia o próprio critério de recuo elementwise da guarda (`rt64-p09-world-interpolation-guard.patch` `auditTriangles`); como o lerp elementwise é convexo, matrizes chaveadas não esticam por construção. A métrica não pode ser citada como prova independente da correção.
  - **B2 (Mislabeled default path / escopo do fix)**: os scripts de teste (`run_ab.py:31` e `play.py:26`) configuram `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0`. No Tooie de produção, o padrão é definir `"1"` (`include/tooie_diag.h:102-104`). No caminho padrão de produção, a guarda de interpolação permanece **inerte**. Logo, o fix afeta unicamente o modo opt-in 60 FPS com FORCE=0; o run `nat1` foi incorretamente rotulado como "default path" porque também usou FORCE=0.
  - **B3 (Prova visual não-jogador ausente)**: sem par GPU casado com fase alinhada para atores não-jogadores (Klungo, Mumbo/Bottles). Linhas de auditoria não carregam identidade de objeto. O streak alaranjado de penas do Mumbo em `hA_3593.png` aparece em OFF, ON e `nat1`, sendo geometria de fonte da pose (`HYPOTHESIS`), sem prova visual de correção para não-jogadores.
  - **B4 (Vínculo de binário do crash `ong2`)**: o core do `ong2` proveio do build de árvore suja (`build/TooieRecompiled`), de modo que o hash exato daquele binário permanece `UNKNOWN`. A explicação de hang em SIGTERM é `STRONG_INFERENCE` (caminho de eventos SDL2 não inspecionado).
  - **B5 (Testes unitários pós-patch plume)**: os testes unitários (`cleanpin-tests.exits`) precedem a aplicação do patch do plume e não foram re-executados contra o binário final `965bbabb`.
  - **B6 (Distinção da árvore suja do RT64)**: as ~14k linhas não versionadas no RT64 continuam existindo no checkout de trabalho (`BanjoRecomp/lib/rt64`), pré-existentes de fases anteriores (P08/diagnósticos); a documentação deve registrar claramente que o binário validado deriva estritamente da cadeia de pin limpo com patches.
  - **B7 (Validação de gameplay pelo usuário)**: ausência de teste visual pelo usuário (`USER_OBSERVED`) a 60 FPS com o binário reproduzível.
- **Invariante de produção** (histórico; `CORRECTED` 2026-10-21: o padrão segue OFF=CURRENT/30, mas agora é o valor padrão da opção Interpolation, não uma trava fixa nem um `setenv`): o caminho padrão de produção **permanece CURRENT/30**. P09 formal segue **`PENDING / NOT_FORMALLY_OPEN`**. Nenhuma alteração de código gerado, sem commit/push.
- **`NEXT_SINGLE_ACTION` (histórico): resolver os bloqueadores B2 e B5 do gate técnico validando o build de pin limpo (binário `965bbabb`) sob o escopo explícito de opt-in (`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0`) e registrando o log limpo de execução dos testes unitários pós-plume, antes de qualquer teste de gameplay dirigido pelo usuário a 60 FPS.**

## Histórico — investigação causal iteração 3 (2026-10-20/21)

Iteração 3 retomada após reset de cota de sessão (`/home/wesley/.cache/tooie-glitch-cause-and-fix/resumed-cause-and-fix.md`, `resumed-haiku-review.md`, `resumed-gate.json`):
- **Causa raiz A (`PROVEN` na fonte)**: em `RT64/src/hle/rt64_rigid_body.cpp:30-34`, o portão de translação linear divide `curVelMag /= dotCurVel` antes de checar `VelocityTolerance=5`. Com `dotCurVel` limitado inferiormente a $10^{-6}$ para curvas $\ge 90^\circ$, qualquer reversão/mudança de sentido desativa `lerpTranslation` retendo a translação no valor `CURRENT` independentemente da velocidade real. Triângulos entre ossos adjacentes (onde um inverte e trava enquanto vizinhos interpolam) esticam em hastes ("rods"), e peças rígidas desacopladas (faixa da mochila do Banjo, `image_018.png`) saem da trajetória.
- **Correção no owning layer**: introduzido `lerpTranslationSpeedOnly` avaliando a velocidade linear bruta em `rt64_rigid_body.{h,cpp}`. Em `rt64_p09_world_interpolation_guard.h` (e `rt64_p09_translation_coherence.h::gateSpeedOnlyEnabled`), translações travadas exclusivamente pela amplificação por cosseno (`!lerpTranslation && lerpTranslationSpeedOnly`) são liberadas para interpolação. Spikes reais de velocidade ($\ge 10\times$ e $\ge 5$ unidades) continuam retidos. Opt-in de teste `TOOIE_TEST_P09_GATE_SPEED_ONLY=0` restaura a decisão antiga para teste A/B.
- **Evidência primária em GPU e métricas independentes**:
  - Pares de frames GPU casados (OFF vs ON) renderizados na title scene (`runs/off1..off3` vs `runs/on1..on8`), alinhados por offset determinístico de conteúdo; controle OFF vs OFF comprovou identidade total de pixels (`diffpix>8 = 0`).
  - Métrica de overshoot de vértice independente do critério da guarda (`tools/vover.py`, teste de corda $>0.5\times\text{chord}+3$): redução de 26.185 para 2.673 vértices no intervalo de título wl 1000-2500; triângulos esticados cruzando matrizes zerados.
  - Mochila do Banjo (`image_018.png` / `final_backpack_1889_1890.png`): renderização GPU mostra a faixa azul recolocada no corpo em wl 1889 e wl 1890, restando pequeno resíduo pontual em 1890.
  - Taxa de vértices congelados em CURRENT caiu de 4.6% para 0.6% (abaixo do baseline OFF 0.8%), e vértices próximos do ponto médio (30-70%) subiram para 97.6%.
- **Veredito do gate independente Haiku 5.5 (`resumed-gate.json`): `passed: false`**.
  - **F1 (par GPU casado)**: PARCIAL-CLOSED para title scene.
  - **F2 (estiramento zero)**: PARCIAL (métrica independente de corda apoia, mas `crossStretched=0` segue critério próprio).
  - **F3 (snapping de articulações held)**: ABERTO (avaliado apenas em Banjo/Kazooie wl1604; corte de cena wl4226 ainda retém 56 matrizes; objetos não-jogadores não medidos).
  - **F4 (gameplay real)**: ABERTO (apenas attract demo wl3000-9000 avaliado; sem teste pelo usuário a 60 FPS).
  - **F7 (fechamento elementwise)**: HEURÍSTICA A NÍVEL DE CLASSE (não prova anatômica).
  - **F8 (mochila)**: PARCIAL (confirmado em 2 frames GPU, resíduo visível em wl1890).
  - **Bloqueadores formais do Gate**:
    1. Exits de build da iteração 3 não verificados (sem logs de compilação em disco; ninja dry-run inconclusivo).
    2. Contradição nos códigos de saída dos runs de jogo: `runs/ong2/meta.json` terminou com `rc=-11` (SIGSEGV) não explicado/não mencionado no documento da causa; 6 runs terminaram com `rc=None` (não recolhidos após SIGTERM).
    3. Árvore de trabalho carrega código de diagnóstico RT64 unversioned (14.263 linhas em 32 arquivos eram preexistentes de fases anteriores como P08 sob preservação de worktree, enquanto a iteração 3 introduziu apenas 5 inserções em rt64_rigid_body.{h,cpp} e rt64_workload_queue.cpp); o replay em pin limpo `6f1c2d99` aplica os 6 patches, mas não reproduz a árvore compilada suja.
    4. Ausência de teste em objetos articulados não-jogadores (F3).
    5. Ausência de teste de gameplay dirigido pelo usuário a 60 FPS (F4).
- **Invariante de produção**: o caminho padrão de produção **permanece estritamente CURRENT/30** (`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1`, `tooie_menu::kForcedRefreshRate = 30`). P09 formal segue **`PENDING / NOT_FORMALLY_OPEN`**. Nenhuma alteração de código gerado, sem commit/push.
- **`NEXT_SINGLE_ACTION`: investigar e reproduzir em ambiente isolado a causa do crash SIGSEGV (`rc=-11`) no run `ong2` (attract demo gameplay em workload 4200) e verificar a reprodutibilidade do build com log salvo antes de qualquer teste de gameplay pelo usuário.**

## Histórico — guardas de interpolação articulada iteração 1/2 (2026-10-20)

Usuário marcou 14 PNGs com glitches na pasta `/home/wesley/Imagens/TooieRecomp/interpolated-burst-20261020-175030/` (`001,002,005,006,007,008,009,010,012,013,014,015,016,018`; todas `curW = 0.500000`, 60/30 Hz, `PRE_VI_RESOLVED_COLOR_TARGET`). Leitor de imagem confirmou: hastes esticadas ("rods") na crista da cabeça da Kazooie (`image_001`…`016`) e faixa/prisma azul na mochila do Banjo (`image_018.png`). Causa A provada na CPU (portão AUTO desarmando translação enquanto rotação slerps estica juntas跨-WORLD; forçar coerência remove ~79% dos triângulos esticados no wl 1594/1619). Guarda de fechamento articular (recuo elementwise) atua como cura heurística não-rígida a nível de classe. Guardas implementadas no RT64 (`patches/rt64/rt64-p09-world-interpolation-guard.patch`, SHA256 `1b771a1ab9205f6ffba0c1f8085e53979f38fed031319aedfe04d8cf8bb520d6`), inertes no caminho padrão CURRENT/30 (`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1`). Build `TooieRecompiled` exit 0 (SHA256 `ebf5924cfd1dfd1471e44a0ae438e022a779ac3e32f6e9c8211bbb18da140b76`), unit test `./build/p09_world_interpolation_guard_test` PASS 27/27, replay limpo em pin RT64 `6f1c2d99` PASS. **Gate independente (`gate.json` / `haiku-review.md`): `passed: false`**. Pendente: F1 (par GPU casado renderizado com guardas ON; wl 1619 só tem GPU original vs CPU reprojetada), F2 (métrica de estiramento zero circular), F3 (congelamento não medido de rotação em held multi-slot), F4 (gameplay não testado), e F8 (mochila em 018 não provada corrigida). P09 formal segue `PENDING / NOT_FORMALLY_OPEN`. Sem commit/push. Estados abaixo históricos.

## Histórico — burst F8 de PNGs interpolados implementado (2026-10-20)

Atalho **F8** salva 20 PNGs (`image_001..020.png`, estágio `PRE_VI_RESOLVED_COLOR_TARGET`) para o usuário apontar glitches. **`RUNTIME_SHORTCUT_20_PNG_PASS`**: pasta `/home/wesley/Imagens/TooieRecomp/interpolated-burst-20261020-174942/`, 20 PNGs distintos/decodificados e 20 linhas de manifesto INTERPOLATED (pesos 1/3 ou 2/3), DONE20/20; root verificou title upright/cores em image010. Não prova glitch capturado/corrigido nem pixels desktop/VI/UI. Teste isolado atual PID `1431958` / ticks `10790476`, opt-in60/CURRENT0 sem heavy, SHA256 binário `7fb2d73e1bb883e96510a84d5e7acbb76374df99cd2f4c089eef883d7d9de0cc`; configuração/saves pessoais preservados. Build serial final0, unitPASS0; patch `af9cd9d9…4767b`, replay reportado e caveat memória não coerente na evidence [[plan/evidence/p09-interpolated-frame-burst-20261020]]. **`NEXT_SINGLE_ACTION` (`SUPERSEDED` 2026-10-21: F8 foi removido a pedido do usuário): ~~usuário apertar F8 na cena desejada~~.** Sem commit/push; estados abaixo históricos, snapshot antigo foi substituído por esta tarefa.

## CURRENT — captura vazia, gap de observabilidade de admissão (2026-10-20)

Último estado substitui blocker de fechamento abaixo: root autorizou término somente do PID isolado `1365219`, identidade reverificada; SIGKILL desse PID e saída comprovada. Nova instância aprovada PID `1375991` / ticks `10284012`, cache `snapshot-recovery-20261020T192455Z`. Retry acoplado executado (pré→arm 0.370768 ms, arm→pós 20.252024 ms), mas screenshots pré/pós mostram transição caverna/cobra sem alvo: **`DISCARDED_AS_TARGETED_CAPTURE / SCENE_TRANSITION`**, snapshot válido `NO_WITNESS`/zero pares. Sem terceira captura. **`NEXT_SINGLE_ACTION`: implementar telemetria mínima default-OFF de cena/workload e admissão no snapshot existente, antes de mais capturas**, com map/demo phase de fonte real identificada. Ainda não existe vazio com alvo válido ao armar, nem prova de falha de admissão nesse alvo; telemetria não implementada. PID novo permanece ativo, marcador ausente/frozen. Sem alteração código/produção/commit/push.

Retry único correlacionado **`NOT_EXECUTED / OWNED_PROCESS_CLOSE_BLOCKED`**: captura é one-shot por processo; PID isolado `1365219` / ticks `10225543` permaneceu ativo após SIGTERM e `WM_DELETE_WINDOW` na janela própria verificada, ambos com espera 3 s. Guardas recusaram nova instância; nenhum force/SIGKILL. Helper acoplado preparado em cache, não executado. **`NEXT_SINGLE_ACTION`: resolver encerramento da instância isolada congelada, depois executar retry único numa instância nova antes de qualquer edição diagnóstica.** Fonte/proveniência na evidence; não houve segunda captura.

Ver [[plan/evidence/p09-visual-test-mode-20261020]]. Autorização nova permite capturas autônomas. Isolado aprovado PID `1365219` / start_ticks `10225543`, cache `snapshot-recovery-20261020T191510Z`. Janela do PID mostrou Mumbo/água/penas visíveis, mas armamento ocorreu 21.45 s após screenshot e pós-screenshot já era título: cena ao armar `NOT_PROVEN`, nenhum glitch provado. Captura única sem timer: inspetor exit 0, bounds válidos, `NO_WITNESS`, zero pares; não repetida. Sonnet real `claude-sonnet-5-5-medium`, sessão `6df435e2-ffe9-4988-a7b2-75675a3d12dd`, exit 0: modo snapshot-only não exporta razões finas de rejeição dos tokens; vazio não distingue gate de admissão/cena/workload. **`NEXT_SINGLE_ACTION`: arbitrar diagnóstico mínimo por-armamento de admissão com linha única ao freeze**, antes de nova captura com screenshot/marcador acoplados. Proposta e fontes na evidence; não implementada. Código/produção/config pessoal preservados; sem commit/push. Entradas seguintes históricas.

## Histórico — recuperação isolada `WAITING_USER_SCENE` (2026-10-20 18:00 UTC)

Parecer Sonnet solicitado concluído via Cursor, `claude-sonnet-5-5-medium`, sessão `6df435e2-ffe9-4988-a7b2-75675a3d12dd`, exit 0 / JSON success. Hipóteses ranqueadas e limites em [[plan/evidence/p09-visual-test-mode-20261020]]; nova hipótese auxiliar: gate AUTO de translação versus rotação no balanço. Sem novo proof runtime ou cobertura anatômica; snapshot CPU não discrimina sozinho essas causas. Usuário: **“Ainda não está pronta”**; captura permanece desarmada, sem cronômetro. Nenhuma implementação/novo teste/lançamento/armamento neste parecer.

Suplemento Sonnet único, mesma sessão/modelo, exit 0: corrigiu o mecanismo potencial para **translação em CURRENT enquanto rotação interpola**, condicionado a flags/`AUTO` ainda desconhecidos nas testemunhas. Corrigiu `NO_WITNESS` como insuficiente para atribuir causa ao filtro e retirou exclusões anatômicas. Relatório permanece estático auxiliar com ressalva de navegação incompleta; não é causa runtime comprovada.

Novo `USER_OBSERVED`: glitches em partes que balançam durante movimentos — focinho e aba da mochila do Banjo, penas da cabeça da Kazooie e do Mumbo, entre outras. Ocorrência no frame intermediário é impressão do usuário (`USER_HYPOTHESIS / HYPOTHESIS`), sem correlação de frame renderizado nem identidade resource/bone inferida. Snapshot CPU não equivale a prova do frame apresentado pela GPU; ver limites e próximo discriminador na evidence. Estado de espera e protocolo de armamento preservados.

Ver [[plan/evidence/p09-visual-test-mode-20261020]]. Execução isolada PID `1321557` / start_ticks `9775814`, binário aprovado, configurada 60 FPS + snapshot somente; marcador inexistente, sem watcher/cronômetro, configuração pessoal preservada. Metadata/log `/home/wesley/.cache/tooie-interpolation-next-20261020/snapshot-recovery-20261020T180013Z/`. Duas capturas antigas encontradas e decodificadas: `NO_WITNESS`, zero pares; última armada por timer de 90 s, cena `UNKNOWN`, `NOT_ACCEPTED_AS_TARGETED_GLITCH_EVIDENCE`. Não provam correção ou ausência do glitch. **`NEXT_SINGLE_ACTION`: aguardar confirmação de cena pronta; então armar uma vez.** Gates host anteriores mantidos; sem código/produção/commit/push. Estados seguintes históricos.

## Histórico — `WAITING_USER_SCENE`, snapshot host aprovado

Ver [[plan/evidence/p09-visual-test-mode-20261020]]. Snapshot: build/revisões host **PASS**, unidade123/123, I/O entre processos e 13 negativos semânticos rejeitados; não validado em gameplay. Novo jogo isolado PID inicial `1188686` aberto, configurado60 + novo snapshot sem heavy; FPS medidos `UNKNOWN`. Marcador inexistente. **`NEXT_SINGLE_ACTION`: aguardar usuário avisar cena pronta e só então armar uma vez; não armar por cronômetro.** Captura real `NOT_EXECUTED`; anatomia/proveniência das testemunhas `UNKNOWN`; correção permanente `NOT_IMPLEMENTED`. Caminho normalCURRENT/30 preservado, sem commit/push; execução sempre delegada. Estados abaixo são históricos, inclusive primeiro jogo fechado e gates antigos pendentes.

## CURRENT — glitches reproduzidos no teste opt-in; jogo fechado

Ver [[plan/evidence/p09-visual-test-mode-20261020]]. Usuário: **“Os glitches reapareceram”** (`USER_OBSERVED / USER_CONFIRMED_FOR_TESTED_RUN`) no modo **configurado** opt-in 60, CURRENT desligado, sem coletores pesados. Depois: **“Jogo fechado; não conferi contador”**. FPS efetivos `UNKNOWN`; regiões individuais e cena exata não foram confirmadas nesse relato. Focinho Banjo/penas **da cabeça** Kazooie continuam prioridades, demais glitches incluídos. Snapshot CPU parcial **`NOT_ACCEPTED_FOR_GAME_CAPTURE`**; CURRENT/30 normal preservado; correção permanente `NOT_IMPLEMENTED`, sem novo teste visual posterior, commit/push ou encerramento/abertura formal de fases.

`NEXT_SINGLE_ACTION`: concluir gates independentes do diagnóstico snapshot-only, alcance dos hooks, persistência, corrupção e novo I/O antes de relançar captura. Execução sempre delegada; root orquestra/arbitra. Estados abaixo são históricos.

## CURRENT (2026-10-20) — VISUAL_TEST_MODE_AUTHORIZED_AND_LAUNCHED

Ver [[plan/evidence/p09-visual-test-mode-20261020]]. Nova autorização do usuário permite
retirar o paliativo **nos testes**. Opt-in `TOOIE_TEST_INTERPOLATION_FPS=60`
(ou 120) desativa CURRENT quando não explicitamente definido e força a taxa
manual escolhida; sem opt-in, caminho estável CURRENT/30 preservado.
Build e testes de configuração/endpoint passaram; revisões Sonnet/Haiku sem
bloqueios para este modo. Jogo aberto em teste 60 FPS sem coletores pesados;
resultado visual e taxa efetiva **PENDING / UNKNOWN**.
Reprodução: Demo 1 pós-intro; gameplay lateral para focinho; Talon Trot
`Z + C-Esquerda` lateral para penas da **cabeça** da Kazooie.
`NEXT_SINGLE_ACTION`: obter relato visual desta execução. Snapshot CPU segue
parcial/não aceito para captura. Esta entrada prevalece sobre próximos passos
históricos abaixo; P09 não abre formalmente e nenhum commit/push foi feito.

## CURRENT — diagnóstico CPU parcial, revisão final pendente

Ver [[plan/evidence/p09-interpolation-resumption-20261020]]. Grok via Cursor implementou snapshot/replay CPU default-OFF limitado; último build reportado exit 0 e teste 97/97. **`NOT_ACCEPTED_FOR_GAME_CAPTURE`**: revisões anteriores acharam blockers e último ajuste não recebeu gate independente completo; novo teste I/O Haiku criado mas `NOT_RUN`. Root/builder/reviewer Codex atingiram cota; nenhum esgotamento da conta Cursor/agy foi comprovado. Não houve jogo/captura/aceite visual/60+ FPS. CURRENT/Manual 30 preservados; correção permanente `NOT_IMPLEMENTED`; P09 não aberto formalmente, P08 pendente; sem commit/push. Testemunhas prioritárias `USER_OBSERVED`: focinho do Banjo e penas **da cabeça** de Kazooie, sem excluir outros glitches nem presumir anatomia de resources.

`NEXT_SINGLE_ACTION`: retomar gate independente final de snapshot-only, alcançabilidade de hooks, persistência e rejeição de corrupção, incluindo novo teste I/O em processos separados, antes de captura. Execução sempre delegada: Grok código/testes Cursor; Sonnet/Haiku revisão Cursor; Gemini Markdown `agy`; root orquestra/arbitra. Seções seguintes preservam estados históricos.

## CURRENT (2026-10-20) — INTERPOLATION_RESEARCH_RESUMED

Continuação `IN_PROGRESS`, delegada aos agentes: testemunhas prioritárias
`USER_OBSERVED` são focinho do Banjo e penas **da cabeça** de Kazooie; outros
glitches permanecem no escopo. Não inferir modelo/bone/resource dessas descrições.

Read [[plan/evidence/p09-interpolation-resumption-20261020]]. Investigation only;
no production code/patch changes, game launch, new capture, commit or push.
Published baseline: `73bfd430`; CURRENT `tfPos` default-ON and Manual 30 remain.
Four supporting host tests passed again; this is not GPU/visual acceptance.
The old tie-capture files in `/tmp` are absent. The selective WORLD path also
feeds lighting/texgen (`PROVEN_STATIC`); its role in regressions is `HYPOTHESIS`.
P09 remains `PENDING / NOT_FORMALLY_OPEN`; P08 formal gates are not closed here.
`NEXT_SINGLE_ACTION`: implement a bounded default-OFF CPU snapshot/replay
diagnostic for local edge provenance and lighting/texgen gates; no heavy global
pair search, alias, remap or shader change. This diagnostic is not implemented yet.

## Prior (2026-10-19) — BOUNDED_SETTINGS_AND_BGM_VOLUME_INTEGRATED_AND_VALIDATED

Read [[plan/evidence/p08-settings-functionality-20261019]], [[plan/evidence/menu-title-tooie-wordmark-20261019]], and [[plan/evidence/checkpoint-20261019-review-and-patch-replay]]. Details in canonical evidence.

- **Task Status & Base State (BOUNDED_TASK_COMPLETED / PROVEN)**:
  - Canonical tested base HEAD: `d4d1074016a819d7150923c2b19a91e69889b7a9`.
  - Bounded settings task finished. Phase P08 controller gate remains pending; no global phase closure.
  - Commit and push explicitly authorized on 2026-10-20 for this bounded checkpoint. Pre-existing local dirty tooling and config state stay outside the commit. External dependencies (`RecompFrontend` `d0d90ba`, `N64ModernRuntime` `ca568b6`) remain protected dirty; changes are versioned through the project patches. No generated hand-edits.
  - Full build: `cmake --build build --target TooieRecompiled tooie_bgm_volume_test tooie_bgm_hooks_test tooie_menu_policy_test tooie_menu_config_test -j2` exit 0 (13.1s).
  - Binary SHA256: `e2059b516a90f09e50e77f0622218014798f6a155f6ffe0d2b6a66735427f9f9`. All 4 tests PASS exit 0.
- **Menu Settings Policy & Disabled Numeric Style Fix (PROVEN)**:
  - Note & Jinjo Saving safely hidden (`hidden = true`, default `Off`, schema preserved for compatibility).
  - Non-trivial BT options disabled in UI with `"Currently unavailable in Banjo-Tooie."` (`UI_DISABLED / NOT_IMPLEMENTED_BT_CONSUMER`): Analog Camera, Sensitivity, 3 Inverts, Cutscene Aspect, HUD Placement (`hr_option`).
  - Generic frontend disabled guard + numeric style fix: `ConfigOptionNumber::update_disabled` now calls `slider->set_enabled(!disabled)`; `Slider::do_step` blocks mouse/keyboard stepping.
  - Visual pass confirmed by Claude Haiku session `a7c1b458-e32c-4f51-9d80-eaf5f6b57aef` exit 0 on `general-disabled-final.png` and `graphics-locked-final.png` (interaction strictly performed by root). Real drags preserve Sensitivity 3 and Framerate 30; Analog Off.
  - Forced 30 FPS cap (`tooie_menu::kForcedRefreshRate = 30`): deliberate user policy cap due to CURRENT workaround repeating vertex poses above native; RT64 supports 60+ Hz, not a technical lack of 60Hz support.
  - Resolution 3x (720p base) and 4x (960p base) added with multiplicative downsampling (3x DS 2x = 1440p downsampled to 720p). Auto resolution uses integer window VI scale and disables downsampling.
  - Aspect ratio window auto-resize on SDL event thread: 1600x900 -> 1200x900 (Original 4:3) -> 1600x900 (Expand 16:9), preserving height 900 and usable display bounds; ignored in fullscreen (1920x1080 unchanged).
- **BGM Volume Control Architecture & Live Integration (PROVEN)**:
  - Banjo-Tooie audio contract: 6 sequence players at `0x80079FA0` (stride `0x274`, `seqp = player + 0x1E0`), authored s16 volume at `player + 0x0`, 6 tracks at `0x801357D0` (stride `0x50`, active id s16 at `+0x28`).
  - CSP event dispatch `func_80025ED0` (type 10, music) vs SoundPlayer `func_80020BE0` (type 3, SFX - untouched).
  - Setter `func_80017404` stores unscaled volume then calls CSP; start `func_80016ED0` calls CSP directly; frame fade in `func_800FB968`.
  - Double-event proposal from `17404` REFUTED. Integrated architecture uses lookup overrides in `boot_hw_stubs.cpp`:
    1. Single-scale CSP wrapper (`tooie_bgm_on_csp_set_vol`) scales once for the 6 known sequence player pointers; unknown pointers pass unchanged.
    2. Post-update hook (`tooie_bgm_on_update`) on `800FB968` reads `player + 0x0` for active tracks (`track + 0x28 >= 0`) and reposts scaled volume via original `func_80025ED0` using a local `recomp_context` copy ONLY when live gain changed. Steady 100% emits zero extra events; authored fades are never mutated.
    3. UI atomic percent default 100 with no `Config` pointer captures.
- **Launcher IPC & Practical Audio QA (PROVEN)**:
  - Separate child launcher preview process connected via datagram `socketpair` (`CLOEXEC`, child stdin dup with `SO_TYPE` check, parent 5ms monitor 1-byte send `MSG_NOSIGNAL`, child `update_gfx` drains atomic gain). No filesystem polling or hot telemetry; no SDL queue changes.
  - Practical audio captured via owned null sink `tooie_bgm_menu_qa` in `/home/wesley/.cache/tooie-menu-settings-20261019/`:
    - Preview stream (SAME child alive across volume shifts): `preview-100.wav` (RMS .0522), `preview-0-real.wav` (RMS .0000017, peak 1 LSB, zero after 0.5s), `preview-50-real.wav` (RMS .0167), `preview-100-restored.wav` (RMS .0373).
    - In-game live volume restoration (no restart): `game-0.wav` (RMS 0, peak 0) -> `game-100-restored.wav` (RMS .0528, peak .3998).
    - Epistemic note: Songs differ across passages; do NOT claim exact PCM 50% ratio; math verified in unit tests. Early captures renamed `preview-*-ui-not-applied.wav` are invalid UI_NOT_APPLIED (click not drag, UI still 100) and excluded.
    - Scope: user confirmed only effects/voices in the final 0% demo clip on 2026-10-20; voice IDs using sequence player vs SoundPlayer remain UNKNOWN; global audio parity NOT claimed. PulseAudio null sink module `536870913` unloaded.
- **Clean Pin Patches & Review**:
  - 3 clean patches: `n64modernruntime-resolution-3x-4x.patch` (SHA256 `6955ca32...`), `recompfrontend-resolution-3x-4x.patch` (SHA256 `60eb3140...`), `recompfrontend-disabled-option-guard.patch` (SHA256 `9298ec60...`, superseding old hash).
  - Clean-base replay verified on temp fixtures (both apply and reverse match exactly). Full applicator script live NOT_RUN. Original config hashes unchanged.
  - Claude Sonnet final STATIC review `c4500e0c-e5c9-4b1b-bcf4-5a054a8fe733` exit 0 (zero blockers in helper/context/socket scope; not independent MIPS reaudit; process 143 discarded, not quota).
- **User confirmation (2026-10-20, USER_OBSERVED / USER_CONFIRMED_FOR_TESTED_DEMO_CLIP)**: with BGM at 0% in `final-game-0.wav`, user reported “Apenas efeitos/vozes”. Music removal and preserved effects/voices confirmed for this clip; campaign-wide voice classification remains UNKNOWN.
- **Next Single Action**: `verify published HEAD against origin/main after authorized push`.

## Áudio tic tic — fix de fila e validação de intro (2026-10-19)

- **Contexto**: Correção do tempo de fila SDL na intro ([[plan/evidence/p07-audio-tic-tic-investigation-20261019]]).
- **Commit HEAD**: `5af4510744ddc90d25f9223ab48e9cff76f5baca` | Status: `USER_CONFIRMED_FOR_TESTED_INTRO_RECORDING`.
- **Fix de Produção (Default)**: `main.cpp` divide bytes SDL por taxa e canais host (48 kHz float32). Sem flag de teste.
- **Diagnóstico (Default OFF)**: `TOOIE_AUDIO_CAPTURE_DIR` com worker pós-janela e caps rigorosos (24 MiB / 2400 chunks).
- **Evidência Runtime**: Pré-fix apresentava 47 skips espúrios (filas reais de 46–59 ms). Pós-fix obteve 0/579 skips e pre/post idênticos.
- **Validação do Usuário**: Usuário escutou `intro-queue-time-fix.wav` (33s Pulse) e atestou que o ruído "Desapareceu".
- **Rastreabilidade**: REQ-AUD-05 parcialmente validado para intro; P07-T06 parcial; P07-T07 PENDING/UNTESTED.
- **NEXT_SINGLE_ACTION**: QA focado de áudio em gameplay real (SFX/voz/pausa) via `./build/TooieRecompiled` padrão.

## CURRENT (2026-10-19) — HOST_LAUNCHER_TOOIE_WORDMARK_AND_CHECKPOINT_PUBLICATION_CONFIRMED

Read [[plan/evidence/menu-title-tooie-wordmark-20261019]], [[plan/evidence/checkpoint-20261019-review-and-patch-replay]], and [[plan/evidence/p08-attract-movement-residuals]]. Details in canonical evidence.

- **Checkpoint 2c52af0c Published (PROVEN)**: Published exact on `origin/main` (199 paths, exit 0, `checkpoint-publication-proof.json`).
- **Launcher Wordmark & Asset Isolation (PROVEN)**: Two-line vector logo ("BANJO-TOOIE" blue/yellow, "RECOMPILED" red/yellow) via `tools/make_tooie_launcher_logo.py` (`d13cb8d9...`) into `ui_assets/TooieLogo.svg` (`036d3cb2...`). CMake target `TooieLauncherLogo` copies to `build/ui_assets/`; `BanjoRecomp/assets/Logo.svg` remains unchanged and tracked (clean).
- **Build (PROVEN)**: Incremental build `cmake --build build --target TooieRecompiled -j2` exit 0 (NOT clean build), binary `build/TooieRecompiled` (`ffe3d7a5...`).
- **Settled Visual QA (VISUALLY_CONFIRMED)**: Claude Haiku 5.5 medium (exit 0) confirmed 1600x900 and 1024x768 4:3 settled layouts (legible, no clipping). Root visually inspected ONLY `frame_1.png` (1600x900); no root 4:3 claim.
- **Entry Animation Follow-up (BOUNDED_PASS)**: Claude Haiku 5.5 medium (exit 0, `menu-title-haiku-animation-addendum.txt`) viewed 5 discrete frames at 0.28/0.79/1.42/2.57/5.87s (`t_0_2s.png`..`t_5s.png`). Logo entry composition PASS; smoothness/easing NOT_TESTED (discrete frames); no gameplay QA.
- **Repository & Runtime State**: Wordmark task uncommitted; pre-existing local tool/config state preserved; gameplay invariants intact.
- **Next Single Action**: `extend oracle comparison to another attract demo when user requests next game adjustment`.

## Checkpoint publicado (2c52af0c) (2026-10-19) — RT64_CLEAN_PIN_REPLAY_AND_GETTER_GATE_RESOLVED

Read [[plan/evidence/checkpoint-20261019-review-and-patch-replay]], [[plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-demo-movement-precision-diagnosis]], and [[plan/evidence/p08-attract-movement-residuals]].
Checkpoint published to origin/main as commit 2c52af0c7d8287d9003db839ffbe636d585e900a (199 paths, verified by checkpoint-publication-proof.json). Pre-commit review completed with both Grok 4.7 Medium sessions ending with `end_turn`; findings P1 and P2 resolved.

- **P1 Resolution — RT64 Clean-Pin Patch Replay & Applicator Chain (PROVEN)**:
  - Initial review identified that older incremental RT64 patches (`rt64-present-rate-decimator.patch`, `rt64-present-success-counter.patch`, `p09-exact-use-pathology-correlation.patch`) were based on the dirty worktree and failed `git apply --check` (exit 1) on clean RT64 pin `6f1c2d99a4ea571c139f449c326fd176ba8f3496`.
  - Replaced for clean pin apply by 3 new non-overlapping patches in `patches/rt64/` against base `6f1c2d99`:
    - `patches/rt64/rt64-rsp-endpoint-current.patch` (SHA256 `49925480...`, 16,629 bytes): production CURRENT `tfPos` snap and post-snap fog in `RSPProcessCS.hlsl`, push constant expansion, and CPU data path.
    - `patches/rt64/rt64-present-fps-decimator-and-counter.patch` (SHA256 `6d746f27...`, 5,415 bytes): cumulative presentation decimator (`rt64_present_decimator.h`, `rt64_present_stats.h`), `viOriginalRate` write protection under `configurationMutex`, and `presentSuccessCount` increment.
    - `patches/rt64/rt64-p09-exact-use-correlation-header.patch` (SHA256 `0d4c250a...`, 43,083 bytes): diagnostic correlation header required by host unit test.
    - `patches/frontend/recompfrontend-overlay-update-callback.patch` (SHA256 `37266d0b...`, 2,732 bytes, pin `d0d90ba`): RecompFrontend overlay update hook.
  - `tools/apply_runtime_patches.sh` registered with the 3 RT64 clean-pin patches and frontend patch (+49 insertions, SHA256 `87a6455f...`; `bash -n` exit 0, `git diff --check` exit 0). Applicator script was NOT run against live checkouts to protect uncommitted diagnostic state.
  - Scratch replay on isolated clone `rt64-scratch-apply`: reverse `--check` exit 0; 14 covered paths verified (6 byte-identical to live, 8 differ strictly due to excluded diagnostics).
  - Standalone tests compiled and passed against scratch: `rt64_present_decimator_test` PASS exit 0; `p09_exact_use_correlation_test` 1354/1354 PASS exit 0; `p09_cpu_shader_clip_semantics` 14/14 PASS exit 0.
  - Old incremental patches remain in tree for historical provenance, but are SUPERSEDED for clean-pin apply. Pathology C++ hook integration was explicitly NOT reproduced from clean pin; not all diagnostics are claimed reproducible from pin.
- **P2 Resolution — Model-Getter Diagnostic Gate Fix (PROVEN)**:
  - In `src/game/backpack_lookup_diag.cpp`, `__wrap_func_80092B04` unconditionally called `v7_note_model_getter`, which did guest memory reads, atomic counter increments, and acquired `g_secondary_sway_mu` to write `g_durable_bins` on default gameplay.
  - Gated in `include/tooie_diag.h` and `src/game/tooie_diag.cpp`: `v7_note_model_getter` returns early when `!g_getter_durable_needed` (evaluated in `ensure_init()` across consumers requesting durable metadata: `TOOIE_DIAG_SECONDARY_SWAY`, `TOOIE_DIAG_FEATHER_HANDOFF_V7`, `TOOIE_DIAG_FEATHER_POSTPROCESS_Q`, temporal coherency, `TOOIE_DIAG_RSP_ENDPOINT_BINDING`, and strict `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION`).
  - Validated by host test `tests/getter_diag_gate_test.cpp` PASS exit 0.
  - Rebuilt binary `build/TooieRecompiled` SHA256 `74a0cea04a2bc3704860388673c40b2e73ee34d284f3a2afa0faebe91a9c05f5` (size: 29,418,200 bytes, exit 0).
  - **Binary hash distinction**: Rebuilt binary `74a0cea0...` is distinct from prior user-tested candidate binary A6A2 (SHA256 `a6a2c023...`, size: 29,413,928 bytes). User visual confirmation ("Sim, continua funcionando") was conducted on A6A2; do NOT claim user visual confirmation for `74a0cea0...`.
- **Preserved State & Invariants**:
  - Aspect-aware widescreen culling remains default ON (`TOOIE_TEST_MAP_VIS_ASPECT_AWARE=1`, `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`). Narrow allowlist: pure draw `func_800B97A4` (kind 3) + preserved `func_8011447C` (kind 2); mixed renderer `func_800DE498` stays on original unexpanded `v0`.
  - Widescreen object edge pop fix is visually confirmed for the tested area on candidate binary A6A2 (`VISUALLY_CONFIRMED_FOR_TESTED_AREA`, `USER_REPORTED`). Broader maps/objects/movement parity remains an untested residual.
  - USER_DEMO1 trajectory (355 samples), actor records (546 words), LCG RNG snapshots (14), and Fire Egg sweep hit (`v0=0x80132f60`, `func_800CD6B4=1`) match original bit-exact.
  - Demo 7 cutscene stop fix preserved (`n64recomp-zero-size-static.patch`).
  - CURRENT `tfPos` workaround default ON (palliative); intermediate frames repeat vertex poses above native; continuous interpolation remains `NOT_IMPLEMENTED`.
  - Audio glitch remains deferred; 8-preset UI selector remains `PLANNED` / `NOT_IMPLEMENTED`; formal P09 phase remains `PENDING / NOT_FORMALLY_OPEN`.
- **Operational Boundaries / What Was Not Run**:
  - No game launch or termination during review; user may have game running.
  - Native A6A2 gate not repeated; cited from cache.
  - Applicator script not executed against live trees.
  - Root owns Git stage, commit, and push; Git commit will establish publication truth.

```text
PRECOMMIT_REVIEW_STATUS: PASSED_WITH_FINDINGS_RESOLVED (P1 and P2 resolved; clean-pin replay verified; automated gates exit 0)
P1_RT64_CLEAN_PIN_CHAIN: RESOLVED (3 new patches + frontend patch in tools/apply_runtime_patches.sh; old incremental patches SUPERSEDED)
P2_GETTER_DIAG_GATE: RESOLVED (v7_note_model_getter gated by g_getter_durable_needed; getter_diag_gate_test PASS; binary 74a0cea0 exit 0)
BINARY_HASH_DISTINCTION: PROVEN (latest build 74a0cea0 distinct from user-tested candidate A6A2 a6a2c023; user visual confirmation applies to A6A2 only)
WIDESCREEN_EDGE_QA_STATUS: VISUALLY_CONFIRMED_FOR_TESTED_AREA (USER_REPORTED on candidate A6A2; broader game parity untested residual)
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: extend oracle comparison to another attract demo when user requests next game adjustment
```

## Prior (2026-10-19) — USER_DEMO1_FRUSTUM_PARITY_LANDED_AND_USER_VISUALLY_CONFIRMED_FOR_TESTED_AREA

Read [[plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-demo-movement-precision-diagnosis]], and [[plan/evidence/p08-attract-movement-residuals]].
Grok 4.7 Medium implementation session `01a15118-c738-7dd3-98ed-cc5db5021bb5` completed `end_turn` (38 turns; root observed CLI exit 0). The narrow fix landed in candidate binary A6A2 (SHA256 `a6a2c023a9e3b0688cac15249cda61b5c8b8ccdf68c7c40f91d8c512356bef5e`).
Source edits owned in `src/game/boot_hw_stubs.cpp`, `src/game/tooie_demo1_projectile.h`, and `tests/tooie_demo1_upd_layout_test.cpp`; `RecompiledFuncs` untouched, no generated hand-edits.

- **Scope Boundary & Allowlist Architecture (PROVEN)**:
  - `tooie_wrap_func_800E3E8C` no longer opens a `TooieCacecDrawScope` (shared broad DRAW scope dropped; nested CACEC only expands if an outer scope is active).
  - Pure draw caller `func_800B97A4` registered via `tooie_wrap_func_800B97A4` with scope kind 3 when `tooie_cacec_pure_draw_vram(kCacecPureDrawB97A4)` is true (guest VRAM `0x800B97A4`; accept path emits G_MTX / G_DL / G_MOVEMEM and matrix calls without RNG/simulation side effects).
  - Existing `tooie_wrap_func_8011447C` scope kind 2 preserved for object visual pop culling.
  - Mixed renderer `func_800DE498` (which stores E3E8C `v0` to `0x8012C940` and on accept executes callbacks and writes renderer context) stays on original `v0` because no scope wraps it.
  - Unclassified callers stay on original `v0`. No default flag OFF, no RNG save/restore, no host-PC/RA production gates.
- **Production Default-ON Parity Verification (PROVEN)**:
  - Feature remains production default ON (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`). Both fresh default-ON captures completed exit 0 with matching verification.
  - **TARGET Capture** (`demo1-sep-target-current/target.csv`, SHA256 `016a102c92f279aef896bcab0802be27b1671adab06f8bc374bf227573b398ec`, owned PID 1776728, 66.084s, exit 0):
    - All 355 paired trajectory samples `w0..w11` (`tgtrec`) match original flag-OFF / oracle2 bit-exact (0 mismatches; old buggy default-ON diverged post 83).
    - All 546 ActorRecord words (`updrec` 39 words x 14 pre/post snapshots across posts 80..86) match original bit-exact.
    - All 14 raw LCG RNG snapshots (`a2`) match original bit-exact (resolving PRE82 RNG split).
    - All 14 `+0x60` words match original `0003006c` (was `0004006c` under buggy default-ON).
  - **PROJECTILE Capture** (`demo1-sep-projectile-current/projectile.csv`, SHA256 `570f42e0ad8d31a883c2345afc5e0b000a3fde22baa7edc7722ab35b031d1531`, owned PID 1774174, 67.274s, exit 0):
    - Post 355 sweep `func_800CD820` leave returns `v0 = 0x80132f60` (hit) and narrowphase `func_800CD6B4` leave returns `1`, exactly matching original hit (old buggy default-ON missed with `v0 = 0`).
    - 22 sweep ENTER pairs and 21 `sueggmove9` ENTER pairs match original in 6 geometry words and gamespeed `gs`.
  - **Build & Tests**: Ninja exit 0 (up-to-date); all unit test binaries (`upd_layout`, `target_selector`, `projectile`, `parity`, `demo7_drift`, `mips_rounding`, `coverage`) exit 0. Test `tooie_demo1_upd_layout_test` verifies `pure_draw_accepts=1` in harness. Unit test models old policy vs new policy; no actual old binary unit test execution exit 1 is claimed.
- **Epistemic Boundaries & Status**:
  - Positive draw accept exists in unit harness; runtime TARGET window recorded `cacecx = 0` (expected when mixed DE498 scope is dropped; runtime cacecx is NOT promoted from 0).
  - Graphical QA of widescreen object-edge pop previously `NOT_TESTED` is now superseded by direct user visual confirmation: user launched `./build/TooieRecompiled` normally without flags in a previously affected gameplay area and confirmed: *"Sim, continua funcionando"*.
  - Classification: **`VISUALLY_CONFIRMED_FOR_TESTED_AREA`**, **`USER_REPORTED`**.
  - Epistemic scope: This confirms preservation of the observed visual expansion in the gameplay area tested by the user. Do NOT claim all maps, all objects, or global movement parity proven. Broader game parity remains an untested residual.
- **User Direction & Next Step**:
  - User explicitly chose KEEP VISUAL EXPANSION ACTIVE (feature stays default ON; candidate binary A6A2).
  - User visual test closes QA for the tested area. No new action needed on the bounded Demo 1 fix. User may have the game open; do not touch running processes.
  - Invariants maintained: P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio glitch deferral maintained.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
SCOPE_BOUNDARY_FIX: PROVEN (shared E3E8C broad scope dropped; allowlist pure-draw B97A4 kind 3 + preserved 8011447C kind 2; DE498 original v0)
DEFAULT_ON_TARGET_PARITY: PROVEN (355x12 trajectory words, 546 record words, 14 RNG snapshots, +0x60=0003006c exact original under default-ON binary A6A2)
DEFAULT_ON_PROJECTILE_PARITY: PROVEN (post 355 Fire Egg sweep hit v0=0x80132f60, CD6B4=1, 22 sweep + 21 move pairs exact under default-ON binary A6A2)
BUILD_AND_TEST_STATUS: PASS (ninja exit 0; layout/selector/projectile/parity/demo7/mips/coverage exit 0; pure_draw_accepts=1 in harness)
WIDESCREEN_EDGE_QA_STATUS: VISUALLY_CONFIRMED_FOR_TESTED_AREA (USER_REPORTED: "Sim, continua funcionando" on normal noflags launch; cacecx=0 in target window unpromoted; broader game/objects untested residual)
USER_CHOICE: RESOLVED (KEEP EXPANSION VISUAL ACTIVE; feature stays default ON; user visual QA completed for tested area)
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: extend oracle comparison to another attract demo when continuing broader fidelity
```

## Prior (2026-10-19) — USER_DEMO1_SOURCE_PROVEN_CALL_CHAIN_AND_GROK_ROUND2 (SUPERSEDED BY LANDED FIX)

Read [[plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-demo-movement-precision-diagnosis]], and [[plan/evidence/p08-attract-movement-residuals]].
Prior single-feature counterfactual causality documentation remains SUCCESS and canonical (`demo1-frustum-causality-checkpoint-gemini.json`): setting `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` as an experimental per-process flag restored 355 NPC prefix words, all 546 ActorRecord words, 14 RNG snapshots, and post 355 Fire Egg sweep hit (`v0 = 0x80132f60`, narrowphase `func_800CD6B4` LEAVE = 1).
User confirmed Demo 7 cutscene stopping is **VISUALLY FIXED** and Fire Egg miss persists on current default production binary (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`). User reaffirmed objective is general subtle gameplay/action parity across movement, kinematics, and state transitions, not an artificial aim or steering patch; no global clock hack.
**User Choice Resolved**: KEEP EXPANSION VISUAL ACTIVE while proper fix; **NO temporary OFF default**. Setting flag OFF (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`) is strictly an experimental per-process counterfactual diagnostic; it is NOT a production mitigation. Production invariants preserved: shader CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio glitch deferral maintained.

- **Source Findings & Gating Architecture (`PROVEN`)**:
  - Broadly treating `func_800E3E8C` (`E3E8C`) as DRAW scope allows aspect-expanded accept (`v0 = 1`) inside mixed renderer routine `func_800DE498` (`0x800DE6EC`).
  - `func_800DE498` stores `v0` to `0x8012C940`. If nonzero (accepted), it takes the visible path: executes registered pre-draw callback `0x800DE738: jalr $v0` with `$a0` loaded from `0x8012C9AC`, calls asset resolver `800D674C`, stores renderer processing context `s0+0x14/0x18/0x1C`, and emits display lists. If culled (`v0 == 0`), it branches to `0x800DE718`, calls cleanup helper `func_800DE318`, and returns 0.
  - **Renderer Context vs. Actor State**: Stores to `s0+0x14`, `s0+0x18`, and `s0+0x1C` are writes to the **renderer processing context** (`s0`), NOT actor state writes (`PROVEN` distinction; do not conflate renderer context with actor state).
  - **Registration Chain**: Standard actor draw entry `func_80101870` -> `func_80101808` calls `func_800DF47C(func_801015D0, arg0)`, registering callback `func_801015D0` at `0x8012C9A8` and `Actor*` at `0x8012C9AC`, then enters `func_800DE448` -> `func_800DE498`.
  - **Pre-Draw Dispatch**: Callback `func_801015D0` calls `func_80101934(Actor*)` at `0x80101784`, which indexes jump table `D_801243E8` via `(actor->unk74 & 0xF)`. Table entry `+0x04` invokes `func_80101A50(Actor*)`.
  - **RNG Stepping and Actor Mutation**: `func_80101A50` calls `func_800DC0C0` at `0x80101AA0` (RA `0x80101AA8`), advancing global LCG RNG seed `D_8012C7F0`. It tests random float `$f0 < D_80126434` (`0.03f`). On match, it directly mutates guest actor state byte `sb $t2, 0x7A($a2)` (setting bit `0x10` or `0x20` on `Actor->unk7A`). Both arms call `func_800DF744(1, ...)` and `func_800DF744(2, ...)`, writing to global renderer selector array `D_8012C850`.
  - **Epistemic Classification**: Semantic identity as facial/eye-blink animation is **`STRONG_INFERENCE`** (suggested by 3% probability check and selectors 1/2, but without explicit asset/animation binding proof in source). Control flow gating, global LCG RNG stepping, actor byte `0x7A` mutation, and renderer array writes are strictly **`PROVEN`**.
- **Paired Native Telemetry Arbitration (`PROVEN`)**:
  - Telemetry (`demo1-rng-paired-native-chain-gate.json`): Current has 32 RNG steps vs Oracle 27 across posts 79..84.
  - Sequence step gap grows monotonically across posts: Post 79: 0 (6 vs 6); Post 80: 0 (4 vs 4); Post 81: +1 (7 vs 6; **first extra step**); Post 82: +1 (5 vs 4); Post 83: +2 (6 vs 4); Post 84: +1 (4 vs 3). Cumulative delta = +5 steps across window.
  - LCG chain exhibits **ZERO breaks** across all observed steps and snapshots (`breaks: []`). Every transition satisfies $s' = (1664525 \times s + 1013904223) \pmod{2^{32}}$.
  - Expected oracle after-states, once calculated, never match actual observed stores once desynchronized.
  - First extra RNG call is definitively the **SECOND `func_80101A50` call in post 81 (seq 146 in Current, before `6239b638` -> after `036f8437`)**, NOT global `func_8001608C` (which executes in both runtimes).
  - Preceded directly by `cacecx` extra accept owner `func_800DE498` at seq 142 on position `(-2818, 300, -2237)`.
  - Later differences are a deterministic chain shift, not independent new consumers.
- **Native PC Mapper & Symbol Resolution (`PROVEN`)**:
  - Old `T/t`-only cache (`demo1-rng-ee14-nm.json`) was **INVALID and incomplete** (omitted recompiled weak `W/w` functions).
  - Completed `nm T t W w` (`demo1-rng-ee14-origin-resolution.json`) validated binary `EE14` (`ee14d6cb1c144a09ccec7f393dc2e986ac26fb0826893d2e0d490df9d19a683c`) with positive function sizes.
  - Return address is guest original `0x80101AA8` (`jal func_800DC0C0` at `0x80101AA0`).
- **Critical Operational & Quota Update**:
  - **NO SONNET**: Sonnet quota is expired. All further Sonnet calls are strictly forbidden.
  - **Haiku**: Approaching quota limit; reserved/unused now.
  - **Grok 4.7 Medium**: Designated **SOLE** code writer and test executor. Resumed implementation session `01a15118-c738-7dd3-98ed-cc5db5021bb5`: round 1 cap 50 exit 1 with source proof and no edit (`demo1-frustum-parity-grok-implementation-report.txt`); round 2 active budget 70 (`demo1-frustum-parity-grok-implementation-round2.json`, completed `end_turn` exit 0).
  - **Gemini**: Designated for **ALL** Markdown and documentation maintenance.
  - **Native Agent `demo7_capture`**: Read-only Grok review only; NO other capture authority.
  - **Opus**: DONE (read-only existing report `demo1-rng-consumer-opus.json`, not active job).
  - Current code EE14 diagnostics is NOT a fix; original default vis ON CURRENT palliative / FPS / audio unchanged.
- **Main Implementation Objective**:
  - Correct shared `func_800E3E8C` consumer selection to separate genuine visual DRAW passes from logic/simulation callbacks.
  - Preserve existing `func_8011447C` unless primary simulation harm proof is established (guest outparam write alone is NOT sim evidence).
  - Allowed pure DRAW writes: Gfx buffers, render-only temps.
  - `+0x60` semantic remains strictly UNKNOWN and is not necessary for this cause.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
SINGLE_FEATURE_CAUSALITY: PROVEN (TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0 restores 355x12 prefix, 39x14 record words, 14 RNG snapshots, and post 355 Fire Egg hit)
SOURCE_MECHANISM: PROVEN (shared E3E8C broad DRAW scope enables mixed DE498 gate; registered callback 801015D0 -> 80101934 -> D_801243E8 -> 80101A50 steps global LCG RNG D_8012C7F0, mutates actor byte 0x7A, writes D_8012C850 selectors via DF744)
SEMANTIC_CLASSIFICATION: facial/eye-blink animation is STRONG_INFERENCE; control flow, RNG step, byte 0x7A mutation, renderer writes are PROVEN
FIRST_EXTRA_RNG_CALL: PROVEN (second func_80101A50 in post 81 seq 146 curr, preceded by cacecx extra accept in func_800DE498 seq 142; NOT global 8001608C)
PAIRED_RNG_GAP: PROVEN (grows 1..4 across posts, 32 curr vs 27 oracle in posts 79..84; zero breaks in chain; oracle after calculated never matches observed store)
NATIVE_PC_MAPPER: PROVEN (old T/t cache INVALID; completed nm T t W w validated binary EE14 with positive sizes; RA guest 80101aa8)
USER_CHOICE: RESOLVED (KEEP EXPANSION VISUAL ACTIVE while proper fix; NO temp OFF default; env flag 0 is EXPERIMENT ONLY)
GROK_STATUS: COMPLETED (session 01a15118-c738-7dd3-98ed-cc5db5021bb5 completed end_turn exit 0, binary A6A2)
AGENT_ROLE_ROUTING: Grok PRIMARY code/tests; Gemini ALL Markdown; Haiku minimal/unused; Native demo7_capture read-only; Sonnet NO calls; Opus DONE
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: user visual check of widescreen edges in known affected gameplay area, feature ON; if fails narrower remaining mixed-render separation requires next task
```

## Prior (2026-10-18) — USER_DEMO1_OBJECT_FRUSTUM_COUNTERFACTUAL_CAUSALITY_PROVEN

Read [[plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-demo-movement-precision-diagnosis]], and [[plan/evidence/p08-attract-movement-residuals]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on the current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). User reaffirmed the objective is **general subtle gameplay/action parity** across movement, physics, and state, not an artificial aim or hit steering adjustment; no global clock hack.
User requires keeping the object visual pop fix production default ON (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`); shader CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio deferral maintained. Setting flag OFF (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`) is strictly an experimental per-process counterfactual diagnostic; NO production source or config change has been made.

- **Single-Feature Counterfactual Causality (PROVEN)**:
  - On the unchanged binary (`c56ce4e17b554020c36eec14a9e48861693679e9426161031e10aef4527a67e2`) with the TARGET logger (`TOOIE_DIAG_DEMO1_TARGET=1`), setting ONLY `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` (explicit-off getter at `boot_hw_stubs.cpp:4264..4267`, wrapper 4451 calling original `800CACEC` and returning before aspect-aware expansion; no other `TOOIE_TEST_*` flag combined; CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-object-original/target.csv`, SHA256 `1f43e7c570f21ec082bc7afac88cf01789cebbd8c2ec4e142f0aee9eb791f928`, 415 rows, 66.217s, exit 0/0, FINISH 357) restored:
    - All 355 `tgtrec` posts 2..356 x 12 raw words bit-exact with the original Angrylion oracle reference (`demo1-first-writer-oracle-2`).
    - All 14 pre/post snapshots across frames 80..86 x 39 ActorRecord words (546 words total) bit-exact with Oracle (`demo1-object-frustum-record39-comparison.json` is `[]` empty list).
    - Record `+0x60` high 16 bits: was `0004` (curr `0004006c`) under default aspect-aware ON vs original `0003` (`0003006c`); with flag OFF, it is now `0003` (`0003006c`), matching Oracle exactly.
    - All 14 raw RNG `D_8012C7F0` snapshots bit-exact with Oracle, completely resolving the PRE82 divergence ($k=4$ `036f8437` vs $k=5$ `c125942a`) and POST83 geometry divergence.
    - All 14 gamespeed `gs` words exact `0x3d4cccce`.
  - In comparison, default Current with aspect-aware ON had 274 posts (83..356) differing in position.
- **Actual Fire Egg Collision Restored (PROVEN)**:
  - In a dedicated PROJECTILE-only capture with the same flag OFF (`TOOIE_DIAG_DEMO1_PROJECTILE=1` and `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`, controller 1600930 / game 1600931, exit 0/0, 66.824s, 313 rows, FINISH 391, CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-current-object-original/projectile.csv`, SHA256 `3d54fe91ac1bc570a95a87241322f7d58593575aac951c4b4250d397a3344258`):
    - Actual Fire Egg sweep at post 355: `func_800CD820` sweep leave returns `v0 = 0x80132f60` (nonzero hit, matching Oracle `89a6bad1...`), whereas default Current returns `v0 = 0` (miss).
    - Narrowphase `func_800CD6B4` LEAVE = 1 on both off-current and Oracle; default Current has no such callback at 355.
    - All 22 sweep ENTER input pairs and 21 `sueggmove` ENTER input pairs are exact in `w0..w5` geometry and `w11` `gs`.
    - Sweep/move counts: 44 sweep / 42 move off/oracle vs default 64 / 62 (projectile stops at hit).
    - Bound centers at posts 354 and 355: all 6 selected `0x8008A018` LEAVE center triples exact with Oracle, including actual target post 355 `w0=c5af7c8d, w1=c401d0fa, w2=44a89e51` = `(-5615.56885, -519.265259, 1348.94739)`, completely restoring the prior ~427-unit bound center difference.
- **Causal Verdict & Epistemic Boundaries**:
  - `OBJECT_FRUSTUM_ASPECT_AWARE=1` is **CAUSAL** for observed RNG divergence, NPC trajectory divergence, and Fire Egg collision miss in USER_DEMO1.
  - The underlying call consumer and side-effects remain **UNKNOWN**.
  - Do NOT claim all NPCs globally fixed or physical fix done.
  - Do NOT globally REFUTE widescreen capability; update prior UNTESTED statement to PROVEN at feature level with mechanism UNKNOWN.
  - Source ABI and canonical getter roles from prior investigations are strictly preserved.
- **Background Workloads & Active Agents**:
  - Sonnet medium code job (`demo1-frustum-parity-fix-sonnet.json` / `report.txt`): RUNNING, code writer only, default acceptance capture ONLY after READY.
  - Grok field 60 old 30-turn audit exited with code 1 INCOMPLETE (`demo1-field60-and-rng-grok.json` and `.report.txt`), noted typo (Cpre82=Opost82) and found no field role; census stopped as feature-level causality is now established.
  - New main Grok READONLY job (`demo1-frustum-simulation-owner-grok.json` / `report.txt`): RUNNING bounded CACEC feedback; immutable baseline copy `demo1-frustum-default-baseline.cpp`.
  - Opus 5.5 medium READ-ONLY consumer design job (`demo1-rng-consumer-opus.json`): COMPLETED (exit 0, `is_error=false`, 14 turns, 242.8s). Proposed two entries (`func_800DC090` and `func_800DC0C0`) + writers (`800DC310`, `800DC324`, `800DC330`, `800DC354`), static coverage NOT gate tested. Opus used raw grep in violation of navigation policy, noted procedurally. Wide RNG trace proposal is now conditional, not automatically next because minimal counterfactual already isolated the feature.
- **Preserved Invariants**:
  - Demo 7 generator fix (`patches/runtime/n64recomp-zero-size-static.patch` and official 420-file regeneration) strictly preserved; user visual PASS maintained.
  - P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON.
  - Present FPS counter and Graphics options toggle remain verified.
  - RT64 below-native present decimator remains operational.
  - Audio glitch remains explicitly deferred per user directive.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
SINGLE_FEATURE_CAUSALITY: PROVEN (TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0 restores 355x12 prefix, 39x14 record words, 14 RNG snapshots, and post 355 Fire Egg hit)
UNDERLYING_MECHANISM: UNKNOWN (exact call consumer / side-effects of 800CACEC aspect-aware expand in simulation still unisolated)
CURRENT_CF_TARGET_CSV: /home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-object-original/target.csv (SHA256 1f43e7c5..., 415 rows, 56 updrec, FINISH 357)
CURRENT_CF_PROJECTILE_CSV: /home/wesley/.cache/tooie-fps-investigation/demo1-projectile-current-object-original/projectile.csv (SHA256 3d54fe91..., 313 rows, FINISH 391)
CF_TARGET_PREFIX_PARITY: 355/355_EXACT_MATCH (all 12 words bit-exact with Oracle 73c1faa...)
CF_RECORD_WORDS_PARITY: 546/546_EXACT_MATCH (14 snapshots x 39 words bit-exact; demo1-object-frustum-record39-comparison.json is [])
CF_RECORD_PLUS_60_STATUS: RESTORED_TO_ORACLE_0003006c (was 0004006c under default aspect-aware ON)
CF_RNG_D8012C7F0_STATUS: 14/14_EXACT_MATCH (all 14 snapshots bit-exact with Oracle; PRE82 split resolved)
CF_FIRE_EGG_HIT_STATUS: HIT_RESTORED_AT_POST355 (v0=0x80132f60, CD6B4=1, 22/21 sweep/move pairs exact, actual target center restored)
USER_DEMO7_STATUS: VISUALLY_FIXED (generator patch preserved)
USER_DEMO1_PRODUCTION_STATUS: USER_REPORTED_MISS (persists on production binary with default ON; OFF is experimental CF only)
WIDESCREEN_CAPABILITY_STATUS: NOT_GLOBALLY_REFUTED (prior UNTESTED statement updated to PROVEN at feature level with mechanism UNKNOWN)
ACTIVE_BACKGROUND_AGENTS: Sonnet job demo1-frustum-parity-fix-sonnet RUNNING; Grok job demo1-frustum-simulation-owner-grok RUNNING
COMPLETED_BACKGROUND_AGENTS: Opus job demo1-rng-consumer-opus DONE (exit 0, 14 turns; noted raw grep limit); Grok field60 exit 1 INCOMPLETE
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: owning-layer separation of expanded DRAW from original simulation, preserving visible object fix (awaiting Sonnet code writer / Grok simulation owner analysis)
```

## Prior (2026-10-18) — USER_DEMO1_FIRST_WRITER_PAIRED_CAPTURE_AND_RECORD_FIELD60_RNG_ANALYSIS (SUPERSEDED BY COUNTERFACTUAL CAUSAL PROOF)

Read [[plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-demo-movement-precision-diagnosis]], and [[plan/evidence/p08-attract-movement-residuals]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on the current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). User reaffirmed the objective is **general subtle gameplay/action parity** across movement, physics, and state, not an artificial aim or hit steering adjustment; no global clock hack. While the historical geometric miss was cataloged in earlier recomp studies (BASE `876d299f`, §16/17, margin +36.85), the fresh causal root on the current binary is strictly **UNKNOWN**.

- Prior Prefix Parity & Critical VI Cadence Wording (PROVEN & INDEPENDENTLY CONFIRMED):
  - Prior 420 exact non-clock player/input/camera/gs/anim/phys match at EXACT PC `0x800151BC` ENTRY (consumed 1..420) independently confirmed by Haiku (`demo1-actual-pair-haiku-report.txt`, 0 field mismatches across 420 paired rows).
  - VI handler cadence difference observed (100/419 transitions, +40 oracle entries); causal role in NPC/mob timing UNKNOWN/OPEN HYPOTHESIS, player 420 exact does NOT refute NPC timing; `--nospeedlimit` is logical only, not real-time proof. No global clock fix claim.
- Grok First NPC Prefix Audit Completed (DONE):
  - Grok prefix audit finished cleanly (`demo1-first-npc-prefix-grok-report.txt` and `.json`, 7 turns, stopReason `end_turn`, CLI exit 0, replacing earlier pending status).
  - 355 `tgtrec` pairs (posts 2..356) joined on post. Trajectory handle `0x80132f30` (post 2) with callback word `w3 = 0x80085C98`.
  - Ambiguous markers `0x80132f00` (post 28) and `0x80132f60` (post 140 projectile target) separated; `0x80132f60` is NOT the sampled trajectory.
  - Position words `w0..w2` match posts 2..82 (81 pairs). First position diff occurs at post 83 (delta `0.207508441`).
  - Word `w10` first differs at post 121 (`04311038` vs `04311048`), `w9` at 171, `w5..w7` at 215. `w11` (`gs`) raw exact on all 355 pairs.
  - In-memory mutation test verified: flipping bit 0 of `w0` at post 40 shifted the first diff to post 40.
- Preceding First-Writer Diagnostic Failure & Fix (PROVEN):
  - Initial first-writer capture yielded 0 `updrec` rows (diagnostic FAIL).
  - Causes:
    1. Host RA unreliability in recomp: generated code for `func_800FF62C` calls `LOOKUP_FUNC(0x80100368)` without setting `ctx->r31` immediately before jal; guest RA is stale/caller-dependent on host; RA range guard unsound in recomp.
    2. MARKER vs RECORD arg confusion: `func_80100368(a0)` receives actor RECORD (`s0` from `801067C4`), but wrapper checked `a0 == s_d1t_marker` (MarkerHandle), which never matched (`accepted_real_call_arg = 0`).
    3. Post dump mistake: flush dumped 39 words at the MarkerHandle address instead of resolving the actor record.
  - Corrected default-OFF via Sonnet: resolved record dynamically via `upd_expected_record`, direct-call SP scope (`ctx->r29 == scope_entry_r29 - 0x58`), post flush re-resolves record from marker (or emits `postmiss`).
  - Regression fixture `tests/tooie_demo1_upd_layout_test.cpp` on `demo1_tgtrec_ident_{current,oracle}.csv` (355x2 rows): old-wrong build exits 1 (FAIL: 0/355 accepted); new build exits 0 (PASS: 355/355 accepted), covering 5 relocations on Current and 7 on Oracle.
- Independent Haiku ABI Gate (PROVEN):
  - `demo1-writer-record-abi-haiku-report.txt` confirmed `func_80100368` receives actor RECORD in a0 and calls `*(record+0x10)` unchanged; marker `80132f30` is invalid as a0.
  - Direct-call SP scope (`ctx->r29 == scope_entry_r29 - 0x58`) correctly isolates direct calls from `800FF62C` while excluding nested callee calls.
  - Haiku mapper criticism corrected to `NOT_TESTED`; independent generated source `80106790` maps `marker+0x1A >> 5` through `vector_at(*D_80136EE0)`.
- NEW Actual FIRSTWRITER 2 Paired Capture Telemetry (PROVEN):
  - Current: `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-2/target.csv` (SHA256 `fbb2770bc1270762849aaf25f5dacefa4b18d04976ae8c3900618a6da652c75f`, controller 1584474, game 1584475, exit 0/0, 66.459s). Binary SHA256 `c56ce4e17b554020c36eec14a9e48861693679e9426161031e10aef4527a67e2`.
  - Oracle: `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-oracle-2/target.csv` (SHA256 `73c1faaace84937324c565889580c620513498971495a663ad356da70f06313c`, controller 1585301, instance 1585302, exit 0/0, 101.350s). Core SHA256 `4b782df4545ecb84a1aeee69c8159f893cfb503c5b38da78efb630d4baf8e199`.
  - Both captures produced exactly 415 valid rows (exit 0/0), 56 `updrec` rows (posts 80..86 x pre/post x 4 chunks of 10/10/10/9 words; 39 words total).
  - Finish diagnostics: `w5 = 0` (invalid bindings), `w7 = 7` (predumps emitted), `w8 = 0` (postmiss rows).
  - Baseline invariance: all 355 `tgtrec` posts 2..356 x 12 words match pre-writer TARGET2 baseline bit-for-bit.
- Proven Comparisons & Geometry Findings:
  - **Record +0x60 First Diff**: Already differs at first available sample PRE80: Current `0004006c` vs Oracle `0003006c` (difference in upper halfword `0004` vs `0003`). Semantic meaning is strictly **UNKNOWN**; do not promote field role to state/timer/pointer/fault without owning source. All other 38 words are **BIT-EXACT** through PRE83 inclusive!
  - **RNG `D_8012C7F0` First Diff & Step Distances**: Matches PRE/POST 80 and 81 (`1cf1d2b5`, `c91d98c4`, `28f1cd53`). Recurrence confirmed from generated source: $s' = (1664525 \times s + 1013904223) \pmod{2^{32}}$. Shared POST81 = `28f1cd53`. First sampled diff PRE82: Oracle $k=4$ (`036f8437`) vs Current $k=5$ (`c125942a`). Step distance offset (Oracle $\to$ Current) grows: PRE82 $+1$, POST82 $+1$, PRE83 $+2$, POST83 $+3$, PRE84 $+4$. These are LCG sequence step distances, NOT proven actual call counts until other writes to `D_8012C7F0` are excluded.
  - **Gamespeed `gs`**: All 14 sampled `gs` w11 bits match `0x3d4cccce` identically across both runtimes.
  - **Geometry First Diff**: POST83: +4/+8/+C (norm `0.207508441`), plus RAW +48/+54 (+48 cur `438c677a` vs ora `438cbb8e`, +54 cur `4389fee4` vs ora `438cbb8e`), cause UNKNOWN. Pre83 positions are exact.
  - **Entity Identity**: Tracked stream is FIRST BOUND OTHER ACTOR `0x80132f30` (callback `0x80085C98`); actual projectile target `0x80132f60` remains separate.
  - **Post Bracket Nuance**: Post bracket spans to the next loop getter call or logic exit; it can contain downstream actor prep/work; NOT an isolated-update proof.
- Epistemic Corrections & Active Workload:
  - **Sonnet Widescreen Inference Correction**: Sonnet's report asserted that because divergence occurs in pre-draw simulation state, widescreen changes are refuted. That inference is **INVALID**: prior-frame draw/visibility feedback has not been excluded, therefore a pre-draw difference alone cannot refute the hypothesis; no assertion is made that feedback actually occurs here. Widescreen feedback remains an **UNTESTED HYPOTHESIS**, NOT REFUTED.
  - Suspect `C401C` audio still needs proof; no canonical promotion.
  - Completed and active background jobs: Grok field 60 30-turn audit exited with code 1 INCOMPLETE; Opus 5.5 consumer design completed exit 0; Sonnet medium code job running; new main Grok simulation owner job running.
- Preserved Invariants:
  - Demo 7 generator fix (`patches/runtime/n64recomp-zero-size-static.patch` and official 420-file regeneration) strictly preserved; user visual PASS maintained.
  - No production fix or parity claim for Demo 1.
  - P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON.
  - Present FPS counter and Graphics options toggle remain verified.
  - RT64 below-native present decimator remains operational.
  - Audio glitch remains explicitly deferred per user directive.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
CURRENT_WRITER2_CSV_PATH: /home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-2/target.csv (415 rows, 56 updrec, FINISH 357)
ORACLE_WRITER2_CSV_PATH: /home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-oracle-2/target.csv (415 rows, 56 updrec, FINISH 357)
USER_DEMO7_STATUS: VISUALLY_FIXED (generator patch preserved)
USER_DEMO1_EGG_RESULT: USER_REPORTED_MISS (persists on user binary 87c5...)
USER_DEMO1_PREFIX_PARITY: 420/420_EXACT_MATCH (consumed 1..420 independently confirmed by Haiku)
FIRST_WRITER_DIAG_FIX: PROVEN (Sonnet host-RA / record-arg fix; 355x2 fixture old FAIL 1 / new PASS 0; covers relocations 5/7)
FIRST_WRITER_ABI_GATE: PROVEN (Haiku 80100368 record arg confirmed, direct SP scope confirmed; mapper criticism marked NOT_TESTED)
GROK_PREFIX_REPORT: DONE (end_turn 7 turns, CLI exit 0; replaces earlier pending; 355 tgtrec pairs, post 83 first pos diff)
WRITER2_CAPTURE_STATUS: PROVEN (valid 415/415 rows, exit 0/0, 56 updrec, invalid_binding=0, predumps=7, postmiss=0)
RECORD_PLUS_60_DIFF: PROVEN (current 0004006c vs orig 0003006c already PRE80; meaning UNKNOWN; other 38 words exact thru PRE83)
RNG_D8012C7F0_DIFF: PROVEN (POST81 same 28f1cd53; LCG 1664525x+1013904223u32; orig PRE82 k=4 / curr k=5; step offset PRE83 +2, POST83 +3, PRE84 +4; sequence distances NOT proven call counts)
GS_W11_STATUS: BIT_EXACT (all 14 sampled gs w11 bits match 3d4cccce)
GEOMETRY_FIRST_DIFF: POST83 (+4/+8/+C pos norm 0.207508441, RAW +48/+54; pre83 positions exact; cause UNKNOWN)
TRACKED_ACTOR_IDENTITY: FIRST BOUND OTHER ACTOR 80132f30 (callback 80085C98; actual shot candidate 80132f60 remains separate)
SONNET_WIDESCREEN_CLAIM: CORRECTED (predraw diff refuting widescreen was INVALID INFERENCE; prior-frame draw feedback UNTESTED not REFUTED)
ACTIVE_BACKGROUND_AGENTS: Grok job demo1-field60-and-rng-grok RUNNING; Opus job demo1-rng-consumer-opus RUNNING (both NOTPASS; no cause/fix)
PRESERVED_STATE: Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: trace RNG consumers between POST81 and PRE82 (read-only design, no cause/fix)
```

## Prior (2026-10-18) — USER_DEMO1_PROJECTILE_DIAGNOSTIC_AND_LIVENESS_TRACKING (SUPERSEDED BY PAIR 2 & 3 SWEEP/TARGET DIAGNOSTICS)

Read [[plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-demo-movement-precision-diagnosis]], and [[plan/evidence/p08-attract-movement-residuals]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on the current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). User reaffirmed the objective is **general subtle gameplay/action parity** across movement, physics, and state, not an artificial aim or hit steering adjustment; no global clock hack. While the historical geometric miss was cataloged in earlier recomp studies (BASE `876d299f`, §16/17, margin +36.85), the fresh causal root on the current binary is strictly **UNKNOWN**.

- Prior Prefix Parity (PROVEN & INDEPENDENTLY CONFIRMED):
  - Prior 420 exact non-clock player/input/camera/gs/anim/phys match at EXACT PC `0x800151BC` ENTRY (consumed 1..420) independently confirmed by Haiku (`demo1-actual-pair-haiku-report.txt`, 0 field mismatches across 420 paired rows).
  - VI cadence delta (100/419 transitions) and +40 oracle handler entries are NOT proven root cause; possible NPC/timer cadence remains an OPEN HYPOTHESIS; `--nospeedlimit` is logical only, not wall time.
- Sonnet Projectile Diagnostic Implementation (PROVEN):
  - Sonnet session completed with `is_error=false` (report `demo1-projectile-diagnostic-sonnet-report.txt`, JSON `demo1-projectile-diagnostic-sonnet.json`). Build exit 0, all unit/regression tests reported 0.
  - Isolated default-OFF flags: `TOOIE_DIAG_DEMO1_PROJECTILE=1` (Tooie) and `P08_DEMO1_PROJECTILE=1` with `P08_DEMO1_PROJECTILE_CSV` (Angrylion oracle). No typos (exact flag names verified).
  - Five entry/return hook points: `_baeggsetup_entrypoint_5` @ `0x80084A60`, `_cheggfire_entrypoint_2` @ `0x800871F0`, `_sueggmove_entrypoint_9` @ `0x8008A480`, `_suegg_entrypoint_9` @ `0x8008A3C0`, and core2 sweep `func_800CD820`. Plus per-frame `objpoll` (37 columns) for overlay-relative callback tracking.
  - Binary SHA256: `af0723b4b7f802446b455401b1c9e83a50aa61772ec37a598aa451df7f666705`. Oracle core SHA256: `41b59e2802be5782a2f3ab4f685bbad2fee062bd4da72e8ee01c6d6c4b7ebe88`. Diagnostic ONLY; no production behavior fix.
- Haiku Static Gate Report (`demo1-projectile-haiku-gate-report.txt`):
  - Evaluated static code: ABI, exactly-once original call, ctx/reg preservation, 37-col schema, and default-OFF gate: PASS.
  - Coverage caveat: gate validates direct and `LOOKUP_FUNC` sites; indirect calls via overlay-relative descriptor/callback pointers (`0x808xxxxx`) are NOT guaranteed to be intercepted by Tooie wrappers (which is why `objpoll` exists). Missing callback alone is NOT proof of divergence.
- Owned Controlled Projectile Captures (PROVEN):
  - Current Tooie: Controller PID 1282788, game PID/pgid 1282791, elapsed 66.243s, exit 0/0, FINISH post 391, 189 data rows + 1 header. CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-current/projectile.csv` (SHA256 `6b5686b77b723180f08954d3573be31d73294140b22815ae39f90725432775c7`). Flag ONLY `TOOIE_DIAG_DEMO1_PROJECTILE=1`; user game check passed. Counts: setup 1 pair, fire 1 pair (post 333), movement 31 pairs, sweep 32 pairs, objpoll 57.
  - Pinned Angrylion Oracle: Controller PID 1283584, game PID/pgid 1283587, elapsed 94.041s, exit 0/0, FINISH post 391, fresh isolated config/states/saves, `--nospeedlimit` (PERTURBED_LOGICAL_ONLY). 92 data rows + 1 header. CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-oracle/projectile.csv` (SHA256 `8bd18fd1444d6f8f6305b200f0c5afc68d0401739b6c4f10af29d42261568aab`). Counts: setup 1 pair, fire 2 enter / 1 leave, movement 7 pairs, sweep 7 pairs, objpoll 57. Controller JSONLs record exact cmd/env/hashes.
- Bounded Telemetry Findings & Epistemic Boundaries (PROVEN):
  - `setup_enter`/`leave` and `fire spawn_enter`/`leave` raw payloads are bit-exact between Tooie and Oracle.
  - All 7 actually matched movement and sweep pairs (`post 333..340`) have bit-exact raw payloads.
  - All 7 oracle sweep leave rows show `v0 = 0` (same as Current: no hit detected in this window). Baseline no-hit proven for these frames.
  - First raw `objpoll` difference occurs at `post 341` (~40 XYZ difference), BUT on Oracle the old tracked pointer `0x801d0a2c` freezes at `341..342`, exhibits a heap link at `343`, and then MIPS code words at `388`.
  - Object liveness, relocation, or free status is UNKNOWN (possible DEFRAG HYPOTHESIS); this is NOT proven as a first gameplay divergence! Current tracked pointer `0x801c767c` continues movement normally.
  - Snapshot UNKNOWN: target/muzzle final normals and callback delta-time are not captured (`w11` unused). Do not infer VI root.
- Background Workloads & Active Agents:
  - Prior Grok main 40-turn audit PID 1274832 was interrupted via SIGINT after 13 read-only turns (redundant gates before runtime); supervisor PID 1270040 was terminated via SIGTERM. Classification: `INTERRUPTED_REDUNDANT_AUDIT_DIRECT_CAPTURE` (not PASS). Raw JSON may be empty, source transcript preserved. Not active.
  - Grok data-only job PID 1284587 (max 12 turns, outputs `demo1-projectile-compare-grok.json` and `report.txt`), INCOMPLETE.
  - Sonnet liveness + callback DT diagnostic job PID 1286150 (outputs `demo1-projectile-liveness-sonnet.json` and `report.txt`), COMPLETED.
- Preserved Invariants:
  - Demo 7 generator fix (`patches/runtime/n64recomp-zero-size-static.patch` and official 420-file regeneration) strictly preserved; user visual PASS maintained.
  - User observation that FireEgg still MISSES on user build `87c5...` preserved.
  - P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON.
  - Present FPS counter and Graphics options toggle remain verified.
  - RT64 below-native present decimator remains operational.
  - Audio glitch remains explicitly deferred per user directive.
  - No global clock, aim, or collision hypotheses promoted to specs.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
CURRENT_PROJECTILE_BINARY_SHA256: af0723b4b7f802446b455401b1c9e83a50aa61772ec37a598aa451df7f666705
ORACLE_CORE_SHA256: 41b59e2802be5782a2f3ab4f685bbad2fee062bd4da72e8ee01c6d6c4b7ebe88
CURRENT_PROJECTILE_CSV_SHA256: 6b5686b77b723180f08954d3573be31d73294140b22815ae39f90725432775c7 (189 rows)
ORACLE_PROJECTILE_CSV_SHA256: 8bd18fd1444d6f8f6305b200f0c5afc68d0401739b6c4f10af29d42261568aab (92 rows)
USER_DEMO7_STATUS: VISUALLY_FIXED (generator patch preserved)
USER_DEMO1_EGG_RESULT: USER_REPORTED_MISS (persists on user binary 87c5...)
USER_DEMO1_PREFIX_PARITY: 420/420_EXACT_MATCH (consumed 1..420 independently confirmed by Haiku)
PROJECTILE_SPAWN_AND_EARLY_SWEEPS: BIT_EXACT_MATCH (setup/fire/sweeps 333..340 matched; sweep leave v0=0)
ORACLE_TRACKED_PTR_FREEZE_AT_341: PROVEN (0x801d0a2c freezes 341..342, heap link 343; liveness/reloc UNKNOWN)
COLLISION_HIT_STATUS: UNKNOWN (baseline no-hit proven for early window; overall hit UNKNOWN)
ACTIVE_BACKGROUND_AGENTS: PID 1284587 (Grok compare), PID 1286150 (Sonnet liveness+DT diag)
PRESERVED_STATE: Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: await liveness diagnostic gates and fresh bounded paired capture using stable identity; not global aim/clock fix
```

## Prior (2026-10-18) — USER_DEMO1_PAIRED_PARITY_PREFIX_MATCH_AND_HIT_UNKNOWN (SUPERSEDED BY PROJECTILE CAPTURE)

Read [[plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-demo-movement-precision-diagnosis]], and [[plan/evidence/p08-attract-movement-residuals]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on the current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). User clarified the latest objective is **general subtle gameplay/action parity** across movement, physics, and state, not an artificial aim or hit steering adjustment; no global clock hack. While the historical geometric miss was cataloged in earlier recomp studies (BASE `876d299f`, §16/17, margin +36.85), the fresh causal root on the current binary is strictly **UNKNOWN**.

- Diagnostic Implementation & Build Verification (PROVEN):
  - Sonnet CLI session 22383 implemented isolated, default-OFF flags `TOOIE_DIAG_DEMO1_PARITY=1` and `P08_DEMO1_PARITY=1` mirrored at EXACT PC `0x800151BC` ENTRY, stage 0 sub 0, glidx `0..420` (max 421 rows), player 0 only.
  - Schema covers 62 columns: cumulative guest VI handler entries `0x8001443C` and logic counter, raw 6-byte pad (`r0..r5`), precise 9-group fields (`px,py,pz`, `vx,vy,vz`, `ptype`, `velxz`, `tspd`, `yaw`, `ideal`, `phys_yaw`, `bs_prev`/`cur`/`next`, `anim`, camera `camx,camy,camz,camctrl,sel`, gamespeed `gs`).
  - Unit test `tests/tooie_demo1_parity_test` passed exit 0; ninja build exit 0 (diagnostic binary SHA256 `5b6fc6d069f00411cfd2ade4b94d7fa15055a44206bc100ffbd5ba569af31bc6`); oracle core rebuild exit 0 (lib SHA256 `c2a9e7fa7a3476c4a6eacb35bedc30c3d8128c16a28e190fbf0c0e96c05d9c04`).
- Controlled Paired Captures (PROVEN):
  - Current Tooie: Controller PID 1073249, game PID/pgid 1073258, elapsed 67.679s, exit 0, reason `window_consumed_420`. Console log `/home/wesley/.cache/tooie-fps-investigation/demo1-parity-current/console.log` (501,453 bytes, SHA256 `57920a649162c7bec4b17dd3d0e86ef41eed0fb813c98008a6323e5fe83492aa`). Header + 420 data rows.
  - Pinned Angrylion Oracle: Controller PID 1074236, oracle PID/pgid 1074257, elapsed 98.713s, exit 0, reason `window_consumed_420`. CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-parity-oracle/demo1.csv` (179,297 bytes, SHA256 `fa45b8d202a791eba76b6ef0f2b20265ed2297cddc25eb0c90b11811f55736e3`). Header + 420 data rows. Wall time under `--nospeedlimit` is perturbed; logical parity only.
  - Owned capture processes exited cleanly; no processes relaunched; no user processes touched.
- Paired Comparator Verdict & Divergence Analysis (PROVEN):
  - Evaluated via `demo1-parity-compare.py` -> `demo1-parity-compare.json` and Grok final paired report `demo1-paired-main-grok-report.txt`.
  - **420 exact non-clock matched pairs** (consumed 1..420). Both sides skip only consumed 0 (post 1), confirming identical hook boundary without port split.
  - Zero 1-ULP differences, zero persistent geometric or action differences across sampled player and camera fields from consumed 1 through 420: raw `r0..r5`, `gs_bits` (matches at every consumed: `0x3D4CCCCE` = 0.05 and `0x3D088889` = 0.03333, including C-up row 322), `yaw`, `ideal`, `phys_yaw`, `px,py,pz` and bit words, `vx,vy,vz` and bit words, `velxz`, `tspd`, `accel`, `mult`, `ptype`, `bs_prev/cur/next`, `anim_id/timer/dur/phase`, `camx/camy/camz, camctrl, sel`.
  - **First in-window divergence**: VI-handler entry cadence (`0x8001443C`). In-window delta-VI mismatches on 100 of 419 transitions. First step consumed 1->2: Tooie dvi 0 (vi 3134), oracle dvi 6 (vi 3414->3420). Cumulative VI growth consumed 1->420: Tooie +847, oracle +887 (+40 entries). This is an emulator/runtime entry cadence difference under `--nospeedlimit`; it did NOT move any sampled guest fields.
  - Verdict: `player/input/camera/gs prefix MATCH through consumed 420. HIT UNKNOWN. No code fix. Not PASS. Not a visual-miss proof. Not a realtime proof. No aim, hit, or clock change.`
- Fire-Edge & Projectile Dispatch Telemetry:
  - Consumed 322 / post 323 (buttons `0x2008` = C-up with Z, stick `(-9, 84)`): player entry pose matches bit-exact between Tooie and oracle (`px,py,pz = -4692.11475, -426.484711, 1129.06921`, `yaw = 291.797241°`, `velxz = 287.608307`, `bs_cur = 9`, `anim_id = 42`, `cam = -4086.1438, -160.789032, 950.552246`). This entry pose differs from the later SHOT leave pose on `87c5...` due to sample phase and binary.
  - Oracle append tail (`/tmp/p08-angrylion-attract-eggs.log`, 56,989 bytes, 426 lines) reveals `cheggfire` sys dispatch occurs at raw post 333 (consumed 332): `800871f0` overlay 535 ep 2 (`obj64 = c5932bf8` [-4709.5], `obj6c = 448e0aee` [1136.34]).
  - Actor low-9 masking (`+0x6C & 0x1FF`) remains strictly INVALID.
- Independent Audits & Preserved Disagreements:
  - Grok Contract Audit (`workload-grok-parity-audit-report.txt`): PID 1065434 remains ACTIVE in background (user explicitly requested keeping agents running across quota resets). Audited compiler flags (`RecompiledFuncs` with `RECOMP_MIPS_ROUND_COMPAT=1`; `TooieRecompiled` without it), lookup table binding mechanism, native libm math overrides (`800136E4`, `800138D0`, `80013970`), stick helpers (`800BCCC0`, `800BCCD8`), and hypotheses H1-H4. Differentiated diagnostic binary `5b6fc6d0` from user-reported miss binary `87c5a08b`.
  - Haiku Capture Audit (`workload-haiku-capture-audit-report.txt`): 20 static gates G1-G20 evaluated. Confirmed 62-column schema parity. Flagged G15 oracle CSV append mode risk (successfully averted by Grok's fresh path) and G14 run-tagging gap.
- Active & Historical Process Tracking:
  - Active background agent: PID 1065434 (Grok independent contract audit).
  - Historical exited PIDs: Driver PID 1067821, Grok paired runner PID 1067822, Tooie controller PID 1073249 / game PID 1073258, Oracle controller PID 1074236 / oracle PID 1074257, earlier SHOT capture PID 1059296, user game PID 1050817.
- Preserved Prior State:
  - Demo 7 generator fix (`patches/runtime/n64recomp-zero-size-static.patch` and official 420-file regeneration) strictly preserved; user visual PASS maintained.
  - User observation that FireEgg still MISSES on current build `87c5...` preserved.
  - P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON.
  - Present FPS counter and Graphics options toggle remain verified.
  - RT64 below-native present decimator remains operational.
  - Audio glitch remains explicitly deferred per user directive.
  - No global clock, aim, or collision hypotheses promoted to specs.

`NEXT_SINGLE_ACTION`: same-phase bounded diagnostic on both Tooie and pinned oracle around raw post 333 (the cheggfire line) at `800871f0` overlay 535 entrypoint 2 and sweep callee identity/radius/result word, default OFF on both sides, without clock/aim/hit changes.

## Prior (2026-10-17) — USER_DEMO7_FIX_PROVEN_AND_VERIFIED

Read [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-attract-movement-residuals]], [[plan/evidence/p08-provisional-follow-camera-and-egg-differential]], and [[plan/evidence/p09-fps-counter-and-settings-toggle]].
The user-observed player drift during Demo 7 in the attract sequence (TDL Stomping Plains, stage 0 sub 8) has been forensically isolated, root-caused, and resolved at the owning generator layer without hand-editing generated code. Independent final gate evaluation by Grok on fixed runtime capture `demo7-fixed-root.log` (121 rows, controller 396.13s) confirms **Criterion 1 (Bounded Demo 7 Drift) PASS**. A 90.032s serial timing test (`timing-fixed-pid-serial-90s.log`) with PID-tagged serialization fix in `src/game/tooie_diag.cpp` verified guest VI cadence at ~60.012 Hz (NO evidence of a 2x VI clock in the measured window). Prior FPS counter and settings toggle, Phase 1 default CURRENT `tfPos` workaround, RT64 below-native decimator, and audio glitch deferral remain strictly preserved.

- Root Cause Discovery & Verification (PROVEN):
  - In original N64 hardware, entering State 116 (`0x74`, `bsrest` overlay 84) calls INIT ep9 (`0x808005B8`) -> `0x8080020C` -> `0x80800308`, which calls the static stop helper at `0x80800000` (`static_344_80800000`). This helper invokes `baphysics_set_target_horizontal_velocity(ps, 0)` (`tspd = 0.0`) and clears velocity vectors to `(0, 0, 0)`.
  - In recompiled C (`RecompiledFuncs/funcs_416.c:4188`), `static_344_80800000` was emitted with an **EMPTY BODY** `;}` due to a static function end-boundary defect in `N64Recomp/src/main.cpp`. The decompilation ELF contained zero-size NOTYPE markers (`bsrest_VRAM` and `bsrest_TEXT_START`) at `0x80800000`, causing `cur_func_end == static_func_addr` and truncating the helper body. Sibling `static_304_80800324` in `.bsledge` was similarly empty.
  - The missing stop helper caused Banjo to retain pre-cinematic walking momentum (`tspd = 500.0`, `velxz ~ 514..520`) throughout the overhead cinematic slide.
- Generator Fix & Official Tooling Regeneration (PROVEN):
  - Fixed `N64Recomp/src/main.cpp`: skips section function symbols equal to the static address that are zero-sized in that section; adds bounds checks before indexing.
  - Versioned patch: `patches/runtime/n64recomp-zero-size-static.patch` (SHA256 `48ca1e7710a7b6ecd2f4cfe3c8ac064c11b2307ad98f35ee53b70b3f1cfebb58`), registered in `tools/apply_runtime_patches.sh`.
  - Regression test: `tests/n64recomp_zero_size_static_test.sh` exits 0 PASS on patched generator, exits 1 FAIL on old generator.
  - Decompilation inputs restored: pinned fork `25033be6ec9e35975483f064d086391d2381ca4c`, ROM SHA1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, matching ELF `banjotooie_decompressed.elf` (SHA1 `2b2d04c5b6484594f750b097daf670f7a8a24167`, size 91855676).
  - Full regeneration executed via official tooling (`tools/run_recomp.sh` / N64Recomp); all 420 files regenerated; exactly 5 files changed (2 restored helpers in `funcs_416.c`, 4 rounding aliases semantically identical).
  - Zero hand-editing of generated code. Full build `ninja -C build TooieRecompiled tooie_demo7_drift_test` exit 0; `python3 tools/check_generated_fresh.py` exit 0.
- Independent Grok Final Gate Evaluation (PROVEN):
  - Fixed capture: `demo7-fixed-root.log` (121 rows `glidx 0..120`, controller 396.13s wall time).
  - **Criterion 1 (Bounded Demo 7 Drift)**: **PASS**.
    - 119 pose pairs ($i \to i+1$, $i=1..119$): 0 tolerance failures (pos/vel/cam $\le 0.009$, anim $\le 0.00051$, tspd $\le 0.051$). Max error pos 3D: 0.005771, vel 3D: 0.006328, cam 3D: 0.008015, tspd: 0.044263.
    - Pad inputs match same-index $i \to i$ 120/120.
    - Sequence 11 `tspd_set a1=0` nested inside sequence 10 `bs_setState(0x74)` verified on trace (pre `tspd = 500.0`, post `tspd = 0.0`, leave `vel = (0, 0, 0)`).
    - Stationarity (`glidx 38..120`, 83 rows): `bs_cur = 116`, `tspd = 0.0`, position fixed at `(-80.46, 0.00, 4496.25)`, `velxz = 0.0`.
- Second User Scope (Criterion 2 — Global Real-Time Speed & Cadence):
  - **Status: NOT YET PROVEN RESOLVED / BOUNDED CADENCE CONFIRMED**.
  - One matched bounded scene does not certify global speed. Raw gamespeed snapshots across sample phases (0.05 vs 0.03333 on 42/119 rows) reflect sample-phase differences between hook sites, not a per-update integration error.
  - Serial 90.032s timing run (`timing-fixed-pid-serial-90s.log`) with PID-tagged serialization fix (`g_timing_mu` in `src/game/tooie_diag.cpp`, build exit 0) completed with process exit 0 (owned instance closed).
  - Guest execution stream (PID 938341) over 59.038s delta interval (wall 20.004s..79.042s) measured 3543 VIs => **60.0121955 Hz** and 1435 logic updates => **24.3063789 Hz** across changing scenes (PID 938331 had 0 VIs).
  - Bounded conclusion: **NO evidence of a 2x VI clock** in the measured window. This does NOT prove universal camera/mob/campaign timing parity; user-specific non-Demo7 velocity reproduction questions remain open. Production bug is CLOSED for the tested window.
- Preserved Prior State:
  - Present FPS counter and Graphics options toggle remain verified and ready for gameplay QA.
  - Phase 1 default CURRENT `tfPos` workaround remains default-ON; continuous interpolation remains not implemented.
  - Phase 2 below-native decimator in RT64 remains operational.
  - Audio glitch remains explicitly DEFERRED per user directive.

`NEXT_SINGLE_ACTION`: normal clean user visual QA Demo 7, and concrete non-Demo 7 speed reproduction if persists.

## Prior (2026-10-17) — USER_DEMO7_STATE116_AND_BOUNDED_DIAGNOSTIC_VERIFIED (SUPERSEDED BY DEMO 7 FIX)

Read [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-attract-movement-residuals]], [[plan/evidence/p08-provisional-follow-camera-and-egg-differential]], and [[plan/evidence/p09-fps-counter-and-settings-toggle]].
Bounded diagnostic capture (121 rows `gl0..120`) executed by root and verified. Sonnet primary MIPS disassembly of `bs_setState` (`0x8009E5D8`) proved `*(player + 0x120)` maps `+0: prev`, `+4: cur`, `+8: next`, correcting prior claims that `bs = 4` was current state. During the hands-off slide, Banjo executes State 116 (`0x74`, `_bsrest_entrypoint_12`) with `anim = 111` while `bs_prev = 4`. Angrylion oracle Phase 2 tick-start sampler (`0x800151BC`) was run. Expanded user criteria separate Demo 7 cutscene parity from global real-time speed preservation. Prior FPS counter and settings toggle completion, Phase 1 default CURRENT `tfPos` workaround, below-native decimator, and audio glitch deferral remain strictly preserved.


- State Struct Layout & Kinematics Correction (CORRECTED/SUPERSEDED & PROVEN):
  - Prior claim that Banjo was executing state `bs = 4` during the overhead cinematic slide is CORRECTED/SUPERSEDED.
  - Original Tooie MIPS disassembly of `bs_setState` at `0x8009E5D8` (`funcs_350.c:1434-1551`) maps `*(player + 0x120)` as: `+0: prev`, `+4: cur`, `+8: next`.
  - Legacy `[demo-sync]` field logged `MEM_W(player + 0x120)` (+0 = `bs_prev`), which read 4 while current state was actually `bs_cur = 116` (`0x74`). Do NOT call state 4 cinematic.
  - `glidx 35..37`: `bs_prev = 2`, `bs_cur = 4`.
  - `glidx 38`: `bs_prev = 4`, `bs_cur = 116` (`0x74`), `ptype = 2`, `tspd = 500.0`, `anim = 111`, `velxz = 441.46`.
  - `glidx 50..120`: `bs_cur = 116` persists with neutral stick / remap (`(0, 0)`, ang 90, dist 0); `tspd = 500.0`; `velxz ~ 514..520`; world position integrates continuously across XZ; Y reaches terrain floor -200.00 at `glidx = 106..120`.
- State 116 (`0x74`) Source Proof (PROVEN from ROM / MIPS):
  - Stride 12 table `D_80117F38` entry 116 word +0 is `0x800839A8` = `_bsrest_entrypoint_12` (overlay 84 `bsrest`, ROM `0x01F33AF0`, RAM `0x80800000`).
  - Method table at `0x80800930`: `[0]` ep11 END (`0x80800660`), `[1]` ep9 INIT (`0x808005B8`), `[2]` ep10 UPDATE (`0x808005F8`), `[3]` core2 interrupt (`0x80099B94`).
  - Physics invariance: NONE of INIT, UPDATE, or END resets `tspd` (`phys + 0x3C`), `velocity` (`phys + 0x10`), or `ptype` (`phys + 0x00`).
  - Exit gate in UPDATE (ep10): checks `0x8008E35C` (state 119) or `0x8080032C`. For anim 111, `0x8080032C` returns 1 (permitting exit) ONLY if state stack top `0x8009CC68(player) == 0` (`depth = (*(ps+0xF0))+0x80`, top at `ptr + depth*16 - 16`).
  - Persistent 116 may be original scripted cinema; do not promote a faulty stack or exit bug without original oracle baseline. Fix layer UNKNOWN.
- Isolated Bounded Diagnostic & Root Capture Verification (PROVEN):
  - Sonnet implemented DEFAULT-OFF isolated flag `TOOIE_DIAG_DEMO7_DRIFT=1`, avoiding 400k-line diagnostic cascades. Hooked at overlay 698 ep0 exit, stage 0 sub 8 glidx 0..120, player 0 only, max 121 rows. Host test `tooie_demo7_drift_test` exits 0 PASS.
  - Root full build exit 0 (`demo7-root-full-build.log`) and bounded capture completed (`demo7-bounded-root.log`, 121 rows `gl0..120`, controller 407.21s, clean PID exit).
  - Kinematics in bounded capture match prior heavy capture (`velxz * gs` physical integration); no production fix yet.
- Logging Nuance & Map Clarification:
  - Map field `MEM_H(0x10, 0x80127630)` clears to 0 during mode-5 body, but exact guest clearing writer is untraced (categorical claims of after-warp consumption mechanisms removed absent primary proof). Filtering `map == 282` drops 100% of body.
  - Raw `cammode` halfword at record + 0x16 overlaps lower half of float at +0x14; contaminated. Viewport camera at `0x8012D500` cleanly marks overhead cut at `glidx = 38`.
- Oracle Parity Status:
  - Phase 1 run (`P08_DEMO7_SUB8=1`, `demo7-oracle-isolated/demo7.csv`) yielded 5 rows (0, 33..36) because PC `0x8009C914` was not called per-frame and output buffered; SIGINT 802.31s normal 0 (`NOT PASS`). Oracle row `idx + 1` aligns with Tooie position/velocity before 116 (pos max error 0.0058, vel max error 0.0064 rounded), though anim timers/durations show possible phase difference.
  - Sonnet corrected instrumentation (`tools/p08_attract_egg_hook.h` and mupen patch) to single per-logic PC `0x800151BC` tick-start sampler with `fflush()` per row, no yaw mix.
  - Phase 2 capture IN PROGRESS (`demo7-oracle-phase2/demo7.csv`, PID 750322, `--nospeedlimit`). Do NOT claim results until finished.
- Expanded Objective & Two Acceptance Criteria:
  - (1) Faithful Demo 7 stationary cinema parity versus original N64 hardware.
  - (2) Preserve guest logic/VI/motion/animation/camera real-time speed, not FPS poses (running 2 instances diagnostics with `--nospeedlimit` does NOT prove wall throughput). No global fix from 1 map.
- Preserved Prior State:
  - Present FPS counter and Graphics options toggle remain verified and ready for gameplay QA.
  - Phase 1 default CURRENT `tfPos` workaround remains default-ON; continuous interpolation remains not implemented.
  - Phase 2 below-native decimator in RT64 remains operational.
  - Audio glitch remains explicitly DEFERRED per user directive.

`NEXT_SINGLE_ACTION`: finish Phase 2 original Angrylion oracle capture (`demo7-oracle-phase2/demo7.csv`) and compare first divergence by validated phase against root bounded capture state 116 world position, velocity, ptype, tspd, anim, camera, and gs; choose owning layer and faithful fix only after comparative evidence.

## Prior (2026-10-17) — USER_DEMO7_CUTSCENE_DRIFT_INVESTIGATION_BOUNDED (SUPERSEDED BY STATE 116 FINDINGS)

Read [[plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[plan/evidence/p08-attract-movement-residuals]], [[plan/evidence/p08-provisional-follow-camera-and-egg-differential]], and [[plan/evidence/p09-fps-counter-and-settings-toggle]].
User diagnostic capture analyzed via offline streaming parser (provenance: user capture `demo7-user-capture.log` SHA256 `74f859128f586306626df1eb964e161983c6c916c9509c14b04ff8b577d99467`, 407,919 lines; analysis dump `demo7-user-capture-analysis.txt`; verdict `demo7-user-capture-verdict.txt`; Grok CLI session `01a14b7c-c6ef-7bd3-9a0b-3fef4d99acf4`, exit 0). No code, build, or runtime execution this task (`PARSED_NOT_A_GAME_PASS`; zero tests reported `PASS`). Prior FPS counter and settings toggle completion, Phase 1 default CURRENT `tfPos` workaround completion, below-native decimator completion, and audio glitch deferral remain strictly preserved.

- Scene Mapping & Logger Nuance (PROVEN):
  - Spoken user sequence: `intro -> Demo 1, 2, 3 -> intro -> Demo 4, 5, 6 -> intro -> Demo 7, 8, 9` (hands off keyboard/controller).
  - Independent gate: 6,607 `[demo-sync]` lines parsed, 0 unparsed. Stage 0, sub 8 has 1,244 consecutive rows (`glidx 1..1244`, `cnt = 1245` on every line), 0 gaps, 0 skips, `tick_delta == 2`.
  - Logger map field (`MEM_H(0x10, 0x80127630)` at `0x80127640`) is live on boundary tails (278, 314, 395, 296, 185, 264), but clears to 0 during mode-5 clip body after warp consumption. Filtering `map == 282` drops 100% of the Demo 7 body. Exact guest clearing writer not traced.
  - Scene identity is confirmed by `stage = 0, sub = 8` in registry order, preceded by intro sub 7 (`map = 395`) and followed by Demo 8 sub 9 (`map = 264`, join at glidx 1245 has 0.00 XZ jump).
  - Raw `cammode` at record + 0x16 overlaps half of float at +0x14 (only 3/1244 values in [-1, 64]); contaminated. Viewport camera (`0x8012D500`) cleanly marks overhead cut at `glidx = 38` (dY 345 -> 755, `camctrl` 341 -> 3.53).
- Kinematics & Motion Owner Narrowing (PROVEN):
  - `glidx 1..33`: idle stand at `(-116.00, 0.00, 4523.00)`, `bs = 0`, `anim = 111`, `tspd = 0`, `velxz = 0` (~1.1s guest pre-walk).
  - `glidx 34..49`: stick-driven walk (~0.5s guest time), `bs = 1` then `2` then `4`, `tspd` reaches 500, `velxz` accelerates ~514.
  - `glidx 38`: `bs = 2 -> 4`, `anim = 12 -> 111`, `tspd = 500.0`, viewport camera moves overhead.
  - `glidx 50`: recorded stick released (`remap = (0, 0)`, `bastick = (0, 0, 0)`; `pad x = 1` residue byte, bstick angle 90 persists harmlessly). Position is `(116.70, -22.24, 4336.06)`.
  - `glidx 50..90`: `bs = 4`, `anim = 111`, `tspd = 500`, `velxz ~ 514..520` remain; world XZ moves steadily to `glidx 90` `(650.40, -149.10, 3898.99)`, Y falls from -22.24 toward -149.10 (-200 at gl 106).
  - Stream gate across 41 steps `glidx 50..90`: median ratio `deltaXZ / (velxz * gs) = 1.000354467227695` (min 0.66643885, max 1.00159322). Body-wide ratio (367 steps pred>5, step>1): median `1.00038`, residual median 0.000.
  - Physical integration explains steady slide. Motion owner is narrowed from UNKNOWN mechanism to guest-world displacement with nonzero retained target speed/velocity during idle-looking pose (`anim 111`, `bs 4`).
- Epistemic Boundaries & Open Blockers:
  - Root cause and interpreted guest function writer of `tspd` (`phys + 0x3C`) and `velocity` (`phys + 0x10`) remain STILL UNKNOWN. No code correction made.
  - No claim that all cutscenes share this root.
  - Collision remains `USER_OBSERVED` (no collider field logged); oracle baseline for Demo 7 not run yet.
  - Actual role of `bs = 4` and formal name of `animation 111` not reproven from symbols in this pass.
  - Diagnostic overhead: `TOOIE_DIAG_DEMO_SYNC` also forced movement diagnostics (407k lines with 227k `tooie-move`); avoid full-run captures.
  - No freeze physics, no unconditional zeroing of velocity. CURRENT/FPS/audio unchanged.
- Demo 5 Status Preservation:
  - Bounded helper `tooie_ud5_bainput_window` is strictly gated to `sub == 5`. Demo 5 is not globally resolved from the bounded helper; open Demo 5 residuals in `p08-attract-movement-residuals` remain preserved.
- Preserved Prior State:
  - Present FPS counter and Graphics options toggle remain verified and ready for gameplay QA.
  - Phase 1 default CURRENT `tfPos` workaround remains default-ON; continuous interpolation remains not implemented.
  - Phase 2 below-native decimator in RT64 remains operational.
  - Audio glitch remains explicitly DEFERRED per user directive.

`NEXT_SINGLE_ACTION`: locate interpreted guest state `bs = 4` init/update/end and physics mode/target-speed writer at `bs = 2 -> bs = 4` (`glidx = 38`), then trace if needed bounded stage 0 sub 8 `glidx 35..60` without `map == 282` filter; compare against Angrylion oracle where necessary before faithful fix; avoid full-run captures due to `DEMO_SYNC` execution weight (407k lines).

## Prior (2026-10-17) — P09_FPS_COUNTER_AND_SETTINGS_TOGGLE_VERIFIED

Read [[plan/evidence/p09-fps-counter-and-settings-toggle]], [[plan/evidence/p09-default-current-workaround-and-fps-plan]], [[plan/evidence/p09-current-work]], [[plan/evidence/p09-session-handoff]], and [[specs/Tooie/graphics-and-presentation]].
Under explicit user authorization, the top-left visible present FPS counter and extra settings toggle have been verified across codebase implementation, standalone versioned patch generation, host unit tests, and documentation. The counter measures actual successful swapchain present cadence rather than selected target or simulation rate. The toggle defaults to OFF (`false`), persists in `graphics.json`, and reuses the existing frontend RmlUi render-thread hook without parallel custom UI. In-game runtime QA is pending.

- Settings & Persistence:
  - Option: "Show Present FPS" added to `add_graphics_options` in `src/game/config.cpp:186-198` and `include/banjo_config.h` (`banjo::configkeys::graphics::show_fps_counter`).
  - Default: `false` (OFF). Persists automatically through `graphics.json`.
  - User directions: In-game or launcher configuration menu -> Graphics tab -> check "Show Present FPS" to enable. Setting updates live via change callback `tooie::set_fps_overlay_enabled()`.
- Counter & Cadence Architecture:
  - Measurement: `RT64::presentSuccessCount` atomic in `src/hle/rt64_present_stats.h` incremented strictly upon successful `ext.swapChain->present(...)` in `rt64_present_queue.cpp:513`. Decimator skips and failed presents are not counted.
  - Pacing separation: `presentTimestamp` in `rt64_present_queue.cpp:508` remains a pacing clock for `preciseSleepUntil` (above-native target); it is not an FPS display metric. Developer ImPlot present-time graph (`rt64_state.cpp:2465`) remains dev-mode only.
  - Meter: `tooie::PresentRateMeter` (`include/tooie_fps_meter.h`) computes rounded integer rate over a 1 s monotonic window. Stalls > 3 s reset window and display `"-- FPS"`.
  - Overlay: `recompui::set_overlay_update_callback` added to RecompFrontend (`recompui/src/base/ui_state.cpp`), called on render thread in `draw_hook()`. Displays `"%u FPS"` at 16 dp from top-left with dark backing; not a shown context (unaffected by `hide_all_contexts()`, zero input capture).
- Versioned Patches:
  - RecompFrontend: `patches/frontend/recompfrontend-overlay-update-callback.patch` (SHA256: `37266d0bd13d42c0b8f174c0118b6a9b2490784c5880f05e6ee37ab811cd48a7`) on clean `d0d90ba`.
  - RT64: `patches/rt64/rt64-present-success-counter.patch` (SHA256: `173c9b7cafc4b9750fb610b56faca8125399679a874d1c159f6c53e420c31c68`) on dirty `6f1c2d99`.
  - Reverse apply check: `git apply --reverse --check` exits 0 for both patches.
- Build & Verification Gates:
  - Host test: `./build/tooie_fps_meter_test` exits 0 (`tooie_fps_meter_test PASS`). Covers 1 s window, midpoint rounding, zero presents, stalls > 3 s, backwards clock, and resets.
  - Diff check: `git diff --check` passes exit 0 across TooieRecomp, RT64, and RecompFrontend.
- Epistemic Boundaries & Critical Distinctions:
  - Present Cadence vs. Motion Rate: With default-ON CURRENT `tfPos` workaround active, presentation rates above native repeat the current vertex pose. The counter reading near 60 FPS while visible motion updates at ~30 Hz reflects actual swapchain presents, not 60 Hz simulation/motion.
  - Runtime QA NOT performed: No game process was launched during this verification; on-screen visual presentation in live gameplay is pending root/user review.
  - Eight-preset UI widget: Remains `PLANNED` / `NOT_IMPLEMENTED` (`REQ-GFX-06`). The FPS counter is an independent settings toggle.
  - Native guest logic Hz: `viOriginalRate` remains formally `UNKNOWN` (unmeasured).

`NEXT_SINGLE_ACTION`: with user authorization, launch `./build/TooieRecompiled`, enable "Show Present FPS" in Graphics options, and visually verify top-left overlay presentation and pose repeat behavior in gameplay.

## Prior (2026-10-17) — P09_RT64_PRESENT_DECIMATOR_IMPLEMENTED_AND_BUILD_GATES_PASSED

Read [[plan/evidence/p09-default-current-workaround-and-fps-plan]], [[plan/evidence/p09-current-work]], [[plan/evidence/p09-session-handoff]], and [[specs/Tooie/graphics-and-presentation]].
RT64 below-native presentation decimator implemented and verified across host unit tests, address data race, and root full/incremental compilation gates. P09 Phase 1 default CURRENT `tfPos` workaround remains active; Phase 2 below-native presentation cap mechanism is operational in RT64; RecompFrontend 8-preset UI widget remains planned; in-game runtime QA is pending.

- Pacing fix mechanism:
  - Header `src/hle/rt64_present_decimator.h` introduces `PresentDecimator` integer accumulator (`ticks += targetRate`, `ticks >= originalRate`), generating exact average presentation cadences for non-divisors (45, 70, ...) and divisors (20, 30) without float drift or integer overflow (`uint32_t`).
  - Condition: applies when `manualRate && !present.paused && (targetRate > 0) && (viOriginalRate > 0) && (targetRate < viOriginalRate)`.
  - Present queue integration (`src/hle/rt64_present_queue.cpp`): skipped presentations reuse the existing catch-up skip path (`skipInterpolation()`, `notifyPresentId(present)`, framebuffer-operation release, and `threadAdvanceBarrier`).
  - Workload ACK path static proof: all guest workloads are consumed and acknowledged in order; guest logic, audio tasks, and VI interrupts are never delayed, slept, or stalled.
- Concurrency & cadence fixes (Sonnet review followup):
  - Proven data race eliminated: `rt64_workload_queue.cpp:1021` previously wrote `ext.sharedResources->viOriginalRate` without holding a lock while the present thread read it under `configurationMutex`. Write is now wrapped under `const std::scoped_lock configurationLock(ext.sharedResources->configurationMutex)`. Lock ordering is verified safe (leaf lock; no lock inversion).
  - Catch-up skip explicitly resets the decimator (`presentDecimator = PresentDecimator()`).
  - Paused presents pass `paused == true` into `shouldPresent()`, resetting cadence so the first unpaused frame displays immediately without dropping.
- Build & test gates:
  - Host test: `tests/rt64_present_decimator_test.cpp` added to `CMakeLists.txt` (`rt64_present_decimator_test`). Tests 20/60, 30/60, 45/60, 20/30, initial workload display, pass-through on non-manual/original/equal-above-native/unknown/paused, rate change resync, and pause-reset. Test exits 0 (`rt64_present_decimator_test PASS`).
  - Companion test: `tooie_rsp_endpoint_current_test` exits 0 (20/20 assertions PASS).
  - Root full build: `cmake --build build --target TooieRecompiled rt64_present_decimator_test tooie_rsp_endpoint_current_test -j2` exit 0 (logged in `/home/wesley/.cache/tooie-fps-investigation/root-build.log`, `[362/362] Linking CXX executable TooieRecompiled`).
  - Final incremental gate: logged in `/home/wesley/.cache/tooie-fps-investigation/root-build-final.log` completed exit 0 (`[6/6] Linking CXX executable TooieRecompiled`).
  - Patch: `patches/rt64/rt64-present-rate-decimator.patch` applied cleanly over RT64 dirty baseline (historical; superseded for clean pin apply by `patches/rt64/rt64-present-fps-decimator-and-counter.patch`).
  - Diff check: `git diff --check` passes exit 0 across Tooie and RT64 trees.
- Epistemic boundaries & explicit limits:
  - Runtime QA NOT performed: no game process was launched for this verification; no actual in-game FPS measurements or speed guarantees are claimed.
  - Workload ACK path is a static proof only.
  - Above-native rates: extra presents occur, but because default-ON CURRENT `tfPos` workaround is active, intermediate frames repeat the current vertex pose. Smooth motion at rates above native remains impossible until permanent continuous interpolation is implemented.
  - Dynamic guest logic Hz: numeric `viOriginalRate` remains formally `UNKNOWN` (unmeasured).
  - Eight-preset UI widget: remains `PLANNED` / `NOT_IMPLEMENTED`.
  - P09 formal status: remains `PENDING / NOT_FORMALLY_OPEN`.

`NEXT_SINGLE_ACTION`: implement the 8-preset UI selector widget in RecompFrontend and perform runtime QA with FPS measurements.

## Prior (2026-10-17) — WHOLE_PROJECT_SPEC_AND_DOCUMENTATION_AUDIT_COMPLETED

Read [[plan/evidence/documentation-spec-audit-20261016]], [[specs/index]], [[plan/evidence/p09-current-work]], [[plan/evidence/p09-default-current-workaround-and-fps-plan]], and [[plan/evidence/p09-session-handoff]].
Whole-project documentation and specification audit completed across Rounds 1–7 under user authorization. All 286 documents in the `docs/` vault and 7 first-party Markdown documents outside `docs/` were inventoried; complete 12-domain TooieRecomp specification suite established under `docs/specs/Tooie/` and integrated into the master requirements matrix [[specs/index]]. All gameplay state, runtime facts, and technical gates are strictly preserved.

- Documentation audit summary: Complete audit ledger recorded in [[plan/evidence/documentation-spec-audit-20261016]]. Total first-party documentation scope is 293 markdown files (286 inside `docs/`, 7 outside: `AGENTS.md`, `GEMINI.md`, `tools/oracle/README.md`, `metadata/coverage_report.md`, `metadata/rsp_tasks.inventory.md`, `.claude/skills/graft/SKILL.md`, `.grok/skills/graft/SKILL.md`). Generated metadata reports (`coverage_report.md`, `rsp_tasks.inventory.md`) are explicitly classified as read-only generated documents (do not hand-edit). Generated graph `graft/`, sibling checkouts (`BanjoRecomp`, `ROM`, `banjo-tooie-Recomp`), and build caches (`build/`, `RecompiledFuncs/`, etc.) are explicitly excluded.
- Epistemic & scope clarity: Raw `/tmp` transcripts from earlier CLI rounds were lost across restart; persisted git Markdown serves as canonical evidence. Whole-tree structural inventory is explicitly distinguished from targeted semantic source verification. No exhaustive line-by-line review of all 286 notes or universal QA proof is claimed.
- Corrections & alignments: Dynamic overlay count synchronized to 884 overlay slots (`kTooieOverlayCount = 884`, ~851 populated, 33 empty gap slots) per `src/game/generated/overlay_dispatch.inc.h` and `metadata/coverage_report.md`, preserving Round 3 correction. Guest logic Hz and animation Hz reaffirmed strictly `UNKNOWN`. Missing frontmatter warnings resolved (0 missing frontmatter); orphan navigation resolved.
- Round 7 Targeted Save Spec & Census Correction: `docs/specs/Tooie/save-system.md` factual error resolved: differentiated Ultra64 OS SDK `osEepromProbe` return value (`0x02` for `SaveType::Eep16k` / `AllowAll`, `0x01` for `Eep4k` in register `r2`/`v0`, per `N64ModernRuntime/librecomp/src/eep.cpp:11-24` and `src/game/boot_hw_stubs.cpp:656-659`) from raw Joybus/PIF status word (`0x0080` was historically raw hardware status bit, not SDK return; no guessing raw protocol values); reconciled REQ-SAV-03 criteria as required behavior vs actual untested acceptance (`PARTIAL / PENDING_QA`); documented REQ-SAV-01 provenance (`pi.cpp:get_save_size` = 0x800 and boot probe initialization in `func_800166DC`); defined atomic persistence as host crash-safety against torn writes rather than universal save corruption prevention; clarified low-level Joybus SI packet parsing bypass in favor of native SDK replacements. Reconciled filesystem spec counts: 45 files total in `docs/specs/` (13 Tooie: 12 domain + 1 index; 14 BK: 13 topics + 1 index; 17 BT: 16 insumos + 1 index; 1 root master hub) without conflating 41 domain/topic specs with 4 hub documents. Reconciled warning origin: of 59 non-blocking warnings, 58 are pre-existing and 1 large-file warning is newly introduced by the thorough multi-round audit ledger itself (`documentation-spec-audit-20261016.md`, 55k+ chars).
- Validation checks: `markdown_vault_audit` confirmed 0 errors, 15 diagnostic warnings (14 pre-existing large files + 1 audit ledger), 44 lint warnings (historical markdown), 59 total non-blocking warnings. `git diff --check` passed exit 0.
- Preserved gameplay state: P09 Phase 1 default CURRENT `tfPos` workaround (`apply_default_before_renderer_init()`) is default-ON; explicit `0` rolls back to experimental interpolation. Permanent interpolation fix is `NOT_IMPLEMENTED`. FPS presets {20, 30, 45, 60, 70, 100, 120, 144} are `PLANNED` and `NOT_IMPLEMENTED`, blocked by below-native pacing in RT64 `WorkloadQueue`. Audio glitch is `DEFERRED` per user instruction. Formal P09 phase status is `PENDING / NOT_FORMALLY_OPEN`. Historical unit/build test results not rerun this task.

`NEXT_SINGLE_ACTION`: singular focused audit/measurement of native cadence and present-queue backpressure to choose a below-native presentation cap without clock changes; then implement presets with actual tests all 8 and timing QA.

## Prior (2026-10-16) — P09_DEFAULT_WORKAROUND_IMPLEMENTED_AND_FPS_PRESETS_PLANNED

Read [[plan/evidence/p09-current-work]], [[plan/evidence/p09-default-current-workaround-and-fps-plan]], and [[plan/evidence/p09-session-handoff]].
Following user directive to install the palliative workaround as the default run before addressing FPS selection, Phase 1 default workaround is implemented, verified by unit test (20/20 PASS) and independent Haiku review, and verified in root normal gameplay QA. Phase 2 FPS presets {20, 30, 45, 60, 70, 100, 120, 144} are planned but NOT implemented, blocked by actual below-native presentation pacing constraints in RT64 `WorkloadQueue`.

- Status & flags: `DEFAULT_WORKAROUND_IMPLEMENTED: YES`, `PERMANENT_INTERPOLATION_FIX_NOT_IMPLEMENTED: YES`, `FPS_PRESETS_PLANNED_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.
- Workaround mechanism: `apply_default_before_renderer_init()` (`src/game/tooie_rsp_endpoint_current.cpp:14-38`) runs in `src/main/main.cpp:1153` before `activate_tooie_guest_preemption()` and `recomp::start()`. When `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` is UNSET, sets `"1"`. Explicit values (`"0"`, `""`, `"1"`, `"keep"`) are preserved. Explicit `0` restores experimental original interpolation. Normal `./build/TooieRecompiled` requires no activation flag.
- Scope & presentation behavior: Workaround is strictly scoped to Tooie integration; RT64 default remains OFF. Uses shader-local `tfPos` CURRENT bypass (`RSPProcessCS.hlsl:78-93`, where `tfPos` also directly drives fog calculation), NOT global weights (`TOOIE_DIAG_FORCE_CURRENT_FRAME` unchanged). Vertex positions and fog are CURRENT; other weights, presentation cadence, and guest logic unchanged. No continuous position interpolation claim at 60+ FPS.
- Future diagnostic rule: Future interpolation diagnostics MUST explicitly set `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0` to prevent workaround from masking visual glitches while CPU witness assumes interpolation. Heavy collectors remain OFF.
- Build & test evidence: `cmake --build build --target tooie_rsp_endpoint_current_test` exit 0; `./build/tooie_rsp_endpoint_current_test` exit 0 (20/20 assertions PASS, saved in `/tmp/p09-default-workaround/policy-test-root.log`). Executable `TooieRecompiled` linked exit 0 (binary SHA256 `0d2e4ba5fa7e4ed0347d83180ff4c831c54b8f47567fa012de15dac3dc736b84`). RT64 HEAD `6f1c2d99` untouched (31 dirty files +13850/-44 preserved). Ordering check is static source order, not runtime/GPU trace. Untouched tests (38, 7, 1354) NOT RUN this round.
- Run provenance & QA: Writer report "Game was not launched" is writer-scoped sandbox provenance. Root actually launched `./build/TooieRecompiled` without flags (`/tmp/p09-default-workaround/normal-launch.log`, size ~852KB). User observed: "Está rodando bem. O áudio tem o mesmo glitch de sempre que já tinha desde quase o início do projeto e pretendo resolver o áudio só mais para frente." Existing audio glitch explicitly deferred; no audio changes.
- Phase 2 pacing facts: UI already accepts manual integer 20..240 step 1 (`ui_config_tab_graphics.cpp:311-313`); runtime accepts 10..1000. These generic values are NOT the requested 8-preset selector. In RT64 (`rt64_workload_queue.cpp:1175`), pacing/interpolation runs only when `targetRate > viOriginalRate`. For below-native targets (`targetRate <= viOriginalRate`), `generateInterpolated=false`, `displayFrames=1`, sending one present per native workload without lower-rate pacing or sleep. Hence 20/30 honesty is not guaranteed (native guest Hz currently UNKNOWN). Preserving ALL guest updates/VI/audio without skipping guest work requires a true presentation decimation hook; backpressure `waitForWorkloadId` is UNVERIFIED. User `graphics.json` unchanged (`Display`, manual 60).
- Deferred research queue: Prior research on first-20 tie groups, nearest-join NDC ties, and Sonnet analysis is preserved as historical and deferred queue; root is not reopened and aliases are not implemented.

`NEXT_SINGLE_ACTION`: singular focused audit/measurement of native cadence and present-queue backpressure to choose a below-native presentation cap without clock changes; then implement presets with actual tests all 8 and timing QA.

## Prior (2026-10-16) — P09_PATHOLOGICAL_TIE_GROUPS_CAPTURED_AND_ARBITRATED

Read [[plan/evidence/p09-current-work]], [[plan/evidence/p09-pathological-tie-groups]], [[plan/evidence/p09-exact-use-pathology-correlation]], and [[plan/evidence/p09-causal-baseline-and-fix-direction]].
Bounded first-20 pathological tie-group diagnostic implemented, validated by host tests (1354/1354 exit 0), compiled in TooieRecompiled (rebuilt binary SHA256 `84d31479...`, exit 0), and captured in gameplay runtime (`EXHAUSTED` at 24 attempts, 6 plans, 162 pathological pair occurrences, all ambiguous TIE, 179 unique keys).

- Captures & hashes: `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-correlation.txt` (SHA256 `74b0b2e5f219522c42b041a6a5cea9005bea8f57ddd0fc595df8f69024528c7d`) and sibling `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-ties.txt` (SHA256 `5b1fc21a2af77dbab9abc42919465e45a65a13014321cee405468cc6fd59b2a8`). Versioned patch `patches/rt64/p09-exact-use-pathology-correlation.patch` SHA256 `7b93258cecadcb0b18b1f4a22f0a278c70a1cc20f3e1f0ed8fbb112b0bd7f38f`.
- Group structure: 20 groups stored (`kP09TieGroupCap=20`), 142 omitted (`skippedGroups=142`), 0 member truncation. 12 mult=2, 8 mult=4 = 56 parent face-corner occurrences across distinct 9-field keys. All 20 groups have exactly 2 distinct `storedVertex` records per group at identical parent `WORLD` and printed coordinates (not bitwise identity). All 20 child keys and 56 members returned `MEMBERSHIP=NOT_SELECTED` (`REASON=NONE`) against local active exhaustive plan.
- Independent review & arbitration: Haiku static review PASS (noted unchecked `fclose` before rename, non-blocking); Haiku capture analysis clarified `selected=YES` (nearest search choice) vs `MEMBERSHIP=NOT_SELECTED` (plan membership) as separate non-contradictory fields. Sonnet solution escalation executed; Root arbitration confirmed Sonnet withdrew P1 point-cert (`k`-points, reciprocity, 0.08), global plan miss claim, and raw ID persistence. All three Sonnet options (P1, P2, P3) remain `HYPOTHESIS`.
- Scope & boundaries: `PRODUCTION_FIX_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.

`NEXT_SINGLE_ACTION`: a READ-ONLY focused audit of existing paired-boundary-edge/provenance data and its correlation to the sampled pathological uses, starting with producer/topology semantics and what current artifacts actually contain; no new lerp/alias or telemetry implementation automatically. Root stops at newly identified evidence blocker after Sonnet analysis as authorized, not claiming fix.

## Prior (2026-10-15) — P09_EXACT_USE_GAMEPLAY_CAPTURED_ALL_AMBIGUOUS_TIE

Read [[plan/evidence/p09-current-work]], [[plan/evidence/p09-exact-use-pathology-correlation]], and [[plan/evidence/p09-causal-baseline-and-fix-direction]].
Causal baseline is established (`CAUSE_ALREADY_ESTABLISHED: YES`, root class `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS` proven decisively via `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` and `TOOIE_DIAG_FORCE_CURRENT_FRAME=1`), while production fix is not implemented (`PRODUCTION_FIX_NOT_IMPLEMENTED: YES`).
Armed gameplay capture executed; process reached limit (`EXHAUSTED` at 24 attempts). Meaningful runtime data captured, but all 194 pathological pair occurrences are ambiguous TIE; coverage rate is undecidable. This existing 194-TIE capture remains an exact-use diagnostic blocker for nearest-NDC join authorization onto alias keys, NOT an unknown root cause. No production fix is implemented.

- Capture file & hash: `/tmp/p09-correlation-implementation/capture-gameplay-20261015.txt` (SHA256 `b6be018a3619a78a7c05ba7d0b8fa6c64e27d22e5cde6bedbebeebdf771203e7`). Initial empty capture retained as history. Game left running in `EXHAUSTED` state (no new searches without rearm).
- Accounting & plans: 2135 walks, 93 eligible dispatches, 24 attempts (4 with path pair, 20 with no path pair), 4 seam searches, 4 local plans built (`PLAN_BUILT=4`). No plan failure, capacity cap, or truncation (`PLAN_INACTIVE=0`, `PLAN_NOT_EXHAUSTIVE=0`, `PAIRS_DROPPED_PENDING_CAP=0`, `VERTS_DROPPED_BONE_CAP=0`, `UNIQUE_TABLE_OVERFLOW=0`, `RECORDED_COHORT_TRUNCATED=NO`).
- Pathology correlation: `PATH_PAIR_TOTAL=194`, `PATH_PAIR_FULLY_COVERED=0`, `PATH_PAIR_ONE_SIDED=0`, `PATH_PAIR_NOT_COVERED=0`, `PATH_PAIR_AMBIGUOUS=194`, `PATH_PAIR_AMBIGUOUS_TIE=194`. Saturated by 194 ties (`PATH_PAIR_FULL_OF_ALL_OBSERVED=0.000000`, definitive rate `NA`). Cumulative occurrences across 4 plans, not 194 unique certified edges.
- Keys & nonpathological universe: 214 unique path keys (1 selected, 213 not selected). Nonpathological: 454 pairs, 481 unique keys (4 selected, 477 not selected). 25 keys on both path and nonpath.
- CPU timings (runtime diagnostic only, NOT game FPS or visual proof): attempt peak 202.762 ms (total 590.915 ms), plan peak 200.776 ms (total 551.464 ms), gate total 10.382 ms.
- Haiku source review: proven that `p09EmitNearestJoinPairs` sets `bestTie = true` on exact float equality `gap == bestGap` in NDC XY distance, and classifier prioritizes TIE over key checks. Coincident parent hypothesis is UNKNOWN; identical coordinates do not prove ambiguity is false because sourcekeys/WORLD policy matter. No tie suppression or dedup production fix authorized.
- Arbitration & fix direction: Haiku proposal for new CLIP_LERP/NDC_LERP visual discriminator is SUPERSEDED by 2026-09-14 closure in `p09-articulated-temporal-coherence-root.md`. Next discriminator serves selection for the known cause, not a fresh search for root.

`NEXT_SINGLE_ACTION` (historical): one bounded diagnostic discriminator logging first ~20 tie groups, exact full sourcekeys/originalWORLD and prev/cur positions, gap/multiplicity and candidate memberships to distinguish repeated same-use from different legitimate-use; then review, before alias/interpolation edits (currently NOT_IMPLEMENTED; task stops at newly observed blocker per scope rule).

## Prior (2026-10-15) — P09_EXACT_USE_PATHOLOGY_CORRELATION_CAPTURED_EMPTY_AND_RELAUNCH_WAITING

Read [[plan/evidence/p09-current-work]] and [[plan/evidence/p09-exact-use-pathology-correlation]].
First actual capture executed; output is empty (`CAPTURED_EMPTY / NO_ELIGIBLE_SAMPLES`). Root relaunched with dedicated flag and confirmed waiting; user positioning character gameplay before root arming.

- First capture observation (`capture-empty-20261015.txt`): `ARM_COUNT=1`, `REARM_COUNT=0`, `MARKER_CONSUME_FAILURES=0`. `ELIGIBILITY_WALKS=181`, `ELIGIBLE_DISPATCHES=0`, `ATTEMPTS=0`, `PATH_PAIR_TOTAL=0`. Ineligible skip breakdown (first-failure order): `SKIPPED_INELIGIBLE_NO_WORKLOAD=61`, `SKIPPED_INELIGIBLE_NOT_INTERMEDIATE=59`, `SKIPPED_INELIGIBLE_NO_TARGET_CALL=61`; `NO_GEOMETRY=0`, `NO_FRAME_MATCH=0`, `NO_MODEL_BIN=0`. User estimated run at ~30 seconds; scene was `UNKNOWN`.
- Classification: `CAPTURED_EMPTY / NO_ELIGIBLE_SAMPLES`. This is NOT evidence of zero coverage, false positives, or correctness. Former runtime-not-captured state preserved as historical; meaningful pathology sample, visual confirmation, and real CPU execution timings remain `NOT_CAPTURED`.
- Haiku gate review (`haiku-empty-capture-output.json`): read-only check confirmed first-failure gate ordering (`NO_WORKLOAD` -> `NO_GEOMETRY` -> `NO_FRAME_MATCH` -> `NOT_INTERMEDIATE` -> `NO_MODEL_BIN` -> `NO_TARGET_CALL`) and 0x62B target display lists (`p09V4VisibleTargetIndex`). Do NOT promote its inference that movement is strictly required or that modelbin is resolved globally as proven: counters record first failure; zero `no_model_bin` applies only to the 61 dispatches reaching that gate.
- Relaunch status: root relaunched with dedicated flag (`TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled`), confirmed status `STATE=WAITING_FOR_REQUEST`, zero walks. User positioning character gameplay before root arming.

`NEXT_SINGLE_ACTION` (historical): wait for user ready in character scene then arm and inspect eligible/attempt counters; still no aliases or production fix.

## Prior (2026-10-15) — P09_EXACT_USE_PATHOLOGY_CORRELATION_IMPLEMENTED_AND_REVIEWED

Read [[plan/evidence/p09-current-work]] and [[plan/evidence/p09-exact-use-pathology-correlation]].
Exact-use pathology correlation diagnostic implemented, validated by host tests, compiled in TooieRecompiled, and approved by independent Haiku review.

- Diagnostics & flags: `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION` (default-OFF, CPU-only, standalone). Request marker `/tmp/tooie-p09-seam-edge-pathology-correlation.arm` with transactional `::remove == 0` consume, startup stale-marker deletion/ignore, pre-walk 2000 ms cadence gate (`p09CorrelationPreWalk`), and finite safety limits (max 24 heavy attempts, 8 seam searches).
- Methodology: 9-field keys on both pair members; same-call local plan avoids global transform-all-before-RSP overwrite; strict all-observed metric `PATH_PAIR_FULL_OF_ALL_OBSERVED` includes ambiguous cases in denominator.
- Gates & review: `p09_exact_use_correlation_test` 1231/1231 passed exit 0; `TooieRecompiled` linked 35/35 exit 0; companion tests 38/38 and 7/7 exit 0; git diff --check exit 0; patch `patches/rt64/p09-exact-use-pathology-correlation.patch` (SHA256 `5108ad3f...`) validated apply/reverse RC:0 on dirty baseline over RT64 `6f1c2d99`. Haiku final review PASS (static/helper/build scope); root arbitration confirmed hash list integrity and exit 0 logs.
- Runtime boundary: runtime pathology coverage, character scene capture, visual proof, CPU timings remain `NOT_CAPTURED`. No aliases, no production fix, formal P09 pending, cleanup/commit not ready (`NO_COMMIT=YES`, `NO_PUSH=YES`).

`NEXT_SINGLE_ACTION` (historical): user runs one scene capture with `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled`, arming via `/tmp/tooie-p09-seam-edge-pathology-correlation.arm` in character scene, and inspects correlation output; still no aliases or production fix.

## Prior (2026-10-15) — P09_V2_SEAM_EDGE_CAPTURE_REVIEW_CASE2

Read [[plan/evidence/p09-current-work]] and [[plan/evidence/p09-seam-edge-v2-capture-review]].
Review of preserved snapshot `/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt` completed.
Operational decision: `CASE2`. Gate 3 `PASS` scoped to 12 same-call detail records and latest-workload accounting only.

- 12 certified same-call detail records verified: 12/12 `REVERSED`, residuals 0.0 in PREV/CUR (<= tol), disjoint vertices and distinct original worlds, 24/24 competitor margins pass (`nextBest - best > marginNeed`). 135 cross-call certified pairs uninspected (detail output capped at 12); competitor choices not independently auditable.
- Counter semantics arbitrated via `rt64_transform_processor.cpp`: initial Haiku CASE3 suspicion resolved and `CORRECTED/SUPERSEDED` (151 tolpass = 135 cert + 16 ambig; 2063 hash gap is repeated edge inserts; 18 cross boundary gap is isolated projection groups).
- Visual & perf: user qualitative OFF normal / V2 slow observed; no measured FPS or exclusive search causality. Visual correction pending; no production fix, formal P09 pending, cleanup/commit not ready.
- Scope: advances exclusively to default-off CPU exact-use pathology correlation; not production seam identity approval or alias implementation.

`NEXT_SINGLE_ACTION` (historical): implement ONLY default-off CPU exact-use pathology correlation checking both pathological uses against full key (workloadId, fbPair, projection, gameCall, triangleLocal, corner, faceCorner, storedVertex, originalWorld); report total, covered, uncovered, ambiguous, and nonpathological selected; no aliases or performance patch now; correlation remains NOT_IMPLEMENTED/NOT_CAPTURED.

## Prior (2026-10-15) — P09_V2_RUNTIME_EXHAUSTIVE_SEARCH_CAPTURED

Read [[plan/evidence/p09-current-work]] and [[plan/evidence/p09-seam-edge-dry-run-v2]].
User ran V2 (`TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`).
Process finished; no active game process or live flags. Preserved snapshot:
`/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt` (SHA256 `256d5fdd935c7d105810d885f4bbdb4047453606cdc5544bc56ef1e81bbbe6e5`).
Full counter/timing details documented once in [[plan/evidence/p09-current-work]].

- Search accounting (latest workload): boundaries 4627, same-call evaluated 66588, cross-call raw 664920 (duplicates 554413, unique evaluated 110507), certified 55 same / 135 cross, exact uses 732. Session 2200 workloads cumulative (not latest workload count).
- Search exhaustiveness: `SAME_CALL_SEARCH_EXHAUSTIVE=YES`, `CROSS_CALL_BUCKET_SEARCH_EXHAUSTIVE=YES`, `CROSS_CALL_UNIQUE_PAIR_GENERATION_EXHAUSTIVE=YES`, all search dropped 0, `EMERGENCY_LIMIT_HIT=NO`, `RETRIEVAL_CELL_SMALLER_THAN_EDGE_SCALE=NO` (correcting fictitious `RETRIEVAL_RADIUS_RISK`), `LATEST_WORKLOAD_COUNTS_EXHAUSTIVE=YES`, `ALIAS_PLAN_EXHAUSTIVE=YES`. Text detail cap (12/190 accepted, 178 dropped) does not invalidate exhaustive search (`DETAIL_OUTPUT_ONLY_SATURATION=YES`).
- Acceptance scope: search accounting accepted ONLY FOR LATEST WORKLOAD. Structural/coherence review pending (`CERTIFIED` alone insufficient to prove edge identity). Pathology coverage remains `NOT_CAPTURED`.
- Visual observation: `INCONCLUSIVE` due to extreme slowdown reported by user. Diagnostic changes no rendering, no production fix; persistent glitches do not refute diagnostic. Strong CPU-overhead hypothesis supported by self-times (search 64ms, session peak 439ms), NOT full frame profiler, NOT measured FPS, NOT controlled A-B. Haiku review (`/tmp/p09-haiku-performance.json`) noted callsite cpp:3259 (not unconditional every-frame). A-B qualitativo (`USER_OBSERVED`): volta ao normal sem a flag e desaceleração severa associada ao V2 ativo; confirma associação relatada, mas não é FPS medido, benchmark controlado, prova de custo exclusivo de busca versus outra telemetria nem causalidade de thread/frequência; self-times existentes inalterados (64ms latest, 439ms pico).
- Build & phase status: Retain build `PASS` from 14/10 with freshness caveat (`BYTE_IDENTITY=UNKNOWN_BUILD_DID_NOT_RECOMPILE_SOURCE`). `P09_FORMAL_PHASE=PENDING / NOT_CLAIMED`. `PRODUCTION_FIX_UPDATED=NO`. `CLEANUP_READY=NO`. `COMMIT_READY=NO`.

`NEXT_SINGLE_ACTION`: inspect certified paired-edge A/B provenance and residual/tolerance/margin details from preserved snapshot; only after coherent certification proceed exact-use pathology correlation. Do not implement aliases yet.

## Prior (2026-10-14) — P09_V2_SEARCH_EXHAUSTIVENESS_VALIDATION_GATE

Read [[plan/evidence/p09-current-work]] and [[plan/evidence/p09-session-handoff]].
Actual validation gate verified by orchestrator on 2026-10-14 23:51 -0300:
- Build: `cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test` exit 0 (`[3/3] Linking TooieRecompiled`).
- Unit tests: `./build/window_display_test` exit 0 (`PASS: 38/38 assertions passed`), `./build/tintro_projection_policy_test` exit 0 (`PASS: 7/7 assertions passed`).
- Trees & diff checks: `git -C TooieRecomp diff --check` exit 0; `git -C BanjoRecomp/lib/rt64 diff --check` exit 0.
- Source edits: `NONE`. Game launch: `NOT_RUN`. No runtime capture, no visual validation, no production fix.
- Git HEADs: Tooie `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`, BanjoRecomp `ec859632cfa584e2272d32e46051543429a2baf4`, RT64 `6f1c2d99a4ea571c139f449c326fd176ba8f3496`.
- Freshness caveat (mandatory): incremental build reused the RT64 object compiled on 2026-09-24 (`rt64_transform_processor.cpp.o`) and relinked `TooieRecompiled` on 2026-10-14 23:51. `source mtime <= object mtime <= binary mtime`. V2 symbols (`RT64::p09SeamEdgeDryRunV2WouldAliasExactUse`, `RT64::p09SeamEdgeDryRunV2AliasPlanExhaustive`) are confirmed present via `nm` in both object and executable. This establishes an `INFERENCE` of freshness, NOT byte-for-byte proof of compiling this exact source; `BYTE_IDENTITY=UNKNOWN_BUILD_DID_NOT_RECOMPILE_SOURCE`. Do not claim RT64 source recompiled or that full P09 is proven.
- Artifact hashes: Source SHA256 `941ae09e8891889c2cf3057d06aaa5a413f2efdf6b4be7fe1a020a786c221a07`, Object SHA256 `2f9887fe227c6aff0bf63d018f9610aca8d62e1784be5a010eeb00b310e1b932`, Executable SHA256 `17873fec359484654d8deeb7ae40302f4a4a44b7a40c6ea0e792c63e91152953`.
- Multi-model routing: Grok real model `grok-4.7` medium (not fast, `context_window=200000`), Gemini documentation (Gemini 3.8 Flash High), Haiku `claude-haiku-5-5` medium independent read-only review; Sonnet/Opus not used. Initial CLI permission attempts failed/cancelled before gate; final gate completed.
- Haiku review summary (`/tmp/p09-haiku-review.json`): static check confirmed repair present in `rt64_transform_processor.cpp` for same-call (:2270-2302), cross-call (:2326-2425), and fail-closed safety stops (:2700-2712), but did not follow `gP09V2Plan.exhaustive` assignment to final refusal path (do not claim full review).
- Status: `ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`. `RUNTIME_V2_EXHAUSTIVE_CAPTURE=NOT_CAPTURED`. `PATHOLOGY_CORRELATION=NOT_CAPTURED`. `VISUAL_CONFIRMATION=NOT_RUN`. `P09_FORMAL_PHASE=PENDING / NOT_CLAIMED`. `PRODUCTION_FIX_UPDATED=NO`. `CLEANUP_READY=NO`. `COMMIT_READY=NO`.
- Vault audit: marked pending execution by orchestrator after edits.

`NEXT_SINGLE_ACTION`: user performs isolated V2 capture in character scene, first preserving existing output, enabling ONLY `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`; inspect all exhaustive fields incl `RETRIEVAL_RADIUS_RISK=NO`, certification and exact-use provenance. Captures and pathology coverage remain `NOT_CAPTURED`; no aliases or production fix; cleanup/commit not ready; P09 formal remains pending.

## Prior (2026-09-24) — P09_PAIRED_EDGE_V2_SEARCH_EXHAUSTIVENESS_REPAIR

Read [[plan/evidence/p09-seam-edge-dry-run-v2]].
The previous runtime file is not a negative cross-call result: same-call
evaluation stopped at 16384, the 48-entry bucket walk dropped 180588
entries, and the cross-call dedup probe failed. V2 identity, tolerance
(`EDGE_SCALE_COEFFICIENT=1e-4`, no 0.25 floor, no world-translation term),
and rendering are unchanged. Search storage is per workload. Same-call
pairs are all evaluated, with streaming top-2 partners. Cross-call
retrieval is an exact 32-unit cell plus its 26 neighbors. Each unordered
pair is evaluated once from the lower edge id. Fixed candidate caps are
gone. Absolute bounds that remain are safety stops: a hit sets
`EMERGENCY_LIMIT_HIT=YES` and refuses an exhaustive alias plan. Certified
details name both edges. Pathology coverage stays `NOT_CAPTURED`. The game
was not launched, so the repaired counters are not captured yet.

`ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`.
`PRODUCTION_FIX_UPDATED=NO`.
`CLEANUP_READY=NO`.
`COMMIT_READY=NO`.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`
on the character scene and confirm search, certification, and the exact-use
plan are no longer truncated before any pathology correlation.

## Prior (2026-09-24) — P09_PAIRED_EDGE_SEAM_ALIAS_DRY_RUN_V2

Read [[plan/evidence/p09-seam-edge-dry-run-v2]].
`TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1` writes
`/tmp/tooie-p09-seam-edge-dry-run-v2.txt`. V1
`TOOIE_DIAG_P09_SEAM_ALIAS_DRY_RUN` is unchanged. Certification is a
vertex-disjoint boundary-edge pair: both endpoints, both orientations
allowed, PREV and CURRENT, row-vector `mul(localPos4, WORLD)`, no 0.25
floor, and no world-translation term. The cross-call hash only retrieves.
Ambiguous partners stay rigid. Exact-use membership is
`p09SeamEdgeDryRunV2WouldAliasExactUse` (workload, fb pair, projection,
GameCall, triangle, corner, face corner, stored vertex, original WORLD).
It is not a WORLD-slot plan. Pathology coverage counters stay
`NOT_CAPTURED`. The first runtime capture saturated search, so its
cross-call certified count of 0 is not a negative result.
`MEMORY_OVERHEAD_STATUS=PARTIAL_ESTIMATE`.
`BUFFER_GROWTH_AUDIT_COMPLETE=NO`. No alias, remap, clone, shader, VIEW,
or PROJECTION change. Game was not launched for the repair below. Offline
before that repair: TooieRecompiled linked, `window_display_test` 38/38,
`tintro_projection_policy_test` 7/7.

`ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`.
`CURRENT_WORLD_INDEX_FIX_STATUS=INSUFFICIENT_GRANULARITY`.
`PRODUCTION_FIX_UPDATED=NO`.

## Prior (2026-09-24) — P09_SEAM_ALIAS_DRY_RUN_VALIDITY_REVIEW

Read [[plan/evidence/p09-seam-alias-dry-run-validity-review]]. The user
captured 29 cross-call and zero same-call certified pairs in the latest
workload; 9857/11903 session workloads had at least one pair. Static review
finds the current two-frame point coincidence rule insufficient as seam
identity (`CASE_B`), and the shared `BOUNDS_SATURATED=YES` flag can describe
search or alias-plan truncation. The 54,234-byte peak is an incomplete
arithmetic estimate. Corrected V2 pathological JOINs have not been joined
to exact GameCall/face-corner/vertex/WORLD uses in the dry-run plan.
No production alias/remap or shader change was made.

`NEXT_SINGLE_ACTION`: implement a versioned default-off CPU seam dry-run
with paired-edge/provenance identity, per-limit saturation counters and
exact-use plan keys for the subsequent corrected V2 coverage correlation.
Do not materialize aliases yet.

## Prior (2026-09-24) — P09_CALL_LOCAL_SEAM_ALIAS_DRY_RUN_PROTOTYPE

Read [[plan/evidence/p09-seam-alias-dry-run]].
`TOOIE_DIAG_P09_SEAM_ALIAS_DRY_RUN=1` writes
`/tmp/tooie-p09-seam-alias-dry-run.txt`. It plans call-local seam aliases
and vertex clones from GameCall face slices after WORLD frame matching.
It does not change lerp policy, buffers, `worldIndices`, shaders, VIEW, or
PROJECTION. GameCall identity and original-slot frame match are available
at that layer. A future alias insert remains before `State::fullSync`
upload and must inherit the original slot's match. Ambiguous partners stay
rigid. Unrelated vertices in the same GameCall stay rigid by design.
Cross-call lookup is a bounded spatial hash and is not seam identity.
`WORLD_INDEX_IS_16_BIT=YES`. Runtime seam/alias/clone counts are
`NOT_CAPTURED`. `SNOUT_SPECIFIC_ROOT=UNPROVEN`.
`CURRENT_WORLD_INDEX_FIX_STATUS=INSUFFICIENT_GRANULARITY`.
`PRODUCTION_FIX_UPDATED=NO`.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_P09_SEAM_ALIAS_DRY_RUN=1 ./build/TooieRecompiled`
on the character scene and read the dry-run file. Leave both WORLD-index
visual toggles off. Do not duplicate matrices or remap `worldIndices` yet.

## Prior (2026-09-24) — P09_CALL_LOCAL_WORLD_INTERPOLATION_ARCHITECTURE_REVIEW

Read [[plan/evidence/p09-call-local-world-interpolation-architecture-review]].
The user observed that the multi-only WORLD-index candidate brings Kazooie
feather glitches back and still shows Klungo stripes, Mumbo flicker and Banjo
snout deformation. `CURRENT_WORLD_INDEX_FIX_STATUS=INSUFFICIENT_GRANULARITY`.
Static RT64 review found that GameCall face slices persist on CPU, but a
WORLD slot receives one interpolation result. Shared vertices can carry only
one WORLD index. `State::fullSync` uploads draw data before render-time
`TransformProcessor`, so a late remap alone is insufficient. The proposed
default-off prototype reserves candidate matrix/vertex aliases before the
existing upload, then certifies endpoint seams after frame matching and uses
elementwise only for certified call uses. It remains conditional on CPU
candidate/false-positive counts, 16-bit index capacity, and companion buffer
invariants. No code, game run, GPU operation, commit or push in this review.

## Prior (2026-09-23) — P09_MULTI_ONLY_WORLD_SELECTOR_VISUAL_CANDIDATE

Read [[plan/evidence/p09-multi-only-world-selector-visual-candidate]].
The selector audit showed mixed WORLD indices
(`CURRENT_SELECTOR_SELECTED_MIXED_USAGE_COUNT=20` on a representative
workload, session peak 31). A second default-off candidate,
`TOOIE_TEST_P09_MULTI_ONLY_ELEMENTWISE_WORLD=1`, applies the same
16-entry WORLD lerp only when that workload's GameCalls use the index
in multi-WORLD calls and in no single-WORLD call. Mixed indices keep
`RigidBody::lerp`. The original toggle is unchanged and wins if both
are set. Enable one candidate. Shader, VIEW, and PROJECTION are
unchanged. Do not ship. Do not add matrix duplication or remapping
until this visual result is in.

`ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`.
`CURRENT_ORIGINAL_SELECTOR_STATUS=PARTIAL_FIX_WITH_REGRESSIONS`.

`NEXT_SINGLE_ACTION`: `TOOIE_TEST_P09_MULTI_ONLY_ELEMENTWISE_WORLD=1
./build/TooieRecompiled` on the same scene. Leave
`TOOIE_DIAG_RSP_ENDPOINT_BINDING` and `TOOIE_DIAG_P09_SELECTOR_AUDIT`
off. Report feathers, backpack/top, Mumbo feathers, Klungo lines,
Mumbo disappearance, Banjo snout, and any new squash/shear/wobble.

## Prior (2026-09-23) — P09_SELECTIVE_WORLD_SELECTOR_GRANULARITY_AUDIT

Read [[plan/evidence/p09-selective-world-selector-granularity-audit]].
User A/B: `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1` corrects Kazooie
head feathers, Banjo backpack/top flap, and Mumbo feathers. Banjo
snout/muzzle stays glitched. Klungo gains moving line/cutting artifacts.
Mumbo gains an occasional near-one-frame disappearance. Camera and
ordinary rigid objects look normal. Candidate stays default-off.
`CURRENT_VISUAL_CANDIDATE_STATUS=PARTIAL_FIX_WITH_REGRESSIONS`.

Selector granularity is the WORLD slot. Marking a slot replaces
`lerpWorldTransforms[t]` for every later consumer of that index.
`TRANSFORMPROCESSOR_WORLD_INDEX_SELECTOR_GRANULARITY_SAFE=NO`.
Cross-GameCall constraints escape. Snout class is `NOT_PROVEN`.
The audit file was captured after this note; the multi-only candidate
above is the follow-up. Shader unchanged.

## Prior (2026-09-17) — P09_GENERIC_SELECTIVE_WORLD_INTERPOLATION_PRODUCTION_DESIGN

Read [[plan/evidence/p09-generic-selective-world-interpolation-production-design]].
Default-off CPU candidate `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1`
applies exact 16-entry WORLD lerp only to world indices used by GameCalls
with more than one distinct `worldIndices` slot. Other WORLDs keep
`RigidBody::lerp`. VIEW/PROJECTION interpolation unchanged. Failed W-sign
shader snap is disabled. User visual result is recorded in the 2026-09-23
audit above. Do not promote.

## Prior (2026-09-16) — P09_STANDARD_SLERP_WORLD_PRODUCTION_CANDIDATE_DISCRIMINATOR

Read [[plan/evidence/p09-standard-slerp-world-production-candidate-discriminator]].
CPU-only `STANDARD_SLERP_WORLD` arm is implemented on repaired V2
shader-equivalent pairing. Clip is
`mul(LIVE_POS4, mul(STANDARD_SLERP_WORLD, mul(LIVE_VIEW, LIVE_PROJECTION)))`.
Rotation only uses explicit shortest-arc SLERP (`p09StandardSlerp`);
T/S/skew/perspective match production decompose/recompose. Production
HLSL and Transform/Projection interpolation are unchanged. Game was not
launched. Offline: TooieRecompiled linked, `window_display_test` 38/38,
`tintro_projection_policy_test` 7/7, synth 14/14,
`STANDARD_WORLD_SLERP_REFERENCE_VALID=YES`.

Prior ELEMENTWISE recapture (`PROVEN`):
`ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`.
Do not ship elementwise WORLD. Quaternion-law correlation is not yet a
sufficient cause.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste STANDARD_SLERP_WORLD_* JOIN /
worldspace / pathology / rigidity keys.

## Prior (2026-09-16) — P09_ELEMENTWISE_WORLD_INTERPOLATION_CAUSAL_DISCRIMINATOR

Read [[plan/evidence/p09-elementwise-world-interpolation-causal-discriminator]].
User recapture: elementwise WORLD is linear, fixes 156/156 corrected
pathologies, reduces articulated gap, and introduces material
nonrigidity. Do not promote it to production.

## Prior (2026-09-16) — P09_REPAIR_CPU_DIAGNOSTIC_SHADER_SEMANTICS

Read [[plan/evidence/p09-cpu-diagnostic-shader-semantics-repair]].
CPU clip diagnostics now use `mul(pos4,mul(WORLD,VIEWPROJ))`, matching
SPIR-V `OpMatrixTimesMatrix(W,Q)` then `OpVectorTimesMatrix(p,WQ)`.
Physical WORLD is `mul(pos4,WORLD)`; camera-relative is
`mul(pos4,mul(WORLD,VIEW))`. Status schema is
`P09_RSP_ENDPOINT_BINDING_V2_SHADER_EQUIVALENT`. Legacy MODELVIEW/hierarchy
and stale `VIEW_WORLD_ROOT_CLASS=CROSS_STREAM_TEMPORAL_COHORT_MISMATCH`
claims are superseded. Pose reject reasons are separated; actual pose
endpoints are checked. No production HLSL/upload change. Game root
`UNRESOLVED`. Offline acceptance is `PASS`: synth 9/9, static SPIR-V
match, TooieRecompiled linked, `window_display_test` 38/38,
`tintro_projection_policy_test` 7/7.

## Prior (2026-09-15) — P09_CPU_SHADER_MATRIX_SEMANTICS_AND_HOMOGENEOUS_AUDIT

Read [[plan/evidence/p09-cpu-shader-matrix-semantics-and-homogeneous-audit]].
`PROVEN`: compiled Vulkan shader applies `p_row * (WORLD * VIEWPROJ)`;
old CPU helpers applied `(VIEWPROJ * WORLD) * p_column`. That mismatch is
the reason this repair exists. Old factorial/pose/WORLD metrics are not
shader-equivalent. FORCE_CURRENT visual evidence survives separately.

## Prior (2026-09-15) — P09_IMPLEMENT_EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW

Read [[plan/evidence/p09-exact-order-validated-camera-pose-view]].
CPU arm `EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW` is implemented, default-off,
on the same LLLL samples. Standard mathematical SLERP (not the RT64 helper).
Exact order `((POSE_DERIVED_LIVE_VIEW*LIVE_PROJ)*LIVE_WORLD)*LIVE_POS4`.
Pose JOIN / pathology / relative-path / WORLD_H.w / WORLD SLERP-law keys
remain `NOT_CAPTURED`. `ROOT_CAUSE_CLASS=UNRESOLVED`. Do not ship a
production fix.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste the `POSE_VIEW_*` /
`POSE_DERIVED_VIEW_MATCHES_SAFE_CLASS` / pathology coverage /
relative-deviation / `WORLD_H_W_*` / `WORLD_SLERP_LAW_*` keys.

## Prior (2026-09-15) — P09_INDEPENDENT_ARCHITECTURE_ROOT_CAUSE_REVIEW

Read [[plan/evidence/p09-independent-architecture-root-cause-review]].
Latest status confirms 249 pathological JOINs on aligned cohorts, zero on
misaligned cohorts; mapping-coherent fallback `REFUTED` for this capture.
`ROOT_CAUSE_CLASS=UNRESOLVED`. CLCL returns to the endpoint reference;
do not ship it or infer a camera root solely from its safe metrics.

Static/CPU findings: VIEW interpolates inverse-camera entries directly.
The bundled quaternion helper plus RT64's `1-weight` has correct endpoints
but is not mathematical constant-speed SLERP. WORLD JOIN drops homogeneous
w; its xyz ratios do not prove coherence of the complete projected input.
These are proof boundaries, not authorization for hierarchy or W fixes.

## Prior (2026-09-15) — P09_MAPPING_COHERENCE_FALLBACK_COUNTERFACTUAL

Do **not** ship CLCL. Do not implement production yet.

Static correction of the 12 class-B mismatches: they are **not**
proven VIEW-prev-workload-A vs WORLD-prev-workload-B. VIEW is
mapped and interpolates PREV→CURRENT; WORLD is unmapped so
TransformProcessor duplicates CURRENT as both endpoints. The
defect may be the same numeric `curFrameWeight` applied to
VIEW PREV→CURRENT vs WORLD CURRENT→CURRENT.

CPU diagnostic keeps class A–E, `MISMATCH_CALL_*`,
`COHORT_ALIGNED_LIVE` (unmappable, no synthesized WORLD PREV),
and adds `MAPPING_COHERENT_LIVE`: interpolate VIEW+WORLD only
when both have a valid PREV pair; otherwise snap both streams
to CURRENT for that GameCall/sample. Exact order
`((VIEW*LIVE_PROJ)*WORLD)*LIVE_POS`. Shader unchanged.
New keys `NOT_CAPTURED`. Evidence:
[[plan/evidence/p09-mapping-coherence-fallback-counterfactual]].

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste
`MAPPING_COHERENT_*` `MAPPING_COHERENT_IS_LOCALIZED`
`TEMPORAL_COHORT_MISMATCH_CORRELATES_WITH_PATHOLOGY`
`PATHOLOGICAL_ON_B_*` `CLASS_B_MAPPING_ASYMMETRY_EXPLAINS_CATASTROPHIC_GLITCH`
`CLASS_B_WORLD_PREV_RECOVERABLE`
`COHORT_ALIGNED_*` `MISMATCH_CALL_*`. No GPU readback. No commit.

## Prior (2026-09-15) — P09_TEMPORAL_COHORT_MISMATCH_CAUSAL_PROOF

User recapture of the previous diagnostic:

```text
COHORT_BOTH_MAPPED=12896
COHORT_VIEW_MAPPED_WORLD_UNMAPPED=12
COHORT_VIEW_MATCH_PREV_WORKLOAD_MISMATCH=0
LLLL_PATHOLOGICAL=288 (CLCL fixes all 288)
SAME_COHORT_COMPOSITE_MATCHES_SAFE_CLASS=NO
```

CPU classifies JOIN/pathology by temporal cohort class A–E and
runs `COHORT_ALIGNED_LIVE` without CURRENT snap. Evidence:
[[plan/evidence/p09-temporal-cohort-mismatch-causal-proof]].

## Prior (2026-09-15) — P09_EXACT_ORDER_VIEW_WORLD_TEMPORAL_COHORT_FORENSIC

User recapture: LIVE world-space JOIN preserves attachments
(MAX 1.006 vs endpoints 1). Hierarchy interpolation is `REFUTED`.
Previous MODELVIEW counterfactual reordered PROJECTION and is
`NO` for root cause. Exact-order factorial still holds: CLCL is
the safe class; VIEW is the strongest single factor. Do **not**
implement CURRENT VIEW+WORLD. Do not reopen hierarchy.

CPU diagnostic now traces VIEW/WORLD source cohorts (including
VIEW match-time prev workload vs ordinal interp prev), exact-order
LLLL/CLLL/LLCL/CLCL pathological attribution, homogeneous W on
that set, and SAME_COHORT_COMPOSITE
`lerp((PREV_VIEW*LIVE_PROJ)*PREV_WORLD,(CUR_VIEW*LIVE_PROJ)*CUR_WORLD)`
without moving PROJECTION. Status ~1 Hz. Shader unchanged.
New keys `NOT_CAPTURED`. Evidence:
[[plan/evidence/p09-exact-order-view-world-temporal-cohort-forensic]].

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste
`/tmp/tooie-rsp-endpoint-binding-status.txt`
`VIEW_PREV_SOURCE` / `VIEW_CURRENT_SOURCE` /
`WORLD_PREV_SOURCE` / `WORLD_CURRENT_SOURCE` /
`VIEW_WORLD_*_TEMPORAL_COHORT_ALIGNED` /
`VIEW_WORLD_USE_SAME_NUMERIC_WEIGHT` /
`VIEW_WORLD_WEIGHT_HAS_SAME_TEMPORAL_INTERVAL` /
`LLLL_PATHOLOGICAL_*` /
`VIEW_WORLD_INTERACTION_MANIFESTS_AS_HOMOGENEOUS_W_CANCELLATION` /
`SAME_COHORT_COMPOSITE_*` / `CLCL_COUNTERFACTUAL_VALID` /
`VIEW_WORLD_ROOT_CLASS`. No GPU readback. No commit.

## Prior (2026-09-15) — P09_VIEW_WORLD_INTERACTION_ROOT_DISCRIMINATOR

16-way factorial captured: CLCL is the minimum set matching
ALL_CURRENT. World-space JOIN recapture later closed hierarchy.
Previous MODELVIEW order is invalid. Evidence:
[[plan/evidence/p09-view-world-interaction-root-discriminator]].

## Prior (2026-09-14) — P09_TFPOS_FACTORIAL_TERM_ATTRIBUTION

World-matrix non-rigidity is `REFUTED` as the dominant P09 cause
(user recapture: decompose 123630/0 valid, LIVE JOIN MAX 48860 vs
RIGID_WORLD 48902, correlation NO). PR22 stays closed.

LIVE FAIL / POLICY_A FAIL / shader-local ALL CURRENT tfPos PASS.
CPU 16-way LIVE/CURRENT factorial of VIEW, PROJECTION, WORLD,
POSITION. User recapture: CLCL is the safe class. Evidence:
[[plan/evidence/p09-tfpos-factorial-term-attribution]].

## Prior (2026-09-14) — P09_WORLD_MATRIX_INTERPOLATION_RIGIDITY_FORENSIC

Complete JOIN recapture is VALID (`JOIN_PAIR_COUNT=80190`,
coincident=80190). POLICY_A kills JOIN phase disagreement
(LIVE max 46.4 → 0.121) but spatial JOIN P95/P99 barely
changes (LIVE 0.164/0.648 vs POLICY_A 0.164/0.633). Coherent
CURRENT scopes closed (`BEST_COHERENT_SCOPE=NONE`). Production
`worldMat` is `RigidBody::lerp` TRS+SLERP when decompose is
valid (`TransformGroup.decompose` default true), **not**
element-wise; fallback is element-wise 3x3. View cameras
default decompose **off** (element-wise). CPU diagnostic now
measures PREV/CURRENT/LIVE rigidity and a `RIGID_WORLD` JOIN
counterfactual. Runtime rigidity keys `NOT_CAPTURED`. Shader
unchanged. Evidence:
[[plan/evidence/p09-world-matrix-interpolation-rigidity-forensic]].

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste
`/tmp/tooie-rsp-endpoint-binding-status.txt`
`PREV_WORLD_*` / `LIVE_WORLD_*` /
`ENDPOINT_WORLDS_ARE_RIGID` /
`LIVE_WORLD_INTERPOLATION_INTRODUCES_NONRIGIDITY` /
`LIVE_NONRIGIDITY_CORRELATES_WITH_JOIN_ERROR` /
`RIGID_WORLD_JOIN_*` /
`RIGID_WORLD_MATERIALLY_REDUCES_JOIN_ERROR`. No GPU
readback. No commit.

## Prior (2026-09-14) — P09_DIAGNOSTIC_HEAP_CRASH_FORENSIC

JOIN recapture was VALID (`JOIN_PAIR_COUNT=53946`) until a native
`double free or corruption (out)` abort on feather pickup. Core
PID 217569: gfx thread and RT64 render worker were both in
`p09PercentileCopy` on the process-global residual vectors. Overlay
unload is not the free site. `WORLD_TO_BONE_BOUND_COUNT=0` is
last-dispatch, not a mapping contradiction. Status writes are now
tmp+rename. Do not infer `BEST_COHERENT_SCOPE`. POLICY_A kills JOIN
catastrophes but does not solve ordinary P95/P99 or the visual
glitch. Production `RSPProcessCS` unchanged. Evidence:
[[plan/evidence/p09-diagnostic-heap-crash-forensic]].

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste a complete
`/tmp/tooie-rsp-endpoint-binding-status.txt` including
`JOIN_DIAGNOSTIC_VALID`, `JOIN_PAIR_COUNT`, `LIVE_JOIN_*`,
`POLICY_A_JOIN_*`, `*_JOIN_PHASE_DELTA`,
`PER_TRANSFORM_CURRENT_VERTEX_PERCENT`,
`PER_DRAW_CURRENT_VERTEX_PERCENT`,
`COMPONENT_CURRENT_VERTEX_PERCENT`,
`PER_DRAW_CURRENT_GAMECALL_PERCENT`, `PER_*_TRIGGER_COUNT`,
`PER_*_JOIN` metrics, `BEST_COHERENT_SCOPE`,
`CPU_DIAG_SHARED_STATE_LOCKED`, `STATUS_WRITE_ATOMIC`. No GPU
readback. No commit.

## Prior (2026-09-14) — P09_JOIN_DIAGNOSTIC_REPAIR_AND_PER_DRAW_AUDIT

Prior capture was VALID for residual triangles (`34060`) and INVALID
for JOIN (`JOIN_PAIR_COUNT=0`). First dead gate: mixed-world edges
inside one triangle, with logical bones `79/100…` compared to
concrete `worldIndices`. Attachment is V7 JOIN topology (cross-face
/ cross-draw), not a mixed-transform triangle. PER_DRAW envelope
`MAX=1.026 P99=1` is independent of JOIN and was untrustworthy
without CURRENT-snap percent. CPU diagnostic now maps world→bone
from V7 OWN/JOIN DLs, pairs articulated nearest verts, and emits
PER_DRAW vertex/GameCall CURRENT percents. Production
`RSPProcessCS` unchanged.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste `JOIN_DIAGNOSTIC_VALID`,
`JOIN_PAIR_COUNT`, `LIVE_JOIN_*`, `POLICY_A_JOIN_*`,
`*_JOIN_PHASE_DELTA`, `PER_DRAW_CURRENT_VERTEX_PERCENT`,
`PER_DRAW_CURRENT_GAMECALL_PERCENT`,
`PER_DRAW_IS_EFFECTIVELY_ALL_CURRENT`, `PER_*_TRIGGER_COUNT`,
`BEST_COHERENT_SCOPE`. No GPU readback. No commit.

## Prior (2026-09-13) — P09_SAFE_ENDPOINT_SCREENSPACE_INTERPOLATION_FORENSIC

Valid residual capture (`33020` triangles). Zero-cross is the
dominant residual class (`32561`); triangle-consistent CURRENT does
not solve it (`MAX` still `6.89`). POLICY_A cuts catastrophes
(`121`→`13.6`) but the visual glitch remains.

CPU binding diag now also counterfactuals **homogeneous clip lerp**
and **NDC lerp** (crossings → CURRENT), LIVE NDC-segment overshoot,
and edge vs endpoint envelope. Shader/policy unchanged; do not
implement until recapture shows a candidate strongly dominates LIVE
and POLICY_A.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` (normal high-rate) and paste
`CLIP_LERP_*` / `NDC_LERP_*` /
`ZERO_CROSS_LIVE_NDC_OVERSHOOT_*` /
`BEST_TEMPORAL_POSITION_POLICY`. No GPU readback. No commit.

## Prior (2026-09-13) — P09_RESIDUAL_TRIANGLE_DIAGNOSTIC_REPAIR

Prior capture: intermediate feather verts 63805, target DL mask
`0x3fff`, but `RESIDUAL_TRIANGLE_COUNT=0` (`INVALID`). First dead
gate was CPU selection: `anyFeather` + `curOk`/`kP09EdgeFloor`, not
the V4 visible 0x62B face walk. Residual now reuses V4 target DLs
(14 + extra logicals), session-accumulates, and emits skip/valid
keys. Production shader/policy unchanged.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` (normal high-rate) and paste
`/tmp/tooie-rsp-endpoint-binding-status.txt` including
`RESIDUAL_DIAGNOSTIC_VALID`, `RESIDUAL_TRIANGLE_COUNT`,
`RESIDUAL_TARGET_DL_CALL_COUNT` / `RESIDUAL_FACE_COUNT` /
`RESIDUAL_*_SKIP_*`, POLICY_A_*/LIVE_*/CROSS_*/TRIANGLE_CURRENT_*.
No GPU readback. No commit.

## Prior (2026-09-12) — P09_RESIDUAL_VISUAL_DEFORMATION_AFTER_POLICY_A

User visual: normal high-rate `GLITCH`; selective POLICY_A `FAIL`;
shader-local all-CURRENT `PASS`; `TOOIE_DIAG_RSP_MARK_W_CROSSING=1`
`A / PASS`. `GPU_W_SIGN_BRANCH_EXECUTES: PROVEN`. Do not treat the
marker magnitude as a GPU measurement.

The visible feather defect is moderate screen-space stretch
(roughly 0.5x–1x apparent character size), not a 5×/50×
CPU “explosion”. `POLICY_A_EXPLOSION_COUNT=0` does **not** prove
visual correctness. `POLICY_A_MAX_EXPANSION_RATIO` only updated
when `ratio >= 5` (`OLD_POLICY_A_MAX_METRIC_WAS_FILTERED: YES`).

CPU binding diag now records unfiltered LIVE / POLICY_A /
TRIANGLE_CURRENT edge and NDC residuals vs ALL_CURRENT, plus
0/1/2/3 crossing-vertex classes. Production shader unchanged.
Residual numbers are `NOT_CAPTURED` until recapture. Evidence:
[[plan/evidence/p09-projective-w-production-fix]].

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` (normal high-rate, no force-CURRENT, no
W-crossing marker) and paste `/tmp/tooie-rsp-endpoint-binding-status.txt`
POLICY_A_* / LIVE_* / CROSS_* / TRIANGLE_CURRENT_* / ZERO_CROSS_*
keys. No GPU readback. No commit.

## Prior (2026-09-12) — P09_HUMAN_OBVIOUS_GPU_BRANCH_MARKER

The previous `TOOIE_DIAG_RSP_MARK_W_CROSSING=1` NDC offset
`(+0.20,-0.12)` was too subtle for binary user validation.
The same default-off env now applies a human-obvious bounded
horizontal NDC shift only inside `prevClipW * curClipW <= 0`
after `tfPos = curClip`: `tfPos.x += 0.70 * tfPos.w` (~35% of
screen width). User visual: `A / PASS`.
`GPU_W_SIGN_BRANCH_EXECUTES: PROVEN`. Production W-sign policy
unchanged when the env is unset. Evidence:
[[plan/evidence/p09-projective-w-production-fix]].

## Prior (2026-09-12) — P09_SELECTIVE_GPU_BRANCH_EXECUTION_PROOF

Authoritative CPU replay: intermediate feather 68770, endpoint
crossings 926, sidecar=witness 926/926, `POLICY_A_EXPLOSION_COUNT=0`.
Selective `prevW*curW <= 0` is **compiled and default-on** in a normal
`./build/TooieRecompiled` run. Force-CURRENT only ORs into the same
`tfPos = curClip` path. Remaining CPU-vs-GPU contradiction is
whether the GPU W-sign branch actually runs on visible feathers.
The first branch marker (`+0.20,-0.12` NDC) was too subtle.
Evidence: [[plan/evidence/p09-projective-w-production-fix]].

## Prior (2026-09-12) — P09_RSP_SHADER_EXECUTION_AND_BUFFER_VISIBILITY_PROOF

Live session sidecar/witness A/B is **equal** (`108569`/`108569`,
feather `1698`/`1698`, max clip-W and matrix diffs `0`).
`SIDECAR_CONTENT_MATCHES_WITNESS: PROVEN`. Target DL mask `0x3fff`,
feather world mask `0x1ff`. Construction/pair/aspect/W-sign policy
unchanged.

Remaining question: why the visual glitch remains if the GPU
candidate should switch the 1698 crossing feather vertices to
CURRENT. Static upload: endpoint viewProj/world are submitted in
the same BufferUploader batch as interpolated t17/t18, copied
before `RSPProcess` dispatch, after-copy barrier is
`RenderBarrierStage::ALL`, and the graphics worker `wait()`s before
the next overwrite. SPIR-V of the production blob: Binding 22/23,
`prevW*curW <= 0`, CURRENT clip phi, phi feeds `dstPos`. Tasks 1–3
do not close the remaining visual fail. Intermediate-only counts
are instrumented, not yet recaptured live.

Default-off `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` forces
`RSPProcessCS` `tfPos` to endpoint CURRENT (shader-local only).
Evidence: [[plan/evidence/p09-projective-w-production-fix]].

## Prior (2026-09-12) — P09_ENDPOINT_SIDECAR_VS_WITNESS_RECONCILIATION

User live bind file: correct t22/t23 **buffer objects**;
session A/B later proved sidecar content equals the CPU witness
(`108569`/`108569`). The earlier last-dispatch `0` vs VERSION=4
`1292` was last-dispatch overwrite, not a construction bug.
Policy unchanged.

## Prior (2026-09-12) — P09_PROJECTIVE_W_PRODUCTION_CANDIDATE

VERSION=4: 75 explosions, all with clip.w sign change. Per-vertex
current-endpoint fallback implemented. User visual **FAIL**.
Superseded by the failed-candidate audit above.

## Prior (2026-09-09) — P09_RELATIVE_VIEW_WORLD_INTERPOLATION_ROOT_CAUSE

VERSION=2: live explosions 51, SIMPLE also 51 (`ratioS≈live`).
PR23 SIMPLE is `REFUTED` for the captured explosions. Do not port
PR22/23. World DECOMPOSE is not a sufficient root.

Dual B/C signatures: CURRENT viewProj fixes `curId=974`; CURRENT
world fixes `curId=1019/1138`. Working class was independent RT64
interpolation of coupled view and world; VERSION=3 **invalidated**
the combined-relative experiment. Evidence:
[[plan/evidence/p09-relative-view-world-interpolation-root-cause]].

## Prior (2026-09-09) — P09_BONED_INTERPOLATION_COUNTERFACTUAL

Live untagged 0x62B worlds interpolate as **DECOMPOSE** (`AUTO` →
`decompose=true`; `RigidBody::lerp` TRS of already-composed worlds).
Wrong-occurrence pairing is **not** proven for the 44 explosions.
Do **not** add another mixed-time CURRENT visual override.

The same default-off CPU witness (`TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1`)
now reconstructs `G_EX_INTERPOLATE_SIMPLE` offline via
`lerpMatrixComponents` and does **not** feed it to rendering.
Prior status lacked endpoint matrices, so SIMPLE counts need a
**new capture** (`VERSION=2`). Evidence:
[[plan/evidence/p09-boned-interpolation-counterfactual]].

`NEXT_SINGLE_ACTION`: user recapture with
`TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1` only; do not implement
Tooie GeoCmd tagging until SIMPLE numbers land. No GPU readback.
No `FORCE_CURRENT_FRAME`.

## Prior (2026-09-09) — P09_WORLD_OCCURRENCE_IDENTITY_ROOT_CAUSE_AUDIT

`39` worldIndex values are a session union. V5C token-only.
Wrong-occurrence pairing not proven. CPU term is WORLD DECOMPOSE.
Evidence: [[plan/evidence/p09-world-occurrence-identity-root-cause-audit]].

## Prior (2026-09-09) — P09_PASSIVE_HIGH_RATE_TEMPORAL_COHERENCY_WITNESS

Default-off CPU witness reconstructed the live intermediate clip
equation. Capture: 58.596× explosion, vel always 0, view/proj stable.
Evidence: [[plan/evidence/p09-passive-high-rate-temporal-coherency-witness]].

## Prior (2026-09-09) — P09_TEMPORAL_COHERENCY_ROOT_CAUSE

Normal high framerate `GLITCH` (including after removing the
projection-only diagnostic: no leak). Original framerate `FIXED`.
V5C nine-world CURRENT `SAME`. Global
`TOOIE_DIAG_FORCE_CURRENT_FRAME=1` `FIXED`. Projection-only CURRENT
is `INVALID_TEMPORALLY_INCOHERENT` (glitch plus severe ghosting).
That experiment is **not** causal evidence that ProjectionProcessor
is the original root. Its code was removed.

Do not add another per-component CURRENT override. Do not turn
`FORCE_CURRENT_FRAME` into production: it repeats current game frames
instead of interpolating them. Remaining class: interpolation at a
shared intermediate `w`, or AUTO match pairing of previous
view/world. Evidence:
[[plan/evidence/p09-temporal-coherency-root-cause]].

## Prior (2026-09-09) — P09_HIGH_RATE_FORCE_CURRENT_PROJECTION

Projection-only CURRENT is `INVALID_TEMPORALLY_INCOHERENT`. Code
removed. Evidence:
[[plan/evidence/p09-high-rate-force-current-projection]],
[[plan/evidence/p09-temporal-coherency-root-cause]].

## Prior (2026-09-09) — P09_HIGH_RATE_FORCE_CURRENT_FRAME

Global current-frame diagnostic with high refresh and extra renders is
`USER_OBSERVED` `FIXED`. V5C remained `SAME`. Evidence:
[[plan/evidence/p09-high-rate-force-current-frame]]. Isolation:
[[plan/evidence/p09-high-rate-force-current-projection]].

## Prior (2026-09-09) — P09_FEATHER_NO_INTERP_V5C

V5B `NO_INTERP` covered only 79, 100–105. Visible JOIN triangles
directly use parent world transforms 107 (`0x592`/`0x61E`) and 108
(`0x4B0`). V5C extended the same default-off seam to all nine bones.
User visual: `SAME`. Evidence:
[[plan/evidence/p09-feather-no-interp-v5c]].

## Prior (2026-09-09) — P09_BANJORECOMP_TRANSFORM_TAGGING_PORTABILITY_AUDIT

BanjoRecomp PR #22/#23 is a real explicit `gEXMatrixGroup` architecture
Tooie does not have. Live 0x62B uses ungrouped RT64 AUTO. The tagging
port is still not a ready production fix. V5B did **not** cover JOIN
parents 107/108; that interpolation-closure claim is superseded by
[[plan/evidence/p09-feather-no-interp-v5c]]. PR #23 SIMPLE is
causally relevant again as a *candidate* only if V5C is
`FIXED`/`BETTER`. Evidence:
[[plan/evidence/p09-banjorecomp-transform-tagging-portability-audit]].

## Prior (2026-09-08) — P09_ANIMATION_CLIP_TRANSITION_AND_AUTHORED_KEY_FORENSIC

V1 capture plus ROM keys: intra-clip sample time is continuous.
The five negative time events are clip changes whose new t equals
one ADE4 step after start-at-0. `func_800E0170` / `func_800E04C0`
sample cart flag-`0x16` clips. Clip 22 bone 107/108 raw 0x28 matches
authored cubic Euler/scale (max rot error ~1e-6 rad, scale ~1e-8).
The old 30/60/120° 107 steps are authored walk↔idle interpolations
of a 180° Y rest vs identity, not a sampler bug. Do not call GPU
bug. No production fix. Evidence:
[[plan/evidence/p09-animation-clip-transition-and-authored-key-forensic]].

`NEXT_SINGLE_ACTION`: user visual A/B vs original N64/Angrylion of
combined 0x62B Kazooie walk to see whether the authored 107 scale
ramp and 108 nod are the remaining visible wobble. Do not add a
witness. Do not GPU-readback. Do not production-fix. Do not start
presentation.

## Prior (2026-09-08) — P09_FEATHER_ANIM_SAMPLE_TIME_V1

Default-off CPU witness `TOOIE_DIAG_FEATHER_ANIM_SAMPLE_TIME=1`
captured 1451 `baanim_update` ticks and 384 CSV rows. ADE4 only
advances the timer; `func_800E0170` writes 0x28 records. Luna
`ANIMATION_SAMPLE_TIME_BUG` from negative deltas is `CORRECTED` in
[[plan/evidence/p09-animation-clip-transition-and-authored-key-forensic]].
Evidence: [[plan/evidence/p09-feather-anim-sample-time-v1]].

## Prior (2026-09-08) — P09_UPSTREAM_CURRENT_BONE_TRANSFORM_SEMANTIC_FORENSIC

Guest current-matrix compose is `parent ⊗ child` XYZW in `func_800AE8A8`,
then AEAA0/AEBA8 into `AnimMtxList+0x50+(index<<6)`. Offline 991-frame
0x62B window: product error ≤ 1.6e-7 rad. Additive bytes 8/9 are 0.
Bone 107 carries uniform scale and the large quantized local steps
(30°/60°/120° at 1548–1557). Plume roots 103/100 stay locally smooth.
Clip id and `baanim` sample time are not in existing dumps. Do not call
GPU bug. No production fix. Evidence:
[[plan/evidence/p09-upstream-current-bone-transform-semantic-forensic]].

`NEXT_SINGLE_ACTION`: one bounded CPU witness at `baanim_update` /
`0x8008ADE4` for clip id, sample time/frame, and 0x28 TRS of bones
79, 100–105, 107, 108 plus one body bone. Do not GPU-readback. Do not
start presentation/compositing. Do not reopen downstream renderer stages.

## Prior (2026-09-08) — P09_VISIBLE_FEATHER_ALPHA_COVERAGE_TEXTURE_FORENSIC

The twelve plume DLs locally load CI4 64x32 + pal16, set
`G_TT_RGBA16` and `G_AC_THRESHOLD` during triangles, then restore
both. Crest `0x4A6`/`0x4B0` and head `0x48F` use `G_TEXTURE on=0`
and a SHADE-alpha combine with no AC/TT. ST is static ROM; no
`G_MODIFYVTX` ST; texgen cleared. Texture hashes live on the same
GameCall as the faces. AC/TT cannot cover crest and have no
movement-only material driver. Do not call GPU bug. No production
fix. No new capture. Evidence:
[[plan/evidence/p09-visible-feather-alpha-coverage-texture-forensic]].

`NEXT_SINGLE_ACTION`: bounded static / command-stream audit of frame
presentation / compositing / final-image association for the 14
visible feather DLs. Do not add an alpha, ST, or texture witness.
Do not GPU-readback.

## Prior (2026-09-07) — P09_POST_POSITION_DRAW_TEMPORAL_AND_DEPTH_STATE_FORENSIC

Face slices, GameCalls, and GameFrame matching stay on the producing
workload (16-gen V7: 0 association mismatches, exclusive `game_call`
per DL, `fb_pair=3`). Ring reuse is blocked by GPU wait + barrier.
The 14 DLs do not set zCmp/zUpd/ZMODE/primDepth; RT64 raster depth is
otherMode, inherited and body-shared. Do not call GPU bug. No
production fix. No new capture. Evidence:
[[plan/evidence/p09-post-position-draw-temporal-and-depth-state-forensic]].

`NEXT_SINGLE_ACTION`: bounded static / command-stream audit of
color/alpha/coverage and texture-coordinate paths on the 14 visible
feather DLs. Do not add a position, cull, or temporal witness. Do not
GPU-readback.

## Prior (2026-09-07) — P09_VISIBLE_FEATHER_CULLING_WINDING_FACE_VISIBILITY_FORENSIC

Pre-append triangle rejection is absent on all 14 visible feather DLs
(`EXACT_MATCH = static_tri * 1258`, remainder 0). The 14 DLs (and body
`0x48F`/`0xBC`) preserve inherited `G_CULL_*` and never set it; 27 other
`0x62B` DLs explicitly clear cull. `G_CULLDL` is absent and a no-op in
RT64. No negative-determinant feather matrix. Culling/winding status
`A`. Do not call GPU bug. No production fix. No new capture. Evidence:
[[plan/evidence/p09-visible-feather-culling-winding-face-visibility-forensic]].

`NEXT_SINGLE_ACTION`: bounded static / command-stream audit of depth /
occlusion / draw temporal association on the 14 visible feather DLs. Do
not add another position witness. Do not GPU-readback.

## Prior (2026-09-07) — P09_BOUNDED_CREST_POST_HANDOFF_TRANSFERABILITY_AUDIT

Crest `0x4A6`/`0x4B0` consumes the same `Q'[4]`, V=0, RSPProcess layout,
TransformProcessor LERP path, and RasterVS path as the plumes. Nested
`0x03000010` runs before crest `G_VTX`; both crest DLs have zero
`G_MODIFYVTX`. Join slots 0–4/27–31 do not survive into RSPProcess
after V3 global-vertex/worldIndex handoff. Full visible-feather
post-handoff position path is `CLOSED`. Do not call GPU bug. No
production fix. No new capture. Evidence:
[[plan/evidence/p09-bounded-crest-post-handoff-transferability-audit]].

`NEXT_SINGLE_ACTION`: bounded static / command-stream audit of triangle
culling / winding / face visibility on the 14 visible feather DLs. Do
not add another position witness. Do not GPU-readback.

## Prior (2026-09-07) — P09_VISUAL_GEOMETRY_IDENTITY_AND_SCOPE_FORENSIC

The six closed right-plume DLs do **not** cover the user-visible
"head feathers" set. Left plume is a complete parallel six-DL tree
(`0x5C0/0x5D8/0x5F0/0x608/0x61E/0x636`); crest is `0x4A6`/`0x4B0`.
V6B already submitted those DLs. V7/Q'/RSPProcess remain closed only
for the right six. GeoCmd parent of both plumes is 107, not head 108
(`CORRECTED`). No production fix. No new witness. Evidence:
[[plan/evidence/p09-visual-geometry-identity-and-scope-forensic]].

`NEXT_SINGLE_ACTION`: extend V7 exact-admission / CPU-handoff to the
eight additional DLs (left six + crest pair). Do not instrument
RSPProcess. Do not GPU-readback. Do not implement in the forensic turn
that produced this note.

## Prior (2026-09-07) — P09_NARROW_RSPPROCESS_STRUCTURAL_FORENSIC

`RSPProcessCS` is statically closed as a target-local failure.
V7 CPU handoff stays `CORRECT`. Q' stays refuted as localized primary
(shared viewProjIndex 4, 0 large deltas). RSPModify absent. World lerp
closed (V5B). No production fix. No new witness. Evidence:
[[plan/evidence/p09-narrow-rspprocess-structural-forensic]],
[[plan/evidence/p09-feather-postprocess-q-witness-v1]].

`NEXT_SINGLE_ACTION`: determine whether the visible wobble is actually
produced by the six admitted right-plume DLs (`0x534/0x54C/0x564/0x57C/0x592/0x5AA`)
versus other `0x62B` geometry, using existing captures and static
draw/DL identity. Do not instrument RSPProcess. Do not GPU-readback.

## Prior (2026-09-07) — P09_FEATHER_POSTPROCESS_Q_WITNESS_V1

Bounded CPU-only Q' witness is implemented. V7 VERSION=2 CPU handoff
stays `CORRECT`. World lerp stays closed as primary (V5B). No production
fix. No V8. No GPU readback. Evidence:
[[plan/evidence/p09-feather-postprocess-q-witness-v1]],
[[plan/evidence/p09-post-handoff-structural-forensic]].

`NEXT_SINGLE_ACTION`: user runs one capture
`TOOIE_DIAG_FEATHER_POSTPROCESS_Q=1 ./build/TooieRecompiled` and inspects
`/tmp/tooie-kazooie-feather-postprocess-q-v1-status.txt` (`PRELIMINARY_CLASS`,
`TARGET_SAMPLES`, `BODY_CONTROL_SHARES_TARGET_Q`).

## Prior (2026-09-07) — P09_POST_HANDOFF_STRUCTURAL_FORENSIC

V7 VERSION=2 CPU handoff remains `CORRECT`. Static post-handoff audit:
target DLs `0x534/0x54C/0x564/0x57C/0x592/0x5AA` and nested `0x03000040`
(if `seg3=Gfx[0]`) contain **zero** `G_MODIFYVTX`. First remaining
position writer is `RSPProcessCS`. World-matrix lerp stays closed (V5B).
ProjectionProcessor still rewrites Q after handoff (not V5B-covered;
weak for feather-only). Evidence:
[[plan/evidence/p09-post-handoff-structural-forensic]].

`NEXT_SINGLE_ACTION`: implement one bounded CPU reconstruction of
RSPProcess inputs after TransformProcessor+ProjectionProcessor (W', Q',
P, V, viewProjIndex) versus the V7-proven handoff. Do not implement a
production fix. Do not create a broad V8. Do not GPU-readback.

## Prior (2026-09-07) — P09_V7_HANDOFF_COMPARATOR_AND_JOIN_ROLE_REPAIR

Exact direct-DL admission is working (`APPEND_EXACT_TARGET_MATCH=40124`,
mask `0x3F`). Do not reopen low-page matching. `byte_mismatch=3` on
`INITIAL_HANDOFF_OK` rows is a diagnostic stride-4 vs stride-3
`posFloats` artifact, not a CPU upload bug. Join `0x54C`/`0x564`/`0x592`
roles are the proven child slots 0–3 / parent slots 28–31 map. Schema is
now `P09_FEATHER_HANDOFF_V7 / VERSION=2`. Evidence:
[[plan/evidence/p09-feather-v7-cpu-handoff-implementation]].

`NEXT_SINGLE_ACTION`: user runs one cleaned VERSION=2 capture and inspects
`APPEND_POSITION_MATCH` and `JOIN_*_ROLE_RESOLVED`.

## Prior (2026-09-07) — P09_V7_EXACT_TARGET_ADMISSION_REPAIR

The durable `0x62B` model-context bridge is working (1297 normalized
publications, physical base `0x229070`, 0 clears). Zero validated targets in
the last capture were caused by a base-less `0xFFF` candidate prefilter that
systematically missed the six true DLs by `+0x70`. Admission is now six
full-address comparisons against `physical(model_bin)+0x2670+logical*8`.
Evidence: [[plan/evidence/p09-feather-v7-cpu-handoff-implementation]].

`V7_EXACT_TARGET_ADMISSION_REPAIRED=YES`; `JOIN_UNRESOLVED_IS_BLOCKER=YES`;
no causal root is claimed. One cleaned V7 capture is authorized.

`NEXT_SINGLE_ACTION`: user runs one bounded V7 capture
`TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled` and inspects
`APPEND_EXACT_TARGET_MATCH` plus the six per-target exact counts.

## Prior (2026-09-05) — P09_V7_CPU_HANDOFF_IMPLEMENTATION

Implementation evidence: [[plan/evidence/p09-feather-v7-cpu-handoff-implementation]].
The bounded default-off `P09_FEATHER_HANDOFF_V7 / VERSION=1` witness is now
implemented in RT64 owning source. Workload-owned generation and append records
feed the post-append → `State::fullSync` face-slice/source comparison; render
selection records capture actual matrix-source predicates and referenced matrix
bytes. Target quotas are independent for `0x534/0x54C/0x564/0x57C/0x592/0x5AA`.

`V7_IMPLEMENTED=YES`; `JOIN_UNRESOLVED_IS_BLOCKER=YES`; no causal root is
claimed. Build and static checks pass. No game, GPU readback, synchronization,
commit or push was performed. Manual capture is required next:
`TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled`.
`NEXT_SINGLE_ACTION`: user runs one bounded V7 capture and inspects its CSV/status
pair while retaining incomplete join coverage.

The subsequent analysis pass found that both requested V7 output files were
absent; see [[plan/evidence/p09-v7-cpu-handoff-capture-analysis]]. Older V6/V6C
files were excluded, and no causal root was selected. The activation repair now
creates the status heartbeat at enable and records raw APPEND/fullSync/render
hook counters; the next action is one cleaned direct-binary V7 capture.

## Prior (2026-09-05) — P09_POST_TRIANGLE_ARCHITECTURAL_REVIEW

Static review: [[plan/evidence/p09-post-triangle-architectural-review]].
Accepted V6C: own meshes `0x5AA/0x57C/0x534` provenance correct, zero stale;
`0x592` has 468 resolved expected joins and 3,252 unresolved of 3,720.
`0x03000040 -> 0x0011C870`, zero triangles: leftover hypothesis remains REFUTED.
V5B NO_INTERP/SIMPLE exclusions remain closed.

**CORRECTED:** `FIRST_BAD_LAYER=AFTER_RSP_TRIANGLE_SUBMISSION` is not proven.
V6C observes pre-append inputs; distal joins `0x54C/0x564` are not targeted.
The retained CSV contains no unresolved target join samples or final uploads.
`JOIN_UNRESOLVED_IS_BLOCKER=YES` for exclusive downstream localization;
`JOIN_CAN_EXPLAIN_PRIMARY_VISUAL_ARTIFACT=UNKNOWN`.
`ROOT_CLASS=UNKNOWN / INCOMPLETE_JOIN_AND_CPU_HANDOFF_EVIDENCE`, confidence LOW.

RT64 preserves per-vertex world indices through batching; no normal-path
compaction/rebase found. Raster position first consumes the index in
`RSPProcessCS`, not `RSPWorldCS`. First downstream witness: actual face append
versus `State::fullSync` mesh slice/upload sources, with a bounded render-matrix
selection continuation. `workloadId=0` during construction requires a separate
diagnostic generation, not a reused ring-slot key.

`V7_IMPLEMENTED=NO` at review time; superseded by the implementation section
above. Both dirty worktrees and dirty Plume remain preserved.

## Prior (2026-09-04) — P09_REPAIR_V6_TRIANGLE_WITNESS_AFTER_ZERO_TRI

A real V6 capture wrote schema `P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6`
with `TRACKED_BONES_WITH_TRI_MASK=0x00` and no triangle CSV. That is
**not** missing feather geometry: V5B already had GMTX/VERTEX/POSTMATCH
`0x7F`. F3DEX2 `G_TRI1`/`G_TRI2` do call the hooked
`RSP::drawIndexedTri`. V6 stored `FeatherWorldHit` under matrix/match
workload and cleared `p09DiagnosticMatrixToken` at the first
world-transform G_VTX, so TRI rows never passed the post-filter.

V6B classifies from per-vertex `worldIndices` first, keeps raw GBI/RSP
counters, and does not require a tracked active transform or live token
for initial visibility. Default-off. CSV
`/tmp/tooie-kazooie-feather-v6b-tri.csv`.

```text
TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS: REFUTED_FOR_VISIBLE_FEATHER_CHAIN
SIMPLE_INTERPOLATION_CAUSAL_STATUS: REFUTED_AS_PRIMARY_FIX
FULL_VISIBLE_CHAIN_AUTO_IDENTITY_STATUS: CORRECT
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6B
V6B_IMPLEMENTED: YES
V6_HOOK_IS_ON_ACTUAL_RUNTIME_PATH: YES
V6_DESIGN_BUG: WORLDHIT_KEYED_BY_MATRIX_WORKLOAD_NOT_DRAW_WORKLOAD_AND_TOKEN_CLEARED_BEFORE_TRIANGLE
USER_MANUAL_TEST_REQUIRED_NOW: YES
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
NEXT_SINGLE_ACTION: user runs one V6B capture TOOIE_DIAG_SECONDARY_SWAY=1; inspect RAW_TRI_* then TRI_CLASSIFIED_COUNT and /tmp/tooie-kazooie-feather-v6b-tri.csv
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
```

Do not reopen matrix identity, AUTO previous selection, bone103 source,
scale, determinant, quaternion continuity, F2L dest, G_MTX operand,
modelview token lifetime, or SIMPLE as the primary fix.

Feather G_VTX sources are static segment-1 tables. RT64 stores
`worldIndices=curTransformIndex` at `setVertexCommon`. F3DEX2 velocity is 0;
displayed GPU position uses current `worldMats[per-vertex index]`. Join DLs
mix child slots 0–3 with parent 28–31 by design. Own meshes reuse cache
slots 0–N; reload updates transform state. Type-0x18 does not flush or clear
the vertex cache. First post-matrix difference from snout: nested type-2
104/105 plus `G_DL 0x03000040` before own `G_VTX` versus snout first-child
type-0x18 and `G_DL 0x03000010`. Segment 3 base is `RUNTIME_REQUIRED`.

V6 is diagnostic-only, default-off. CSV `/tmp/tooie-kazooie-feather-v6-tri.csv`.
Classes: `MATCH_ACTIVE` / `MIXED_JOIN` / `STALE_OR_LEFTOVER` /
`MIXED_AND_NE_ACTIVE`.

Evidence: [[plan/evidence/p09-kazooie-feather-v6-post-matrix-vertex-path]],
[[plan/evidence/p09-kazooie-feather-v5-full-chain]],
[[plan/evidence/p09-session-handoff]],
[[reverse-engineering/systems/transform-interpolation]],
[[reverse-engineering/systems/geocmd]],
[[reverse-engineering/characters/kazooie]].

## Prior (2026-09-04) — P09_KAZOOIE_FEATHER_V5B_DIAGNOSTIC_GATE_REPAIR

The V5 no-interpolation capture was incomplete: only boneAnimID 103 reached
the token/match/post-match path because `secondary_sway_note_dcf48()` rejected
the six `Sibling62B` feather nodes. The diagnostic gate is now repaired for
the seven proven live `0x62B` identities `79, 100, 101, 102, 103, 104, 105`.
Acceptance requires the live resource, one of those animation IDs, and the
existing proven type-2 node/index mapping; unrelated `0x62B` siblings remain
rejected.

Each accepted transform carries its resource ID into its own matrix token.
The V4 modelview-parallel token stack, per-depth lifetime, child G_MTX slot,
G_POPMTX restore, bounded type-0x18 restore alias, full-width GPR pointer
preservation, RT64 identity matching, and guest matrices are unchanged. The
diagnostic now counts GUEST, MATRIX, GMTX, VERTEX, MATCHED, POSTMATCH, and
NO_INTERP independently for each tracked bone and emits masks. Complete
coverage is mask `0x7F`; any other mask reports incomplete.

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V5B
SIBLING62B_TARGETED_ACCEPT_IMPLEMENTED: YES
UNRELATED_62B_SIBLINGS_STILL_REJECTED: YES
FULL_CHAIN_OWN_TOKEN_SUPPORT: YES
FULL_GPR_POINTER_PRESERVATION_RETAINED: YES
MODELVIEW_STACK_TOKEN_LOGIC_RETAINED: YES
FULL_CHAIN_NO_INTERP_OVERRIDE_SUPPORTED: YES
IDENTITY_MATCHING_UNCHANGED: YES
GUEST_MATRICES_UNCHANGED: YES
FULL_CHAIN_COVERAGE_STATUS_IMPLEMENTED: YES
CAPTURE_CAN_SELF_DECLARE_INCOMPLETE: YES
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: NO (await V5B capture and complete masks)
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
NEXT_SINGLE_ACTION: user runs exactly one V5B no-interpolation capture; inspect masks before interpreting its visual result
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
```

No game was run during this repair.

## Prior (2026-09-04) — P09_KAZOOIE_FEATHER_TRACE_GMTX_TO_ACTUAL_VISIBLE_DISPLAYLIST_AND_RT64_VERTEX_WORKLOAD

Banjo snout V5 remains the clean control (`NO_P09_CAUSAL_FAILURE_OBSERVED`).
Primary token is still `0x62B/+0x124A0` idx 30 anim 103 (`PROVEN_FOR_CAPTURE`).
Guest matrix/rotation remain valid (`FIRST_BAD_GUEST_STAGE: NONE_OBSERVED`).
The fresh V4 capture reaches the real RT64 match path: 328 call contexts,
500 valid candidates, 126 `matchTransform` matches, and seven one-to-one
allocator drops. Five drops are `CURRENT_ALREADY_MAPPED` and two are
`PREVIOUS_ALREADY_USED`; no semantic previous-frame mismatch was observed.
The V4 witness has no final transform-to-DL/triangle association, so no drop
is clearly causal for the visible `0x5AA` feather, while visual causality for
all seven remains `UNKNOWN`. Do not reopen animation producer RE.

Static 0x62B reconstruction (`PROVEN` `STATIC_ROM`/`STATIC_MIPS`): bone 103
is compact DCF48. Its first child is nested type-2 104, **not** a type-0x11
`dl=0x534` (that tip is bone 105 `+0x124D0`). Own visible GeoCmd is
`+0x12580` type-0x11 `dl=0x5AA`; join is `+0x12550` type-0x18 `dl=0x592`.
All those DLs contain F3DEX2 `G_VTX=0x01` directly plus nested
`G_DL 0x03000040`. Command order is `MIXED`. V3 `VERTEX_BIND_COUNT=0` is
`TOKEN_EXPIRES_BEFORE_CHILD_DL`: child 104/105 G_MTX cleared a scalar
token before any G_VTX. Snout V5 bound because snout's first child is
type-0x18 with G_VTX immediately after destA.

V4 (default-off) keeps the dest-keyed G_MTX token on a modelview-parallel
stack, restores it on `G_POPMTX`, and treats type-0x18 same-depth G_MTX as
a one-shot restore alias. Binding reaches `setVertexCommon` and the real
`GameFrame::matchTransform` path. The seven drops are not sufficient to target
the allocator because none is proven visible-causal. A diagnostic-only
post-match interpolation A/B is now implemented: with
`TOOIE_DIAG_SECONDARY_SWAY=1 TOOIE_DIAG_FEATHER_INTERP_SIMPLE=1`, only the
unique primary feather match gets SIMPLE-like component interpolation after
the existing current/previous pair is selected. AUTO identity and V4
matching telemetry remain unchanged; the switch is default-off.

```text
RE_KNOWLEDGE_BASE_PATH: docs/reverse-engineering/
PRIMARY_WITNESS: KAZOOIE_CARRY_BANJO_HEAD_FEATHER
LIVE_FEATHER_RESOURCE: 0x62B
PRIMARY_FEATHER: 0x62B+0x124A0 idx30 anim103
PRIMARY_FEATHER_VISIBLE_GEOCMD: 0x12580 (type 0x11 dl=0x5AA) and 0x12550 (type 0x18 dl=0x592)
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V4
GUEST_MATRIX_VALID: YES
GUEST_ROTATION_VALID: YES
FIRST_BAD_GUEST_STAGE: NONE_OBSERVED
FEATHER_COMMAND_ORDER: MIXED
DOES_PRIMARY_DL_CONTAIN_VERTEX_DIRECTLY: YES
WHY_V3_SET_VERTEX_WITNESS_SAW_ZERO_ROWS: TOKEN_EXPIRES_BEFORE_CHILD_DL
CORRECT_TOKEN_BINDING_SEAM: setVertexCommon with modelview-stacked token + type-0x18 restore alias
FEATHER_V4_IMPLEMENTED: YES
V4_EXPECTED_TO_REACH_GAMEFRAME_MATCH: PROVEN (133 bound tokens)
V4_EXPECTED_TO_REACH_MATCHTRANSFORM: PROVEN (126 matched)
FEATHER_RT64_MATCH_STATUS: AUTO_IDENTITY_CORRECT_OBSERVED (126 matched / 7 allocator drops)
V4_DROP_AUDIT: NO_CLEARLY_CAUSAL_DROP; 5 CURRENT_ALREADY_MAPPED, 2 PREVIOUS_ALREADY_USED
MISSING_GROUP_CAUSAL_STATUS: DIAGNOSTIC_A_B_READY (not a production conclusion)
BANJO_SNOUT_ROOT: NO_P09_CAUSAL_FAILURE_OBSERVED
PRODUCTION_FIX: NO
ROOT_CLASS: MORE_EVIDENCE_REQUIRED; OBSERVABILITY_REPAIRED_STATIC
ROOT_CONFIDENCE: HIGH observability / LOW graphical root
P09_KNOWLEDGE_PROMOTED_TO_RE_DOCS: YES
AB_SWITCH_IMPLEMENTED: YES (default-off; primary 0x62B/+0x124A0 bone30 anim103)
AB_IDENTITY_MATCHING_UNCHANGED: YES
INTERP_MODE_CONTROL_COUNT: IMPLEMENTED
INTERP_MODE_SIMPLE_OVERRIDE_COUNT: IMPLEMENTED
BUILD_RESULT: PASS (TooieRecompiled; Tooie HEAD ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf dirty; RT64 6f1c2d99 dirty rsp token stack)
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
NEXT_SINGLE_ACTION: user runs one B capture with SIMPLE override and reports visual classification only
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
USER_MANUAL_TEST_REQUIRED_NOW: YES (one B run; no control rerun)
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## CURRENT (2026-09-05) — V7 EXACT TARGET VALIDATION IMPLEMENTED

The unsafe low-page match remains only a candidate prefilter. Candidates now
retain full `current_dl` and are validated against the game-side model-bin
identity using the V6C full runtime-address formula. Only validated records
increment tracked append counts or `OBSERVED_TARGET_MASK`; unvalidated
candidates are bounded and labeled with rejection reasons.

```text
V7_TARGET_MATCH_COLLISION_SAFE_FOR_CAUSAL_USE: YES (after exact validation)
LOW_PAGE_RESOLVER_ROLE_AFTER_REPAIR: CANDIDATE_PREFILTER_ONLY
TARGET_VALIDATION_USES_FULL_ADDRESS: YES
TARGET_VALIDATION_USES_MODEL_CONTEXT: YES
V7_VALIDATION_STRENGTH_EQUIVALENT_TO_V6C: YES
TARGET_CANDIDATE_COUNTERS_IMPLEMENTED: YES
TARGET_VALIDATED_COUNTERS_IMPLEMENTED: YES
TARGET_REJECTED_COUNTERS_IMPLEMENTED: YES
OBSERVED_TARGET_MASK_COUNTS_VALIDATED_ONLY: YES
TRACKED_APPEND_COUNTS_VALIDATED_ONLY: YES
MANDATORY_COVERAGE_USES_VALIDATED_ONLY: YES
UNVALIDATED_CANDIDATE_CAN_REACH_CAUSAL_ANALYSIS: NO
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
TOOIE_DIFF_CHECK: PASS
RT64_DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES (one capture authorized; agent did not run it)
USER_TEST_COMMAND: cd /home/wesley/projetos/N64-Recomp/TooieRecomp && rm -f /tmp/tooie-kazooie-feather-v7-cpu-handoff.csv /tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt && TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled
NEXT_SINGLE_ACTION: run one bounded V7 capture with the exact-identity bridge
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## CURRENT (2026-09-05) — V7 LOW-PAGE COLLISION AUDIT

The admission repair is not collision-safe for causal use. The runtime fallback
implemented in `p09V7TargetIndex` is:

```text
(current_dl & 0xFFFu) == ((0x2670u + logical_index * 8u) & 0xFFFu)
```

It compares 12 address bits and discards the upper 20 bits. For example,
`0x5010` and `0x6010` both satisfy the `0x534` page pattern `0x010`, while
being different display-list addresses. The exact V6C formula includes the
physical model-bin base and is therefore not equivalent to this fallback.

`INITIAL_HANDOFF` checks face-slice, source-array, matrix, and (when present)
`sourceDl == record.currentDl`; those are internal consistency checks and do
not independently establish model identity. `p09V7RecordAppend` writes the
candidate and updates `OBSERVED_TARGET_MASK` before those checks. A false
low-page candidate can therefore reach the V7 CSV and causal completeness
logic as a trusted target. The repository has no complete static table of all
runtime model/DL addresses, so an attributable non-feather collision count is
`UNKNOWN`, not zero.

```text
V7_TARGET_MATCH_COLLISION_SAFE_FOR_CAUSAL_USE: NO
LOW_PAGE_MATCH_CAN_COLLIDE: YES
FALSE_POSITIVE_TARGET_ADMISSION_POSSIBLE: YES
APPEND_MATCH_IS_ONLY_CANDIDATE: YES
CANDIDATE_LATER_VALIDATED_WITH_MODEL_IDENTITY: NO
CANDIDATE_LATER_VALIDATED_WITH_FULL_DL_ADDRESS: YES (only record.currentDl == gameCall.sourceDl self-consistency; no independent target validation)
CANDIDATE_LATER_VALIDATED_WITH_GENERATION_AND_MODEL_CONTEXT: NO
FALSE_CANDIDATE_CAN_REACH_CAUSAL_CSV_AS_VALID_TARGET: YES
KNOWN_COLLIDING_NON_FEATHER_DLS_FOUND: UNKNOWN
COLLISION_COUNT: UNKNOWN
V7_CAN_REPORT_FALSE_POSITIVE_CANDIDATES: NO
CODE_CHANGE_REQUIRED: YES (future diagnostic-only validation design; not applied here)
USER_MANUAL_TEST_REQUIRED_NOW: NO (capture remains blocked)
NEXT_SINGLE_ACTION: add an independent model/DL validation state before authorizing capture
```

This is an audit correction only. No production behavior, game run, GPU
readback, synchronization, commit, or push was performed.

Evidence: [[plan/evidence/p09-kazooie-feather-gmtx-to-dl-trace]],
[[plan/evidence/p09-kazooie-feather-v4-ab-static-audit]],
[[plan/evidence/p09-kazooie-feather-v3-capture-analysis]],
[[plan/evidence/p09-kazooie-feather-v2-producer-witness]],
[[plan/evidence/p09-kazooie-feather-v1-capture]],
[[plan/evidence/p09-kazooie-carry-banjo-head-feathers]],
[[reverse-engineering/characters/kazooie]],
[[reverse-engineering/systems/geocmd]],
[[reverse-engineering/systems/transform-interpolation]].

## Prior (2026-09-03) — P09_IMPLEMENT_REAL_RT64_MATCH_WITNESS_V4

V3 token plumbing to RT64 G_MTX is `PROVEN` (2567/2568 in the preserved
capture). The V2 float buffer at `D_8007BCD0` is **REFUTED**. Generated code
uses **`D_8007BCB0`** as the mlMtx cursor; `guMtxF2L(float=*cursor,
Mtx=gfx_ctx+4)`; G_MTX operand is the Gfx Mtx ring, not GeoCmd `node+4`.
V4 now witnesses the real `GameFrame` call/candidate/one-to-one match path;
flush rows are secondary only. No production fix.
The first V4 capture is `CORRECTED`: its matrix events carry real workload
IDs, but vertex events carry workload `0`, overwriting token identity before
the match witness. Zero V4 CALL/candidate rows therefore do not prove an
RT64 rejection.

```text
RE_KNOWLEDGE_BASE_PATH: docs/reverse-engineering/
LIVE_BANJO_BODY_RESOURCE: 0x62B
X62B_SNOUT_NODE_OFFSET: 0x11CB8
LIVE_0X62B_TYPE2_HANDLER: func_800DCF48 (PROVEN_RUNTIME)
FUNC_80019AA0_CURSOR: D_8007BCB0
D_8007BCD0_EXACT_ROLE: REFUTED as 19AA0/mlMtxApply cursor
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V4
RT64_MATCH_WITNESS: REAL_MATCHTRANSFORM
RT64_CALL_MATCH_WITNESS: ENABLED
FLUSH_SENTINEL_ROWS: SECONDARY_ONLY
MISSING_GROUP_CAUSAL_STATUS: HYPOTHESIS
PRODUCTION_FIX: NO
ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
V4_CAPTURE_ROOT: DIAGNOSTIC_ARGUMENT_MISMATCH_MATRIX_VS_VERTEX_WORKLOAD
V4_CAPTURE_CONFIDENCE: HIGH (witness failure only)
NEXT_SINGLE_ACTION: preserve matrix workload identity across the vertex callback, rebuild, then capture once
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
USER_MANUAL_TEST_REQUIRED_NOW: NO
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[plan/evidence/p09-19aa0-gmtx-rt64-match-stage]],
[[plan/evidence/p09-v2-snout-runtime-correlation]],
[[plan/evidence/p09-v4-real-match-witness]],
[[reverse-engineering/systems/transform-interpolation]].

### Prior — P09_TRACE_SINGLE_G_MTX_IDENTITY

Live type-2 slot is `func_800DCF48` (`PROVEN_RUNTIME`). The registered
`0x62B` GeoCmd root is in-place `model_bin+0x119F0`. Absence of `+0x11CB8`
from the DE2A4 census is a **list-head wrap** artifact: the snout is a
sibling after type `0x0A` `@0x11CA0` in the neck child list; child
`+0x11CC8` and visible DL `+0x11CF8` were live. The fresh sibling-aware
capture proves snout→DCF48 and its nested ADE2C current-matrix selection.
The witness does not yet capture a previous matrix or RT64 identity. Static
trace now proves `ADE2C` returns a local bone slot, `func_80019AA0` composes it
into the float matrix arena, and `mlMtxApply`/`guMtxF2L` writes the fixed matrix
destination used by the plain single `G_MTX`. The 17 ADE2C addresses are
rotating animation-matrix source tables; rotating addresses alone are not an
RT64 identity bug. The first bounded runtime capture reached DCF48 (4,728
snout samples), but every token was rejected by a diagnostic pointer mistake:
the cursor is `D_8007BCD0`, not `0x8007BCB0`, and `mlMtxApply` loads its real
destination from that cursor rather than its incoming `ctx->r5`. The RT64 CSV
was therefore absent; runtime identity remains unproven. No production fix.

```text
RE_KNOWLEDGE_BASE_PATH: docs/reverse-engineering/
LIVE_BANJO_BODY_RESOURCE: 0x62B
X62B_SNOUT_NODE_OFFSET: 0x11CB8
TRUE_0X62B_ASSET_SIZE: 0x13178
LIVE_0X62B_TYPE2_HANDLER: func_800DCF48 (PROVEN_RUNTIME)
LIVE_SNOUT_HANDLER_REACHABILITY: PROVEN_RUNTIME (DCF48 accept=2243)
E2AA4_PRIMARY_SNOUT_WITNESS: DEMOTED
X62B_SNOUT_USES_E2AA4: NO
TOOIE_DIAG_SECONDARY_SWAY: default-off DE498 0x62B tree CSV + DCF48 + ADE2C
NEW_SNOUT_WITNESS_IMPLEMENTED: YES
SINGLE_G_MTX_IDENTITY_WITNESS_IMPLEMENTED: YES (STATIC; RUNTIME_BLOCKED_ON_CORRECTED_POINTER_CAPTURE)
CANDIDATE_IMPLEMENTED: NO
PRODUCTION_FIX: NO
ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CURRENT_MOST_LIKELY_LAYER: SINGLE_G_MTX_IDENTITY_UNRESOLVED (UNTESTED)
NEXT_SINGLE_ACTION: correct the bounded cursor/destination witness, then inspect one capture's guest/RT64 identity CSV
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
USER_MANUAL_TEST_REQUIRED_NOW: YES
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[plan/evidence/p09-single-gmtx-identity-static-trace]],
[[plan/evidence/p09-single-gmtx-identity-witness]],
[[reverse-engineering/systems/transform-interpolation]],
[[plan/evidence/p09-live-0x62b-tree-dcf48-capture]],
[[plan/evidence/p09-live-0x62b-tree-reachability]],
[[plan/evidence/p09-de2a4-dcf48-runtime-capture]],
[[plan/evidence/p09-live-geocmd-type2-dispatch]].

### Prior — P09_FIX_MODEL_BOUNDARIES_AND_MOVE_SNOUT_WITNESS_TO_GEOCMD02_E2AA4_SCOPE

The previous E2180 “inside span” counter used `rel < 0x20000`, which is
**not** decompressed `0x62B` (`0x13178`). Capture `a1=0x80241E50`
(`rel=0x1BD20`) is past `0x80226130+0x13178=0x802392A8`. Generic E2180
is a multi-caller population. Default-off witness now identifies the
snout at `func_800E2AA4` (`LOOKUP` bound) with
`a1 == model_bin + 0x11CB8` inside the true asset range. Nested E2180
is a tiny TLS token around that call only. No production fix.

```text
RE_KNOWLEDGE_BASE_PATH: docs/reverse-engineering/
LIVE_BANJO_BODY_RESOURCE: 0x62B
X62B_SNOUT_NODE_OFFSET: 0x11CB8
TRUE_0X62B_ASSET_SIZE: 0x13178
OLD_0X14960_ASSUMPTION: RETIRED
OLD_ALLOCATION_SPAN_0X20000: RETIRED
GENERIC_E2180_DISCOVERY_FILTER: DEMOTED
TOOIE_DIAG_SECONDARY_SWAY: default-off E2AA4 snout + true asset span
NEW_SNOUT_WITNESS_IMPLEMENTED: YES
CANDIDATE_IMPLEMENTED: NO
PRODUCTION_FIX: NO
ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CURRENT_MOST_LIKELY_LAYER: MATRIX_IDENTITY / INTERPOLATION_TAGGING
NEXT_SINGLE_ACTION: one TOOIE_DIAG_SECONDARY_SWAY=1 capture; read E2AA4 split rejects + e2aa4 tuples
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
USER_MANUAL_TEST_REQUIRED_NOW: YES
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[plan/evidence/p09-e2aa4-snout-witness]],
[[plan/evidence/p09-corrected-snout-e2180-capture]],
[[plan/evidence/p09-62b-snout-e2180-layout]].

### P09_PROVE_LIVE_0X62B_SNOUT_NODE_LAYOUT_AND_E2180_ARGUMENT_SEMANTICS

Independent parse of cart asset `0x62B` (skip-2 raw deflate, 78200 bytes,
66 bones). Live snout is GeoCmd `0x02` at **`0x11CB8`**, INDEX 11,
boneAnimID 42. `0x14960` is `0x62A` only and is past the end of `0x62B`.
E2180 `a1` is the GeoCmd node (`PROVEN` generated C). Default-off witness
now splits span/type/bone/pointer rejects and accepts `0x11CB8` or type2
plus bone 42 / INDEX 11 inside the durable `0x62B` span. No production fix.

```text
RE_KNOWLEDGE_BASE_PATH: docs/reverse-engineering/
LIVE_BANJO_BODY_RESOURCE: 0x62B
X62B_SNOUT_NODE_OFFSET: 0x11CB8
OLD_0X14960_ASSUMPTION: RETIRED
OLD_R31_FILTER: RETIRED
OLD_80092444_TLS_FOR_E2180: RETIRED
TOOIE_DIAG_SECONDARY_SWAY: default-off durable 0x62B + 0x11CB8/type2/bone42
NEW_SNOUT_WITNESS_IMPLEMENTED: YES
CANDIDATE_IMPLEMENTED: NO
PRODUCTION_FIX: NO
ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CURRENT_MOST_LIKELY_LAYER: MATRIX_IDENTITY / INTERPOLATION_TAGGING
NEXT_SINGLE_ACTION: static E2180/GeoCmd reverse engineering of the live a1 population and node-field semantics
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
USER_MANUAL_TEST_REQUIRED_NOW: YES
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[plan/evidence/p09-62b-snout-e2180-layout]],
[[plan/evidence/p09-persistent-snout-filter-zero]].

### P09 corrected 0x62B snout E2180 capture (2026-09-02)

The fresh capture postdates the current executable and is usable for the
corrected filter, but it accepts no snout node. Durable registration found one
unique `0x62B` model bin (`0x80226130`) with 2,693 model rows and 5,386
registration hits. E2180 entered 1,062 times; 216 entries were inside that
registered span. None had relative offset `0x11CB8`, type 2, or boneAnimID 42.
The one bounded in-span tuple was `a1=0x80241E50`, model-relative `0x1BD20`,
with raw `WORD0=0x000A0004` and `HWORD_A=0x0020`.

Evidence: [[plan/evidence/p09-corrected-snout-e2180-capture]].

```text
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES
CAPTURE_USABLE: YES (corrected filter; no accepted snout sample)
DE448_0X62B_REGISTRATION_COUNT: 5386 registration hits (2693 model rows)
REGISTERED_0X62B_BIN_COUNT: 1 (0x80226130)
E2180_RAW_ENTRY_COUNT: 1062
E2180_NODE_INSIDE_0X62B_COUNT: 216
E2180_REL_0X11CB8_COUNT: 0
E2180_TYPE2_COUNT: 0
E2180_BONE42_COUNT: 0
SNOUT_FILTER_ACCEPT_COUNT: 0
SNOUT_FILTER_REJECTION_BREAKDOWN: NO_DURABLE_MODEL=472, OUTSIDE_SPAN=846, WRONG_TYPE=1062, WRONG_BONE_ANIM_ID=1062
SNOUT_CSV_ROWS: 0 (missing)
FIRST_BAD_LAYER: E2180 live-node identity/field decoding
ROOT_CLASS: RUNTIME_E2180_POPULATION_OR_NODE_SEMANTICS_MISMATCH
ROOT_CONFIDENCE: LOW
NEXT_SINGLE_ACTION: static E2180/GeoCmd reverse engineering of the live a1 population and node-field semantics
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
```

This is not a matrix or interpolation witness. Do not request another
identical capture and do not implement a production fix. The generated path
still proves E2180 receives the saved E2AA4 `a1`; what remains unproven is
which live GeoCmd population and field encoding that pointer represents.

### P09_RECONCILE_DEFERRED_MODEL_RENDER_CONTEXT_AND_LIVE_SECONDARY_ANIMATION_PRODUCER

Canonical RE tree: `docs/reverse-engineering/`. Forensic layer:
`docs/plan/evidence/`. This round:
[[plan/evidence/p09-deferred-render-context-e0b8c]].

`func_800DE448` immediately calls `func_800DE498` then `func_800DE2A4` jalr
GeoCmd. Wrapped `800E2180` is **not** inside `func_80092444` TLS. Durable
identity is the live `0x62B` `model_bin`. `func_800E0B8C` is a record writer
and is **not** called every BK walk frame (`unkC==2` on overlay 162). No
production fix. Default-off snout witness no longer uses 92444 TLS.

```text
RE_KNOWLEDGE_BASE_PATH: docs/reverse-engineering/
LIVE_BANJO_BODY_RESOURCE: 0x62B
OLD_R31_FILTER: RETIRED
OLD_80092444_TLS_FOR_E2180: RETIRED
TOOIE_DIAG_SECONDARY_SWAY: default-off durable 0x62B model_bin + GeoCmd02/bone42
NEW_PERSISTENT_MODEL_WITNESS_IMPLEMENTED: YES
CANDIDATE_IMPLEMENTED: NO
PRODUCTION_FIX: NO
ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CURRENT_MOST_LIKELY_LAYER: MATRIX_IDENTITY / INTERPOLATION_TAGGING
NEXT_SINGLE_ACTION: targeted E2180 argument/node-layout reverse engineering
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
USER_MANUAL_TEST_REQUIRED_NOW: NO
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

### P09 corrected-filter capture attempt (2026-09-02) — SUPERSEDED

The only artifacts found (`2026-09-02T14:29:09.301Z`) predate the current
executable (`2026-09-02T15:16:20.887Z`) and current corrected source
(`tooie_diag.cpp` at `2026-09-02T14:57:54.597Z`). Their status still reports
the retired `0x14960` filter, so their zero-accept result is not evidence
against the corrected `0x11CB8` filter. No corrected tuple or matrix witness
is available. Evidence:
[[plan/evidence/p09-persistent-snout-filter-zero]].

```text
DE448_0X62B_REGISTRATION_COUNT: 6054
REGISTERED_0X62B_BIN_COUNT: 1 (0x8020E340)
E2180_RAW_ENTRY_COUNT: 1614
SNOUT_FILTER_ACCEPT_COUNT: 0
SNOUT_FILTER_REJECTION_BREAKDOWN: NO_DURABLE_MODEL=496, WRONG_NODE=1118
SNOUT_CSV_ROWS: 0 (missing)
E2180_IN_REGISTERED_0X62B_SPAN_COUNT: UNKNOWN
E2180_TYPE2_COUNT: UNKNOWN
E2180_BONE42_COUNT: UNKNOWN
E0B8C_RAW_ENTRY_COUNT: 0 (conditional writer; non-diagnostic)
GUEST_ANIMATION_STATE_VALID: UNKNOWN
SNOUT_CURRENT_GUEST_MATRIX_VALID: UNKNOWN
SNOUT_PREVIOUS_MATRIX_VALID: UNKNOWN
FIRST_BAD_LAYER: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CANDIDATE_IMPLEMENTED: NO
NEXT_SINGLE_ACTION: targeted E2180 argument/node-layout reverse engineering
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
```

`WRONG_NODE` currently merges node type, boneAnimID, and registered-span
predicates. Do not infer a guest-transform or interpolation root, and do not
request another identical run.

### P09 authoritative snout witness attempt (2026-09-02)

The fresh secondary-sway capture reconfirmed the player scope (1,934/1,934
DE448 samples) and the live resource sequence `0x62A` → `0x62B`, but emitted
no `bones.csv` or `snout.csv`. The old counters were post-filter counters, so
zero does not distinguish wrapper bypass from filter rejection. Core2
`LOOKUP_FUNC` address-taken dispatch was the missing observation boundary;
linker `--wrap` alone was not sufficient for an authoritative runtime-map
hook.

The diagnostic now explicitly binds E0B8C/E2180 through
`add_loaded_function` and records raw wrapper-entry and snout rejection
counters. This remains default-off and makes no production transform change.

```text
PLAYER_MODEL_ROWS: 1934
PLAYER_MODEL_0x62A: 135 (frames 503–637; bin 0x801D5020)
PLAYER_MODEL_0x62B: 1799 (frames 638–2436; bin 0x80224AA0)
BONES_CSV_ROWS: 0 (missing in submitted capture)
SNOUT_CSV_ROWS: 0 (missing in submitted capture)
HIP3_GUEST_STATE_VALID: UNKNOWN
SNOUT42_CURRENT_GUEST_TRANSFORM_VALID: UNKNOWN
PREVIOUS_CURRENT_LOGICAL_PAIRING: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CANDIDATE_IMPLEMENTED: NO
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES (new diagnostic binary)
NEXT_SINGLE_ACTION: one fresh secondary-sway capture with runtime-map hooks
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

### P09 repaired runtime-map witness (2026-09-02)

The fresh capture was inspected directly after binding both wrappers with
`add_loaded_function`. It is usable for the player identity, but not for a
guest bone or matrix witness. E2180 raw entry proves that its runtime-map
binding is active; E0B8C has no raw entry during this walk, so this is a hard
stop on further blind captures.

```text
PLAYER_MODEL_ROWS: 2126
PLAYER_MODEL_0x62A: 142 (frames 495–636; bin 0x801D5010)
PLAYER_MODEL_0x62B: 1984 (frames 637–2620; bin 0x80224B40)
OUTER_PLAYER_SCOPE_HITS: 2126
DE448_IN_PLAYER_SCOPE_HITS: 2126
E0B8C_RAW_ENTRY_COUNT: 0
E0B8C_FILTER_ACCEPT_COUNT: 0
E0B8C_REJECTION_BREAKDOWN: no entry; no post-filter rejection observed
E2180_RAW_ENTRY_COUNT: 3491
E2180_FILTER_ACCEPT_COUNT: 0
E2180_REJECTION_BREAKDOWN: NO_SCOPE=3491, NO_GETTER=0, WRONG_RESOURCE=0, WRONG_NODE=0
BONES_CSV_ROWS: 0 (missing)
SNOUT_CSV_ROWS: 0 (missing)
CAPTURE_USABLE: NO (transform witness; player identity usable)
E0B8C_RUNTIME_BINDING_WORKING: NO (not demonstrated; no live entry)
E2180_RUNTIME_BINDING_WORKING: YES
FILTERS_WORKING: PARTIAL
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CANDIDATE_IMPLEMENTED: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: reconcile the actual E0B8C runtime dispatch/call path
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

The generated player path reaches `GeoCmd02 → func_800E2AA4 →
func_800E2180`; E2180 is not evidence that E0B8C must also run. The zero
E0B8C entry therefore does not prove a wrapper-map failure by itself. It does
prove that this capture cannot classify bawobble state, current matrices, or
interpolation identity. Do not request another identical run.

Do not enable `TOOIE_DIAG_BACKPACK_FLAP` for the model-id capture.
Do not treat E2AA4 as the snout root until live id is `0x62A`/`0x62B`.
Do not follow SUPERSEDED `NEXT_SINGLE_ACTION` blocks below.

Canonical RE: [[reverse-engineering/README]]. Evidence:
[[plan/evidence/p09-session-handoff]],
[[plan/evidence/p09-player-model-witness-r31-repair]],
[[plan/evidence/p09-secondary-sway-static-root]].

### Historical failed capture (r31 filter, SUPERSEDED repair target)

`/tmp/tooie-secondary-sway-status.txt` reported `DE448_TOTAL_SEEN=31020`,
zero filtered hits. Repair is in CURRENT. Do not restore the r31 whitelist.

```text
CAPTURE_USABLE: NO
NEXT_SINGLE_ACTION: SUPERSEDED — see CURRENT
```

## SUPERSEDED (2026-09-02) — P09_MAP_BANJO_WOBBLE_PART3_TO_EXACT_GEOMETRY_AND_SNOUT

Static ROM/MIPS mapping. That round had **no** `TOOIE_DIAG_SECONDARY_SWAY`.
Uncommitted P09 backpack diagnostics were preserved. Findings below remain
valid; the live-model witness and RE tree supersede the "no new hook" /
`NEXT_SINGLE_ACTION` lines.

`D_80801448` / `badata_entrypoint_27` is an **animation-id** table. `0x56` / `0xEC` / `0x13A` decompress as animations (`000000c8` / `00000028` / `0000003f`), **not** `0000000B` models. That prior body-model claim is `CORRECTED/SUPERSEDED`.

Live BK walk geometry is overlay 118 packed list `+0xC160` = **`0x61F`** (`STRONG_INFERENCE` / later `MEDIUM` static, ExtraSegments, **zero GeoCmd 0x02**). High-detail Banjo `0x62A`/`0x62B` have GeoCmd 0x02 but are absent from overlay 118's first 256KB.

Wobble `func_800E0B8C(part=3)` writes type-3 Euler. Consumer is `func_800E0E94` from `func_800AF04C` using bone `+0xC` = **boneAnimID**. Part 3 is hip, **not** the snout. Snout is **boneAnimID 42**. `PART3_TAG5_RELATION` is `REFUTED` as identity (tag 5 on `0x62A` is the right hand).

```text
BANJO_WALK_ANIM_RESOURCE_ID: 0x56  (NOT a model)
BANJO_LIVE_WALK_MODEL: 0x61F STRONG_HYPOTHESIS; 0x62A WEAKENED
BANJO_SNOUT_NODE: boneAnimID 42 (0x61F idx 4; 0x62A idx 10 GeoCmd 0x02 + DL198)
BANJO_SNOUT_CONFIDENCE: MEDIUM
WOBBLE_PART3: boneAnimID 3 HIP
WOBBLE_PART3_DRIVES_SNOUT: INDIRECT (descendant inherit)
BANJO_SNOUT_USES_E2AA4: NO if 0x61F; YES if 0x62A
SNOUT_MATRIX_GROUP_TAG: ABSENT
PART3_TAG5_RELATION: REFUTED (identity)
ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CANDIDATE_CONFIDENCE: LOW
FILTER_CONFIDENCE: LOW (live model unresolved for state-vs-tagging seam)
NEW_SNOUT_WITNESS_IMPLEMENTED: NO
CANDIDATE_IMPLEMENTED: NO
NEXT_SINGLE_ACTION: SUPERSEDED — do not use; see CURRENT block
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p09-banjo-wobble-part3-20260902T060241Z/
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Canonical writeup of that round: [[plan/evidence/p09-secondary-sway-static-root]].

## SUPERSEDED (2026-09-01) — P09_FIND_REAL_BACKPACK_MODEL_PART_RENDER_PATH

The visible Banjo backpack is not drawn by `func_80092444` / `LOOKUP_FUNC(0x80084500)`.
That caller path is `REFUTED_FOR_THIS_RUNTIME_PATH` (1503/1503 `PATH_A2_ZERO_RETURN_18`,
lookup executed 0). Overlay 113 (`babackpack`) owns a separate attached model.

MATCHED overlay C (`ckpack.c`) shows:

- `babackpack_entrypoint_3` is the draw entry
- `babackpack_entrypoint_1` → `func_800D674C(unk0)` is the model pointer getter
- `babackpack_set_state(2)` loads asset `0x621` at scale 1.0 (primary gameplay backpack)
- draw submits `func_800DE448(position, arg2, scale, arg4, model_bin)` then common `func_800DE498`

Child dual-matrix GeoCmd analog: `func_800E2AA4` → `func_800E2180` emits
`G_MTX 0xDB060008` (prev via `func_800AF5D8`) then `G_MTX 0xDB060004` (curr via
`func_800AF5E4`). BanjoRecomp tags the homologous GeoCmd2/bone path in
`patches/transform_tagging.c`. Tooie has no equivalent tagging. Flap part ID is
`UNKNOWN` until a filtered capture lists the dual-mtx nodes under model `0x621`.

`TOOIE_DIAG_BACKPACK_FLAP=1` now wraps `func_800D674C` / `func_800DE448` /
`func_800E2180` only. Obsolete `func_80092444` / `get_function` / syscall 113/3
probes are retired. No production interpolation patch.

```text
OLD_BABACKPACK_LOOKUP_HYPOTHESIS: REFUTED_FOR_THIS_RUNTIME_PATH
BABACKPACK_ACTUAL_DRAW_ENTRYPOINT: babackpack_entrypoint_3
BACKPACK_MODEL_RESOURCE_ID: 0x621 (primary); 0x625/0x60A/0x61B/0x831 (tiny states); state-7 preload
MODEL_RENDER_ENTRY: func_800DE448 → func_800DE498
GEOCMD_CHILD_DUAL_MTX: func_800E2180
TOOIE_MISSING_TAGGING_EQUIVALENT: YES
CANDIDATE_IMPLEMENTED: NO
DIAGNOSTIC_DEFAULT_OFF: YES
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_SINGLE_ACTION: one short TOOIE_DIAG_BACKPACK_FLAP=1 capture while Banjo walks
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
ARTIFACT_DIRECTORY: /tmp/p09-backpack-real-render-path-20260901T231354Z/
GPU_READBACK_USED: NO
```

```sh
cd /home/wesley/projetos/N64-Recomp/TooieRecomp && \
rm -f /tmp/tooie-backpack-flap-* && \
TOOIE_DIAG_BACKPACK_FLAP=1 ./build/TooieRecompiled
```

Walk Banjo so the backpack flap moves. Stop. Collect
`/tmp/tooie-backpack-flap-status.txt` and
`/tmp/tooie-backpack-flap-witness.csv`.

## CURRENT (2026-08-31) — T_INTRO_TRUE_SOURCE_ASPECT_PRODUCTION

T_INTRO projection handling is now production-default in Tooie at the native
`func_800E44FC` projection owner. The policy is T_INTRO-only, render-only,
dynamic, and applies to both Scene A and Scene B; gameplay and RT64 global
code are unchanged. It derives the cinematic source aspect from the actual
perspective matrix as `abs(P[1][1]/P[0][0])`, so RT64's false VI `304/228`
source does not impose `K=4/3` at 16:9.

```text
PRODUCTION_POLICY: true cinematic source-aspect compensation
SOURCE_ASPECT_DERIVATION: abs(P[1][1] / P[0][0]) at func_800E44FC
SOURCE_ASPECT_HARDCODED: NO
CANDIDATE_A_FINAL_STATUS: REMOVED
CANDIDATE_B_ENV_FLAG_FINAL_STATUS: REMOVED; production default
TINTRO_TRUE_SOURCE_ASPECT_VISUAL_RESULT: VISUALLY_CONFIRMED_FOR_TESTED_16X9_SCENES
TINTRO_21X9_PLUS_FINAL_STATUS: OPEN_REVALIDATION_REQUIRED
PROJECTION_POLICY_REGRESSION_TEST: ADDED
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST_RESULT: PASS
COMMIT_SCOPE: production projection policy, P08 cleanup, regression test, canonical docs
NO_PUSH: YES
```

### P09 D674C nonmatching-resource capture — no owner witness (2026-09-02)

The follow-up capture populated the bounded D674C observer: 44,620 resource
lookups, 32 retained nonmatching resource/pointer pairs, and 18,095 evictions.
The fixed IDs still had zero hits; DE448/DE498 had zero filtered hits; E2AA4
had zero total hits; and all 1,167 E2180 calls were rejected for no active
model context. Draw scope depth was 1 with no overflow. The witness and
anomaly files remained empty.

Static inspection resolves D674C's argument as a resource-table key and its
return as the loaded resource/model pointer passed to DE448. The `0x621` and
`0x625` values are static `babackpack` setup keys, not a universal runtime
filter identity. The capture has no callsite/owner or frame association, so
the affected four parts and any interpolation/tagging root remain unknown.

```text
RUNTIME_WITNESS_CAPTURED: NO
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
NEXT_SINGLE_ACTION: static MIPS/GeoCmd reverse-engineer Banjo body/snout and attached-model ownership
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

The prior candidate section below is retained as historical evidence and is
superseded by this production promotion. The removed P08 probes and temporary
tools are catalogued in the promotion artifact.

## SUPERSEDED (2026-08-31) — T_INTRO_PROJECTION_SOURCE_ASPECT_CANDIDATES

The previous high-confidence root is retained: RT64 AUTO scales the T_INTRO
projection from VI `304/228`, producing `K=4/3` at 16:9 and changing CLIP_X.
This round added two Tooie-side, default-off diagnostic candidates at the
native `func_800E44FC` projection seam. Candidate A restores the guest
projection after RT64's later AUTO adjustment; Candidate B compensates using
the cinematic source aspect derived as `abs(P[1][1]/P[0][0]) = 1.767441860`.
Neither changes RT64 globally, geometry, culling, `800CAA24`, `800CACEC`, or
`CAB70`; both require the T_INTRO mode/map predicate and gameplay is excluded.

```text
EXISTING_ASPECT_EXTENSION_API: PARTIAL (aspect getters/calculators; no scene-local RT64 source setter)
TOOIE_CAN_OVERRIDE_SOURCE_ASPECT_WITHOUT_RT64_CORE_CHANGE: YES (Tooie pre-compensation)
CANDIDATE_A: TOOIE_TEST_TINTRO_PROJECTION_ORIGINAL=1 (default OFF)
CANDIDATE_B: TOOIE_TEST_TINTRO_TRUE_SOURCE_ASPECT=1 (default OFF)
CANDIDATE_A_EFFECTIVE_K_16_9: 1.000000
CANDIDATE_A_VIEWPROJ00_16_9: ~0.905148
CANDIDATE_B_EFFECTIVE_K_16_9: ~1.005850
CANDIDATE_B_VIEWPROJ00_16_9: ~0.8999 (true-source expectation)
SCENE_A_CANDIDATE_ACTIVE: YES
SCENE_B_CANDIDATE_ACTIVE: YES
GAMEPLAY_CANDIDATE_ACTIVE: NO
RENDER_ONLY_CHANGE: YES
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
USER_MANUAL_TEST_REQUIRED_NOW: YES
PREFERRED_USER_TEST_CANDIDATE: B
NEXT_SINGLE_ACTION: TOOIE_TEST_TINTRO_TRUE_SOURCE_ASPECT=1 ./build/TooieRecompiled
ARTIFACT_DIRECTORY: /tmp/p08-tintro-projection-candidate-20260831T034438Z/
ARTIFACT_CHECKSUM_MANIFEST: 665ae932cfe019b1cebd04fa6dc1f78c6b70f8d3388faee6c5522b1e0f7474ed
GPU_READBACK_USED: NO
```

## CURRENT (2026-08-31) — T_INTRO_RT64_TRIANGLE_TRANSLATION_DIVERGENCE

Same Scene-A frame-5 guest triangles followed through RT64 CPU translation.
Guest RSP clip matches captured golden (`0017aff0` m00=`0.905148`). Expand
then scales the cinematic viewProj first column by `1/K` with `K=4/3`
(VI `304/228` vs window `16/9`), **not** the 0.584795% guest-projection
aspect delta. First divergence is CLIP_X. Presentation-envelope Original vs
Expand remains `REFUTED` (guest NDC X still `[-1,+1]`).

```text
TINTRO_16X9_RT64_ROOT_CLASS: RT64_PROJECTION_TRANSLATION
ROOT_CONFIDENCE: HIGH
FIRST_RT64_DIVERGENCE_STAGE: CLIP_X
FIRST_RT64_DIVERGENCE_FUNCTION: RT64::adjustProjectionMatrix / ProjectionProcessor::processScene
FIRST_RT64_DIVERGENCE_FILE: BanjoRecomp/lib/rt64/src/render/rt64_projection_processor.cpp
FIRST_RT64_DIVERGENCE_VALUE: cinematic viewProj[0][0] 0.905148 -> 0.678861 (x0.75)
RT64_EXPAND_STAGE: ProjectionProcessor::processScene (after HLE, before GPU)
RT64_SOURCE_ASPECT: 304/228 = 1.333333333
RT64_TARGET_ASPECT: 16/9 = 1.777777778
RT64_EFFECTIVE_K: 1.333333333
RT64_ORIGINAL_EXPAND_TRIANGLE_DIFFERENCE_COUNT: 5
MAX_RT64_EDGE_VERTEX_NDC_DELTA: 2.423362 (all verts); in-|x|<=1.05: 0.058037; unit-clip: 0.250000
RT64_TRANSFORM_CAN_EXPLAIN_LARGE_VISUAL_FAILURE: YES
CLIP_CLASSIFICATION_MATCH: NO (2/5 Expand mismatches)
WITNESS_WINDING_MATCH: YES
EXPAND_FLIPS_ANY_WITNESS_WINDING: NO
GUEST_RT64_RASTER_STATE_MATCH: YES
SCENE_B_SHOWS_SAME_RT64_DIVERGENCE: YES
TINTRO_16X9_AUTHORED_EXTENT: STRONG_HYPOTHESIS (extra FOV contents; not independent of 4/3 scale)
TINTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN
RT64_PRESENTATION_TRANSFORM_AS_ROOT: REFUTED
CORRECTED_ORIGINAL_EXPAND_OWNER_DIFFERENCE_COUNT: 0
OLD_16872_USES_VIS_HALF_0754: YES
OLD_16872_RESULT_VALID: NO
FIX_LAYER: TOOIE
FIX_SCOPE: TINTRO
CANDIDATE_IMPLEMENTED: diagnostic-only (TOOIE_DIAG_RT64_TINTRO_TRI=1 default OFF)
GPU_READBACK_USED: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: implement a Tooie-side default-off T_INTRO/cutscene candidate that prevents RT64 AUTO from applying K=4/3 to the cinematic viewProj (prefer cutscene Original or scene-local skip of G_EX_ASPECT_AUTO), then re-check Scene-A frame 5 walls vs this Expand scale; do not change RT64 globally
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
ARTIFACT_DIRECTORY: /tmp/p08-tintro-rt64-triangle-translation-20260831T032400Z/
ARTIFACT_CHECKSUM_MANIFEST: 4971dfafeea24b2c44d365c9df11350dbbcc4bb7f172991550de54731ff3f07a
```

Five covering WALL witnesses on leaf `0x0031f6d8` (idx 77/81/47/78/67).
Do not widen CAB70. Do not force WALL7799. Do not reintroduce ±0.754 as
horizontal crop. Isolation:
`APP_FOLDER_PATH=/tmp/tooie-intro-16x9-xdg/TooieRecompiled`.

## CURRENT (2026-08-31) — T_INTRO_PRESENTATION_ENVELOPE_RECONCILIATION

The previous `±0.754` Original-vs-Expand owner comparison is
`REFUTED/SUPERSEDED`: its analyzer applied a host/presentation scalar to guest
X. The actual T_INTRO cinematic scissor is `(0,28)-(304,200)`, giving full
guest X NDC `[-1,+1]` and vertical/Y letterbox NDC
`[-0.754385965,+0.754385965]`. RT64 Original on a 16:9 host is contain-fit
pillarbox; Expand is full-width. Both present the same guest X range.

```text
RT64_ORIGINAL_CHANGES_GUEST_NDC_ENVELOPE: NO
RT64_EXPAND_CHANGES_GUEST_NDC_ENVELOPE: NO
CORRECTED_ORIGINAL_EXPAND_OWNER_DIFFERENCE_COUNT: 0
OLD_16872_USES_VIS_HALF_0754: YES
OLD_16872_RESULT_VALID: NO
OWNER_78_SURFACE_CLASS: UNKNOWN
B1_TRUE_NONWALL_MISMATCH_PIXEL_COUNT: 1764
B1_WALL_TO_WALL_REPLACEMENT_PIXEL_COUNT: 4224
SOURCE_TARGET_ASPECT_DELTA_PERCENT: 0.584795%
CAMERA_FRAMING_EXPOSES_AUTHORED_EXTENT: not promoted for 16:9 from invalid ±0.754
TINTRO_16X9_AUTHORED_EXTENT: STRONG_HYPOTHESIS
GUEST_CPU_RENDER_PATH_AS_ROOT: STRONGLY_WEAKENED
RT64_PRESENTATION_TRANSFORM_AS_ROOT: REFUTED
GPU_READBACK_USED: NO
```

The trusted Angrylion owner replay remains valid; only its old presentation
remap was corrected. New artifact: `/tmp/p08-tintro-presentation-reconciliation-20260831T025942Z/`
(manifest SHA `c08eeab9d38cf89be1c79b8184bb95e67b53ab3e703ad118901d856cb4dd6dc3`).
No production candidate, commit, or push.

## CURRENT (2026-08-31) — TRUSTED_CPU_RDP_PIXEL_OWNER_REFERENCE

The pinned local Angrylion RDP Plus CPU core (`9c8b9ed3e7d7f00dff8bc872ccdd3fba1a3673fc`)
was used directly for an offline fixed-point owner replay. Scene A frame 5
replayed 156 environment triangles and produced 0 unresolved owner pixels;
wall-class ownership was 60.82% in the left 20% and 19.31% in the right 20%.
The replay preserved captured leaf draw order and nested-G_DL flattened
execution order, guest 304x228 scissor/viewport, RDP edge setup, coverage,
and framebuffer write eligibility. No GPU readback was used.

```text
OWNER_REPLAY_EXECUTION_ORDER_PROVEN: YES
RDP_SUBPIXEL_RULES_FROM_REFERENCE: YES
RDP_COVERAGE_RULES_FROM_REFERENCE: YES
B1_ACTUAL_OVERLAP_PIXEL_COUNT: 5988
B1_EXPECTED_WALL_ID_FINAL_OWNER: NO (exact ID 7c8e8bba7a3ac2a9)
B1_WALL_WRITE_ELIGIBILITY: YES; coverage 8/8 on all 5988 samples
B1_ALT_WRITE_ELIGIBILITY: YES; coverage 8/8 on 5987, 7/8 on 1 sample
B1_FINAL_OWNER_CLASS: WALL 4224 / UNKNOWN 1764
PRIMARY_PIXEL_OWNER_CAUSE: LATER_DRAW_WINS
ORIGINAL_EXPAND_OWNER_DIFFERENCE_COUNT: 16872 host-envelope pixels
GUEST_CPU_RENDER_PATH_AS_ROOT: STRONGLY_WEAKENED
```

The exact expected B-1 wall triangle (draw index 47, captured draw order
48) and prior alternate (index 12, draw order 13) both pass the reference
write path. Later index 67 (WALL) and index 78 (UNKNOWN) own the final B-1
pixels. This is not a proven alpha/Z/state defect. Original and Expand use
identical guest clip input here; the difference is the already documented
presented envelope (Original ±0.754 versus Expand ±1.000), so guest-side RDP
ownership is refuted as the widescreen root.

Artifact: `/tmp/p08-tintro-rdp-owner-reference-20260831T011600Z/` (manifest
`dbf77934a31b2244a5b5af1568413a03c88a0ce823f3886f4b102a4fe9cbf83e`). No
production candidate, commit, or push.

## CURRENT (2026-08-30) — T_INTRO_VISIBLE_SURFACE_OWNERSHIP_SCENE_A_VS_B

Scene-A frame 5 has 156 submitted/render-eligible environment triangles;
Scene-B equivalent capture frame 2409 has 80. The complete task-root walk
observed roots `0x80160cd0` and `0x80159600`; first-env roots were A=`0x80160cd0`
and B=`0x80159600`. E2/E3 RDP SETOTHERMODE state is now decoded through nested
G_DL calls. B-1's wall and nearer alternate both have Z compare/update OFF and
the same 2-cycle/bilinear/force-blend/alpha-none class; wall order is 48 versus
alternate order 13. Thus no state-caused mismatch or exact pixel owner is
proven; no fix or geometry. Artifact:
`/tmp/p08-tintro-render-state-owner-20260830T180800Z/` (SHA
`3d342e999430f817d0333a539968f87935c5b66a848e29864e857edd6a1c3b14`).

## Prior (2026-08-30) — T_INTRO_PER_DRAW_TRANSFORM_OWNERSHIP

```text
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
SCENE_A_FIRST_ENV_FRAME:      5
SCENE_A_FIRST_HOLE_WITNESS_FRAME: 5
OPA7417_ZERO_TRIANGLE_ROOT:   WRONG_RESOURCE + WRONG_MODEL_MATRIX
SCENE_A_LEFT_EDGE_GDL:        0x0031f6d8 / 0x0031f8f8
SCENE_A_LEFT_EDGE_MODEL_MATRIX: 0x8017b0b0 T(camera)
SCENE_A_RIGHT_EDGE_GDL:       0x0031f6d8 / 0x0031f8f8
PRIMARY_HOLE_FRAME:           5
PRIMARY_HOLE_SIDE:            LEFT
REAL_HOLE_FIRST_DROP_STAGE:   NO_CPU_DROP
A_B_EQUIVALENT_DRAW_OWNER:    NO
A_B_EQUIVALENT_FIRST_DROP:    YES
SCENE_A_VISUAL_ROOT_CLASS:    MORE_EVIDENCE_REQUIRED
SCENE_B_VISUAL_ROOT_CLASS:    MORE_EVIDENCE_REQUIRED
SHARED_VISUAL_ROOT:           STRONG_HYPOTHESIS
IMPLEMENTED_FINAL_FIX:        NO
IMPLEMENTATION_COMPLEXITY:    LOW_MEDIUM
NEXT_SINGLE_ACTION:           Dense CPU NDC occupancy grid of 0x0031f6d8/0x0031f8f8 at Scene-A frame 5 (exclude 0x0027f100); content-stable ID for Scene B
NEXT_RECOMMENDED_MODEL:       GPT-5.6 Luna
ARTIFACT_DIRECTORY:           /tmp/p08-tintro-per-draw-owner-20260830T193644Z/
ARTIFACT_CHECKSUM_MANIFEST:   88a9885d0abc13fb0e0bf3d12008058ee8c89e00404598876148644bb5a66fa9
GPU_READBACK_USED:            NO
BUILD_RESULT:                 PASS
WINDOW_DISPLAY_TEST:          PASS 38/38
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

G_DL `0x00123340` is a setup-only list (ENDDL, no triangles). First
environment triangles are at frame 5 under `0x0031f6d8`/`0x0031f8f8` with
modelview translation equal to the opening camera. Type-13 rejected leaves
are still not the hole owner. Do not widen CAB70. Do not force WALL7799.

## Prior (2026-08-30) — T_INTRO_SCREEN_EDGE_OWNER_AUDIT

```text
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
SCENE_A_OPENING_FRAME:        3 (stable VP sample frame 315)
SCENE_A_OPENING_CAMERA:       (529.3794, 6943.8896, -1904.1279)
OPA_7417_TRIANGLES_IN_CLIP:   0 at the exact opening pose
SCREEN_EDGE_TRIANGLE_OWNER:   NONE_PROVEN
TYPE13_REJECTED_LEAF_OWNER:   NO
SCENE_A_VISUAL_ROOT_CLASS:    MORE_EVIDENCE_REQUIRED
SHARED_VISUAL_ROOT:           STRONG_HYPOTHESIS
ARTIFACT_DIRECTORY:           /tmp/p08-tintro-screen-edge-owner-20260830T190608Z/
ARTIFACT_CHECKSUM_MANIFEST:   1d9fad539b87a0c7ffd4fdeffa7840c1a97bf8f8ab148596d0a1660ce7f1f21b
```

Static OPA-7417 + assumed opening transform produced zero in-frustum
triangles. That result is now explained as WRONG_RESOURCE/WRONG_MODEL_MATRIX.

## Prior (2026-08-30) — T_INTRO_TYPE13_OPENING_LEFT_REJECT

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
SCENE_A:                      SCENE_A_FIRST_VISIBLE_AFTER_LAUNCH
SCENE_B:                      SCENE_B_NEW_GAME_INTRO
FUNC_800E3DC0_ROLE:           type-13 handler; jal CAB70 via *0x8012D500
FUNC_800CAB70_ROLE:           pure 4-plane AABB p-vertex predicate
TYPE13_REJECT_OWNER:          800CAB70
LIVE_PLANE_MAPPING_PROVEN:    YES (0=LEFT, 2=TOP, 3=BOTTOM; 1=RIGHT/FAR mixed)
SCENE_A_OPENING_LEFT_WALL_LEAVES: 6 AABBs; closest 0x8037beb8
SCENE_A_OPENING_RIGHT_WALL_LEAVES: none at this camera
SCENE_A_FIRST_REJECT_PLANE:   LEFT
SCENE_A_FIRST_REJECT_VALUE:   +34.2265015 (0x8037beb8 p-vertex 520,7020,-2980)
SCENE_A_FIRST_REJECT_THRESHOLD: 0 (n·p+d >= 0 culls)
SOURCE_ASPECT:                1.767442
TARGET_ASPECT:                1.777778
SOURCE_REJECT_TARGET_ACCEPT:  NO
FOUR_THREE_IDENTITY:          PROVEN (k==1 no-op)
A_B_EQUIVALENT_CAMERA_FOUND:  YES (exact)
A_B_SAME_WALL_LEAVES:         YES (same AABBs)
A_B_SAME_REJECT_PLANE:        YES
A_B_SAME_REJECT_RESULT:       YES (identical signed distances)
TOOIE_TYPE13_MATCHES_BANJO_GEOCMDD_CLASS: YES
TRAVERSAL_SEMANTICS_MATCH:    HIGH
DEMO_OR_GAMEPLAY_SIDE_EFFECT_RISK: MEDIUM
CANDIDATE_IMPLEMENTED:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
SCENE_A_ROOT:                 PROVEN (LEFT CAB70 at opening)
SCENE_B_ROOT:                 PROVEN (same)
SHARED_ROOT:                  PROVEN
INTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN (separate)
IMPLEMENTED_FINAL_FIX:        NO
NEXT_SINGLE_ACTION:           Identify SUBMITTED on-screen L/R 20% geometry at opening; do not add TOOIE_TEST_INTRO_TYPE13_ASPECT
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-tintro-type13-reject-plane-20260830T184303Z/
ARTIFACT_CHECKSUM_MANIFEST:   93b735b8ad433c291ea32340fae6627b268047636987a553f504f3909cfe967a
GPU_READBACK_USED:            NO
BUILD_RESULT:                 PASS
WINDOW_DISPLAY_TEST:          PASS 38/38
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

0x8012D500 is a viewport **pointer**, not the plane block. Live CAB70 vp
at opening is `0x801e0c60`. Guest frustum is already 1.767:1. The 6
left-edge type-13 leaves are original LEFT rejects (dist +34.23…+576.19);
aspect-widen does not accept them. NDC xmax ≤ -1.13. Do not copy Banjo
GeoCmdD `TRUE`. Prior WALL7759/7799 remains late SCENE_A (TOP at kOpenCam),
kept, not discarded.

## Prior (2026-08-30) — TWO_INTRO_SCENES_SEPARATED

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
PRODUCTION_MILESTONE_HEAD:    23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve; diags default OFF)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
SCENE_A:                      SCENE_A_FIRST_VISIBLE_AFTER_LAUNCH
SCENE_A_EPOCH:                1
SCENE_A_MAP:                  0 (mode-struct; NOT assigned MAP_18B)
SCENE_A_OVERLAY:              674 first 3D
SCENE_A_OPA_MODEL:            G_DL 0x00123340
SCENE_A_CAMERA_OWNER:         T_INTRO; cam (529.38, 6943.89, -1904.13)
SCENE_A_ROOT_DRAW:            func_80013EF0 + func_800E3DC0/800CAB70
SCENE_B:                      SCENE_B_NEW_GAME_INTRO
SCENE_B_EPOCH:                5 (frontend run)
SCENE_B_MAP:                  0 (mode-struct; NOT assigned MAP_18B)
SCENE_B_OVERLAY:              post-gameselect 440/638
SCENE_B_OPA_MODEL:            G_DL 0x00123340 (same)
SCENE_B_CAMERA_OWNER:         T_INTRO replay; opens at SCENE_A camera
SCENE_B_ROOT_DRAW:            func_80013EF0 + func_800E3DC0/800CAB70
PRIOR_WALL7759_INVESTIGATION_BELONGS_TO: SCENE_A
  (late first-play T_INTRO pose kOpenCam ~6946; mode=3 map=0)
SCENE_A_B_SHARED_MAP_RENDERER: YES
SCENE_A_B_SHARED_VISIBILITY_STACK: YES
SCENE_A_FAILURE_CLASS:        VISIBILITY_REJECT
SCENE_B_FAILURE_CLASS:        VISIBILITY_REJECT
SCENE_A_WALL_OWNER:           G_DL 0x00123340
SCENE_A_FIRST_DROP_STAGE:     VISIBILITY_REJECT (type-13 AABB)
SCENE_A_FIRST_DROP_FUNCTION:  func_800E3DC0 / func_800CAB70
SCENE_A_WALL_DL_SUBMISSION:   PROVEN_PRESENT
SCENE_A_MATCHES_DK64_INTERNAL_BOUND_PATTERN: NO
SCENE_B_MATCHES_DK64_INTERNAL_BOUND_PATTERN: NO
SHARED_ROOT_PLAUSIBLE:        HIGH
SHARED_ROOT_CANDIDATE:        same T_INTRO model + type-13 AABB; two plays
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           At SCENE_A opening cam (529.38, 6943.89, -1904.13) prove type-13 first reject plane (X vs TOP) for lateral-wall leaves of G_DL 0x00123340; CPU-only; do not force WALL7799
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-two-intro-scenes-20260830T181514Z/
ARTIFACT_CHECKSUM_MANIFEST:   4aae5e68e62eb61f5fc7dba8e6b9007d06ba70f61e5da1e762f680bde56b048a
GPU_READBACK_USED:            NO
BUILD_RESULT:                 PASS
WINDOW_DISPLAY_TEST:          PASS 38/38
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

Do **not** call every cinematic “the intro”. Stable names:

- `SCENE_A_FIRST_VISIBLE_AFTER_LAUNCH` — first 3D after launch, T_INTRO
  mode=3 map=0, G_DL `0x00123340`, opening camera y≈6943. HIGH severity.
- `SCENE_B_NEW_GAME_INTRO` — post-file-select replay of the same T_INTRO
  (frontend epoch 5). Same G_DL and opening camera. LOWER user severity.

Prior WALL7799/7759 artifacts remain valid for a **late pose** of SCENE_A
(kOpenCam ≈ frame 6946 of an unskipped first play). They are **not**
SCENE_B. Map-struct is 0; overlay 676 `a0=0x18B` is only a resource hint.

Gameplay `800CAA24` / `800CACEC` stay production. No intro production fix.

## Prior (2026-08-30) — WALL7799_TYPE13_AABB_TOP_CULL (late SCENE_A pose)

Scoped: kOpenCam of an **unskipped first T_INTRO play** (`mode=3 map=0`,
open_frame≈6946). That is late `SCENE_A`, not `SCENE_B`. Type-13 is a
24-byte AABB frustum gate (`func_800E3DC0`/`func_800CAB70`). WALL7799
(`0x819A8`) is original-rejected there by TOP, not X.
`TOOIE_TEST_INTRO_7799_VIS` not added. Artifact
`/tmp/p08-intro-wall7799-gate-20260830T173445Z/`
(manifest `30ad57994bc6635bdf97d1f44c832f80f740811813178084d0c295b2c41f41f4`).

## Prior (2026-08-30) — MAP18B_ORIGINAL_PRESENTATION_ENVELOPE

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
PRODUCTION_MILESTONE_HEAD:    23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve; historical intro-wall probe removed)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
INTRO_16X9_AUTHORED_EXTENT_AS_ROOT: REFUTED/SUPERSEDED
INTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN
INTRO_16X9_CPU_CULL_ROOT:     REFUTED
INTRO_16X9_MISSING_DL_ROOT:   REFUTED (7759/7779/7644/SKY18); OPEN (WALL7799)
INTRO_16X9_PRESENTATION_REVEALS_ORIGINALLY_CROPPED_NDC: REFUTED (X-crop)
INTRO_GUEST_PROJECTION_ALREADY_WIDE: PROVEN (1.767442)
INTRO_16X9_NEEDS_NEW_GEOMETRY: NO
INTRO_16X9_ROOT:              STRONG_HYPOTHESIS (upper-left empty w/o 7799)
GEOMETRY_EXTENSION_16X9_NECESSARY: NO
INTRO_21X9_PLUS_GEOMETRY_EXTENSION_NEED: OPEN / LIKELY
Clamp16x9:                    REFUTED
GROUP81_AS_OPENING_HOLE_ROOT: REFUTED
GROUP84_AS_OPENING_HOLE_ROOT: REFUTED
IMPLEMENTED_CANDIDATE:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           Prove why nested leaf 0x819A8 (WALL7799 gfx 7783-7807) is never G_DL-appended at the opening pose; CPU-only; do not extend geometry; do not pillarbox ±0.754
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-original-presentation-envelope-20260830T163614Z/
ARTIFACT_CHECKSUM_MANIFEST:   2c2ffed179c34aafc57a3040f83ffc175b4c24257abc2761be7ef895884a8fea
GPU_READBACK_USED:            NO
BUILD_RESULT:                 PASS
WINDOW_DISPLAY_TEST:          PASS 38/38
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

CPU-clip of **submitted** wall/sky triangles through live guest VP at
frame 6954. Lower-left WALL7759 continues to ndc x=-1. Upper-left
(y≥+0.15) is EMPTY at both x=-0.754 and x=-1 because WALL7799 is never
submitted; 7644 only reaches xmin=-0.963. Right is sky/empty. Guest
scissor is full-width 304/304, so the N64 cinematic already uses NDC
±1 horizontally. Do not pillarbox to ±0.754. Do not extend geometry.
Gameplay `800CAA24`/`800CACEC` stay production.
`INTRO_16X9_MISSING_DL_ROOT OPEN (WALL7799)` is `CORRECTED/SUPERSEDED`
by the type-13 AABB TOP-cull proof (current).

## Prior (2026-08-30) — MAP18B_16X9_EDGE_DRAW_SUBMISSION_AND_FIRST_DROP_GATE

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
PRODUCTION_MILESTONE_HEAD:    23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve; historical intro-wall probe removed)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
INTRO_16X9_AUTHORED_EXTENT_AS_ROOT: REFUTED/SUPERSEDED
INTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN
INTRO_16X9_ROOT:              STRONG_HYPOTHESIS
INTRO_WALL_DL_SUBMISSION:     PROVEN_PRESENT
DL_PRESENT_4_3_MISSING_16_9:  NO
INTRO_16X9_FIRST_DROP_STAGE:  NO_CPU_DROP
CPU_CLIP_SPACE_DIFFERENCE:    NONE (opening pose; cinematic ~1.767:1 both aspects)
HOST_VISIBLE_NDC_DIFFERENCE:  YES (Expand vis_ndc=[-1,1]; Original vis_ndc=[-0.754,0.754])
DROP_ASPECT_DEPENDENT:        NO
GROUP81_AS_OPENING_HOLE_ROOT: REFUTED
GROUP84_AS_OPENING_HOLE_ROOT: REFUTED
Clamp16x9:                    REFUTED
INTRO_21X9_PLUS_GEOMETRY_EXTENSION_NEED: OPEN / LIKELY
GEOMETRY_EXTENSION_NOW_PROVEN: NO
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           CPU-clip submitted WALL7759 against ndc x=-1 (Expand edge) vs ndc x=-0.754 (Original presented edge); no geometry extension
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-16x9-dl-drop-20260830T160704Z/
ARTIFACT_CHECKSUM_MANIFEST:   9ca8fa3c374c75b81594fa5f535633ff977f2a542bbb11389646d6ccbdca553a
GPU_READBACK_USED:            NO
BUILD_RESULT:                 PASS
WINDOW_DISPLAY_TEST:          PASS 38/38
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

Isolated `APP_FOLDER_PATH` 1600x900 runs: 16:9 Expand/Full vs 4:3 Original.
Unique G_DL matching: 7759/7779/7644/SKY18 submitted at both opening poses.
WALL7799 G_DL never appears at either aspect. Guest clip identical.
Host presented vis NDC is ±1.000 (Expand) vs ±0.754 (Original). Two 7759
verts sit at ndc_x < -1. Current 16:9 hole is not a missing display list.
Do not extend geometry. Do not reintroduce Clamp16x9.
Gameplay `800CAA24`/`800CACEC` stay production.

## Prior (2026-08-30) — MAP18B_OPENING_WALL_REAL_TRIANGLE_BINDING

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
PRODUCTION_MILESTONE_HEAD:    23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve; no geometry candidate)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
GROUP81_AS_OPENING_HOLE_ROOT: REFUTED
GROUP84_AS_OPENING_HOLE_ROOT: REFUTED
GFX7759_GROUP:                nested type-13 leaf dl_start=7721 under parent 196
GFX7759_VISUAL_ROLE:          WALL
GFX18_GROUP:                  0 (sky/horizon card; AABB does not bound verts)
GFX18_VISUAL_ROLE:            BACKGROUND
XLU_7418:                     ROM alias of OPA 7417 (CORRECTED)
XLU_EDGE_COVERAGE_PRESENT:    NO
SOURCE_4_3_EDGE_COVERAGE_COMPLETE: NO (right is sky/clear, not a missing wall)
INTRO_BACKGROUND_RENDER_PATH: OPA gfx1-22 sky card + VI/fb clear
TARGET_ONLY_HOLE_COUNT:       0 at 16:9 frame 6955; 1 at 16:9 all-frames (1255 L y=+1); 13 at 21:9/32:9 6955 left
LEFT_VISIBLE_EDGE_OWNER:      gfx 7779/7644/7759/7760/7799 WALL family HIGH
RIGHT_VISIBLE_EDGE_OWNER:     NO_MODEL_GEOMETRY / gfx 18 BACKGROUND HIGH
HOLE_BOUNDARY_GROUP:          nested 196/7721 (21:9+ only)
HIGH_Y_HOLE_CONFIRMED_BY_LIVE_RAYS: NO (219/223 not screen-edge)
GEOMETRY_EXTENSION_NOW_PROVEN: NO
TOOIE_MATCHES_BANJO_PATTERN:  LOW
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           Bind 16:9-stable non-wall owners (gfx 10993/3214/8336) and frame-1255 LEFT y=+1 7799 miss; do not extend 81/84 or 7759 yet
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-real-edge-owner-20260830T145138Z/
ARTIFACT_CHECKSUM_MANIFEST:   380e7c2aec4d849f6fb3d7197173adfd2f533066890e3d497ec74c8fdcd4560f
GPU_READBACK_USED:            NO
BUILD_RESULT:                 SKIP
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

CPU vertical edge sweep of OPA 7417 plus XLU identity. 7418 is a table
alias of 7417. Live 16:9 still hits the left WRAP wall family. Right 4:3
is already sky. No geometry candidate. Do not port Banjo verts. Do not
reintroduce Clamp16x9.

## Prior (2026-08-30) — MAP18B_INTRO_BOUNDARY_OWNERSHIP_AND_GEOMETRY_EXTENSION_CANDIDATE

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
PRODUCTION_MILESTONE_HEAD:    23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve; no intro-geometry candidate source)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
GROUP81_VERTEX_COUNT:         7
GROUP81_TRIANGLE_COUNT:       3
GROUP84_VERTEX_COUNT:         21
GROUP84_TRIANGLE_COUNT:       9
GROUP81_DL:                   gfx 2561-2585
GROUP84_DL:                   gfx 2643-2691
RIGHT_BOUNDARY_SURFACE:       OPA group 81 tri gfx 2583 ids (1404,1405,1406)
LEFT_BOUNDARY_SURFACE:        OPA group 84 9-tri multi-material leaf
WRAP_MODE:                    CLAMP,CLAMP
MATERIAL_STATE_REUSABLE:      YES
TEXTURE_CAN_REPEAT:           NO
UV_CAN_BE_CONTINUED:          clamp-stretch only
STATIC_EXTENSION_FEASIBLE:    YES
STATIC_EXTENSION_SAFE:        NO (81/84 are not live frustum-edge owners)
DYNAMIC_EXTENSION_FEASIBLE:   YES (architecture only)
BANJO_PATCH_ASPECT_STRATEGY:  STATIC world-space extra geo; not aspect-derived
TOOIE_CAN_USE_SAME_ARCHITECTURAL_PATTERN: PARTIAL
HIGH_Y_SEPARATE_HOLE_EXISTS:  YES (aerial; groups 219/223 backdrop)
CHOSEN_ARCHITECTURE:          MORE_EVIDENCE_REQUIRED
CANDIDATE_IMPLEMENTED:        NO
INTRO_CLAMP16X9_VISUAL_RESULT: NO_CHANGE
FAILED_CLAMP_CHANGE_REMOVED:    YES
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           Bind the user opening-shot wall to the OPA triangles actually hit by live frustum-edge rays (gfx 7759 left / gfx 18 far-right), including XLU 7418; do not extend groups 81/84 yet
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-geometry-extension-candidate-20260830T143035Z/
ARTIFACT_CHECKSUM_MANIFEST:   6000f3f889a2693191175de272570e69ae73d93cd4be8eead9580c7adaa213e4
GPU_READBACK_USED:            NO
BUILD_RESULT:                 SKIP
WINDOW_DISPLAY_TEST:          SKIP
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

CPU-parsed OPA 7417. Previous CSV "type-3 groups" are type-13
DRAW_DISTANCE leaves; Gfx opcode is in w0. Groups 81/84 have exact
DL/vertex/material bind. Live frame-6955 frustum-edge rays do **not**
hit those leaves (right el=0 misses OPA even at 4:3). No geometry
candidate. Do not port Banjo verts. Do not reintroduce Clamp16x9.

## Prior (2026-08-30) — MAP18B_LIVE_CAMERA_AND_WALL_ENVELOPE_CAPTURE

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
PRODUCTION_MILESTONE_HEAD:    23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve; intro-camera diagnostic untracked)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
GAMEPLAY_MAP_ROOT:            resolved / production fix active
GAMEPLAY_OBJECT_ROOT:         resolved / production fix active
MAP18B_CAMERA_OWNER:          D_8012D500 + func_800CB124 + host func_800E44FC
INTRO_REAL_CAMERA_POSITION:   (-1243.8077, 3682.9285, -1254.0330) frame 6955 constraining descent
INTRO_OPENING_CAMERA_POSITION:(529.3794, 6943.8896, -1904.1279) frame 3 aerial
INTRO_REAL_CAMERA_FORWARD:    (0.74934, 0.00698, -0.66215)
INTRO_REAL_VFOV:              30.7071 deg (viewport+0xF4 cot; NOT vp+0x18)
INTRO_REAL_TARGET_ASPECT:     1.777778 (1600x900 Expand/Clamp16x9)
INTRO_REAL_TARGET_HFOV:       52.0369 deg
LIVE_MAX_SAFE_HORIZONTAL_FOV: 40.2985 deg
LIVE_MAX_SAFE_ASPECT:         1.3363:1
SAFE_4_3:                     YES
SAFE_16_10:                   NO
SAFE_16_9:                    NO
SAFE_21_9:                    NO
SAFE_32_9:                    NO
INTRO_WALL_GEO_GROUP:         81 right / 84 left (type-3 OPA 7417)
INTRO_WALL_LOCAL_AABB:        group 81 min=(2336,1258,-4308) max=(4460,2563,-3486)
BOUNDARY_PLANAR:              NO (volumetric groups; faces unbound)
DYNAMIC_CAMERA_FULL_WIDTH_FEASIBLE: NO
GEOMETRY_EXTENSION_FEASIBLE:  YES
GEOMETRY_EXTENSION_COMPLEXITY: MEDIUM
BANJO_TOOIE_STRUCTURAL_SIMILARITY: HIGH
INTRO_FINAL_SOLUTION_CLASS:   MORE_EVIDENCE_REQUIRED
INTRO_CLAMP16X9_VISUAL_RESULT: NO_CHANGE
FAILED_CLAMP_CHANGE_REMOVED:    YES
D_80127630_MAP_395:           NEVER (T_INTRO mode=3 map=0; map18b_calls=0)
REPRESENTATIVE_ALCOVE_LIVE:   NO occupancy; do not promote 60°/1.4849:1
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           CPU-parse OPA 7417 vertex/material/DL ownership for live groups 81/84
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-live-camera-envelope-20260830T133604Z/
ARTIFACT_CHECKSUM_MANIFEST:   3c4b0229c98c233364abc69def03f61f23749e21f2c11dac5216b3c913dbf8d7
GPU_READBACK_USED:            NO
BUILD_RESULT:                 PASS
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

Isolated CPU capture of T_INTRO (historical `TOOIE_DIAG_INTRO_CAMERA` probe, removed). Live VFOV
30.7071°. Constraining descent shot is 4:3-safe and fails 16:10+ on the
right (group 81). Camera full-width adaptation is not viable. Geometry
extension is the Banjo analog but vertex/material/DL are still unbound.
No production intro fix. Do not port Banjo verts. Do not reintroduce Clamp16x9.

## Prior (2026-08-30) — INTRO_ARBITRARY_ASPECT_AUDIT

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
INTRO_REAL_CAMERA_POSITION:     UNKNOWN (no isolated ROM/runtime capture)
INTRO_REAL_VFOV:                UNKNOWN (prior 60° was inference)
MAX_SAFE_HORIZONTAL_FOV:        81.2153° representative inference only
MAX_SAFE_ASPECT_AT_CURRENT_VFOV: 1.4849:1 representative inference only
RECOMMENDED_INTRO_SOLUTION_CLASS: MORE_EVIDENCE_REQUIRED
INTRO_ASPECT_AUDIT_ARTIFACT:    /tmp/p08-intro-arbitrary-aspect-fix-design-20260830T122919Z/
ARTIFACT_DIRECTORY:           /tmp/p08-intro-source-geometry-framing-20260830T115524Z/
ARTIFACT_CHECKSUM_MANIFEST:   28c90810dcd26bfd9e7015417089cbd14425d0dc271514558727a372db63a867
```

CPU/static parse of MAP_18B OPA/XLU. Representative 4:3 rays stay
inside the alcove; Expand/16:9 rays exit. Live pose was UNKNOWN.
Do not port Banjo `ASSET_149D` vertices.

## Prior (2026-08-30) — CLEAN_PINNED_BUILD_VALIDATION

```text
CURRENT_HEAD:                 23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve; original trees not touched)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
GAMEPLAY_MAP_ROOT:            resolved / production fix active
GAMEPLAY_OBJECT_ROOT:         resolved / production fix active
CLEAN_PINNED_BUILD_VALIDATION: PASS
CLEAN_CONFIGURE_RESULT:       PASS
CLEAN_BUILD_RESULT:           PASS
CLEAN_SMOKE_RESULT:           SKIP (no isolated ROM; do not use shared ~/.config/TooieRecompiled)
CAPTURE_ABI_NEEDED_BY_CLEAN_BUILD: NO
CAPTURE_ABI_REFERENCED_ONLY_BY_DIRTY_DIAGNOSTICS: YES
MILESTONE_REPRODUCIBLE_FROM_CLEAN_PINS: YES
INTRO_WALL_RESIDUAL:          STILL_PRESENT
INTRO_MISSING_WALL_CLASS:     SOURCE_GEOMETRY / CAMERA_FRAMING
INTRO_ROOT_STATUS:            STRONG_HYPOTHESIS
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR:           INTRO_MAP_18B_SOURCE_EXTENT_VS_EXPAND_FRUSTUM
NEXT_SINGLE_ACTION:           CPU dump MAP_18B_CS_STARTUP_INTRO OPA/XLU vertex XZ vs source 4:3 and target Expand
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-clean-milestone-validation-20260830T035059Z/
ARTIFACT_CHECKSUM_MANIFEST:   af72f29f96026b3d678d0f6914ea21b6aeaae648ea6c9fe25a4e5e1ea7976228
```

Isolated detached worktrees at the three pins built `TooieRecompiled`
with Clang/Ninja after `tools/apply_runtime_patches.sh` on the isolated
N64ModernRuntime only. Clean RT64 `6f1c2d99` does not reference the
no-op capture ABI. Do not push.

## Prior (2026-08-30) — OBJECT_FRUSTUM_PRODUCTION + INTRO_WALL_ROOT

```text
CURRENT_HEAD:                 23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
800CACEC_AS_OBJECT_POP_GATE:  PROVEN
OBJECT_FRUSTUM_ASPECT_AWARE_VISUAL_RESULT: VISUALLY_CONFIRMED
OBJECT_FRUSTUM_PRODUCTION_PROMOTED: YES
PRODUCTION_OBJECT_FIX_MATCHES_VALIDATED_DIAGNOSTIC: YES
FOUR_THREE_IDENTITY:          PROVEN
ARBITRARY_ASPECT_SUPPORT:     YES
DEMO_ORIGINAL_CULL_RESULT_PRESERVED: YES
MAP_VIS_PRODUCTION_FIX:       ACTIVE
INTRO_WALL_RESIDUAL:          STILL_PRESENT
INTRO_STATE_FUNCTION:         T_INTRO mode=3 map=0; MAP_18B_CS_STARTUP_INTRO (395/0x18B)
INTRO_CAMERA_PATH:            chintrocam overlay
INTRO_MAP_OR_MODEL_SOURCE:    MAP_18B_CS_STARTUP_INTRO
INTRO_LATERAL_WALL_GEOMETRY_PRESENT: UNKNOWN
INTRO_MISSING_WALL_PASSES_800CAA24: UNKNOWN
INTRO_MISSING_WALL_CLASS:     SOURCE_GEOMETRY / CAMERA_FRAMING
INTRO_FIRST_DROP_GATE:        NONE_PROVEN
INTRO_CAMERA_EXPAND_EXPOSES_UNSAFE_REGION: UNKNOWN
BANJO_INTRO_GEOMETRY_FIX:     hotpatch_intro_opa_map_model ASSET_149D extra wall/grass DL
TOOIE_STRUCTURAL_ANALOG:      MAP_18B OPA/XLU + Expand; do not port Banjo verts
INTRO_ROOT_STATUS:            STRONG_HYPOTHESIS
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR:           INTRO_MAP_18B_SOURCE_EXTENT_VS_EXPAND_FRUSTUM
NEXT_SINGLE_ACTION:           CPU dump MAP_18B_CS_STARTUP_INTRO OPA/XLU vertex XZ vs source 4:3 and target Expand
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-object-production-intro-root-20260830T031102Z/
ARTIFACT_CHECKSUM_MANIFEST:   3d69e9a80426875a31e3c65c1c07e538c61a93d9b49d9d54df2c39a1a2ba0fe8
```

User: `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1` gameplay objects stay
visible. Promoted to production (ON unless env `=0`). DRAW-only H-expand
(`800E3E8C` / `8011447C`); occupancy `8011458C` and `chlightfader` keep
original v0. Attract 62s prod new_accept=279. Intro walls still missing:
independent of both gameplay fixes. Banjo intro needed extra OPA
triangles + pillarbox. No intro production fix. This commit is the
gameplay `800CAA24` + `800CACEC` production milestone. Do not push.

## Prior (2026-08-30) — OBJECT_MODEL_FRUSTUM_BANJORECOMP_PORT

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_VIS_PRODUCTION_PROMOTED:  YES
MAP_SECTION_800CAA24_ROOT:    PROVEN_PARTIAL
GAMEPLAY_MAP_WALL_FIX:        VISUALLY_SUCCESSFUL
GAMEPLAY_OBJECT_POP:          STILL_PRESENT (user rerun)
SUMMARY_FRAMES:               8861
SUMMARY_TRANSITION_COUNT:     96 logged / 902 dropped
GAMEPLAY_INSTANCES_CAPTURED:  YES
INTRO_INSTANCES_EXCLUDED:     YES
OBJECT_A_RUNTIME_IDENTITY:    chsilo 0x801ad888 getter=0x80086e60 xyz=(-6706,-922,3702) r=95 MEDIUM
OBJECT_B_RUNTIME_IDENTITY:    chnests 0x801adf3c getter=0x80085ac8 xyz=(-5849,-921,3709) r=70 MEDIUM
CONTROL_C_RUNTIME_IDENTITY:   chnests 0x801adc30 getter=0x80085ac8 xyz=(-7127,-920,4412) LOW
OBJECT_A_TRANSITION_CLASS:    OBJECT_MODEL_FRUSTUM_SIDE_PLANE
OBJECT_B_TRANSITION_CLASS:    OBJECT_MODEL_FRUSTUM_SIDE_PLANE
SOURCE_REJECT_TARGET_ACCEPT_OBSERVED: YES (89/96 logs)
CACEC_BANJO_ANALOG_STATUS:    PROVEN
CACEC_SEES_EXPANDED_PLANES:   NO
800CACEC_AS_OBJECT_POP_GATE:  PROVEN
PREVIOUS_CACEC_NO_CHANGE:     intro/map walls, not this object-pop path
DEMO_ORIGINAL_CULL_RESULT_REQUIRED: YES
OBJECT_FRUSTUM_COUNTERFACTUAL_IMPLEMENTED: YES
OBJECT_FRUSTUM_COUNTERFACTUAL_FLAG: TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE
CACEC_SOURCE_REJECT_TARGET_ACCEPT_COUNT: 244 (attract B)
CACEC_TARGET_NEW_DRAW_COUNT:  244
OBJECT_FRUSTUM_COUNTERFACTUAL_RESULT: STRONG_HIT
FOUR_THREE_IDENTITY:          PROVEN
ARBITRARY_ASPECT_SUPPORT:     YES
INTRO_WALL_RESIDUAL:          OPEN_MIXED
USER_MANUAL_TEST_REQUIRED_NOW: YES
FIX_IMPLEMENTED:              YES (800CAA24 production); object frustum diagnostic only, not promoted
NEXT_DISCRIMINATOR:           USER_VISUAL_OBJECT_FRUSTUM_ASPECT
NEXT_SINGLE_ACTION:           TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1 ./build/TooieRecompiled
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-object-model-frustum-banjo-port-20260830T024845Z/
ARTIFACT_CHECKSUM_MANIFEST:   ab92e43b8cfe48e81ae44b85beed830d8751f621d54aeadc7bd0b8ae55f9e126
```

User rerun with `TOOIE_DIAG_WS_OBJECT_POP=1` captured gameplay
`chnests`/`chsilo`/`chsignpost`. Distance fell while `800CACEC` went
ACCEPT→REJECT and source-4:3 NO / target-Expand YES. Same k H-expand
as production `800CAA24` is implemented behind
`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE` (default OFF, not blanket
TRUE). Attract 62s: 244 source-reject/target-accept applied. Do not
promote until the user confirms A/B stay visible. Keep original cull
for demo callbacks (Banjo PR #29). Do not combine vis flags.

## Prior (2026-08-30) — OBJECT_POP_WITNESS_ANALYSIS (INCONCLUSIVE)

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_VIS_ASPECT_AWARE_VISUAL_RESULT: VISUALLY_CONFIRMED_FOR_TESTED_GAMEPLAY_MAP_GEOMETRY
MAP_VIS_PRODUCTION_PROMOTED:  YES
PRODUCTION_MAP_FIX_MATCHES_VALIDATED_DIAGNOSTIC: YES
FOUR_THREE_IDENTITY:          PROVEN
ARBITRARY_ASPECT_SUPPORT:     YES
RT64_CHANGE_NEEDED:           NO
OBJECT_POP_SCREENSHOT_SEQUENCE: PROVEN
OBJECT_POP_CLASSIFICATION:    INCONCLUSIVE
OBJECT_A_CLASS:               UNKNOWN
OBJECT_B_CLASS:               UNKNOWN
CONTROL_OBJECT_C_CLASS:       UNKNOWN
LOCKED_THIS_RUN:              chintrochar (0x80086C98) — NOT A/B
TYPE6_GAMEPLAY_OBJECT_POP_STATUS: REOPENED_TARGETED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
800CACEC_STRUCTURAL_ANALOG_OF_8024DB50: PROVEN
800CACEC_AS_OBJECT_POP_GATE:  STRONG_HYPOTHESIS
ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT: REFUTED
MAP_SECTION_800CAA24_ROOT:    PROVEN_PARTIAL
INTRO_WALL_RESIDUAL:          OPEN_MIXED
INTRO_RESIDUAL_INDEPENDENT_FROM_GAMEPLAY_MAP_FIX: STRONG_HYPOTHESIS
NORMAL_GAMEPLAY_AND_DEMO_OBJECT_POP_SHARED: USER_OBSERVED
OBJECT_POP_DIAGNOSTIC_FLAG:   TOOIE_DIAG_WS_OBJECT_POP
OBJECT_POP_SUMMARY_PATH:      /tmp/tooie-ws-object-pop-summary.txt
USER_MANUAL_TEST_REQUIRED_NOW: YES
FIX_IMPLEMENTED:              YES (800CAA24 production only; no object fix)
NEXT_DISCRIMINATOR:           USER_VISUAL_OBJECT_POP_DIAG_CACEC
NEXT_SINGLE_ACTION:           TOOIE_DIAG_WS_OBJECT_POP=1 ./build/TooieRecompiled (do not combine vis flags). Pan at the screenshot objects; summary auto-writes.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-object-pop-witness-analysis-20260830T022303Z/
ARTIFACT_CHECKSUM_MANIFEST:   471db3eaf6f138143cfcf7443adbeb6eeef183aa40d38c9a31ef54610b98dbef
```

Completed diagnostic: frames=8315 pool=24 locked=3 logs=0. Locked
`_chintrochar_entrypoint_0`. `800EFFB4` never called. Type-6 vis did not
toggle on captured intro instances; A/B were not in the pool. `800CACEC`
is Banjo `viewport_func_8024DB50` (sphere vs 4 side planes). Diagnostic
now tracks `800CACEC`, evicts intro, never locks intro. No object
production fix. If later proven: same k H-expand as `800CAA24` for DRAW
only; keep original v0 for demo/callback semantics; no blanket TRUE.

## Prior (2026-08-30) — WIDESCREEN_EDGE_GEOMETRY MAP_VIS_PRODUCTION + OBJECT_POP_WITNESS

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_VIS_ASPECT_AWARE_VISUAL_RESULT: VISUALLY_CONFIRMED_FOR_TESTED_GAMEPLAY_MAP_GEOMETRY
MAP_VIS_PRODUCTION_PROMOTED:  YES
PRODUCTION_MAP_FIX_MATCHES_VALIDATED_DIAGNOSTIC: YES (near-match attract; same k/math)
FOUR_THREE_IDENTITY:          PROVEN
ARBITRARY_ASPECT_SUPPORT:     YES
RT64_CHANGE_NEEDED:           NO
OBJECT_POP_SCREENSHOT_SEQUENCE: PROVEN
OBJECT_A_CLASS:               UNKNOWN
OBJECT_B_CLASS:               UNKNOWN
CONTROL_OBJECT_C_CLASS:       UNKNOWN
TYPE6_GAMEPLAY_OBJECT_POP_STATUS: REOPENED_TARGETED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT: REFUTED
MAP_SECTION_800CAA24_ROOT:    PROVEN_PARTIAL
INTRO_WALL_RESIDUAL:          OPEN_MIXED
INTRO_RESIDUAL_INDEPENDENT_FROM_GAMEPLAY_MAP_FIX: STRONG_HYPOTHESIS
NORMAL_GAMEPLAY_AND_DEMO_OBJECT_POP_SHARED: USER_OBSERVED
OBJECT_POP_DIAGNOSTIC_FLAG:   TOOIE_DIAG_WS_OBJECT_POP
OBJECT_POP_SUMMARY_PATH:      /tmp/tooie-ws-object-pop-summary.txt
USER_MANUAL_TEST_REQUIRED_NOW: YES
FIX_IMPLEMENTED:              YES (800CAA24 production only; no object fix)
NEXT_DISCRIMINATOR:           USER_VISUAL_OBJECT_POP_DIAG
NEXT_SINGLE_ACTION:           TOOIE_DIAG_WS_OBJECT_POP=1 ./build/TooieRecompiled (do not combine with TOOIE_TEST_DISABLE_MAP_VIS_CULL). Pan at the screenshot objects; summary auto-writes.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-map-production-and-object-witness-20260830T020240Z/
ARTIFACT_CHECKSUM_MANIFEST:   9c7566804674e3057f71414d1cf659396e47e75fb705dfdf4bd046b0ba5e86d8
```

User: gameplay walls look good under aspect-aware. Three screenshots prove
individual A/B object pop (gray dome then orange; blue nest control stays).
Same pop in demo. Intro walls still broken. Production 800CAA24 H-expand is
ON unless `TOOIE_TEST_MAP_VIS_ASPECT_AWARE=0`. Attract P@5880 new_accept=3425
vs diagnostic C 3504. Identities remain UNKNOWN (do not guess from appearance).

## Prior (2026-08-30) — WIDESCREEN_EDGE_GEOMETRY 800CAA24_FINAL_SEMANTICS

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MAP_VIS_ASPECT_AWARE_CLASSIFICATION: STRONG_CANDIDATE
FUNC_800CAA24_ROLE: world float AABB vs 4 frustum SIDE planes (no near/far)
FUNC_800E3D0C_ROLE: map-section wrapper; two 800CAA24 tests on D_8012D500
INDEPENDENT_GEOCMD_LIKE_CULL_EXISTS: YES (800E3DC0/800CAB70 = BK GeoCmdD homolog)
DISTINCT_GEOCMD_LAYER_BEYOND_E3DC0: NO
INTRA_MODEL_GEOCMD_ANALOG_UNTESTED_WORDING: CORRECTED/SUPERSEDED
E3DC0_DIRECT_ROOT: REFUTED (do not relabel)
TARGET_ASPECT_AVAILABLE_TO_GUEST_PATCH: YES
ASPECT_API_IMPLEMENTATION_NEEDED: NO
RT64_CHANGE_NEEDED: NO
FOUR_THREE_IDENTITY: PROVEN (k<=1 leaves orig)
USER_VISUAL_TEST_REQUIRED_NOW: YES
CURRENT_ROOT_SET: 800CAA24 PARTIAL + aspect-aware STRONG_CANDIDATE + leftover items/rocks MIXED + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + STRONG_CANDIDATE_PENDING_USER_VISUAL + OPEN_RESIDUALS
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR:           USER_VISUAL_MAP_VIS_ASPECT_AWARE
NEXT_SINGLE_ACTION:           TOOIE_TEST_MAP_VIS_ASPECT_AWARE=1 ./build/TooieRecompiled (do not combine with TOOIE_TEST_DISABLE_MAP_VIS_CULL). Compare gameplay walls vs leftover items/rocks vs intro MIXED.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-map-vis-final-semantics-20260830T010730Z/
ARTIFACT_CHECKSUM_MANIFEST:   2c66095cc262b4d6b8fad31a4bc13b0609109db4555e223cb638fce456c3eaca
```

Attract aligned frames=5880: A dl=262956; B blanket dl=1063786 forced=401577;
C aspect dl=283719 new_accept=3504 (C/A DL 1.079, B/A 4.045). `k=1.33333`
source=4/3 target=16/9. Prior remaining-static artifact preserved
`/tmp/p08-widescreen-remaining-static-decorative-draw-20260830T002649Z/`
`f7fa715b7bb5888df0222be2ef5a2439243453f67732ad6aafb1414f634c5174`.

## Prior (2026-08-30) — WIDESCREEN_EDGE_GEOMETRY REMAINING_STATIC_DECORATIVE_MODEL_DRAW

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
FUNC_80108C14_ROLE: rare D_80136EE0 walk; overlay-loaded then 800819B4
FUNC_80108C14_IS_PER_FRAME: NO (LIVE 2 vs 800FF62C=1337)
FUNC_80108C14_IS_DRAW: CONDITIONAL
FUNC_80108C14_EMITS_DL: YES structurally; LIVE 0
MODEL_LIST_GLOBAL: D_80136EE0 (same 0x9C type-6 objects)
80108C14_AS_REMAINING_DRAW_ROOT: REFUTED
REMAINING_STATIC_DECORATIVE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
INTRO_RESIDUAL_CLASS: MIXED
STATIC_DECORATIVE_VIEW_COUNTERFACTUAL: NOT_IMPLEMENTED
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET: 800CAA24 PARTIAL + leftover items/rocks MIXED (type-6 Euclidean not aspect; actor draw NO_CHANGE; possible intra-model GeoCmd analog untested; leftover source geometry) + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + OPEN_RESIDUALS
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR:           GLOBAL_RESIDUAL_RERANK_800CAA24_FINAL_SEMANTICS
NEXT_SINGLE_ACTION:           Re-rank remaining gameplay residual after closed object families: 800CAA24 remaining section semantics versus intra-model GeoCmd analog versus leftover source geometry. Do not return to GPU. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-remaining-static-decorative-draw-20260830T002649Z/
ARTIFACT_CHECKSUM_MANIFEST:   f7fa715b7bb5888df0222be2ef5a2439243453f67732ad6aafb1414f634c5174
```

60s attract: `08c14=2 visits=42 skip_id0=41 da298 fail=1 ffab0=0 dl=0`.
Same list as type-6 (`ff62c=1337 vis rej=0 dl=53`). Jalr `obj+0x10` is
GETTER (`chintrochar`/`chnests`/`chsignpost`/…), not DRAW. Gates are
RESOURCE (`asset_id==0` / `800DA298` overlay loaded); `CAN_REJECT_SOTI: NO`.
HARD STOP Parts 14/15. Prior actor-pickup artifact preserved
`/tmp/p08-widescreen-actor-pickup-premodel-view-20260830T000358Z/`
`4706ad90341a23e6bbf299a53acac7495b32f472387dbcc3d7dc77665fec0d5d`.

## Prior (2026-08-30) — WIDESCREEN_EDGE_GEOMETRY ACTOR_PICKUP_PREMODEL_VIEW_VISIBILITY

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
FUNC_8011458C_ROLE: occupancy/near-fog probe via 801143A4; NOT actor draw dispatch
FUNC_8011458C_PER_FRAME: YES
ACTOR_SOURCE_CONTAINER: gccube grid D_80128410 / cube 0xC occupancy
ACTOR_SPATIAL_SELECTION_FUNCTION: 80114184 AABB + 800BD708 cube idx
ACTOR_LIST_CONSUMER: 800CD23C
ACTOR_DRAW_DISPATCH: 800EA628
ACTOR_MODEL_DRAW: 80103508
ACTOR_DL_EMIT: 8010108C
FIRST_NEW_ACTOR_VIEW_GATE: 800BD708 (cube AABB; WORLD_SPATIAL GRID)
ACTOR_VIEW_COUNTERFACTUAL_FLAG: TOOIE_TEST_EXPAND_ACTOR_CUBE_AABB
ACTOR_VIEW_COUNTERFACTUAL_RESULT: PARTIAL_HIT visitation / NO_CHANGE draw-DL
ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET: 800CAA24 PARTIAL + actor cube AABB PARTIAL_HIT visitation / draw NO_CHANGE / REFUTED as Expand DRAW root + bound-sprite view cull REFUTED + type-6 Euclidean not aspect + leftover items/rocks + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + OPEN_RESIDUALS
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR:           REMAINING_STATIC_DECORATIVE_MODEL_DRAW
NEXT_SINGLE_ACTION:           Start at 80108C14 model-list / overlay jalr DRAW vs UPDATE for leftover items/rocks that are not 800CAA24, not 800EA628->8010108C (attract 0108c=0), not bound-sprite, not type-6 Euclidean. Do not automatically return to GPU. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-actor-pickup-premodel-view-20260830T000358Z/
ARTIFACT_CHECKSUM_MANIFEST:   4706ad90341a23e6bbf299a53acac7495b32f472387dbcc3d7dc77665fec0d5d
```

60s attract A (flag=0): `14184=1505 ea628=22722 03508=3 draw108c=0 dl=0 cube_span max=8`.
60s attract B (`TOOIE_TEST_EXPAND_ACTOR_CUBE_AABB=1`): `ea628=257504` (~11×) `expanded=3371` `03508=3 draw108c=0 dl=0 cube_span max=72`. Extra candidates fail `800EB350` Euclidean / `800CB854` pass-mode. HARD STOP on this actor view family.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY BOUND_SPRITE_INSTANCE_PER_FRAME_DRAW

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
PROP_TYPE_7_ROLE: static bound sprite occupancy (800CBA68 -> gcbound_ep3)
PROP_TYPE_9_ROLE: static bound sprite occupancy (800CBAC8 -> gcbound_ep4)
PROP_TYPE_A_ROLE: SKIPPED (CORRECTED/SUPERSEDED)
PROP_TYPE_C_ROLE: static bound sprite occupancy (800CBB18 -> gcbound_ep5)
BOUND_SPRITE_INSERT_FUNCTION: 800B3CDC via 808001A8_gcboundDll
BOUND_SPRITE_CONTAINER: D_8012AE80+0xE0 (header 0x8012AF60)
BOUND_SPRITE_RECORD_SIZE: 0x18
BOUND_SPRITE_FIRST_PER_FRAME_CONSUMER: 800CB8F0 (lookup; queries 800CBC3C/CBDD4/CC078/CC338)
BOUND_SPRITE_DRAW_FUNCTION: NONE
BOUND_SPRITE_DL_EMIT_FUNCTION: NONE
BOUND_SPRITE_PATH_EXERCISED: YES (60s attract cb8f0=5777 nonnull=5777)
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
SPRITE_800F0064_AS_LATERAL_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET: 800CAA24 PARTIAL + type-6 Euclidean not aspect + 800F0064 REFUTED + bound-sprite view cull REFUTED + actor/pickup OPEN + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + OPEN_RESIDUALS
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR:           ACTOR_PICKUP_PREMODEL_VIEW_VISIBILITY
NEXT_SINGLE_ACTION:           Trace actor/pickup pre-draw vis at 800CAB70/800E3DC0/800CACEC and 8011458C; prove whether any gate can reject SOURCE_OUTSIDE_TARGET_INSIDE. No actor CF until proven. Do not reopen 800F0064 or bound-sprite draw. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-bound-sprite-per-frame-draw-20260829T231221Z/
ARTIFACT_CHECKSUM_MANIFEST:   fde93ff1c4e3059d186c567f810ad753b7fbe9cdc075ede0b193726d62a98d51
```

`800B3CDC` inserts 0x18 occupancy nodes (xyz, half-radius, packed id).
Per-frame consumers are spatial queries, not a renderer. LIVE 60s:
`draw_fn=NONE dl_emit=NONE`. No frustum/FOV/aspect pre-draw gate.
HARD STOP on this family.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY GSPROPS_SPRITE_7_9_A_PER_FRAME_VISIBILITY

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
PROP_TYPE_7_ROLE: static bound sprite (800CBA68 -> gcbound_ep3)
PROP_TYPE_9_ROLE: static bound sprite (800CBAC8 -> gcbound_ep4)
PROP_TYPE_A_ROLE: SKIPPED (CORRECTED/SUPERSEDED)
PROP_TYPE_C_ROLE: static bound sprite (800CBB18 -> gcbound_ep5)
FUNC_800F0064_ROLE: world XZ dist^2 < radius^2 (NOT camera/FOV/aspect)
FUNC_800F0064_CAN_CAUSE_LATERAL_EXPAND_POP: NO
SPRITE_PATH_AUTOMATICALLY_EXERCISED: YES (60s attract; 30s intro t7=0)
T7_SEEN: 47
T9_SEEN: 36
TA_SEEN: 0 (type 0xA skipped)
TC_SEEN: 1
SPRITE_VISIBILITY_TESTS: 703
SPRITE_VISIBILITY_ACCEPTS: 62
SPRITE_VISIBILITY_REJECTS: 641
SPRITE_DRAW_ATTEMPTS (inserts): 90 -> 114 CF
GSPROPS_VIS_COUNTERFACTUAL_REUSED: YES
SPRITE_VIS_COUNTERFACTUAL_CPU_RESULT: PARTIAL_HIT (inserts); REFUTED as Expand pop
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET: 800CAA24 PARTIAL + type-6 Euclidean not aspect + 800F0064 spatial-index REFUTED as FOV + bound-sprite per-frame draw UNKNOWN + actor/pickup OPEN + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + OPEN_RESIDUALS
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR:           BOUND_SPRITE_INSTANCE_PER_FRAME_DRAW
NEXT_SINGLE_ACTION:           Identify who walks D_8012AE80+0xE0 / 800B3CDC products every frame and which camera/FOV gate (if any) drops them before DL. Do not reopen 800F0064 as vis. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-gsprops-sprite-visibility-20260829T224516Z/
ARTIFACT_CHECKSUM_MANIFEST:   40941e011e86c403ec0b9cb0f781a6db3b15ff04cd4d7ea6aae18ce5be604894
```

`gccube_ep6` is MAP-LOAD (LIVE gccube6=2). `800F0064` compares prop world XZ
to bound records; samples are far world pairs, not 4:3 lateral edges.
Independent `TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL=1` forced 70 rejects and
raised inserts 90→114 (`PARTIAL_HIT`) but cannot explain Expand pop.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY LOADED_STATIC_MODEL_PER_FRAME_DRAW

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
MODEL_GEO_FLAG_CONFIRMED_ACTIVE_DURING_GAMEPLAY: UNKNOWN
GSPROPS_ENTRYPOINT_1_ROLE: PROVEN_ONE_SHOT_TYPE6_LOAD
GSPROPS_ENTRYPOINT_1_SELECTOR_RESULT: REFUTED_AS_PER_FRAME_VISIBILITY_SELECTOR
PROP_CELL_800BE09C_RESULT: REFUTED_FOR_STATIC_TYPE6_ROOT
FUNC_80108DC0_ROLE: model/resource constructor into D_80136EE0 (NOT vis)
TYPE6_PROP_RECORD_SIZE: 0x14 (cube-list); loaded object 0x9C
TYPE6_FIRST_PER_FRAME_CONSUMER: 800FF62C
TYPE6_FIRST_PER_FRAME_CONSUMER_ADDRESS: 0x800FF62C
TYPE6_PER_FRAME_ITERATION_SOURCE: D_80136EE0
TYPE6_DRAW_FUNCTION: 8010108C (first visit) then vtable+0x10
TYPE6_DL_EMIT_FUNCTION: 800819B4
TYPE6_FIRST_VISIBILITY_GATE: 8010038C later + 800EFFB4 Euclidean sphere
TYPE6_FIRST_VISIBILITY_GATE_ROLE: dist^2 vs radius^2; NOT frustum/FOV/aspect
TYPE6_VISIBILITY_GATE_STATUS: PROVEN exists; LIVE never rejected tracked intro type-6
TYPE6_CONSUMER_EXERCISED: YES (25s auto-start still intro)
TRACKED_TYPE6_LOADED: 10
TRACKED_TYPE6_CONSUMER_SEEN: 320
TRACKED_TYPE6_DRAW_ATTEMPTS: 84
TRACKED_TYPE6_VISIBILITY_REJECTS: 0
TRACKED_TYPE6_VISIBILITY_ACCEPTS: 0
TRACKED_TYPE6_DL_EMITS: 53
TYPE6_VIS_COUNTERFACTUAL_IMPLEMENTED: YES (TOOIE_TEST_DISABLE_TYPE6_STATIC_VIS_CULL)
TYPE6_VIS_COUNTERFACTUAL_RESULT: NO_CHANGE
TYPE6_AS_OBSERVED_RESIDUAL_ROOT: STRONGLY_WEAKENED (intro)
RESIDUAL_ROCKS_LIKELY_TYPE6: UNKNOWN
RESIDUAL_ITEMS_LIKELY_TYPE6: UNKNOWN
RESIDUAL_GENERAL_ASSETS_LIKELY_TYPE6: UNKNOWN
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET: 800CAA24 PARTIAL + type-6 draw path PROVEN but vis not aspect + sprite/gcbound OPEN + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + OPEN_RESIDUALS
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
GPU_READBACK_USED:            NO
FIX_IMPLEMENTED:              NO
NEXT_DISCRIMINATOR:           GSPROPS_SPRITE_7_9_A_PER_FRAME_VISIBILITY
NEXT_SINGLE_ACTION:           Exercise types 7/9/A -> gcbound -> 800F0064 on an automated attract/demo window that actually dispatches sprites (intro t7=0). Do not combine vis flags. Type-6 consumer is closed.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-loaded-static-model-per-frame-draw-20260829T215504Z/
ARTIFACT_CHECKSUM_MANIFEST:   de43c12dfcaccd59e3b4cd4ca2b6aa88a74dd232e89b757c4e6e56393b4c7940
```

`80108DC0` constructs a 0x9C object and `80106668` appends it to
`D_80136EE0`. `gsprops_1` does not store v0. Per-frame consumer is
`800A5CFC` → `800FF62C` (LIVE 500). Vis is first-visit `8010108C` then
later Euclidean `800EFFB4` (never entered for the 10 tracked intro models).
`+0x64` bit17 skip is gameplay (320 vs 5000 visits); do not bypass.
Independent `TOOIE_TEST_DISABLE_TYPE6_STATIC_VIS_CULL=1` produced
`NO_CHANGE` (draw_attempts 84, dl_emits 53, dl_rsp 34472). Type-6 is
`STRONGLY_WEAKENED` as the widescreen-edge residual root in this window.

Sprite/bound path (`gsprops_2` types 7/9/A → gcbound → `800F0064`) remains
**not exercised** in intro 25s (`t7=0`). Do **not** wrap `80108DC0` as vis.
Do **not** combine MAP_VIS / MODEL_GEO / PROP_CELL / GSPROPS_VIS / TYPE6_VIS.
Do **not** reopen GPU/TMEM/`00349d38`. Intro leftover remains MIXED.

`NEXT_SINGLE_ACTION:` Exercise gsprops types 7/9/A → gcbound → `800F0064`
on an automated attract/demo window that actually dispatches sprites.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY GSPROPS_STATIC_PROP_SELECTOR

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
MODEL_GEO_FLAG_CONFIRMED_ACTIVE_DURING_GAMEPLAY: UNKNOWN
GSPROPS_ENTRYPOINT_1_ADDRESS: overlay 0x808000A0; syscall stub 0x80088B88; DLL 727
GSPROPS_ENTRYPOINT_1_ROLE: PROVEN one-shot type-6 collect/load; NOT per-frame vis; NOT DL emit
GSPROPS_ENTRYPOINT_1_INPUT_CONTAINER: gccube_ep0 stack list (cap ~500); ALL cubes D_80128410
STATIC_PROP_SELECTOR_FUNCTION: gccubeDll_entrypoint_0 + gsprops_0 type==6
STATIC_PROP_SELECTOR_STATUS: PROVEN as one-shot ALL-cube type-6 load; REFUTED as residual per-frame vis
RELATION_TO_800BE09C: INDEPENDENT / PARALLEL
EXTRA_800BE09C_CELLS_DROPPED_AT: NEVER_CONSUMED_BY_GSPROPS
RELATION_TO_800CAA24: INDEPENDENT
GSPROPS_VIS_COUNTERFACTUAL_IMPLEMENTED: YES (TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL; 800F0064 in gsprops_vis_pass only)
GSPROPS_SELECTOR_COUNTERFACTUAL_CPU_RESULT: NO_CHANGE (xzrad=0 intro/25s; collect 11/10 identical)
PROP_CELL_800BE09C_RESULT: STRONG_HYPOTHESIS actor; REFUTED intro RSP; INDEPENDENT of gsprops
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET: 800CAA24 PARTIAL + loaded-static per-frame draw UNKNOWN + sprite/gcbound later + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + OPEN_RESIDUALS
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
GPU_READBACK_USED:            NO
FIX_IMPLEMENTED:              NO
NEXT_DISCRIMINATOR:           LOADED_STATIC_MODEL_PER_FRAME_DRAW
NEXT_SINGLE_ACTION:           Identify who draws the 10 models loaded by gsprops_1/80108DC0 after the one-shot collect; do not assume 80108C14 or 801143A4; do not combine MAP_VIS
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-gsprops-static-prop-selector-20260829T210810Z/
ARTIFACT_CHECKSUM_MANIFEST:   09bb0ef3fc74105e1306577af670ef13d088044768047bad03438b53378e27ce
```

`gspropsDll_entrypoint_1` is a **one-shot load** of every cube's type-6
static models (`gccubeDll_entrypoint_0` walks all 1274 cubes; 11 appended,
10 load OK, 1 fail). It is **not** a per-frame visibility selector.
Sprite/bound path (`gsprops_2` types 7/9/A → gcbound → `800F0064` XZ radius)
was **not exercised** in intro 12s/25s (`t7=0`, `xzrad=0`). Independent
`TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL=1` did not change collect/load and
did not produce a causal RSP increase (`dl_rsp` 9375 vs 9445 is run variance).

`800BE09C` RANGE extras are **never consumed by gsprops** (gsprops already
walks all cubes). `800CAA24` map AABB is **independent**. Do **not** wrap
`80108DC0` as vis (resource/load). Do **not** assume `80108C14` is per-frame
draw (`calls=1`). Do **not** combine MAP_VIS / MODEL_GEO / PROP_CELL /
GSPROPS_VIS. Do **not** reopen GPU/TMEM/`00349d38`. Intro leftover remains
MIXED; do not deepen this round.

USER MAP_VIS still: gameplay maps/walls restored a lot; items/rocks/assets
still vanish at corners. USER MODEL_GEO still like the broken baseline.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY RESIDUAL_PROP_CELL_AND_CUTSCENE_VISIBILITY_AUDIT

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
MODEL_GEO_FLAG_CONFIRMED_ACTIVE_DURING_GAMEPLAY: UNKNOWN (12s/25s intro calls=0)
PROP_CELL_COUNTERFACTUAL_IMPLEMENTED: YES (TOOIE_TEST_DISABLE_PROP_CELL_VIS_CULL; draw-pass RANGE only)
PROP_CELL_COUNTERFACTUAL_CPU_RESULT: listed 153->325; dl_rsp identical 9445; 801143A4 still 0
TOOIE_PROP_SPATIAL_CONTAINER: gccube D_80128410
TOOIE_PROP_CELL_VISIBILITY_FUNCTION: 800BE09C 3x3x3 RANGE via 800BE444
TOOIE_PROP_CELL_VISIBILITY_STATUS: STRONG_HYPOTHESIS actor gate; REFUTED intro RSP; UNKNOWN gsprops static props
INTRO_800CAA24_ACTIVE: YES
INTRO_RESIDUAL_CLASS: MIXED
DEMO_RESIDUAL_CLASS: MIXED / UNKNOWN
CUTSCENE_ASPECT_FALLBACK_FEASIBLE: PARTIAL
BANJORECOMP_CURRENT_WIDESCREEN_CULLING_COMPLETE: NO (issue #125)
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
GPU_READBACK_USED:            NO
FIX_IMPLEMENTED:              NO
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET:             800CAA24 PARTIAL + gsprops selector UNKNOWN + intro leftover MIXED B/C
CURRENT_ROOT_SET_STATUS:      PARTIAL_PROVEN + OPEN_RESIDUALS
NEXT_DISCRIMINATOR:           GSPROPS_STATIC_PROP_SELECTOR
NEXT_SINGLE_ACTION:           Identify the selector that fills gspropsDll_entrypoint_1 (rocks/items/sprites); do not combine MAP_VIS
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-residual-prop-cell-cutscene-audit-20260829T204203Z/
ARTIFACT_CHECKSUM_MANIFEST:   52a0f350cc540708f9a6de44c7e60bc60066c51e6807e71c71395589bee59fbd
```

USER MAP_VIS: gameplay map walls restored a lot; items/rocks/assets still vanish at corners; intro/demo still lose walls.
USER MODEL_GEO: looked like the original broken baseline.
Do not treat 801143A4 as the residual object root. Do not collapse BK viewport_cube_* into 800CAA24.
Prior port-audit artifact `/tmp/p08-widescreen-banjorecomp-visibility-port-audit-20260829T192021Z/` (`8efe75283a7e8fa1d024b36e92dc61a3c57408acbff07ecf6cecbdd9aacd2a0b`) remains valid for SAME_RT64 / CAB70 NO_CHANGE / 00349d38 witness.

User clarification: missing walls/objects are pervasive (intro/cutscenes +
gameplay, many walls AND objects, late-pop-in and early-pop-out). `00349d38`
is a witness triangle only. Do not optimize the global root around its
texture, TMEM, individual depth, or material.

BanjoRecomp and Tooie pin the **same** RT64 revision
`6f1c2d99a4ea571c139f449c326fd176ba8f3496`. Remaining deltas are guest-side
(geometry selection, patches, LOD, culling, model mods). BK bbox/sphere/GeoCmdD
map to CAB70/CACEC/E3DC0 and are already USER-refuted as visual roots. BK
GeoCmd8 LOD has **no** Tooie equivalent. USER MAP_VIS proves `800CAA24`
partial for gameplay map geometry. USER MODEL_GEO and intro smokes (0 calls)
refute `801143A4` as the observed residual object root (gameplay exercise
UNKNOWN). Tooie cube RANGE `800BE09C` (via `800BE444`, draw-pass parents
`8011458C`/`8010A01C`/`80109CC4`/`801099D0`) is independent of `800CAA24`.
PROP_CELL CF listed cubes 153→325; intro `dl_rsp` stayed 9445; `801143A4`
stayed 0. Residual items/rocks/assets are NOT explained by this actor RANGE.
Texture/TMEM not reopened. Performance gating cleanup preserved. No production
fix.

`NEXT_SINGLE_ACTION:` Identify the selector that fills `gspropsDll_entrypoint_1` (rocks/items/sprites). Do not combine MAP_VIS. Do not assume `8011458C`.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_TEXTURE_TMEM_SETUP

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
PERFORMANCE_GATING_CLEANUP_IMPLEMENTED: YES
DIAGNOSTICS_OFF_OVERHEAD:     NEGLIGIBLE
OLD_P08_LOGS_DEFAULT_OFF:     YES
FOCAL_TEXTURE_FLAG:           TOOIE_DIAG_WS_FOCAL_TEXTURE
FOCAL_AUTO_TRIGGER_IMPLEMENTED: YES
AUTO_CAPTURE_DURATION:        300 focal draws (~150-180 frames)
AUTO_CAPTURE_STOP_CONDITION:  s_auto_draw_count >= 300
MANUAL_TOUCH_REQUIRED:        NO
FOCAL_SOURCE_DL:              00349d38 extra-left TRI1
FOCAL_TEXTURE_TRACE_READY:    YES
FOCAL_TMEM_TRACE_READY:       YES
FOCAL_TILE_TRACE_READY:       YES
FOCAL_LOAD_CHAIN_READY:       YES
FOCAL_UV_TRACE_READY:         YES
FOCAL_VS_CENTRAL_COMPARE_READY: YES
FOCAL_TEXTURE_TMEM_ANOMALY:   REFUTED
GPU_READBACK_USED:            NO
DEFAULT_RUNTIME_BEHAVIOR_CHANGED: NO
DEFAULT_RUNTIME_LOGGING:      NONE (standard boot output only)
USER_CAPTURE_REQUIRED_NOW:    NO
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-texture-tmem-setup-20260829T183238Z/
ARTIFACT_CHECKSUM_MANIFEST:   d91118158d0753c17e1ad0370e6150834255ffebffc67b02554692f39ad8b782
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]]. Performance cleanup completed:
software vertex/bounding box calculations in `rt64_framebuffer_renderer.cpp`
are gated under `tooieWsAnyDiagEnabled()`, per-draw `std::filesystem::exists`
calls removed, and file logging uses persistent handles. Standalone flag
`TOOIE_DIAG_WS_FOCAL_TEXTURE=1` with automatic trigger on focal world vertices
eliminates manual `touch` requirement. Texture, TMEM, tile descriptor, load chain,
and UV analysis prove that `FOCAL_TEXTURE_TMEM_ANOMALY` is `REFUTED` (the difference
in `tex_id` between focal and central FRONT is standard expected material difference
for distinct terrain faces).

`NEXT_SINGLE_ACTION:` Design and implement `TARGETED_FOCAL_DEPTH_CAUSAL_TEST_DESIGN` to isolate depth occlusion without GPU readback.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_RASTER_STATE_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          2926
CAPTURE_END_FRAME:            3645
CAMERA_EFFECTIVELY_STATIC:    YES (716/720 at 3584.1,50.7,4893.8)
FOCAL_SOURCE_DL:              00349d38 extra-left TRI1
FOCAL_EVENTS:                 288 TRI1 (1440 total 00349d38 in window)
FOCAL_GPU_SUBMITTED:          YES 286/286
FOCAL_PIPELINE:               000075df080aa190 SPECIALIZED RasterVS+RasterPSMS
FOCAL_DEPTH_TEST_ENABLED:     YES
FOCAL_DEPTH_WRITE_ENABLED:    YES
FOCAL_DEPTH_COMPARE:          LESS
FOCAL_DEPTH_RANGE_VALID:      YES
FOCAL_DEPTH_TEST_RESULT_DIRECTLY_OBSERVABLE: NO
FOCAL_DEPTH_TARGET_ID:        00137840
PRIOR_PROJ1_DEPTH_RESULT_APPLIES_TO_FOCAL: NO
DEPTH_CLEAR_BEFORE_FOCAL:     UNKNOWN
DEPTH_OCCLUSION_RESULT:       UNKNOWN
PRIOR_DEPTH_OCCLUDER_CANDIDATES: 1_WEAK_CORNER_SLIVER (00348470)
ACTIVE_FRAGMENT_DISCARD_SITES_FOR_FOCAL: 0
ALPHA_COMPARE_ROOT:           REFUTED_FOR_FOCAL
COVERAGE_ZERO_ROOT:           REFUTED_FOR_FOCAL
COLOR_WRITE_DISABLED_ROOT:    REFUTED_FOR_FOCAL
BLEND_ZERO_OUTPUT_ROOT:       REFUTED_FOR_FOCAL
FOCAL_INTERSECTS_RASTER_SCISSOR: YES
POST_PROJECTION_NATIVE_COORDINATE_MISMATCH: REFUTED
SOURCE_CLIP_FLAGS_USED_AFTER_TARGET_EXPANSION: NO
FIRST_FOCAL_VS_CENTRAL_RUNTIME_STATE_DIFFERENCE: tex_id
CENTRAL_CONTROL_IS_DEPTH_EQUIVALENT: PARTIAL
CULL_WINDING_ANOMALY:         REFUTED_FOR_FOCAL
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        after GPU submit of 00349d38 TRI1 call=134 proj=2
GPU_READBACK_USED:            NO
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-raster-user-arbitration-20260829T181535Z/
ARTIFACT_CHECKSUM_MANIFEST:   a8ce5b47e43ca92986ce19628cc1c1084e6ff2fa22079dc4f17197bcf8636190
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]]. USER capture at the missing-wall
pose with `TOOIE_DIAG_WS_FOCAL_RASTER=1`. Focal TRI1 is geometric FRONT, survives
cull, intersects scissor, and is GPU-submitted on proj=2 with depth test/write
LESS. Do not inherit proj=1 depth-off. Explicit fragment discard, alpha,
coverage-zero, color-write disable, and blend-zero are refuted for this focal.
CPU-side scissor-clipped AABB finds no strictly closer visible prior depth
writer. Depth-test *result* is not observable. First logged runtime state
difference vs same-DL FRONT TRI2 is TMEM `tex_id`. Do not claim wall pixels.

`NEXT_SINGLE_ACTION:` `TARGETED_FOCAL_TEXTURE_TMEM_TRACE` on `00349d38` TRI1 vs same-DL FRONT TRI2 (`tex_id` 4a7b1a2672b73a09 vs eca4591a3dbd2fcc). CPU-only. No GPU readback. No depth bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_RASTER_STATE_SETUP

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
INSTRUMENTATION_STATUS:       TARGETED_RASTER_STATE_TRACE_IMPLEMENTED
FOCAL_SOURCE_DL:              00349d38 extra-left TRI1
FOCAL_RASTER_FILTER_IMPLEMENTED: YES
PIPELINE_ID_CAPTURE_READY:    YES
SHADER_VARIANT_CAPTURE_READY: YES
ALPHA_COMPARE_CAPTURE_READY:  YES
COVERAGE_CAPTURE_READY:       YES
BLEND_CAPTURE_READY:          YES
COLOR_WRITE_CAPTURE_READY:    YES
DEPTH_CAPTURE_READY:          YES
SCISSOR_CAPTURE_READY:        YES
VIEWPORT_CAPTURE_READY:       YES
POSSCREEN_SEMANTICS_RESOLVED: YES (CPU-only, not in GPU shaders)
SOURCE_TARGET_CLIP_RELATION_CAPTURE_READY: YES
COMBINER_CAPTURE_READY:       YES
TEXTURE_STATE_CAPTURE_READY:  YES
FRAGMENT_DISCARD_SITES_IDENTIFIED: YES (0/7 active for focal)
FOCAL_HOST_SCREEN_BOUNDS_READY: YES
FOCAL_SCISSOR_INTERSECTION_READY: YES
POST_PROJECTION_NATIVE_COORDINATE_MISMATCH: REFUTED
SOURCE_CLIP_FLAGS_USED_AFTER_TARGET_EXPANSION: NO
FIRST_FOCAL_VS_CENTRAL_STATE_DIFFERENCE: NONE_OBSERVED
FOCAL_RASTER_STATE_CAPTURE_READY: YES
USER_CAPTURE_READY:           YES
GPU_READBACK_USED:            NO
FIX_IMPLEMENTED:              NO
DEFAULT_RUNTIME_BEHAVIOR_CHANGED: NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-targeted-raster-state-setup-20260829T173856Z/
ARTIFACT_CHECKSUM_MANIFEST:   fbd3366d1a666196a77c536779cd3b087a0957382ac60bee2cbffe334c81fc14
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]]. Targeted raster state telemetry
implemented in RT64 `FramebufferRenderer::addDrawCall` with dedicated log sink
`/tmp/tooie-ws-focal-raster-state.log` and unified tag `[ws-focal-raster-state]`.
Shader/PSO mapping proves specialized `RasterVS`/`RasterPS` pipeline is identical
between focal TRI1 and central same-DL FRONT TRI2. Alpha compare, coverage, blend,
color write mask (0x0F), depth test/write, and scissor intersection ([100..503] px
in [0..1920]) are 100% healthy. `FIRST_FOCAL_VS_CENTRAL_STATE_DIFFERENCE` is
`NONE_OBSERVED`.

`NEXT_SINGLE_ACTION:` User performs ONE capture at same wall, camera static, with `TOOIE_DIAG_WS_CONTINUOUS=1 TOOIE_DIAG_WS_WALL_BOUNDS=1 TOOIE_DIAG_WS_FOCAL_WINDING=1 TOOIE_DIAG_WS_FOCAL_RASTER=1`. Then Grok 4.6 Medium arbitrates.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          3754
CAPTURE_END_FRAME:            4473
CAMERA_EFFECTIVELY_STATIC:    YES (720/720)
CAM_POS:                      (3598.4, 57.8, 4908.3)
FOCAL_SOURCE_DL:              00349d38 TRI1 (world AABB matches prior extra-left)
GEOMETRIC_EXPECTED_FACING:    FRONT (norm_cam_dot=-0.5196)
SIGNED_AREA_SOURCE/TARGET:    +0.748114 / +0.525758 (CCW/CCW MATCH)
RASTER_CLASSIFIED_FACING:     BACK (Vulkan CLOCKWISE; survives CULL_FRONT)
EXPECTED_RASTER_SURVIVAL:     YES
FACING_MISMATCH:              NO
CULL_SURVIVAL_MISMATCH:       NO
CULL_WINDING_ANOMALY:         REFUTED_FOR_FOCAL
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-winding-user-arbitration-20260829T164524Z/
ARTIFACT_CHECKSUM_MANIFEST:   d0d993e6e015d482e6807350b8d65de993d6348b7f8d96287972e9d1ee13e489
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]]. New USER capture at the same
missing-wall location with `TOOIE_DIAG_WS_FOCAL_WINDING=1`. Real V0/V1/V2
match the prior extra-left TRI1. Focal TRI1 faces the camera, source/target
winding match (generic Sx=0.702785 for this ~1920x1012 window vs 4:3), and
the compensated RT64 cull expects survival. Same-DL FRONT TRI2 cluster shares
that signature. Do not pursue cull. No GPU pixel claim.

`NEXT_SINGLE_ACTION:` TARGETED_RASTER_STATE_TRACE on `00349d38` TRI1 (depth/coverage/blend/other-mode/scissor on native vs wide FB submits). No GPU readback. No cull bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          8278
CAPTURE_END_FRAME:            8997
CAMERA_EFFECTIVELY_STATIC:    YES (720/720)
CAM_POS:                      (3477.8, 113.9, 4682.8)
FOCAL_SOURCE_DL:              00349d38 TRI1 (world AABB matches prior extra-left)
GEOMETRIC_EXPECTED_FACING:    FRONT (norm_cam_dot=-0.5965)
SIGNED_AREA_SOURCE/TARGET:    +1.175925 / +0.881943 (CCW/CCW MATCH)
RASTER_CLASSIFIED_FACING:     BACK (Vulkan CLOCKWISE; survives CULL_FRONT)
EXPECTED_RASTER_SURVIVAL:     YES
FACING_MISMATCH:              NO
CULL_SURVIVAL_MISMATCH:       NO
CULL_WINDING_ANOMALY:         REFUTED_FOR_FOCAL
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-winding-user-arbitration-20260827T193119Z/
ARTIFACT_CHECKSUM_MANIFEST:   f1499333a0fcdb1395f550848e8539026419058f8935c4c8c6b3a9092943667c
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]]. USER capture with
`TOOIE_DIAG_WS_FOCAL_WINDING=1` supplies real V0/V1/V2. Focal TRI1 faces the
camera, source/target winding match, and the compensated RT64 cull expects
survival. Same-DL FRONT TRI2 cluster shares that signature. Do not pursue
cull. No GPU pixel claim.

`NEXT_SINGLE_ACTION:` TARGETED_RASTER_STATE_TRACE on `00349d38` TRI1 (depth/coverage/blend/other-mode/scissor on native vs wide FB submits). No GPU readback. No cull bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_VERTEX_WINDING_DUMP

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
INSTRUMENTATION_STATUS:       IMPLEMENTED_CPU_SIDE_SAFE
GPU_READBACKS_USED:           NO (0 buffer readbacks, 0 image transfers, 0 thread race conditions)
PRIMARY_FOCAL_CANDIDATE:      source_dl = 00349d38 extra-left TRI1
SECONDARY_CONTROL:            source_dl = 0034b800
INTERNAL_CONTROL:             source_dl = 00349d38 central TRI2 triangles
LOG_SINKS:                    /tmp/tooie-ws-focal-winding.log + unified [ws-focal-winding]
ASPECT_SCALE_INVARIANCE_THEOREM: PROVEN (Sx > 0 strictly preserves 2D NDC area sign)
SELFTEST_RESULT:              66/66 checks passed (100% sign preservation across 1:1, 4:3, 16:10, 16:9, 21:9, 32:9)
BUILD_STATUS:                 SUCCESS (ninja exit code 0)
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-vertex-winding-setup-20260827T154601Z/
ARTIFACT_CHECKSUM_MANIFEST:   d29cce81fc78995a14de11979ef1933cbcfa8fb0dbf158cbeb1bddf5cdcacc15
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]]. CPU-side telemetry hooked into `FramebufferRenderer::addDrawCall` extracts real $V_0, V_1, V_2$ object, world, clip, and NDC coordinates, model determinant, signed areas, world normals, camera positions, geometric dot products, and clip masks without any GPU memory readbacks or queue synchronization.

`NEXT_SINGLE_ACTION:` Execute safe focal vertex telemetry capture (or process captured telemetry) to record real $V_0/V_1/V_2$ parameters for `00349d38` extra-left TRI1 vs controls. No GPU readback. No fix. No bypass.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY TARGETED_CULL_WINDING_TRACE

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_SCREENSHOT_CROP_MAPPING: APPROXIMATE
USER_HOLE_NDC_RECT:           APPROXIMATE_VISUAL_GUIDE_ONLY
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
FOCAL_SOURCE_DL:              00349d38 extra-left TRI1 (GBI v0/v1/v2 = 0,1,2)
POSITIVE_X_ASPECT_SCALE_CAN_FLIP_WINDING: NO
SHADER_CULL_IMPLEMENTATION:   specialized PSO CullMode FRONT (not GS/compute)
VIEWPORT_Y_FLIPPED:           NO
NDC_TO_VULKAN_Y_FLIP:         YES (RasterVS -Y and DXC -fvk-invert-y)
FRONT_FACE_COMPENSATION_PRESENT: YES
TRIANGLE_REQUIRES_CLIPPING:   YES (ndcX min -1.3163, ndcY min -1.4054)
CULL_WINDING_ANOMALY:         UNKNOWN (object/world/NDC verts not in capture)
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-targeted-cull-winding-20260827T152926Z/
ARTIFACT_CHECKSUM_MANIFEST:   c260d0fd338f4b14039e65bc70a33601318682f7686ba5c72441cc18e0d2eb56
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]]. Screenshot crop is approximate;
semantic join stays PARTIAL. Aspect expansion is generic `target_aspect /
source_aspect` (positive X scale). Next is a focal vertex/signed-area dump,
not a cull bypass and not GPU readback.

`NEXT_SINGLE_ACTION:` TARGETED_FOCAL_VERTEX_WINDING_DUMP on `00349d38` extra-left TRI1 (central TRI2 control + `0034b800`): object/world/clip/NDC verts and signed areas. No GPU readback. No fix. No bypass.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY CPU_SCREEN_BOUNDS_WALL_CANDIDATE_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
FINAL_RENDER_TARGET_PIXEL_PROBE: NOT_COMPLETED / ABANDONED_FOR_SAFETY
GPU_PIXEL_READBACK_CURRENT_APPROACH: UNSAFE
LIVE_GPU_READBACK:            DO_NOT_RETRY_WITH_CURRENT_APPROACH
WIDE_EXTENT_PRESERVED_THROUGH_PRESENT: PROVEN
VIRENDERER_COMPOSITE_ROOT:    REFUTED as extra-band X crop
POST_RENDER_4_3_CROP_PATH:    NOT_FOUND
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          13309
CAPTURE_END_FRAME:            14028
CAMERA_EFFECTIVELY_STATIC:    YES (720/720)
CAM_POS:                      (3467.4, 105.0, 4735.7)
USER_HOLE_SCREEN_RECT:        x[0.00,0.12] y[0.00,0.65] (top-left origin)
USER_HOLE_NDC_RECT:           x[-1.00,-0.76] y[-0.30,1.00]
WALL_BOUNDS_EVENTS:           41142 in window
TOP_CANDIDATE_SOURCE_DL:      00349d38 extra-left TRI1 (not whole DL)
TOP_CANDIDATE_GPU_SUBMITTED:  YES (318/318 gpu_submitted=1)
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CULL_WINDING_ANOMALY:         UNKNOWN (geom_cull=BACK; winding/det not logged)
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
LAST_IDENTITY_PRESERVING_BOUNDARY: GPU_DRAW_SUBMISSION (this wall cluster)
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-user-wall-bounds-arbitration-20260827T151459Z/
ARTIFACT_CHECKSUM_MANIFEST:   cd50d98a55c0ab24bb1ed3d982ae4b648ba148b3bdc2ed0182f31189dd6018b4
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]].
New USER_SELECTED capture + screenshot join the left hole to MAP_RAM
`00349d38` extra-left TRI1 (nearby vertical opaque, proj=2, depth on). That
draw reaches GPU while the hole is visible. Smoke DLs `0031f6d8` are not this
wall. Distant proj=1 `0031e***` are demoted. Next is cull/winding of this
cluster, not GPU readback.

`NEXT_SINGLE_ACTION:` TARGETED_CULL_WINDING_TRACE on `00349d38` extra-left TRI1 (and sibling `0034b800`): winding, clip-space cross, mtx det, viewport sign; no GPU readback; no fix; no bypass.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY POST_SUBMISSION_COMPOSITE_STATE_TRACE

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
FINAL_RENDER_TARGET_PIXEL_PROBE: NOT_COMPLETED / ABANDONED_FOR_SAFETY
GPU_PIXEL_READBACK_CURRENT_APPROACH: UNSAFE
LIVE_GPU_READBACK:            DO_NOT_RETRY_WITH_CURRENT_APPROACH
CAPTURE_VALID:                YES (prior USER_SELECTED; this round no new user capture)
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          6885
CAPTURE_END_FRAME:            7604
CAMERA_EFFECTIVELY_STATIC:    YES (720/720)
DRAW_TO_GPU_SUBMISSION_FAILURE: REFUTED
USER_EXTRA_BAND_SUBMISSION_JOIN_RATE: 100%
EXTRA_BAND_PROJ1_DRAWS_REACH_GPU_DURING_VISIBLE_GLITCH: PROVEN
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: NOT_PROVEN
MISSING_WALL_DRAW_SEMANTIC_JOIN: NO
COLOR_DEPTH_EXTENT_MATCH:     YES (2164x1140 prior capture)
EXTRA_BAND_DEPTH_TARGET_TRUNCATION: REFUTED
NEAR_PLANE_INTERSECTION_COUNT: 0
FAR_DEPTH_CLIP_ANOMALY:       REFUTED
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: LOW
MSAA_RESOLVE_WIDESCREEN_SAFE: YES
VI_UV_COVERS_WIDE_SOURCE:     PROVEN (smoke uvMax.x=1.0 all 38 presents)
POST_RENDER_4_3_CROP_PATH:    NOT_FOUND
POST_RENDER_SOURCE_CLAMP:     FOUND (shader LowerRight; runtime X does not crop extra-band)
POST_RENDER_PILLARBOX_PATH:   FOUND (window contain-fit / copyFromChanges; not extra-band crop)
WIDE_EXTENT_PRESERVED_THROUGH_PRESENT: PROVEN (smoke); same functions as USER capture
VIRENDERER_COMPOSITE_ROOT:    REFUTED as extra-band X crop; NOT_PROVEN as wall visual root
GPU_FRAGMENT_RASTER_ROOT:     NOT_PROVEN
COMPOSITE_CROP_ROOT:          STRONGLY_WEAKENED
CURRENT_ROOT_CLASS:           POST_GPU_SUBMISSION_FRAGMENT_OR_COMPOSITE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
LAST_IDENTITY_PRESERVING_BOUNDARY: GPU_DRAW_SUBMISSION
DECISION:                     B GPU_FRAGMENT_RASTER_OR_SEMANTIC_BOUNDARY
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-post-submission-composite-audit-20260827T052315Z/
ARTIFACT_CHECKSUM_MANIFEST:   444e33af976f32ba2c4f94409a2210bf80871346281554e9891c66291bb2a2d2
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]].
Pixel probe caused three Vulkan GPU/session incidents and is abandoned. Do not
retry GPU readback. Extra-band proj=1 draws reach GPU during the glitch;
the missing wall mesh is **not** proven to be among them. Static+CPU composite
audit: MSAA resolve is full-extent; VI samples the full wide texture
(`uvMax.x=1`); VI crop/view rects are stubbed `{0,0,1,1}`. Next is semantic
wall-draw localization, not another GPU copy.

`NEXT_SINGLE_ACTION:` SEMANTIC_WALL_DRAW_LOCALIZATION (CPU-side identity of which extra-band proj=1 DLs could be the missing wall; no GPU readback; no fix; no bypass).
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY GPU_DEPTH_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_SELECTED_WIDESCREEN_MISSING_WALL: YES
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          6885
CAPTURE_END_FRAME:            7604
CAMERA_EFFECTIVELY_STATIC:    YES (720/720)
USER_EXTRA_BAND_DRAWS_CLASSIFIED: 56718 (proj1 3288)
USER_EXTRA_BAND_DRAWS_GPU_SUBMITTED: 3288/3288 proj1 by prim identity
USER_EXTRA_BAND_SUBMISSION_JOIN_RATE: 100.0% (prim identity; naive frame+call is desync)
DRAW_TO_GPU_SUBMISSION_FAILURE: REFUTED
GPU_SUBMITTED_EXTRA_BAND_SET_BEHAVIOR: STABLE (Jaccard 1.0)
COLOR_DEPTH_EXTENT_MATCH:     YES (2164x1140)
EXTRA_BAND_DEPTH_TARGET_TRUNCATION: REFUTED_FOR_USER_CAPTURE
NEAR_PLANE_INTERSECTION_COUNT: 0
FAR_PLANE_INTERSECTION_COUNT: 411 (maxNdcZ<=1.0005; depth_clip=0)
FAR_DEPTH_CLIP_ANOMALY:       REFUTED
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: LOW
MISSING_WALL_DRAW_SEMANTIC_JOIN: NO
ARBITRATION_CASE:             C
CURRENT_ROOT_CLASS:           POST_GPU_SUBMISSION_FRAGMENT_OR_COMPOSITE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
LAST_IDENTITY_PRESERVING_BOUNDARY: GPU_DRAW_SUBMISSION
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-gpu-depth-user-arbitration-20260827T033147Z/
ARTIFACT_CHECKSUM_MANIFEST:   6996bd3bcf9974c86d95fc4650f08bc8b061faf407b5fa80ed39288aa94c2eca
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]].
USER_SELECTED capture while the lateral wall was visibly missing and the
camera was fully static. Extra-band proj=1 DrawCalls reach GPU submit. That
does **not** prove the missing wall mesh itself is submitted.
`POST_RT64_DRAW_VISIBILITY_BOUNDARY` is `CORRECTED/SUPERSEDED`. Pixel probe
later abandoned for safety.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY EXTRA_BAND_FRAGMENT_DEPTH_VISIBILITY

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
POST_DRAW_CPU_REJECTION_FOUND: NO
DRAW_SURVIVES_TO_GPU_SUBMISSION: YES (100.0%)
EXTRA_BAND_DRAWS_CLASSIFIED:  29498
EXTRA_BAND_DRAWS_GPU_SUBMITTED: 29502
EXTRA_BAND_DRAW_SUBMISSION_RATE: 100.0%
DEPTH_TEST_STATE_CAPTURE_READY: YES
COLOR_DEPTH_EXTENT_MATCH:     YES
DEPTH_COORDINATE_SPACE:       HOST_WIDESCREEN_PIXELS / NDC
EXTRA_BAND_DEPTH_COVERAGE_MISMATCH: REFUTED
CLIP_STATE_CAPTURE_READY:     YES
BANJO_NEAR_CLIPPING_EFFECT_STAGE: RASTERIZER_DEPTH_CLIP_ENABLE
BANJO_NEAR_CLIPPING_CAN_EXPLAIN_LATERAL_HOLES: NO
FRAGMENT_VISIBILITY_DIRECTLY_OBSERVABLE: NO (proxy active)
PASS_IDENTITY_CAPTURE_READY:  YES
CURRENT_ROOT_CLASS:           POST_RT64_DRAW_VISIBILITY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
USER_CAPTURE_READY:           YES
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-extra-band-depth-visibility-setup-20260827T032333Z/
ARTIFACT_CHECKSUM_MANIFEST:   386063613e37c2b2272957489a0f0b740c05d803b8745503137dc126ab44d712
```

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]].
Artifact: `/tmp/p08-widescreen-extra-band-depth-visibility-setup-20260827T032333Z/`
(manifest `386063613e37c2b2272957489a0f0b740c05d803b8745503137dc126ab44d712`).
Dual verification: PASS. All extra-band draws survive 100% to final GPU draw submission.
Depth extent matches color extent (100%). Depth mismatch is REFUTED.
Next single action: one USER_SELECTED authoritative capture at the visual anomaly.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY RSP_DRAW_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_SELECTED_WIDESCREEN_VISIBILITY_ERROR: YES
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          245023
CAPTURE_END_FRAME:            245742
RSP_PRIMITIVES_TOTAL:         604428
RSP_PRIMITIVES_WITH_DRAW:     604088
RSP_PRIMITIVES_WITHOUT_DRAW:  340
PRIMITIVE_TO_DRAW_JOIN_RATE:  99.9437% (RSP); RECT dual-seq unjoin is instrumentation
RT64_DRAWS_TOTAL:             212560
EXTRA_LEFT_DRAWS:             28831
EXTRA_RIGHT_DRAWS:            21725
ARBITRATION_CASE:             B
RSP_TO_RT64_DRAW_CREATION_FAILURE: REFUTED
RT64_DRAW_PRESENT_DURING_VISUAL_ERROR: PROVEN (structural)
CURRENT_ROOT_CLASS:           POST_RT64_DRAW_VISIBILITY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        after RT64 extra-band DrawCall classification
LAST_IDENTITY_PRESERVING_BOUNDARY: RT64_HOST_DRAW_EXTRA_BAND_CLASSIFICATION
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: MEDIUM
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-rsp-draw-user-arbitration-20260827T030225Z/
ARTIFACT_CHECKSUM_MANIFEST:   2084f416c77a64082a459bf58244cc49a715972efefb454d131a441c13b914ed
```

USER_SELECTED capture while missing wall polygons were visible. Camera static
for 712/720 frames. Every proj=1 world frame (142) has extra-left and
extra-right classified draws; the proj=1 extra-band DL set is STABLE
(Jaccard 1.0). Draw creation is not the hole. Root is after DrawCall
(depth/clip/coverage). No semantic wall→DL join.

`NEXT_SINGLE_ACTION:` EXTRA_BAND_FRAGMENT_DEPTH_VISIBILITY (diagnostic RT64 fragment/depth counters on the stable proj=1 extra-band DL set). No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY RSP_PRIMITIVE_IDENTITY_PROPAGATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
RSP_PRIMITIVE_PROPAGATION:    IMPLEMENTED_AND_VERIFIED
DL_CONTEXT_STACK_ACTIVE:      YES
DL_CONTEXT_EVENTS:            8597
PRIMITIVES_WITH_DL_IDENTITY:  76229
RT64_DRAWS_WITH_PRIM_ID:      45506
JOIN_RATE_DL_TO_PRIMITIVE:    100.0%
JOIN_RATE_PRIMITIVE_TO_DRAW:  100.0%
CURRENT_ROOT_CLASS:           RSP_TO_RT64_GEOMETRY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        RSP_G_DL_TO_RT64_DRAW
LAST_IDENTITY_PRESERVING_BOUNDARY: RT64_HOST_DRAW_EXTRA_BAND_CLASSIFICATION
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-rsp-primitive-identity-setup-20260827T015059Z/
ARTIFACT_CHECKSUM_MANIFEST:   bb7ea94f1a8f71cb101c0cc4cb38832ccd5aa219240f2526669537eadcdd88e0
FIX_IMPLEMENTED:              NO
```

End-to-end identity chain from nested guest `G_DL` -> RSP HLE DL context stack ->
primitive sequence generation (`TRI1`, `TRI2`, `QUAD`, `RECT`) -> `DrawCall` batching
-> `FramebufferRenderer` host widescreen NDC extra-band classification (`minNdcX`,
`maxNdcX`, `extra_left_intersect`, `extra_right_intersect`) is verified and active.
Single unified log `/tmp/tooie-ws-dl-identity.log`. Smoke test confirmed 100% join rate.
Artifacts dual-verified in `/tmp/p08-widescreen-rsp-primitive-identity-setup-20260827T015059Z/`.

`NEXT_SINGLE_ACTION:` USER_SELECTED_AUTHORITATIVE_CAPTURE (user plays to visual anomaly with `TOOIE_DIAG_WS_CONTINUOUS=1` and triggers `touch /tmp/tooie-ws-capture`).
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY POST_GCMAPSECTS_REANCHOR

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_OBSERVED_WIDESCREEN_GLITCH_IN_RUN: YES
OVERLAY662_TOTAL_SYSCALLS:    39
EP4_COUNT:                    27
EP15_COUNT:                   12
EP9_COUNT:                    0
EP13_COUNT:                   0
EP17_COUNT:                   0
GCMAPSECTS_ENTRY_COUNT:       0
GCMAPSECTS_80801174_ACTIVE_FOR_OBSERVED_RUN: NO
GCMAPSECTS_80801174_REQUIRED_FOR_OBSERVED_SYMPTOM: REFUTED_FOR_THIS_RUN
CAA24_GCMAPSECTS_PATH_REQUIRED_FOR_OBSERVED_SYMPTOM: REFUTED_FOR_THIS_RUN
EP4_FUNCTION:                 gcmapsects_entrypoint_4
EP4_ROLE:                     point-in-section AABB query (index or -1)
EP4_CAN_SKIP_GEOMETRY:        NO
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
CURRENT_ROOT_CLASS:           RSP_TO_RT64_GEOMETRY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        RSP_G_DL_TO_RT64_DRAW
LAST_IDENTITY_PRESERVING_BOUNDARY: RSP_HLE_G_DL_VISIT
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: MEDIUM
FIX_IMPLEMENTED:              NO
```

Manual gameplay: missing wall polygons while `func_80801174` never ran.
ep4 is occupancy query (`func_800EFED0` min<point<max vs section AABB);
a0 burst stride 0x9C = Actor.position. ep15 inits/clears `D_801285B0` including
`+0x80`. Static world DLs come from map payload + gccubeDll/gsprops/geo +
GraphicsTask, not 80801174. Artifact
`/tmp/p08-widescreen-post-gcmapsects-reanchor-20260827T012256Z/`
(manifest `3c626fd6b1b1d44520550066ef7c34d6d0c2077bb26b0b8b29325623b33e0e37`).

`NEXT_SINGLE_ACTION:` RSP_PRIMITIVE_IDENTITY_PROPAGATION (nested G_DL → primitives → RT64 extra-band class). No bypass. No fix. No new user capture this round.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY FOCAL_BRANCH_JOIN_REPAIR

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
CAPTURE_VALID:                YES (id=1 frames 1717..2436; no new user capture)
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
FOCAL_CLUSTER_TRANSITION:     MIXED_ASYMMETRIC_NEIGHBOR_DROPOUT
SECTION_BLOCK_ACTIVATION:     NOT_SUPPORTED
FOCAL_BRANCH_EVENTS_IN_WINDOW: 0
OLD_RA_GATE_ROOT_CAUSE:       $ra is jal PC+8 (0x808011CC / 0x808011F4), not 0x80801174; LOOKUP does not materialize r31; helper used 800E3E8C SP; add_loaded_function(0x80801174) misses CreateStatic; dispatch table address-takes EPs so --wrap(ep9/17) never sees syscalls
SPHERE_CALL_PC_IN_80801174:   0x808011C4
SPHERE_EXPECTED_RA:           0x808011CC
CAA24_PATH_CALL_PC_IN_80801174: 0x808011EC
CAA24_EXPECTED_RA:            0x808011F4
80801174_ADDRESS_SEMANTICS:   linked overlay VRAM (not stable LOOKUP key)
DIRECT_GCMAPSECTS_HOOK:       YES
HOOK_MECHANISM:               ld --wrap 80801174/80801640 + unlimited ovl=662 syscall log + LOOKUP rebind after load_overlays
SECTION_POINTER_SOURCE:       ctx->r4
SECTION_INDEX_SOURCE:         ctx->r5
SECTION_ENTRY_SOURCE:         *(a0+0x68)+index*0x58
GC_MAPSECTS_ENTRY_COUNT:      0
SPHERE_EVENTS_IN_GCMAPSECTS:  0
CAA24_EVENTS_IN_GCMAPSECTS:   0
CALL_ID_JOIN_ERRORS:          0
UNKNOWN_GUEST_FRAME_EVENTS:   0
GC_MAPSECTS_HOOK_VERIFIED:    NO
BUILD_RESULT:                 PASS
FIX_IMPLEMENTED:              NO
CURRENT_ROOT_CLASS:           GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    OBSERVABILITY_REPAIR
CURRENT_ROOT_BOUNDARY:        func_80801174_gcmapsects
USER_CAPTURE_READY:           NO
```

45s attract smoke logged one `[ws-gcmapsects-syscall] ep=15 a0=801285b0
text_start=80198ff0` and never ep 9/13/17. `80801640` only calls `80801174`
when `section+0x80 != 0`; ep 15 clears that byte. First-128 stderr dispatch
log is not a 662 histogram. Do not bypass. Do not treat stack `[ws-geo-e]`
nodes `0x800455b0`/`0x80045618` as 80801174. Artifact
`/tmp/p08-widescreen-focal-branch-join-repair-20260827T004617Z/`
(manifest `2c84d1739211aff278e4e469f3af40e816640cc2f0304ff6493d62c9d1f19cfc`).

`NEXT_SINGLE_ACTION:` Get `[ws-gcmapsects-entry] > 0` on the glitch map (attract Wooded/TDL is proven not to tick ep 9/17). Then one USER_SELECTED capture. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior (2026-08-27) — join-repair first wrap install

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
CAPTURE_VALID:                YES (id=1 frames 1717..2436; no new user capture)
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
FOCAL_CLUSTER_TRANSITION:     MIXED_ASYMMETRIC_NEIGHBOR_DROPOUT
SECTION_BLOCK_ACTIVATION:     NOT_SUPPORTED
FOCAL_BRANCH_EVENTS_IN_WINDOW: 0
OLD_RA_GATE_ROOT_CAUSE:       $ra is jal PC+8 (0x808011CC / 0x808011F4), not 0x80801174; LOOKUP does not materialize r31; helper used 800E3E8C SP; add_loaded_function(0x80801174) misses CreateStatic same-section C calls and runtime ram+offset keys
SPHERE_CALL_PC_IN_80801174:   0x808011C4
SPHERE_EXPECTED_RA:           0x808011CC
CAA24_PATH_CALL_PC_IN_80801174: 0x808011EC
CAA24_EXPECTED_RA:            0x808011F4
80801174_ADDRESS_SEMANTICS:   linked overlay VRAM (not stable LOOKUP key)
DIRECT_GCMAPSECTS_HOOK:       YES
HOOK_MECHANISM:               ld --wrap func_80801174_gcmapsects (+ 80801640 / ep 9 / ep 17)
SECTION_POINTER_SOURCE:       ctx->r4
SECTION_INDEX_SOURCE:         ctx->r5
SECTION_ENTRY_SOURCE:         *(a0+0x68)+index*0x58
GC_MAPSECTS_ENTRY_COUNT:      0
SPHERE_EVENTS_IN_GCMAPSECTS:  0
CAA24_EVENTS_IN_GCMAPSECTS:   0
CALL_ID_JOIN_ERRORS:          0
UNKNOWN_GUEST_FRAME_EVENTS:   0
GC_MAPSECTS_HOOK_VERIFIED:    NO
BUILD_RESULT:                 PASS
FIX_IMPLEMENTED:              NO
CURRENT_ROOT_CLASS:           GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    OBSERVABILITY_REPAIR
CURRENT_ROOT_BOUNDARY:        func_80801174_gcmapsects
USER_CAPTURE_READY:           NO
```

Attract/auto-start smoke reached Wooded Hollow 335 and TDL 278 without calling
ep 9/17 / `80801640` / `80801174`. ovl=662 loaded; only logged syscall was
ep=15 (init). Link-level wrap is proven (objdump 80801640 → `__wrap_`). Do not
bypass. Do not treat stack `[ws-geo-e]` nodes `0x800455b0`/`0x80045618` as
80801174. Artifact
`/tmp/p08-widescreen-focal-branch-join-repair-20260827T002831Z/`
(manifest `0cd05b6bbac330111b9bdd4a8fa86630b0b6dee02f968d9b568722723450b4d1`).

`NEXT_SINGLE_ACTION:` Run until `[ws-gcmapsects-entry]` count > 0 on a map that actually ticks `gcmapsects` ep 9/17 (the glitch map, not intro-only attract), then one USER_SELECTED capture. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior (2026-08-26) — WIDESCREEN_EDGE_GEOMETRY FOCAL_BRANCH_TIMELINE user arbitration

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
WORKTREE:                     DIRTY (preserve; diagnostic wraps + docs)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_OBSERVED_LATE_POP_IN:               YES
USER_OBSERVED_EARLY_POP_OUT:             YES
BIDIRECTIONAL_EDGE_VISIBILITY_ERROR:     USER_OBSERVED
USER_MANUAL_CAB70_BYPASS_VISUAL_RESULT:  NO_CHANGE
USER_MANUAL_E3DC0_BYPASS_VISUAL_RESULT:  NO_CHANGE
USER_MANUAL_CACEC_BYPASS_VISUAL_RESULT:  NO_CHANGE
CAB70_DIRECT_VISUAL_ROOT:                REFUTED_FOR_OBSERVED_SYMPTOM
E3DC0_DIRECT_VISUAL_ROOT:                REFUTED_FOR_OBSERVED_SYMPTOM
CACEC_DIRECT_VISUAL_ROOT:                REFUTED_FOR_OBSERVED_SYMPTOM
SPHERE_CULLING_IS_VISUAL_ROOT:           REFUTED_FOR_OBSERVED_SYMPTOM
BBOX_VISUAL_ROOT:                        REFUTED_FOR_OBSERVED_SYMPTOM
FRUSTUM_ASPECT_MISMATCH_FOR_CAPTURED_EVENT: REFUTED
USER_TRIGGER_FOUND:                      YES
CAPTURE_COMPLETE_FOUND:                  YES
CAPTURE_VALID:                           YES
CAPTURE_ID:                              1
CAPTURE_START_FRAME:                     1717
CAPTURE_END_FRAME:                       2436
FOCAL_BRANCH_LOG_PRESENT:                YES (0 bytes; truncate-only)
FOCAL_BRANCH_EVENTS_IN_WINDOW:           0
FOCAL_SECTION_IDENTITY:                  UNKNOWN
FOCAL_SECTION_IDENTITY_STABLE:           UNKNOWN
FOCAL_DL_PHYSICAL:                       0x0035dd20
FOCAL_DL_KSEG0:                          0x8035dd20
FOCAL_DL_STORAGE_CLASS:                  LOADED_MAP_MODEL_PAYLOAD
FOCAL_DL_STATIC_OR_DYNAMIC:              DYNAMIC_RELOCATED
FOCAL_DL_SEMANTIC_CLASS:                 LEAF_DISPLAY_LIST / MAP_MODEL_PAYLOAD
FOCAL_DL_MAP_TERRAIN_BIND:               UNKNOWN
FOCAL_DL_FIRST_SEEN_FRAME:               1720
FOCAL_DL_LAST_SEEN_FRAME:                2432
FOCAL_DL_REAPPEARS:                      NO
LAST_SEEN_IS_CAPTURE_TAIL:               YES
FOCAL_DL_FROM_GCMAPSECTS:                UNKNOWN (pointer) / YES (class selector)
FOCAL_DL_FROM_GCCUBEDLL:                 NO
FOCAL_DL_FROM_GCSECTIONDLL:              NO
FOCAL_DL_REACHES_800CAA24:               YES (static section path)
FOCAL_DL_LOD_CONTROLLED:                 NO
DYNAMIC_BRANCH_JOIN:                     NO
CAA24_CONTROLS_FOCAL_MEMBERSHIP:         UNKNOWN
SPHERE_CONTROLS_FOCAL_MEMBERSHIP:        UNKNOWN
POST_WALK_SELECTION_INVOLVED:            UNKNOWN
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR:      WEAKENED
FOCAL_CLUSTER_TRANSITION:                MIXED_ASYMMETRIC_NEIGHBOR_DROPOUT
SECTION_BLOCK_ACTIVATION:                NOT_SUPPORTED
GUEST_DL_TO_RSP_CONTINUITY:              PROVEN
LAST_IDENTITY_PRESERVING_BOUNDARY:       RSP_HLE_G_DL_VISIT
IDENTITY_LOST_AT:                        RSP_G_DL_TO_RT64_DRAW
DL_TO_RT64_DRAW_JOIN:                    NO
CURRENT_ROOT_CLASS:                      GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:               STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:                   func_80801174_gcmapsects (800E3E8C ∧ 800E3D0C/CAA24) for nested leaf 0x8035dd20
PRIOR_ROOT_CLASS:                        GUEST_DISPLAY_LIST_SELECTION_OR_EMISSION_BOUNDARY (REFINED, not REFUTED)
NEXT_DISCRIMINATOR:                      FOCAL_BRANCH_JOIN_REPAIR
FIX_IMPLEMENTED:                         NO
TEMP_DIAGNOSTIC_CHANGE_THIS_ROUND:       NO
```

New USER_SELECTED window `capture_id=1` frames **1717..2436** is VALID (720/720,
`[ws-capture-complete]` present). Do not mix with prior VALID window 1425..2144
or the smoke start=245. Focal nested leaf `0x8035dd20` is **already present on
all 179 graphics-emit frames 1720..2432** (cadence every 4 VI frames). Neighbor
`0x8035d268` drops at emit 1856; `0x8035e2d8` persists with focal. Signature D
matches this window: no ABSENT→PRESENT onset, so CAA24/sphere membership control
is **UNKNOWN**. `[ws-focal-branch]` count is **0** even while the DL is live;
helper wrappers still logged `[ws-geo-e]`/`[ws-geo-d]` on stack nodes. Setup
artifact `/tmp/p08-widescreen-focal-branch-timeline-setup-20260826T164317Z/` is
**missing from /tmp**. Do not bypass CAA24. Do not reopen CAB70/E3DC0/CACEC.

Canonical: [[plan/evidence/p08-widescreen-edge-geometry]].
Latest Branch Arbitration: `/tmp/p08-widescreen-focal-branch-user-arbitration-20260826T233105Z/`
(manifest `4b1867cb2145af8dc5c33b6916943ce335edf6295a886eaef9d37bddf87a7a65`).
Latest Producer Artifact: `/tmp/p08-widescreen-dl-producer-trace-20260826T150351Z/`
(manifest `40d591c62899a56b60f73f4e990f019b1b1890b22b4e4d6a33b41b6eba48f5ee`).
Latest DL-identity Arbitration: `/tmp/p08-widescreen-dl-framejoined-user-arbitration-20260826T143436Z/`
(manifest `f0f9b493cf4bdac6ebe2378f8ce5b04ce09f8f4eab34058cd58ff7fed6d80f54`).

`NEXT_SINGLE_ACTION:` Repair observation of `func_80801174_gcmapsects` (guest-PC-safe helper RA or overlay-safe hook) so `[ws-focal-branch]` emits on a frame where `0x8035dd20` is already guest-emitted; then recapture at pop-in. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior (2026-08-26) — producer of 0x8035dd20 is map-section selection

Prior VALID USER_SELECTED window was `capture_id=1` frames **1425..2144**,
focal first_seen **1432**, last_seen **2140**. Static producer machinery remains
`gcmapsects` `func_80801174` (sphere `800E3E8C` ∧ AABB `800E3D0C`→`func_800CAA24`).
Visual root is still **NOT PROVEN**. The later 1717..2436 window did not
re-observe that onset.

## Prior (2026-08-25) — WIDESCREEN_EDGE_GEOMETRY class C — CORRECTED/SUPERSEDED as visual root

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
WORKTREE_AT_INVESTIGATION:    CLEAN
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
CURRENT_ROOT_CLASS:           C — CPU_FRUSTUM_VISIBILITY_CULLING_BOUNDARY
CURRENT_ROOT_BOUNDARY:        func_800CAB70
VISUAL_FIX_STATUS:            USER_VALIDATION_PENDING
FIX_IMPLEMENTED:              NO
COMMIT_PUSH:                  NO
```

The prior label `WIDESCREEN_EDGE_GEOMETRY: POST_P08_ENHANCEMENT` is
**CORRECTED/SUPERSEDED**. The earlier `D — HOST_RT64_WIDESCREEN_ADAPTATION_BOUNDARY`
header class is **CORRECTED/SUPERSEDED** as a *previous* current-root claim.
CPU `func_800CAB70` remains **`PROVEN`** as a guest 4:3 frustum *rejection*
boundary (telemetry). It is **`REFUTED_FOR_OBSERVED_SYMPTOM`** as the cause of
the user-visible lateral polygon glitch: manual
`TOOIE_DIAG_CAB70_BYPASS=1` and `TOOIE_DIAG_E3DC0_BYPASS=1` both
`NO_CHANGE`.

Controlled counterfactual experiment telemetry remains valid:
`/tmp/p08-widescreen-cull-counterfactual-20260825T225500Z/`
(manifest `c356a78d52344b43b440f07df4cdb951be6b672c16f485fcc382a9e6990f9057`).

- `CAB70_CAUSES_GUEST_4_3_FRUSTUM_REJECTION:` **`PROVEN`**
- `CAB70_BYPASS_RESTORES_VISIBILITY_PATH:` **`PROVEN`** (telemetry / dispatch)
- `E3DC0_INHERITS_CAB70:` **`PROVEN`**
- `CAB70_CAUSES_OBSERVED_EDGE_POLYGON_GLITCH:` **`REFUTED_FOR_OBSERVED_SYMPTOM`**
- `LATERAL_VISUAL_DEFECT_WITH_CAB70_BYPASS:` **`PRESENT / NO_CHANGE`**
  (prior `RESOLVED` is **CORRECTED/SUPERSEDED**)
- `VISUAL_GLITCH_RESOLVED:` **REFUTED** as a claim; user visual `NO_CHANGE`
- Draft `ar_option != Original` enable condition is **CORRECTED/SUPERSEDED**.
- `FIX_IMPLEMENTED:` `NO`

Do not open P09. Do not mix with `AUDIO_STARTUP_TICK` or `CHARACTER_EDGE_GRAPHICS_GLITCH`.

## Prioridade atual

**PHASE P08 IS ACTIVE / MANUAL_VALIDATION_PENDING; PHASE P09 IS PENDING / NOT_FORMALLY_OPEN (2026-08-21):**
- **Physical Display Validation**: **PASS** — User explicitly confirmed all 14 mandatory display/window integration tests passed (`PHYSICAL_DISPLAY_RESULT: PASS`, `FORMAL_DISPLAY_CHECKLIST_PROVEN: YES`).
- **Physical Gamepad Status**: **PENDING_HARDWARE_UNAVAILABLE** — Formal 15/15 gamepad checklist cannot be executed because controller hardware is temporarily unavailable. `USER_CONFIRMED_CONTROLLER_GAMEPLAY: YES` retained.
- **Open P08 Gates**: **PHYSICAL_GAMEPAD_VALIDATION** plus the bounded
  `0x800979D8` held-input/downstream dynamic acceptance gate.
- **Player & AI Movement Speed Parity Baseline (2026-08-21)**: [[plan/evidence/p08-provisional-player-ai-speed-parity]]. `PLAYER_SCALAR_SPEED_PARITY: PASS`, `AI_ACTOR_SPEED_PARITY: PASS` **within the measured windows only**; `AI_GLOBAL_PARITY: NOT_PROVEN`. Speed magnitude matches within sub-pixel floating point bounds across straight walk (`510.2442 u/s`), trot run (`673.5218 u/s`), jump (`690.0 u/s`), and sampled AI actors (`chdiggerground` path, `subaddie` patrol). USER_DEMO_4 speed magnitude matches (`0.0017..2.87 u/s`), supporting `CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY` for that bounded sample. Historical downhill ~510 speed is `PROVEN_CLEAN` against original N64 hardware.
- **Attract numbering / egg discriminator (2026-08-21)**: user demos are **continuous across intro blocks**, not zero-based Demo 0..3. Canonical evidence: [[plan/evidence/p08-provisional-follow-camera-and-egg-differential]].
  - `USER_DEMO_4` = first gameplay after the first return to intro/music = map **235 Haunted Cavern** (`stage=0 sub=4`), **not** "Demo 3".
  - `EGG_PARITY_DEFECT: PROVEN` — Angrylion fires **FIRE** (`cheggfire` 535); RT64 fires **BLUE** (`cheggnormal` 539). User confirmed Angrylion RDP Plus nightly-build visual PASS for fire egg and for the previously-broken demo behavior.
  - Follow-camera on USER_DEMO_4 is **REPRODUCED**: pad 877/877 match. Bind-tick yaw sincos is now **POST-FIX MATCH** vs ANGRYLION_CONTROL (`006d5328` / `bf4b29cc` / `sp+0x48`=`node+0x74`=`c43efa3b,43c49665,c4a6b798`). Idle CA8B4/CA628 glidx 1..38 is now **bit-identical**. First remaining bit difference is dump glidx 39 `ncbapivot_entrypoint_3` output (player pos/yaw/ideal MATCH). Demo-sync still prints 0.0853° @41 → 12.0481° @50 → 112.6789° @100. Canonical: [[plan/evidence/p08-user-demo4-post-round-full-state-discriminator]].
  - `EGG_PARITY_DEFECT` remains **PROVEN**: Angrylion **FIRE** (`cheggfire` 535); RT64 **BLUE** (`cheggnormal` 539). Independent of ROUND.W.S: **YES**. Independent of remaining camera/demo split: **UNKNOWN**. No egg fix.
  - b042865 `HAUNTED_CAVERN_NO_FIRE_EVENT` is **REFUTED**. `GLOBAL_VISUAL_DRIFT_STATUS=NOT_REPRODUCED_IN_ATTRACT_SET` is **CORRECTED/SUPERSEDED** for USER_DEMO_4. Demo 0..3 remains an incomplete attract slice.
- **ROUND.W.S checkpoint (2026-08-21)**: `5854872c79a7cab1c9395ddcd173cece449ffe11` (`fix: preserve MIPS round.w.s semantics`). `ROUND_W_S_ROOT_CAUSE: PROVEN`. `ORIGINAL_BIND_TICK_DISCRIMINATOR: RESOLVED`. `ROUND_W_S_BIND_AND_IDLE_CAMERA_EFFECT: FIXED`. `CAMERA_COMPLETE_FIX: NO`. `CAMERA_FEEDBACK_ACCUMULATION: STILL_PRESENT`. `ROUND_W_S_SPECIAL_VALUE_CONTRACT: PARTIAL`. Evidence: [[plan/evidence/p08-user-demo4-sincos-semantics-fix]].
- **Agent workspace (2026-08-21)**: open CLI/IDE in **TooieRecomp/**. MCP configs live here and bind markdown-vault to `docs/` ([[MARKDOWN_VAULT_TOOLS]]). Do not start agents at `N64-Recomp/`. `BanjoRecomp` and the cart ROM remain siblings for CMake/oracle only.
- **USER_DEMO_4 `0x800979D8` bainput fix & dynamic validation (2026-08-22)**: [[plan/evidence/p08-user-demo4-800979d8-postfix-validation]], [[plan/evidence/p08-user-demo4-800979d8-independent-review]], [[plan/evidence/p08-user-demo4-bainput-held-state-provenance]].
  - Root fix: `0x800979D8` (`tooie_bainput_func_800979D8`) implemented in `src/game/boot_hw_stubs.cpp`. Unit tests in `tests/bcf28_helpers_test.cpp` PASS.
  - Held-state provenance & dynamic validation: raw pad `(-48,-46,0001)` **MATCH**. AN and RT `prev_state[13]` are both `0`. Both execute `bakey_held` -> `0x800BCE84` (held=1), both return `v0=2` (`1 << input_id` from enabled mask), both execute the camera rotate-right chain `80097B94 -> 80110EFC -> 80110E00 -> 801111D0 -> 80112D90`, write `p60.C=1, p60.E=2`, set `p60+8=5` at `80112D08`, and take the matching `ncbapivot` transition branch.
  - Camera parity: `CAMCTRL_DELTA_41 = 0.0000°`, `CAMCTRL_DELTA_50 = 0.0000°`, `CAMCTRL_DELTA_100 = 0.0000°`. Accumulation drift is **RESOLVED**.
  - Egg parity: `USER_DEMO_4` fires `cheggfire` (535 FIRE) matching Angrylion Oracle (`cheggfire` 535).
  - Dynamic acceptance: instruction-level `0x800BCE84` live-input join is now
    **PROVEN** ([[plan/evidence/p08-user-demo4-bce84-live-input-join]]). The old
    RT `bs_pressed=0` probe read ring 0 (`obj+0x58`), not `unk8[13]` ring 1
    (`obj+0x90`, word `0x115`, LSB 1). Immutable recapture
    `/tmp/p08-979d8-final-20260822T070735Z/` (`SHA256SUMS` PASS twice).
    Worktree dirty; P08 **ACTIVE / MANUAL_VALIDATION_PENDING**; P09
    **NOT_FORMALLY_OPEN**; no commit/push.

**ATTRACT DEMO2 CRASH RESOLVED (2026-08-16):** deterministic attract-loop SIGSEGV
after DEMO2 is RESOLVED and VALIDATED. `INTRO → Wooded Hollow 335 → TDL 278 → CCL 314 → CS Intro 395 → INTRO → Haunted Cavern 235 → MAP_296`
succeed cleanly (historical soak labeled 235 as Mayahem; that map-name is **CORRECTED/SUPERSEDED** to WW Haunted Cavern). `bsrest_entrypoint_11` calls `static_344_808003FC` directly without
LOOKUP_FUNC collision. Level A fix validated across unit tests, static contract, and 300s soak.
See [[plan/evidence/p08-attract-loop-after-demo2-crash]].

**P08 ACTIVE INTEGRATION STATE:** PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The
`func_80013A7C` lookup-zero is **RESOLVED**
([[plan/evidence/p08-core1-ra-wrapper-fix]]). **Visible attract / gameplay
demo is reproduced** (mode-5, map 335 Wooded Hollow). The **Tooie render-submission
matrix fix is RESOLVED and VALIDATED** ([[plan/evidence/p08-render-submission-matrix-fix]]):
`func_800E44FC`, `func_800E4640`, and `func_800E4628` native overrides split
the projection matrix submission into `LOAD P` followed by `MUL V_view`.
**Title Screen / File Selection Input (P08-T10)** is **VALIDATED and COMPLETE**
for frontend input ([[plan/evidence/p08-title-file-select-validation]]):
intro -> file select -> slot navigation -> `CONT_A` confirm is PASS.
Post-confirm destination is `MAP_A1_CS_INTRO` (161) / `mode=3`. The
"direct Spiral Mountain gameplay" claim is **CORRECTED/SUPERSEDED**.
The later synthetic `mode=1` FAIL is **REFUTED** (sequencer never injects
the START presses that skip the opening story cutscenes).
**Original 4:3 / Timing Baseline (P08-T03)** is **VALIDATED and COMPLETE**
([[plan/evidence/p08-original-timing-4x3-baseline]]): 60.0 Hz guest VI, 20/30 Hz logic,
30 Hz animation, host decouple (~935 Hz), and 4:3 content aspect confirmed under clean baseline config.
**Gameplay Demo Motion Synchronization & Camera Clearance Ray Differential (2026-08-17, MEASURED_ORIGINAL)**:
- Full Wooded Hollow attract demo motion investigated under synchronized original-vs-recomp instrumentation ([[plan/evidence/p08-demo-camera-eye-producer-differential]], [[plan/evidence/p08-demo-ncbapivot-branch-predicate]], [[plan/evidence/p08-demo-camera-clearance-rays]]).
- **CORRECTED/SUPERSEDED (independent Sol review, 2026-08-18)**:
  - In `func_80113D30`, all four query inputs (`start`, `direction`, `destination`) are **NUMERICALLY EQUIVALENT** between Original and Recomp at glidx 384 (position Δ < 0.05, direction IEEE-754 bit patterns match to within 1 LSB).
  - In Original N64, `func_800E8918` returns `0` for all four selected records, so all four counters `word_3C[0..3]` exceed 4, yielding `half_0C = 4`; this return is projection state, not a collision result.
  - In Recomp, `func_800E8918` returns nonzero projection state for **Q1** and **Q2**, resetting their counters to `0` and resulting in `half_0C = 1`.
  - First differing directional sample: **Q1** (camera-local RIGHT / +X).
  - `func_800C4C34` is a camera projection/viewport helper, not a collision query. `func_800E8918` returns `record+0x34`, not its direct `v0`.
  - Causal chain to missing 3rd `SET74`: **PROVEN**. Collision attribution: **REFUTED**, root level `R` (NO PRODUCTION FIX).

**PROJECTION RECORD +0x34 — B-STATE (2026-08-20, canonical):** correctly
joined Q1 `+0x34` is **real** (Original 0 vs Recomp 1, 25/25). Explicit
classifier args `a0/a1/a2/a3` match. First difference is hidden
`func_8001A0C0(176,161)` `v0` (Original 0 vs Recomp nonzero) inside
`func_800E81E0` ([[plan/evidence/p08-demo-projection-classifier-rejoin]]).
The 2026-08-19 **B-INPUT (`a3=v1[2]`)** wording is **CORRECTED/SUPERSEDED**.
No production fix.

**PROJECTION v1[2] DIRECTION PRODUCER — JOIN CORRECTED (2026-08-20):**
aligned Q1 `a3` is bit-identical; `0x4313D3A7` vs `0x43136AD5` are consecutive
stream values on **both** runtimes
([[plan/evidence/p08-demo-projection-v1-direction-producer]]).

**func_8001A0C0 STATE & Z-BUFFER DEPTH WRITER — B-WRITER PROVEN (2026-08-20):**
[[plan/evidence/p08-demo-1a0c0-state-differential]],
[[plan/evidence/p08-demo-db-8007bcc4-lifecycle]],
[[plan/evidence/p08-demo-db-target-writer]]. `D_8007BCC4` (base `0x80137840`, size `0x21D80`)
is the **game's 304×228 16-bit N64 Z-Buffer (Depth Buffer)** (`304 * 228 * 2 = 138,624 = 0x21D80`).
`func_8001A0C0` is a pure leaf depth sampler returning `*(u16*)(D_8007BCC4 + 608*a1 + 2*a0)`.
The Recomp post-boot writer to `0x8014F800` (screen pixel $X=176, Y=161$) is
`RT64::Framebuffer::copyNativeToRAM` -> `RT64::NativeTarget::copyToRAM` (`memcpy`),
performing GPU depth buffer writeback for display list commands `gDPSetDepthImage` / `gDPSetColorImage`
emitted by `func_800E7BC8` / `func_800E7C28`. Written values: `0xFFFE` (clear depth) then
`0x83A4..0x8BFC` (geometry depth). Original Mupen64plus pure-interpreter control (Rice Video HLE)
omitted depth writeback to RDRAM, leaving `0x0000`. Class = **B-WRITER**. NO production fix.

Historical context:
> **PATCH 4 (2026-08-13):** Tooie-scoped startup ON;
> `ULTRAMODERN_GUEST_PREEMPTION=0` rollback. Regen real (4298 safepoints).
> ON: C2AB8 retorna, áudio avança, crash em `func_800BCF28`.
> OFF: freeze antigo (d/audio/ovl estáveis ≥58s). Sem yield/clear.
> Evidência: [[plan/evidence/p08-preemption-patch4-validation]].

Handoff técnico: [[plan/evidence/p08-session-handoff]].

## P08 forced-facing independent review (2026-08-16)

The earlier Level B diagnosis of the Wooded Hollow `chdiggerground` return
actor is **CORRECTED/SUPERSEDED** as a proven recompilation defect. Original
MIPS case 11 matches generated instruction-for-instruction: it advances the
return position while preserving yaw/`unk54`. The 180° observation is
therefore original actor/state semantics; no fix was implemented.

See [[plan/evidence/p08-forced-cinematic-facing-independent-review]].
`USER_BUG_B_THIS_ACTOR=REFUTED`; `USER_BUG_B_GLOBAL=OPEN`; `ROOT_CAUSE_LEVEL=R`.

## P08 forced-facing node discriminator (2026-08-16, DeepSeek V4 Flash Free)

The selected-call discriminator is now RESOLVED via a this-run dynamic
capture of `func_80802760_sucoaster` (strong-symbol override, env-gated,
reverted before checkpoint):

```text
FUNC_80802760_RETURNS:  0 (node index 0) on all 3438 calls
NODE_CASE:              B_LIVE_NODE — A_KEEP_OLD (-1) REFUTED (zero)
RETURN_LEG_HANDLER:     state 0x98bb1942 -> dispatch case 11 (u72=0x3008,
                        L_80800A3C) -> moves digger back WITHOUT writing
                        unk54/rotation.y
ORIGINAL_CASE11_WRITES_YAW: NO
ORIGINAL_CASE11_WRITES_UNK54: NO
GENERATED_CASE11_MATCHES_ORIGINAL: YES
ORIGINAL_REVERSE_WALKS_BACKWARD: YES
ORIGINAL_RETURN_STATE_DISPATCH_MATCHES_RECOMP: YES
DEEPSEEK_GENERATED_MATCH_CLAIM: CORRECTED/SUPERSEDED
FUNC_80802760_GENERATED_MATCH_ORIGINAL: UNKNOWN (not compared here)
FIRST_OBSERVED_MECHANISM: case-11 return movement with preserved yaw/unk54
ORIGINAL_MIPS_INTENT:   CASE11 PROVEN; func_80802760 equivalence UNKNOWN
FIX_IMPLEMENTED:        NO
MARKDOWN_VAULT_AUDIT:   PASS_WITH_PREEXISTING_WARNINGS (errors=0; 50 warnings)
```

## P08 unnamed same-section JAL fix — independent acceptance (2026-08-16)

The GPT-5.6 Luna X-HIGH review accepts the N64Recomp fix with a non-blocking
runtime-environment caveat. Implementation history above is preserved; the only
code change made during review was hardening the static regression test.

```text
BASE_HEAD:                                  1f2aba14665c064ce5af829955f1ac089792de77
PATCH_BASE_CORRECT:                         YES
PATCH_HASH_CORRECT:                         YES
UNNAMED_SAME_SECTION_CREATESTATIC:          PASS
CROSS_SECTION_POLICY_PRESERVED:             YES
NON_RELOCATABLE_POLICY_PRESERVED:           YES
HOST_HOOK_POLICY_PRESERVED:                 YES
CREATESTATIC_BOUNDARIES:                    7/7 PASS
BSREST_808003FC_VERDICT:                    A
STATIC_HELPER_SPOTCHECK_1:                  8080020C PASS
STATIC_HELPER_SPOTCHECK_2:                  8080032C PASS
TOTAL_AUDITED_CALLS:                        17
DIRECT_CORRECT_STATIC:                      17
WRONG_STATIC:                               0
CALL_MISSING:                               0
LOOKUP_REMAINS:                             0
STATIC_TEST_STRENGTH:                       B
STATIC_TEST_FALSE_PASS_RISK:                LOW
TEST_STRENGTHENED:                          YES
GENERATOR_POLICY_TEST_EXECUTED:             YES
GENERATOR_POLICY_CASES:                     4/4
CUMULATIVE_PATCH_PRIOR_CONTENT_PRESERVED:   YES
UNRELATED_GENERATOR_SEMANTICS_CHANGED:     NO
REGENERATED_LOCAL_BODIES:                  7/7
REGENERATED_DIRECT_CALLS:                  17/17
REGENERATED_VULNERABLE_LOOKUPS:            0
ATTRACT_STATIC_TEST:                        PASS
STICK_ANGLE_TEST:                           PASS
MATRIX_SUBMISSION_TEST:                    PASS
INPUT_DIRECTION_TEST:                      PASS
BCF28_TEST:                                 PASS
CORE1_RA_TEST:                              PASS
TOOIE_BUILD:                                PASS
CCL_REACHED:                                NO (independent short run; prior soak YES)
BSREST_EP11_REACHED:                        NO (not reached in short run)
CORRECT_LOCAL_TARGET:                       UNKNOWN (not reached in short run)
BAFLAMELINE_WRONG_BIND:                    UNKNOWN (not reached in short run)
INTRO_395_REACHED:                          NO (not reached in short run)
EXIT_139:                                   NO
FIX_VERDICT:                                B
FIX_ACCEPTED:                               YES
ATTRACT_DEMO2_CRASH:                        RESOLVED
MARKDOWN_VAULT_AUDIT:                      PASS (0 errors; pre-existing warnings/tasks)
COMMIT:                                     test checkpoint + docs checkpoint
PUSHED:                                     YES (scheduled origin/main push after checkpoints)
NEXT_SINGLE_ACTION:                         return to the next P08 residual
```

The `keep old yaw via -1` hypothesis is REFUTED. The original-MIPS case-11
comparison proves that the 180° freeze is original return-leg behavior for
this actor. No fix is authorized; the broader USER_BUG_B_GLOBAL remains OPEN.

## NEXT

1. **P08 Status: ACTIVE / MANUAL_VALIDATION_PENDING (Verdict B).**
   - Canonical task audit completed: T01, T03..T06, T08..T11 DONE; T02 display catalog DONE; T07 IMPLEMENTED_MANUAL_VALIDATION_PENDING.
   - Next residual after the original-runtime differential: walk only the post-egg run/jump split (~glidx 392).
   - Windowing, aspect ratio calculations, and display validation helpers implemented in `tooie_window`; fullscreen/focus/display event handling confirmed on reference SDL/RT64 path.
   - Graphics and startup error reporting verified via `tooie_error` without crash or unhandled aborts.
2. **Pending Before Formal P08 Close / P09 Open:**
   - Physical gamepad testing across real physical controllers (USB/Bluetooth N64/Xbox/PlayStation).
   - Live physical multi-monitor display switching and desktop fullscreen visual validation.
3. **P08 / P09 Invariants (Protected):**
   - Closed in P08: RT64 presentation baseline, input mapping, window/scale integration, graphics/driver error handling, launcher preview, static call correctness.
   - Do NOT reopen: attract DEMO2 crash, bsrest 0x808003FC, N64Recomp CreateStatic fix, first-straight ~510, fire-egg miss, cinematic facing, stick-angle 13B70, render-submission matrix, BCF28, core1 RA wrappers, PATCH 4 / preemption, P08-T10, original timing/4:3 baseline. Do not modify shared RT64. PATCH 4 stays ON.

## Estado do produto validado

1. Launcher/RT64/Vulkan e auto-start OK.
2. Música inicial OK.
3. Intro 3D + logo + Jiggy OK.
4. Attract request + type-4 complete + warp consumido OK.
5. C2AB8 behavior and delivery resolved (clean-built same-binary CLASS A;
   clean tracked Clang build PASS; PATCH 4 accepted by verdict B).
6. **`func_800BCF28` SIGSEGV RESOLVED** and independently accepted (verdict B;
   [[plan/evidence/p08-bcf28-binding-fix-independent-review]]).
7. First gameplay demo **VISIBLE** after `func_80013A7C` lookup-zero
   **RESOLVED** ([[plan/evidence/p08-core1-ra-wrapper-fix]],
   [[plan/evidence/p08-visible-demo-movement-diagnosis]]).
8. **Actor vs Camera Transform Discrimination (Level B):** Character
   `Actor.rotation` and CPU model matrix are proven upright and correct;
   divergence isolated to `RENDER_SUBMISSION`
   ([[plan/evidence/p08-actor-camera-transform-discriminator]]).
9. **Render-submission matrix fix (Level A, VALIDATED):** Incremental P LOAD +
   V MUL split implemented in `src/game/transform_hooks.cpp`
   ([[plan/evidence/p08-render-submission-matrix-fix]]). Characters and camera
   render upright and elevated; Wooded Hollow attract demo runs continuously for 75s.
10. **Title Screen / File Selection Input VALIDATED (P08-T10 COMPLETE):**
    Frontend input through `CONT_A` confirm is PASS. Direct Spiral
    Mountain / current `mode=1` regression claims are
    **CORRECTED/SUPERSEDED** / **REFUTED**
    ([[plan/evidence/p08-title-file-select-validation]]).
11. **Original timing / 4:3 baseline VALIDATED (P08-T03 COMPLETE):**
    60.0 Hz VI, 20/30 Hz logic, 30 Hz animation, host presentation decoupled, and 4:3 content aspect confirmed
    ([[plan/evidence/p08-original-timing-4x3-baseline]]).
12. **`func_80013B70` stick-angle secondary entry accepted at function level**
    ([[plan/evidence/p08-stick-angle-thunk-fix-rereview]]). Manual WASD
    runtime validation is **RESOLVED** (no more directional collapse).
    Full analog/gamepad remains **PENDING**. Gameplay-demo movement now
    travels in the broadly correct directions because that clip is
    recorded-pad playback through bastick
    ([[plan/evidence/p08-demo-movement-precision-diagnosis]]).
    **Post-physics velocity writer discriminator COMPLETE** (Grok 4.6 HIGH):
    510 is original slope-scaled physics integration (`func_800EF04C` from
    `func_8009AE08`); the post-physics-writer hypothesis is **REFUTED**;
    first-straight internal fidelity **PASS**. Attract/cinematic facing
    is the historical USER_BUG_B diagnosis, now corrected: this actor is
    refuted by original-MIPS review (global USER_BUG_B remains OPEN;
    [[plan/evidence/p08-forced-cinematic-facing-diagnosis]]).

## Arquitetura e regras

TooieRecomp: `N64Recomp` + `N64ModernRuntime` + `RT64` + `RecompFrontend`.
Não editar `RecompiledFuncs` gerado. Repo Git: `TooieRecomp/`, branch `main`.

## Resolved blockers / do not reopen

- Overlay data relocation / `LOOKUP_FUNC(0)`
- Actor manager artificial spawn
- Resident overlay / gcsection
- Scheduler queue (sem nova prova)
- IPL3 integrity gate
- R_MIPS_26 / do_dma
- Type-4 incomplete / warp gated (clean freeze)
- `func_80026500` as hang PC
- `func_800A907C → func_8001EE34` as object driver (refutado)
- **Freeze de C2AB8 como blocker terminal** — superado causalmente e agora
  **permanentemente** (PATCH 4 CLASS A)
- **`func_800BCF28` SIGSEGV / `0x800BCCC0` gap stub** — RESOLVED and
  independently accepted (verdict B) by
  [[plan/evidence/p08-bcf28-binding-fix-independent-review]]
- **`func_80013A7C` LOOKUP_FUNC(0) / core1 RA wrappers** — RESOLVED
  ([[plan/evidence/p08-core1-ra-wrapper-fix]])
- **Actor.rotation as root cause of supine pose** — REFUTED (Actor.rotation is upright)
- **Tooie render submission matrix split** — RESOLVED ([[plan/evidence/p08-render-submission-matrix-fix]])
- **Title/file-select pending validation** — CORRECTED/SUPERSEDED; P08-T10 is
  VALIDATED / COMPLETE ([[plan/evidence/p08-title-file-select-validation]])

## Current expected flow

```text
intro -> Jiggy -> gsattract -> map 335/entrance 8 -> type-4 complete
 -> warp 3 consumed -> A9924 -> A76F4(5) -> mode 5 -> A9A14 -> gsattract ep1
 -> gameplay demo
```

Observed: C2AB8 crossed; `func_800BCF28` completes; `func_80013A7C`
enters at the native strong symbol; visible mode-5 Wooded Hollow attract demo
renders correctly with upright characters and elevated camera, progressing for 75s.

## Exonerated (current)

Frame-sync, event-3, cothemedll, type-4 stall, warp non-consumption,
`func_80026500` hang, scheduler corruption (not proven), `func_800BCF28` binding,
`func_80013A7C` lookup-zero, `Actor.rotation` simulation state, render-submission matrix split.

## Files to read

0. [[plan/evidence/p08-attract-loop-after-demo2-crash]] (**RESOLVED and ACCEPTED** — DEMO2 attract SIGSEGV)
0. [[plan/evidence/p08-demo-movement-precision-diagnosis]] (first demo shot miss §16; preserved)
0a. [[plan/evidence/p08-forced-cinematic-facing-diagnosis]] (USER_BUG_B historical; currently not reproducible)
0a. [[plan/evidence/p08-stick-angle-thunk-fix]] (accepted stick-angle secondary-entry fix)
0a. [[plan/evidence/p08-stick-angle-thunk-fix-rereview]] (**CURRENT REVIEW** — verdict A; `$r1` blocker resolved)
0aa. [[plan/evidence/p08-stick-angle-thunk-fix-independent-review]] (historical verdict C; blocker resolved)
0b. [[plan/evidence/p08-player-direction-facing-diagnosis]] (Level A diagnosis; fix implemented)
0c. [[plan/evidence/p08-directional-input-movement-diagnosis]] (host→pad bidirectional; not reopened)
1. [[plan/evidence/p08-render-submission-matrix-fix]] (**CURRENT** — render matrix submission fix)
2. [[plan/evidence/p08-render-submission-matrix-diagnosis]] (render matrix diagnosis)
3. [[plan/evidence/p08-actor-camera-transform-discriminator]] (transform discriminator)
4. [[plan/evidence/p08-visible-demo-movement-diagnosis]] (visible demo + movement diagnosis)
4. [[plan/evidence/p08-core1-ra-wrapper-fix]] (RA-wrapper fix; resolved)
5. [[plan/evidence/p08-80013a7c-lookup-zero-diagnosis]] (Level A diagnosis; resolved)
6. [[plan/evidence/p08-bcf28-binding-fix-independent-review]] (verdict B; BCF28 fix accepted)
7. [[plan/evidence/p08-bcf28-binding-fix]] (implementer binding-fix evidence)
8. [[plan/evidence/p08-bcf28-crash-diagnosis]] (root cause A diagnosis)
9. [[plan/evidence/p08-preemption-patch4-clean-build-independent-review]] (verdict B; PATCH 4 accepted)
10. [[plan/evidence/p08-preemption-patch4-clean-build-correction]] (implementer correction)
11. [[plan/evidence/p08-preemption-patch4-independent-review]] (historical verdict C preserved; finding resolved)
12. [[plan/evidence/p08-preemption-patch4-validation]] (implementer behavioral evidence)
13. [[plan/evidence/p08-original-timing-4x3-baseline]] (P08-T03 current blocker)
14. [[plan/evidence/p08-session-handoff]]
15. [[plans/BT/08-rt64-input-and-frontend]]

## P08 unnamed overlay JAL fix & validation (2026-08-16, Gemini 3.7 Flash HIGH)

The attract DEMO2 SIGSEGV is **RESOLVED and VALIDATED** (Level A).
The root cause was N64Recomp falling back to global lookup for unnamed same-section
overlay targets, binding `bsrest_entrypoint_11` (`0x80800668`) to `baflameline_entrypoint_5`
at `0x808003FC`.

**Fix:** In N64Recomp `resolve_jal`, relocatable same-section targets take `CreateStatic`
before the lookup-all fallback.
- All 7 audited `.bsrest` targets generated as local static functions (`static_344_80800xxx`).
- 17 risky lookups reduced to 0.
- `tests/attract_demo2_transition_jal_test.py` PASS.
- 300s autonomous soak with Vulkan confirmed clean progression through CCL (314), CS Intro (395), and subsequent attract demos without SIGSEGV (`EXIT_139=NO`).

```text
FIX_OWNER: N64RECOMP
FIX_IMPLEMENTED: YES
FIX_VALIDATED: YES
PATCH_SHA256: 1f2ce7c0d9708df1024a332c1a162049158e368a4e86b747740bb1ccb4236034
VULNERABLE_LOOKUPS_AFTER: 0
NEXT_SINGLE_ACTION: independent review of the generator fix if all acceptance gates pass; otherwise exactly the failing discriminator.
```

## P08 post-egg motion divergence (2026-08-17, Grok 4.6 HIGH)

The independent phase-corrected differential is now recorded. No production
fix was made and P08 remains active.

```text
PHASE_CORRECTED_ALIGNMENT:        ORIGINAL[N] vs RECOMP[N+1]
CONTROL_REGION:                  xz median 0.0224, yaw median 0.0000°
PAD_MATCH_SELECTED_WINDOW:       765/765
FIRST_EXACT_DIFFERENCE:           glidx 385 ideal_yaw (99.38 vs 99.31)
FIRST_MEANINGFUL_DIVERGENCE:      glidx 392 position XZ 2.997 units
FIRST_BAD_FIELD:                  ideal_yaw
FIRST_BAD_STAGE:                  bastick / yaw-intention path
STATE_SEQUENCE_DIFFERENCE:        only capture-boundary bs4 at 355; persistent
                                  post-egg bs21 and jump bs8 sequence matches
TSPD_MULTIPLIER:                  700/700; 1.0000/1.0000
DRIFT_CLASS:                      TURN_DRIFT -> JUMP_TRAJECTORY_DRIFT
ORIGINAL_FIRST_WRITER_PC:         UNKNOWN
RECOMP_FIRST_WRITER_PC:           UNKNOWN
ROOT_CAUSE_LEVEL:                 C
FIX_IMPLEMENTED:                  NO
```

The exact writer is the only remaining discriminator. The camera is downstream,
the digger scan is UNKNOWN, and this split occurs after the first egg input was
already consumed/fired; do not reopen the first shot or closed capture-phase
movement evidence. Full table: [[plan/evidence/p08-demo-post-egg-motion-divergence]].

## P08 USER_DEMO_4 walkstart ncbapivot fresh join (2026-08-21)

The requested hardware-faithful diagnostic capture completed. RT64 and the
Angrylion oracle used real `DISPLAY=:0` with the AMD RX 580 RADV path and a
720-second bound; the only new gate was `TOOIE_UD4_WALKSTART=1`. RT64 reached
`stage=0/sub=4` with seven ncbapivot entries/leaves (glidx 36..42), and the
oracle produced the paired glidx 39 trace and motion CSV. Pad/glrecord joining
is exact for glidx 38..42, including target glidx 39.

The glidx 39 ncbapivot entry payload matches after pointer relocation for
`n74`/`n80`/`tgt`/pos/yaw/ideal, but **`*(node+0x60)+8` does not**: RT64 `0`,
oracle `5`. That word is already different at ncbapivot enter. `80112C3C`
`sltiu`/`beq` at `0x80112C5C` therefore takes opposite paths (skip jumptable vs
case 4 → `800F1738`).

The previously reported `800137AC` `a2`/`f14` difference is
**CORRECTED/SUPERSEDED**: both are dead at that stub (`f14` overwritten from
`0x800416C8`; `a2` unused). Post-overwrite `80013818` enter `f12`/`f14` MATCH.
The extra oracle `80013818` is an alignment artifact of host `func_800137F4`
calling `func_80013818` without `LOOKUP_FUNC`. Guest FCR31 is not stored in
`recomp_context`; the `80112C5C` split is integer and does not read FCSR.

```text
P08: ACTIVE / MANUAL_VALIDATION_PENDING
P09: PENDING / NOT_FORMALLY_OPEN
CALL_TREE_STRUCTURALLY_ALIGNED:    YES through 800137AC enter
EXTRA_80013818_ORACLE_CALL:        ALIGNMENT_ARTIFACT
800137AC_A2_USED_BY_CHILD:         NO
800137AC_F14_USED_BY_CHILD:        NO
RT64_FULL_FCSR_CAPTURED:           NO (UNEMULATED)
FCSR_DIFFERENCE_CAUSALLY_READ:     NO
FIRST_PROVEN_DISCRIMINATOR:        p60_8 0 vs 5 at glidx 39 ncbapivot enter; 80112C5C
ROOT_CLASS:                        B-NCBAPIVOT-CONTROL-INPUT
ROOT_CAUSE_LEVEL:                  PROVEN for this ncbapivot split; p60 writer outside it
FIX_IMPLEMENTED:                   NO
RT64_LOG:                          /tmp/p08-rt64-display-glidx39-r2.log
ORACLE_LOG:                        /tmp/p08-angrylion-ud4-cam.log + /tmp/p08-angrylion-user_demo4.csv
```

This remains diagnosis-only. Keep `round.w.s`, camera feedback, generated
recompiler output, and egg changes closed. Next: writer of `node+0x60` between
glidx 38 leave and glidx 39 enter. Canonical:
[[plan/evidence/p08-user-demo4-walkstart-ncbapivot-discriminator]].

Reproduction checklist for other agents: [[plan/evidence/p08-walkstart-reproduction-runbook]].

## P08 ideal-yaw writer differential — RESOLVED (2026-08-17)

The exact ideal_yaw writer and its input provenance are now PROVEN by a bounded
original/Recomp store trace
([[plan/evidence/p08-demo-ideal-yaw-writer-trace]]). The original
Pure-Interpreter build was rediscovered at `/tmp/p08-m64p` (core `6dca4c1`,
rice `038882d`) and rebuilt locally (no sudo) with a narrow store tracer;
the Recomp writer was captured via a GDB breakpoint on `yaw_setIdeal`.

```text
ORIGINAL_IDEAL_WRITER_PCS:        {0x8009C930}  (yaw_setIdeal store)
RECOMP_IDEAL_WRITER_PCS:          {0x8009C930}  (same generated function)
WRITER_PC_MATCH:                  YES
IDEAL_YAW_WRITER_FUNCTION:        yaw_setIdeal (0x8009C914)
CALLER_FUNCTION:                  func_8009D088 (case 1; jal at 0x8009D0EC)
WRITER_INPUT_ALREADY_DIFFERS:     YES — glidx 385 a1 = 99.3756 vs 99.3081
RAW_PAD_MATCH:                    YES
REMAP_MATCH:                      YES
STICK_MAG_MATCH:                  YES
STICK_ANGLE_MATCH:                YES (bang identical at every glidx)
PLAYER_CONTROL_CAMERA_MATCH:      NO (first differs glidx 385)
PLAYER_CONTROL_CAMERA_SOURCE:     D_80127708[*(ps+0x184)*4] + 0x1C
SAME_AS_PREVIOUS_DISPLAY_CAMERA:  NO (display D_8012D500 departs at 386)
CAMERA_DIVERGENCE_REL_TO_PLAYER:  UPSTREAM_OF_PLAYER (CORRECTED/SUPERSEDED
                                  from AFTER_PLAYER for the control source)
PRIMARY_CLASS:                    SAME_WRITER_DIFFERENT_INPUT
                                  (first divergent input = CONTROL_CAMERA_INPUT)
INITIAL_0P07_DELTA_CLASS:         PROVEN_UPSTREAM_DIVERGENT_VALUE (writer input
                                   differs; earlier selector/internal camera
                                   state is not excluded)
EARLY_TURN_STORE_TRACE_CONTROL:   PASS (glidx 74..91 delta 0.0000)
ROOT_CAUSE_LEVEL:                 B (identical writer; stick path refuted;
                                  camera-angle writer instruction remains)
FIX_IMPLEMENTED:                  NO
```

The stick path (`bastick_update → func_80013B70`) is REFUTED as the first
divergent input. Independent review accepts the writer provenance with a
bounded correction: `WRITER_SET_COMPLETE=UNKNOWN`,
`ORIGINAL_INDEX_MATCH_RECOMP=UNKNOWN`, and no direct control/display-camera
dependency is proven. The next discriminator is the aligned selector and
complete-width store trace for `D_80127708[index]+0x1C`. No production change
is authorized.

## P08 player-control camera divergence — RESOLVED to Level B (2026-08-17)

The exact player-control camera discriminator
([[plan/evidence/p08-demo-camera-control-divergence]]) is now PROVEN: the
selector `ps+0x184` is `0` in **both** runtimes at every sample, the selected
`D_80127708[0]` record is the same logical camera-control object, the camctrl
store set is complete (`{0x800CA65C}` = `func_800CA628` only, no alias/block
write), and the camctrl writer input first diverges at glidx 385
(`-0.0676` deg). The camera delta maps 1:1 onto the ideal_yaw delta
(`PLAYER_DRIFT_CHAIN_CONFIRMED=YES`).

```text
SELECTOR_MATCH:                 YES  (ps+0x184 == 0, both runtimes)
TABLE_SLOT_MATCH:               YES  (D_80127708[0])
CAMCTRL_WRITER_MATCH:           YES  (func_800CA628 / store 0x800CA65C)
CAMCTRL_STORE_SET_COMPLETE:     YES  ({0x800CA65C} only)
CAMCTRL_PRIMARY_WRITER_FUNCTION: func_800CA628 (caller func_8010FC38 / func_8010FE14)
FIRST_INTERNAL_DIVERGENT_FIELD: camctrl +0x1C at glidx 385 (-0.0676 deg)
CAMCTRL_WRITER_INPUT_DIFFERS:   YES  (glidx 385)
GLIDX385_CLASS:                 FIRST_CAUSAL_CAMERA_STATE
PRIMARY_CLASS:                  SAME_WRITER_DIFFERENT_INPUT
                                (first divergent input = CONTROL_CAMERA_INPUT)
ROOT_CAUSE_LEVEL:               R  (func_800E1A58 REFUTED;
                                   [[plan/evidence/p08-demo-camera-e1a58-differential]])
FIX_IMPLEMENTED:                NO
```

**CORRECTED/SUPERSEDED (2026-08-17):** `func_800E1A58` is identity.
**Mode 2 / `func_80110588` is now PROVEN**
([[plan/evidence/p08-demo-camera-mode-producer-differential]]). FE14 copies
stack images of mode-object `+0x74/+0x80`. Last angle writer is setter
`func_80110790`; its `a1[1]` already differs at consumed 384 (−0.0675°).
Caller is **PROVEN** `ncbapivot` (781) / `ncbapivot_entrypoint_3`
(RA offset `0x24C`; prior `0x801D3E7C` is the same site). dest[1] =
`func_80013B70(eye.x-tgt.x, eye.z-tgt.z)`; look-at inputs already
differ at 384 ([[plan/evidence/p08-demo-camera-overlay-angle-producer]]).
Exact 13B70 operands at that call: eye MATCH=NO (grows at 384),
target MATCH=NO (stable background). Causal component is eye
`obj+0x74`; last writer `func_80112E98`
([[plan/evidence/p08-demo-camera-13b70-inputs]]).
No production change.

## P08 post-egg camera eye vector producer differential — PROVEN (2026-08-17)

The exact causal mechanism creating the `mode_obj+0x74` eye vector divergence
at glidx 384 is **PROVEN** ([[plan/evidence/p08-demo-camera-eye-producer-differential]]):

```text
FUNC_80112E98_ARITHMETIC_MATCH: YES (internal candidate generator is bit-faithful)
DIVERGENCE_MECHANISM:           CAUSAL_INVOCATION_OMISSION_AT_384 (phase lag)
ORIGINAL_N64_SET74_COUNT_384:   3 (1st: 80112094, 2nd: 801132F8, 3rd: 80113028)
RECOMP_SET74_COUNT_384:         2 (1st: 80112094, 2nd: 801132F8; 3rd omitted)
ORIGINAL_3RD_SET74_CANDIDATE:   (-4446.002930, -206.578674, 1266.833740)
RECOMP_RETAINED_2ND_SET74:      (-4439.804688, -203.940430, 1272.204346)
PROVEN_CALLER_TRIGGER:          ncbapivot_entrypoint_3 -> func_801138F4 -> func_80112E98
FIX_IMPLEMENTED:                NO (zero production lines modified)
```

In Original N64, `ncbapivot` invokes `func_801138F4 -> func_80112E98` starting
at glidx 384, writing the 3rd candidate vector to `mode_obj+0x74`. In Recomp,
the branch condition in `ncbapivot` bypasses `func_801138F4` at glidx 384-385,
leaving `mode_obj+0x74` at the 2nd SET74 vector and creating the 0.0645° yaw divergence.

## P08 alleged camera collision differential — REFUTED (2026-08-18)

Independent Sol arbitration ([[plan/evidence/p08-demo-camera-collision-sol-review]])
verified the original MIPS at `0x800C4C34..0x800C4E54`. The routine performs
camera-relative matrix transformation, perspective projection, depth
classification, and viewport bounds/offset handling. It reads no collision
world state and evaluates no candidates or triangles. The previous CLEAR/HIT
labels also conflated `func_800E8918`'s final `lh record+0x34` with the internal
`func_800C4C34` return. Q1/Q2 projection-state returns already differ at the
first captured relevant invocation (glidx 70); 384 is only the first visible
counter consequence. Root level `R`; no production fix.

Next: dynamically match the selected projection record and trace the first
writer/input divergence to `record+0x34`, starting at `func_800E82EC`.

## P08 Z-buffer & post-egg motion hardware oracle resolution (2026-08-20, R-MOTION-ORACLE)

[[plan/evidence/p08-demo-zbuffer-angrylion-oracle]] and [[plan/evidence/p08-demo-post-egg-angrylion-motion-differential]]
resolve the Z-buffer camera and post-egg motion differentials against the hardware-faithful oracle
(Mupen64Plus Pure Interpreter + Angrylion RDP Plus + mupen64plus-rsp-cxd4 LLE):

1. **Z-Buffer (R-ORACLE):** The hardware-faithful `ANGRYLION_CONTROL` run produced live depth in RDRAM (`0x866D..0x8C11`), clear writes `0xFFFC`, and `record+0x34 = 1` across all 25/25 aligned Q1 samples, matching RT64. This is an emulator oracle, not a physical N64 measurement.
2. **Post-Egg Motion (R-MOTION-ORACLE):** The historical motion drift (which reached 58 units under Rice) disappeared completely under `ANGRYLION_CONTROL`. `camctrl` at glidx 385 matches within `+0.0030°` (`53.5799°` vs `53.5829°`), `ideal_yaw` matches identically (`102.59°` vs `102.59°`), maximum XZ error across all 765 Demo 0 frames is `< 0.89 units`, and behavior states are 100% bit-identical (`0` mismatches).
3. **Verdict:** Demo 0 movement, camera-control, and physics match the hardware-faithful oracle within the measured scope. Full render-state parity and global attract-demo fidelity remain open.
4. **Current Status:** P08 remains active. Physical gamepad validation and the held-input/downstream dynamic acceptance for `0x800979D8` remain pending. P09 is not formally opened.

Next single action: user executes docs/plan/evidence/p08-final-physical-validation.md checklist on physical hardware.

## Current P08 residual: USER_DEMO_4 p60 writer provenance

The `glidx=39` camera split is now statically bounded to `80110588 → 80112A84
→ 80112D08`; historical RT64 records `a1=0`/`p60+0xC=0` while the oracle
reads `p60+8=5`; the oracle writer input is `a1=5`/`p60+0xC=1`. This closes
B-P60-WRITER-INPUT for the historical capture. The native helper semantics
are now corrected locally, but downstream acceptance is still open. See
[[plan/evidence/p08-user-demo4-p60-writer-provenance]] and
[[plan/evidence/p08-walkstart-reproduction-runbook]].

## 0x800979D8 independent review (2026-08-22)

[[plan/evidence/p08-user-demo4-800979d8-independent-review]] independently
verified the original guest bodies: `0x800979D8` is a real static bainput
helper, not a zero-valued placeholder. Its correct contract calls
`bakey_held`, uses `800BCE84` when `prev_state[id]==0`, and returns the
logical-input enabled mask. The corrected Tooie mapping is registered and the
full focused test passes. Paired held-state provenance
([[plan/evidence/p08-user-demo4-bainput-held-state-provenance]]) shows the
first remaining split in the old capture is **inside** that mapping at glidx
39: matching pad `0001` and matching `prev_state[13]=0`, AN return `2` vs RT
`0`. Probe `held=` is `prev_state`, not `bakey_held`. Instruction-level
`0x800BCE84` join ([[plan/evidence/p08-user-demo4-bce84-live-input-join]])
now proves the live source word `0x115` / mask `1` on both runtimes; the
old `bs_pressed=0` probe is **CORRECTED/SUPERSEDED**. The joined event also
matches `bakey_held=1`, `0x800979D8=2`, `p60.C/E/+8=1/2/5`, and ncbapivot.
The helper chain is **PROVEN in bounded scope**; do not promote this to global
attract parity. User Demo 1 and Demo 5 movement residuals remain open; see
[[plan/evidence/p08-attract-movement-residuals]].

```text
P08_STATUS:                         ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                         PENDING / NOT_FORMALLY_OPEN
NEXT_SINGLE_ACTION:                 trace USER_DEMO_5 to first movement/state divergence
FIX_COMMIT:                         BLOCKED — recorded log hashes do not match current files
```

## USER_DEMO_5 angular-input checkpoint (2026-08-23)

The new capture `/tmp/p08-demo5-ee780-source-20260823T053510Z/` passed its
SHA256 check twice. `800EE780` is confirmed as a downstream elementwise add;
the glidx-24 divergence is already in its `sp+0x54` input. `800EEBF0` and
`800EF214` are an angle-pair-plus-radius to Cartesian transform:
`x=r*cos(p)*sin(y)`, `y=r*sin(p)`, `z=r*cos(p)*cos(y)`.

Only the yaw/heading input differs (`4391e610` RT vs `43927d8b` AN); pitch and
radius match. The value is the second word at ncbapivot `sp+0x38`, written by
the caller add at `0x80800194` and stored at `0x808001A0` before the
`800EEBF0` call. The add combines `80012C3C` and `800125E8`, but its complete
operand join is not captured.

```text
ROOT_CLASS: B-ANGLE-SOURCE-PRECALL-COMBINATOR
MODEL: Grok 4.6 High (corrected canonical label)
FIX_IMPLEMENTED: NO
EVIDENCE_COMMIT: NONE
PUSHED: NO
NEXT_SINGLE_ACTION: paired trace of the two 0x80800194 add operands
```

## USER_DEMO_5 p60 mode-state review — 2026-08-23

`func_80112C3C` is confirmed: mode 0 passes `in_yaw` through, modes 1/2/5
step via `800F1738`, and modes 3/4 use `800F1DCC`. Existing paired evidence
shows p60+8 first differs at glidx 23 (`RT=5`, `AN=0`), while glidx 24 is the
first yaw-output difference. The static immediate writer is
`func_80112D08`, `sw $a1,0x8($t6)` at `0x80112D14`, reached from
`func_80112A84` at `0x80112C20`; dynamic first-write capture is pending because
RT64 Vulkan could not present under the current headless environment. No fix,
commit, or push.

**USER_DEMO_5 `func_801111D0` call-presence arbitration (2026-08-23):** the
`jal 80112D90` at `0x80111294` is enabled by
`(80112E84(a0)==1) || (8011349C(a0,&sp+0x40)!=0)`, with control branches at
`0x80111230` and `0x80111270`. At glidx 23 `p60+0xC=0` before the function in
both runtimes; RT takes the second gate and calls D90, while AN skips it. The
raw input `(2,1,2002)` matches, so this is not proven as a direct C_RIGHT
test. The exact `8011349C` return was not captured; boundary is
`B-801111D0-PREDICATE-INPUT` with call presence proven and helper provenance
pending. No production change, commit, or push.

## Final immutable checkpoint (2026-08-22T07:07Z)

The fresh paired capture `/tmp/p08-979d8-final-20260822T070735Z/` passes
`sha256sum -c SHA256SUMS` twice. The target join is stage 0/sub 4, glidx 39,
pad `(-48,-46,0001)`, button 13, ordinal 8; BCE84 ring-1 raw `0x115` and
return 1, `bakey_held=1`, `0x800979D8=2`, and p60 C/E/+8 `1/2/5` match in the
bounded ncbapivot chain. Task-specific BCE84/979D8 probes are removed;
reusable diagnostics remain env-gated. Tests/build/diff/generated-source
gates pass. Demo 1/5 movement residuals stay open; this checkpoint does not
start Demo 5.

## USER_DEMO_5 `0x80097B94` checkpoint — 2026-08-23

The wrapper contract is now independently fixed: `0x80097B94` passes action
`0xD` and input id `1` to `0x800979D8`, preserving the helper's return. Oracle
matches Gemini's glidx-23 record `(stage=0, sub=5, pad=(2,1,2002))` and shows
BCE84 ring 0/raw 0/masked 0, final result 0. The focused RT64 event is not the
same record (`pad=(-13,-15,0xF1F1)`) and also returns 0. Consequently the
published helper fix covers the implementation, but the claimed RT=2/AN=0
semantic split is not proven. No root move, production fix, commit, or push.

## USER_DEMO_5 `0x80097B70` checkpoint — 2026-08-24

The independent `0x80097B70` trace confirms action `0xA` (`C_LEFT`), input id
0, and the caller edge `0x800A4928` → `0x80110EFC(node, 0)` at `0x800A4930`.
The earlier joined run shows RT64 returning `1` and reaching the consumer at
glidx 23/24 while Oracle returns `0`; Oracle's record-23 state is
`prev=0/ring=1/raw=0xF3/masked=1/bakey=1`, with input 0 disabled before the
helper. A fresh RT64 run with the same visible `(2,1,2002)` record has
`ring=0/raw=0x72C/masked=0/bakey=0` and also returns `0`.

The result is bounded to `B-ACTION-0A-HISTORY-STATE`: the visible pad is
joined, but the history-selected BCE84 word and input-enabled state are not.
The published `0x800979D8` fix covers the shared helper contract; its upstream
state writer remains unresolved. No root move, production fix, commit, or push.

## Corrected USER_DEMO_5 camera re-anchor (2026-08-24)

The correctly paired replay changes the first camera-control anchor to record
24 MATCH -> record 25 DIFF. The Trot-exit input writer matches in RT64 and
Oracle: `bainput_enable(0,1)` changes `0xFFFFFFBE -> 0xFFFFFFBF` and sets bit
0; action `0xA` returns 1 in both. Thus the old `RT97B70=1/AN=0`, RT-only
`0x80110EFC`/`0x801111D0`/`0x800979D0`, and action-`0xA` causal chain are
superseded, including the old `0x80097B94 RT=2/AN=0` claim.
`B-TROT-ENABLE-RECOVERY` is not the root.

The demo synchronizer samples selected camera record `+0x1C`, selected through
`0x80127708[ps+0x184]`; static `func_800CA628` identifies writer PC
`0x800CA65C`, source2[1]. Record 24 is `293.5294` / `0x4392C3C2`; record 25
is RT64 `293.0478` versus Oracle `293.4005` / `0x4392B342`. The Oracle direct
writer is captured, but RT64's direct writer/raw bits are absent. Classification
is `B-CAMERA-WRITER-OR-SOURCE-INCOMPLETE-JOIN`.

Do not promote the old record-23 p60 `5/0`, D90, yaw, or camera-eye causal
claims; p60 must be rechecked only on record 24->25. Next bounded capture:
RT64 `0x800CA65C` with raw old/source/new bits and source2 inputs. Artifact:
`/tmp/p08-demo5-record25-camera-firstdiff-20260824T024500Z/`.
`P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 PENDING / NOT_FORMALLY OPEN`.
No production change.

### USER_DEMO_5 first-writer arbitration correction

The direct CA628 capture now includes RT64. The first record-25 writer observed
is CA628 ordinal `10329` (RA `0x8010FE24`), but RT64 enters with field
`0x4392861E` and source2[1] `0x43922BE8`; Oracle's paired ordinal `103` has
`0x4392B342` and `0x4392A91A`. Therefore CA628 is a downstream Case-B writer,
not the causal boundary. CA8B4 copies the field and E1A58 preserves it later;
the pre-CA628 field writer is still unresolved and needs a semantic write watch.

### USER_DEMO_5 camera-field identity correction

The direct artifact proves only `stack_angles+0x4` and downstream
`selected_record+0x1C`: CA628 is a downstream Case-B writer. `0x8010FE48` is
the `or s0,a0,zero` instruction inside `func_8010FE34`, while the incoming
heading is loaded from `active_node+0x3C` at `0x8010FEA8`. Neither FE34 nor
FE14 writes `active_node+0x34` or `active_node+0x3C`. The direct static
node+0x3C writer candidate is `func_8010FDEC:0x8010FDFC`; its first causal
runtime write remains uncaptured. Current boundary:
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`. Next single action: watch the joined
active camera node `+0x3C` semantically.

## USER_DEMO_5 input_id=0 enable-word provenance — 2026-08-24

`bainput_isEnabled` is exactly a bit test on `*(u32 *)(*(u32 *)(self+0xA4))`;
the action-0xA path tests bit 0. Exact storage writers are `bainput_init`
(`0x8009811C`) and `bainput_enable` (`0x80098140`). Static search found 29
setter call sites (15 set, 14 clear). In the target Oracle trace the word is
`FFFFFFBE` at glidx 22 and `FFFFFFBF` at glidx 23/24, proving a bit-0 state
transition with unchanged visible pad `(2,1,2002)`. The diagnostic writer hook
captured no init/set/clear event, and the new RT64 recapture was blocked by the
headless Zenity launcher. Thus the immediate runtime setter/predicate remains
unattributed; keep classification `B-ACTION-0A-HISTORY-STATE`.

Artifact: `/tmp/p08-demo5-input0-enable-firstdiff-20260824T013213Z/`.

### USER_DEMO_5 camera heading field correction (2026-08-24)

The latest `node34-write-watch` artifact's decisive `0x800EFA5C` store is to
temporary `stack_angles+0x4`, not `active_node+0x34`. Its input is the already
divergent `active_node+0x3C`, loaded at `func_8010FE34:0x8010FEA8`; CA628's
`selected_record+0x1C` write is downstream. The direct static writer candidate
for `active_node+0x3C` is `func_8010FDEC:0x8010FDFC`, called by
`plcamera_entrypoint_13:0x808004A0`, but no paired semantic node+0x3C write
watch captured it. Current classification:
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`.

Latest semantic join: RT active node `0x801C6610`, Oracle active node
`0x801CACB0`; the earlier `0x8039D820` was observed at an older FE14 entry and
is not a proven join to this capture. No production fix or generated C edit was
made.

### USER_DEMO_5 node+0x3C checkpoint audit correction (2026-08-24)

The newer node3C artifact does not prove the claimed post-player MATCH or
pre-FE34 DIFF. Oracle's raw checkpoint probe used `0x8031CF80` with mode `0`
through the target window; its later `0x801CACB0` read still returned
`0x01015590`, and RT64 emitted no paired raw node3C checkpoints. The angular
values were source/downstream boundary values, not direct semantic reads of
`active_node+0x3C`. Keep the safe boundary as
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`; no node+0x3C writer is dynamically
identified.

### USER_DEMO_5 EFA4C heading provenance (2026-08-24)

Safe pre-pass boundary: `B-EFA4C-HEADING-SOURCE-PROVENANCE-UNRESOLVED`.
EFA4C is a downstream stack writer (`f14` <- `a2` at `0x800EFA50`, store
`0x800EFA5C`). FE34 is a load/consumer **when executed**. FDEC remains
`REFUTED_FOR_TARGET`. Recapture
`/tmp/p08-demo5-efa4c-heading-provenance-20260824T061436Z/` (SHA256 PASS ×2
after chmod): Luna heading bits and the `10970→EFA4C` call were **not
observed**. `R24_EFA4C_EVENT_JOIN: FAIL`. `ROOT_CLASS: CASE E -
CORRECTED/SUPERSEDED`. Do not reuse node+0x3C identity. No fix/commit/push.

### USER_DEMO_5 record24 camera-path conflict arbitration (2026-08-24)

`ROOT_CLASS: JOIN-UNRESOLVED`. The latest Grok artifact is read-only and
SHA256-valid, but its metadata says Medium while the user reports High, and
its `sub=5` capture has colliding slices/two RT remapped-`glidx=24` series.
It does not provide the requested semantic record join or paired RT/Oracle
camera-call/CA628/state tables. RT-only wrapper lines (`ra=0`) cannot prove
`FC38 -> FE14 -> CA628`; Oracle has no matching semantic path. The old
FE34/10970/EFA4C claim is `CORRECTED/SUPERSEDED`, but the alternative is not
proven. `CONFLICT_WINNER: NEITHER — NEW CAPTURE REQUIRED`; no source-level
root cause or production fix is authorized. This is independent of the
separate node+0x3C boundary correction.

### USER_DEMO_5 collision-free map296 join and first camera difference (2026-08-24)

JOIN_VERDICT=PASS: the new raw logs independently contain 28 RT and 28 AN records, 28/28 raw6 matches, one epoch, stage=0/sub=5/map=296/mode=5, and zero duplicate canonical record keys. D_8012D5B0 holds the active demo pointer at +0x0 and the post-consume counter at +0x8 (D_8012D5B8), with six-byte records; camera tags are post-consume context, not logical ordinals. func_800151BC is the proven logic-tick boundary.

Current camera boundary: CAMERA_UPDATE_BOUNDARY_VERDICT=REFUTED for the claimed single 0x800A8E9C -> 0x800A9318 update. Static A9318 is a separate FE34 pass and raw R24 has EFA4C 66/73, FE14 1/1, CA628 2/2, A9318 0/0; R24_CALL_SEQUENCE_MATCH=NO.

The first comparable camera event is logical record 0, camera ordinal 1, CA628 call 1. Old bits match (0x4324BF6C), source bits do not (RT=0x4324B542, AN=0x4324B132): ROOT_CLASS=B-FIRST-CA628-SOURCE. At R24, old/source already differ (0x4324A910 vs 0x4324A7B0), so CA628 is downstream and output is unknown. The old FE34/10970/EFA4C chain is CORRECTED/SUPERSEDED; no deeper source is promoted and no fix was implemented.

Derived artifact: /tmp/p08-demo5-first-camera-diff-from-joined-run-20260824T074500Z/; manifest SHA256 d1139defdfa6903041cef31fd2e4db98ca328bd964e702a4e1dca6683a7cff3f.

### USER_DEMO_5 Record 0 CA628 source provenance & writer trace (2026-08-24)

- **Artifact Directory:** `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`
- **Replay State:** `stage=0, sub=5, map=296, mode=5`, 28/28 bit-perfect record join.
- **Initial Map 296 Checkpoint:** `sub+0x84 = 0x43240367` (164.013290), `node+0x50 = 0x3f3c04a3` (0.734446), `node+0x58 = 0x3f800000` (1.000000) bit-identical in BOTH runtimes. Refutes Case E (Map Init).
- **First Divergent Writer:** `func_80116218` at `0x80116380` (`swc1 $f18, 0x4($s1)`).
- **Target Memory Address:** `$sp + 0x2C` (`0x8004567C`, caller stack frame of `func_80110588`).
- **Target Lifetime:** `STACK` (ephemeral stack-allocated vector `sp+0x28`).
- **Values Stored ($f18):** RT64: `0x4324b542` (164.708038) vs Angrylion: `0x4324b132` (164.692169).
- **Downstream Consumer:** `func_800CA628` Call 1 at `0x8010FE1C / 0x8010FE24` copies `src2[1]` (`*(sp+0x2C)`) into `dst+0x1C` (old value matched `0x4324bf6c`).
- **Classification:** `Case A — Writer input diff / upstream angular math evaluation in func_80116218 / func_80116120`.
- **Operational Boundary:** `B-R0-CA628-SOURCE-WRITER-PROVEN`.

### Independent pre-0x80116380 arbitration: first cam_sub+0x84 transition (2026-08-24)

**CURRENT_STATUS:** `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 NOT FORMALLY OPEN`.

**CURRENT_ROOT_BOUNDARY:** `B-R0-FIRST-CAMSUB84-WRITER-SOURCE` — `func_800EE7F8:0x800EE804` writes persistent `cam_sub+0x84` from `0x80045638`; immediate caller unknown.

**LATEST_PROVEN_FACTS:** 28/28 joined records; `func_80110588` entry base `0x43240367` and node+`0x50` `0x3F3C04A3` match; the earlier `EE7F8` write is RT `0x4324036A` versus AN `0x4324036D`; `0x80110634` already copies the difference to `sp+0x2C`; node+`0x50` first differs at `0x801161E0`; `0x80116380`/`CA628` are downstream.

**REFUTED_OR_SUPERSEDED_CLAIMS:** Gemini's `80116380` root; prior `B-R0-CA628-SOURCE-WRITER-PROVEN` as root; older FE34/10970/EFA4C/node+3C roots. Map 296 is Hailfire Peaks — Icy Side / `MAP_128_HP_ICY_SIDE`, not Mayahem; current TOML has no human-readable map table.

**UNRESOLVED_QUESTIONS:** immediate caller/source producer; conditional `0x800893E0` execution/value; complete `func_80116120` input parity.

**ARTIFACT_PATHS:** `/tmp/p08-demo5-r0-pre-80116380-firstdiff-20260824T082500Z/`, joined `/tmp/p08-demo5-map296-semantic-join-20260824T071156Z/`, source `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`.

**ARTIFACT_MANIFEST_SHA256:** `6c793196c0457e162bcf6ebd1ff8ed2f4e764f1a72d544cf09232edd0148c29b`.

**WORKTREE_STATE:** intentionally dirty and preserved; documentation-only changes this run; no `RecompiledFuncs`/`src/overlays` changes.

**CODE/BUILD/TEST_STATUS:** no production fix; no build/test; `git diff --check` pending.

**WHAT_WAS_DONE_THIS_RUN:** independent raw/static arbitration plus immutable artifact and handoff update.

**WHAT_REMAINS_PENDING:** exact caller of the earlier persistent `EE7F8` write.

**NEXT_SINGLE_ACTION:** paired Record 0 capture with RA/callsite around that write, `f71`, and post-`EE7F8` stack checkpoint; stop there.

**NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High / Terra equivalent, adversarial float provenance.

## USER_DEMO_5 `800EE780 → 800EEBF0 src_dir[1]` arbitration (2026-08-24)

**PROJECT_PHASE:** P08 active causal arbitration; **P08_STATUS:** `ACTIVE / MANUAL_VALIDATION_PENDING`; **P09_STATUS:** `PENDING / NOT_FORMALLY_OPEN`.

**CURRENT_HEAD:** `25595a2eb8241ef7ca4aa135e6681e44abcf8598`. **WORKTREE_STATUS:** intentionally dirty state preserved; this round changed documentation only; no `RecompiledFuncs/**` or `src/overlays` change.

**CURRENT_INVESTIGATION:** `USER_DEMO_5`, `stage=0/sub=5/map=296`, semantic `Hailfire Peaks — Icy Side / MAP_128_HP_ICY_SIDE`.

**CURRENT_ROOT_CLASS:** `B-800EEBF0-INPUT` — `PROVEN`.

**CURRENT_ROOT_BOUNDARY:** `func_800EEBF0` consumes `sp+0x34` (`src_dir[0] = pitch`, `src_dir[1] = yaw`) and radius `a2`, producing `sp+0x54`. At glidx 24, its `src_dir[1]` (yaw) is the first divergent input (RT: `0x4391E610` vs AN: `0x43927D8B`), causing its Cartesian output `sp+0x54` to diverge. `func_800EE780` is an elementwise vector adder (`sp+0x48 = sp+0x28 + sp+0x54`) whose input A is bit-identical and whose output divergence is strictly inherited from `sp+0x54`.

**LAST_PROVEN_MATCH:** glidx 23:
- `func_800EEBF0`: RT64 ord=42 <-> ANGRYLION ord=6 (MATCH, 100% bit-exact).
  - `old = (0xC43BDF50, 0x43BEBF32, 0x439FDC90)`, `src_dir = (0x41C84F23, 0x43928610, 0x446158B2)`, `scale = 0x446158D2`, `out = (0xC43BDF66, 0x43BEBF2F, 0x439FDC60)`.
- `func_800EE780`: RT64 ord=374 <-> ANGRYLION ord=454 (MATCH, 100% bit-exact).
  - `old = (0x45818C16, 0x447DB8E3, 0x458225C9)`, `src_a1 = (0x45990800, 0x441E594A, 0x45705000)`, `src_a2 = (0xC43BDF66, 0x43BEBF2F, 0x439FDC60)`, `out = (0x45818C13, 0x447DB8E2, 0x458225C6)`.

**FIRST_PROVEN_DIFF:** glidx 24:
- `func_800EEBF0`: RT64 ord=43 <-> ANGRYLION ord=7.
  - `old = (0xC43CBBD0, 0x43BEC05C, 0x43A01330)` [MATCH].
  - `src_dir[0]` (pitch) = `0x41C79912` (24.949741°) [MATCH].
  - `src_dir[1]` (yaw) = RT: `0x4391E610` (291.797363°) vs AN: `0x43927D8B` (292.980804°) [DIFF].
  - `scale` = `0x44621A8F` (904.414978) [MATCH].
  - `out` = RT: `(0xC43E589D, 0x43BEC05A, 0x43983EF2)` vs AN: `(0xC43CBBB0, 0x43BEC05A, 0x43A01366)` [DIFF].
- `func_800EE780`: RT64 ord=382 <-> ANGRYLION ord=462.
  - `old = (0x45818C16, 0x447DB8E2, 0x458225C9)` [MATCH].
  - `src_a1` = `(0x45992390, 0x441E58B4, 0x4570492C)` [MATCH].
  - `src_a2` = RT: `(0xC43E589D, 0x43BEC05A, 0x43983EF2)` vs AN: `(0xC43CBBB0, 0x43BEC05A, 0x43A***CURRENTLY_DOWNSTREAM_FUNCTIONS:** `func_800EE780`, `func_800EEBF0`, `func_80112C3C`, `func_80112D08`, `func_80112A84`, and `func_80112D90` are all faithful downstream functions whose internal logic is bit-exact and whose operations faithfully follow their entry inputs / control flow.

**REFUTED_OR_SUPERSEDED_CHAINS:** do not reopen the bounded USER_DEMO_4 `979D8 → BCE84 → P60` chain or the previously superseded generic movement/CA628/FE14 roots. The earlier `80116380` camera claim remains superseded by the prior Record-0 correction.

**LATEST_ARTIFACT:** `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/`.
- **MANIFEST_SHA:** `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`.
- **SHA256_VERIFY:** PASS (verified twice; read-only `0444`/`0555`).
- **RAW_LOG_PROVENANCE:** `NEW_RAW_CAPTURE` (Dual live capture with paired dynamic hooks in both RT64 and ANGRYLION_CONTROL).

**OBSERVATIONAL_GAP_RESOLUTION:** The prior gap (where ANGRYLION `func_80097B70` return = 0 was an inference) is now **RESOLVED**. Both runtimes directly measured `func_80097B70` (`bainput_should_rotate_camera_left`), `func_800979D8`, and branch `0x800A4928` (`beq $v0, $zero, 0x800A4940`):
- `glidx 22`: `func_800979D8` v0=0/0 MATCH; `func_80097B70` v0=0/0 MATCH; branch 0x800A4928 taken=1/1 MATCH (branches over rotation left in both runtimes).
- `glidx 23`: `func_800979D8` v0=1 (RT) vs v0=0 (AN) DIFF; `func_80097B70` v0=1 (RT) vs v0=0 (AN) DIFF; branch 0x800A4928 taken=0 (RT, fallthrough -> calls `func_80110EFC` -> `80110E00` -> `801111D0` -> `80112D90`) vs taken=1 (AN, branches over rotation left).

**CURRENT_UNKNOWNS:** upstream evaluation inside `func_800979D8` where `bakey_held(self, 10)` (`0x800919F4`) / `func_800BCE84` evaluates to `held = 1` in RT64 vs `held = 0` in ANGRYLION_CONTROL at `glidx 23`.

**PENDING_WORK:** Causal arbitration and next upstream boundary investigation of `bakey_held` (`0x800919F4`) and `func_800BCE84` for button 10 (C-Left) at `glidx 23`.

**NEXT_SINGLE_ACTION:** Perform static and mechanical dynamic boundary analysis of `bakey_held` (`0x800919F4`) and controller state query `func_800BCE84` for button 10 (C-Left) at `glidx 23`.

**NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High for mechanical boundary instrumentation / GPT-5.6 Luna High for causal arbitration.

**FIX_IMPLEMENTED:** NO. **COMMIT:** NONE. **PUSHED:** NO.

## Current independent arbitration — USER_DEMO_5 func_80097B70 / func_800979D8 Direct Measurement (glidx 22->23)

- **PROJECT_PHASE:** P08. **P08_STATUS:** ACTIVE / `func_80097B70` and `func_800979D8` directly measured. **P09_STATUS:** PENDING / NOT FORMALLY OPEN.
- **CURRENT_HEAD:** `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
- **LATEST_ARTIFACT:** `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/`; all `SHA256SUMS` entries pass; **MANIFEST_SHA:** `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`.
- **RAW_LOG_PROVENANCE:** `NEW_RAW_CAPTURE` (dual live captures of `TooieRecompiled` and Mupen64Plus `ANGRYLION_CONTROL`).
- **SEMANTIC_JOIN:** `PROVEN`. Normalized join key is `(stage=0, sub=5, map=296, mode=5, glidx, calling_region=func_800A4878, controlling_branch=0x800A4928, bainput_helper=func_800979D8)`.
- **LAST_COMMON_CONTROL_FLOW_EVENT:** glidx 22 (`func_800979D8` v0=0/0 MATCH, `func_80097B70` v0=0/0 MATCH, branch 0x800A4928 taken=1/1 MATCH, `func_80112D90` NOT called in either runtime; `p60+0xC = 0`).
- **FIRST_DIFFERENT_CONTROL_INPUT:** Return value `$v0` of `func_800979D8` and `func_80097B70` at glidx 23: RT64 `$v0 = 1` vs ANGRYLION `$v0 = 0`.
- **FIRST_DIFFERENT_BRANCH:** `0x800A4928: beq $v0, $zero, L_800A4940` (NOT TAKEN in RT64 -> invokes `func_80110EFC` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90`; TAKEN in ANGRYLION -> bypasses camera rotation dispatch).
- **CALL_PRESENCE_AT_TARGET_CALLSITE (func_801111D0 @ 0x80111294 -> func_80112D90):**
  - RT64 (glidx 23): `YES` (executed, setting `p60+0xC = 1`).
  - ANGRYLION (glidx 23): `NO` (`NOT_CALLED / N/A`, `p60+0xC = 0`).
- **P60+0xC STATE TRANSITION:**
  - RT64: `OLD_P60_C = 0` -> `func_80112D90` writes `1` -> `NEW_P60_C = 1`.
  - ANGRYLION: `BEFORE = 0` -> no call -> `AFTER = 0`.
- **CURRENT_ROOT_CLASS:** `B-BAINPUT-HELD-STATE` / `B-ROTATE-CAMERA-LEFT-INPUT`.
- **CURRENT_ROOT_BOUNDARY:** `bakey_held(self, 10)` (`0x800919F4`) query inside `func_800979D8` evaluating to `held = 1` in RT64 vs `held = 0` in ANGRYLION at `glidx 23`.
- **LATEST_PROVEN_CAUSAL_CHAIN:** `bakey_held(self, 10) (1 vs 0)` -> `func_800979D8 v0 (1 vs 0)` -> `func_80097B70 v0 (1 vs 0)` -> branch `0x800A4928 (not taken vs taken)` -> `func_80110EFC` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90 (called vs not called)` -> `p60+0xC (1 vs 0)` -> `func_80112A84 bundle (5 vs 0)` -> `func_80112D08 p60+8 (5 vs 0)` -> `func_80112C3C (5 vs 0)` -> yaw -> `800EEBF0` -> `800EE780` -> camera/movement.
- **NEXT_SINGLE_ACTION:** Perform static and mechanical dynamic boundary analysis of `bakey_held` (`0x800919F4`) and `func_800BCE84` for button 10 (C-Left) at `glidx 23`.
- **NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High / GPT-5.6 Luna High.
- **WORKTREE_STATUS:** intentionally dirty; no `RecompiledFuncs/**` or `src/overlays` changes. **FIX_IMPLEMENTED:** NO. **COMMIT:** NONE. **PUSHED:** NO.

## Superseding arbitration — `800979D8` primary-log correction (2026-08-25)

**ARTIFACT:** `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/`
**MANIFEST_SHA:** `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`
**SHA256_VERIFY:** PASS. **RAW_LOG_PROVENANCE:** `NEW_RAW_CAPTURE`.

The preceding `held RT=1 / AN=0` claim is **CORRECTED/SUPERSEDED** by the
primary logs. In the joined `USER_DEMO_5` (`stage=0/sub=5/map=296/mode=5`,
`glidx=22 -> 23`) target, RT and AN both have semantic `self`, `button=10`,
`input=0`, `prev_state=0`, map `0`, button word `0`, enabled word
`0xffffffbf`, enabled bit 0 `1`, and `bakey_held=1`. Raw self pointers differ
(`RT=0x801e5410`, `AN=0x801e84f0`) and are not join keys.

The first proven difference is `800979D8` leave: `RT v0=1`, `AN v0=0`.
`80097B70` forwards this value faithfully and `0x800A4928` branches on it.
The exact RT `800A4878` entry has no separate marker, but its branch PC and
callsite are directly observed; the semantic join remains **PROVEN**.

Static read-set: `bakey_held` reads `self+0x40`, `key+0x18+button_id`, and
`key+0`; on zero previous state it calls `800BCE84(state, button_id)` at
`0x80091A18`. `800BCE84` reads `state+0x8+button_id`, calls `800BCCC0`,
indexes by button, and returns bit 0. `bainput_isEnabled` reads `self+0xA4`
and its enabled word. Logged map/button-word fields are trace-only, not
functional `bakey_held` reads.

AN has no direct `800BCE84` marker for the mapped-byte/table inputs; therefore
the complete external read-set is **NOT_PROVEN_MATCHING**. `800BCE84` remains
`NEXT_UPSTREAM_CANDIDATE`, not root. **CURRENT_ROOT_CLASS:**
`B-800979D8-COMPLETE-READSET-UNKNOWN` (Case B). **CURRENT_ROOT_BOUNDARY:**
`800979D8` return at glidx 23. **NEXT_SINGLE_ACTION:** dual-runtime glidx
22/23 capture of the complete `bakey_held` pointer chain,
`800BCE84` presence/args/return, and `800979D8` held/return; stop at
`800BCE84`. **NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High. No fix.

## Superseding narrow arbitration — `800979D8` post-`isEnabled` body (2026-08-25)

Artifact `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` is valid,
`NEW_RAW_CAPTURE`, manifest `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`,
and `sha256sum -c SHA256SUMS` passes. At joined glidx 23, `bakey_held=1/1`
and `enabled_word=0xffffffbf` gives enabled bit 0 `1/1` (**PROVEN**).

## Canonical Resolution — `func_800979D8` Exact MIPS Recovery & Static Implementation Mismatch (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is valid,
`NEW_RAW_CAPTURE`, manifest `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`,
and `sha256sum -c SHA256SUMS` passes (dual verification).

1. **Exact Original MIPS Body (`0x800979D8 .. 0x80097A4C`) Recovered and Proven:**
   - `0x800979E8`: calls `bakey_held(self, button_id)` (`0x800919F4`).
   - `0x800979F0`: `beqz $v0, 0x80097A2C` (if not held, jumps to unheld path).
   - `0x800979FC`: calls `bainput_isEnabled(self, input_id)` (`0x80098024`), which queries `*(input + 0x0) & (1 << input_id)`.
   - `0x80097A04`: `bnez $v0, 0x80097A18` (if enabled, skips disable call).
   - `0x80097A10`: calls `func_80097990(self, input_id, 0)` (when enabled == 0, clears bit in `*(input + 0x4)`).
   - `0x80097A1C`: calls `func_80097978(self, input_id)`, which returns `*(input + 0x4) & (1 << input_id)`!
   - `0x80097A24`: jumps to epilogue and returns `$v0` from `func_80097978`.
   - Unheld path (`0x80097A2C`): calls `func_80097990(self, input_id, 1)` (sets bit in `*(input + 0x4)`) and returns `$v0 = 0`.

2. **Root Cause of Divergence at glidx 23:**
   - At `glidx 23`, `bakey_held` returns `1` and `bainput_isEnabled` returns `1` in both runtimes.
   - Original MIPS: executes `0x80097A1C: jal 0x80097978`. `func_80097978` reads `*(input + 0x4) = 0xFFFFFFFE` -> evaluates `bit 0 = 0` -> returns `$v0 = 0`.
   - RT64 host replacement (`tooie_bainput_func_800979D8` in `src/game/boot_hw_stubs.cpp`): incorrectly calls `bainput_isEnabled(self, input_id)` a second time (reading `*(input + 0x0) = 0xFFFFFFBF` -> evaluates `bit 0 = 1`) -> returns `ctx->r2 = 1`.

3. **Classification:**
   - `STATIC_IMPLEMENTATION_MISMATCH_800979D8`: **PROVEN**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-STATIC-HOST-IMPLEMENTATION-MISMATCH`.
   - `CURRENT_ROOT_BOUNDARY`: Callee selection in `func_800979D8` (`0x80097A1C` calls `func_80097978` on `input+0x4`, not `bainput_isEnabled` on `input+0x0`).

4. **Next Actions:**
   - **NEXT_SINGLE_ACTION:** GPT-5.6 Luna High independently arbitrate the exact original-vs-host semantic mismatch before any production fix.
   - **NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High. No fix, commit, or push.

### Corrected/Superseded — `800979D8` bug versus current `input+0x4` root (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is primary
and hash-valid: manifest SHA `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`;
all `SHA256SUMS` entries pass. The recovered MIPS body `0x800979D8..0x80097A4C`
and its post-`isEnabled` body are complete, with proven original semantics.

`STATIC_IMPLEMENTATION_MISMATCH_800979D8=PROVEN`: original `0x80097990`
writes `input+0x4`, while the host uses `bainput_enable` (writes `input+0x0`);
original `0x80097978` reads `input+0x4` at `0x80097984`, while the host's
second query uses `bainput_isEnabled`/`input+0x0`.

### Canonical Checkpoint — `input+0x4` First Temporal Writer & Host `800979D8` Causal Proof (2026-08-25)

Artifact `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA is `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`,
and all 10 `SHA256SUMS` entries pass (dual verification).

1. **First Temporal Writer Event Identified:**
   - At `glidx 22`, occurrence `func_800979D8(self, button_id=10, input_id=0)`:
     - `held == 1` (`bakey_held` returns 1) and `enabled == 0` (`bainput_isEnabled` returns 0) in BOTH runtimes.
     - `*(input + 0x4)` is `0xFFFFFFFF` (bit 0 = 1) in BOTH runtimes immediately before the call.
     - **AN (Original MIPS):** Takes branch `0x80097A04` fallthrough (since `enabled == 0`) and executes `0x80097A10: jal func_80097990(self, 0, 0)`. `func_80097990` executes CLEAR store at `0x800979CC`, transforming `*(input + 0x4)` from `0xFFFFFFFF` (bit 0 = 1) to `0xFFFFFFFE` (bit 0 = 0).
     - **RT (Host C++ Replacement):** `tooie_bainput_func_800979D8` mistakenly calls `bainput_enable(self, 0, 0)` at `0x8009817C` (which writes `*(input + 0x0)`) instead of `func_80097990` (which writes `*(input + 0x4)`). As a result, `*(input + 0x4)` in RT64 **is never modified and remains `0xFFFFFFFF`** (bit 0 = 1).
   - This single host replacement defect at `glidx 22` is the **EXACT CAUSAL PRODUCER** of the `input+0x4` state divergence.

2. **Downstream Target Divergence at `glidx 23`:**
   - At `glidx 23`, `bainput_enable(self, 0, 1)` updates `*(input + 0x0)` to `0xFFFFFFBF` in both runtimes.
   - When `func_800979D8(self, button_id=10, input_id=0)` is called:
     - `held == 1`, `enabled == 1`.
     - **AN:** Executes `0x80097A1C: jal func_80097978(self, 0)`. `func_80097978` reads `*(input + 0x4) = 0xFFFFFFFE` (bit 0 = 0) -> returns `$v0 = 0`. Controlling branch `0x800A4928` branches over camera rotation left (`p60+0xC = 0`).
     - **RT:** Host replacement calls `bainput_isEnabled(self, 0)` on `*(input + 0x0) = 0xFFFFFFBF` (bit 0 = 1) -> returns `ctx->r2 = 1`. Controlling branch `0x800A4928` falls through to `func_80112D90`, setting `p60+0xC = 1` (unwanted camera rotation left).

3. **Classification & Verification:**
   - `FIRST_INPUT_PLUS4_DIFFERING_EVENT`: `glidx 22`, `func_800979D8(self, btn=10, in_id=0)`.
   - `FIRST_INPUT_PLUS4_WRITER_RT`: `NO_WRITE_OBSERVED` (host called `bainput_enable` on `input+0`, omitting write to `input+4`).
   - `FIRST_INPUT_PLUS4_WRITER_AN`: `func_80097990` at `0x800979CC` (CLEAR bit 0).
   - `FIRST_INPUT_PLUS4_WRITER_EVENT_MATCH`: `ABSENT_IN_ONE_RUNTIME`.
   - `FIRST_INPUT_PLUS4_OLD_MATCH`: `YES` (`0xFFFFFFFF`, bit0=1).
   - `FIRST_INPUT_PLUS4_SOURCE_OPERATION_MATCH`: `NO`.
   - `FIRST_INPUT_PLUS4_NEW_MATCH`: `NO` (`0xFFFFFFFE` [bit0=0] vs `0xFFFFFFFF` [bit0=1]).
   - `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF`: **`PROVEN`**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_RT`: `NO`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_AN`: `NO`.
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High arbitrate the first input+4 divergence and the causal proof that the host 800979D8 mismatch produces it.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

## Final root checkpoint — `800979D8` first `input+4` divergence (2026-08-25)

**CORRECTED/SUPERSEDES** the earlier pending-arbitration wording and any
claim that complete external input state matched at glidx 22. The verified
`NEW_RAW_CAPTURE` is `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/`
with manifest SHA `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`;
all checksum entries pass.

- Semantic join is **PROVEN** for `stage=0/sub=5/map=296/mode=5`, player
  semantic self, `btn=10`, `input_id=0`; both runtimes call `800979D8`.
- At glidx22, held `1/1`, enabled `0/0`, and pre-write `input+4`
  `ffffffff/ffffffff` match. AN then performs `func_80097990(self,0,0)`;
  RT calls `bainput_enable(self,0,0)` on `input+0`.
- `PRE_WRITE_STATE_GLIDX22` matches; `POST_WRITE_STATE_GLIDX22` is AN
  `fffffffe` versus RT `ffffffff`. Therefore
  `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF=PROVEN` and
  `ROOT_CAUSE_FOUND=YES`.
- Definitive stores from raw opcodes: SET `0x800979B0` (`ac590004`), CLEAR
  `0x800979CC` (`ac4c0004`). The `0x80097A10` callsite has delay slot
  `0x80097A14`; `0x80097A18` is the `jal` return address. Earlier labels
  assigning CLEAR to `0x800979B0` or a store to `0x800979C4` are
  **CORRECTED/SUPERSEDED**.
- At glidx23, AN's `func_80097978` reads `input+4` and returns `0`; RT's
  second `bainput_isEnabled` reads `input+0` and returns `1`. This makes
  `80097B70=0/1` and `0x800A4928=TAKEN/NOT_TAKEN`; camera-chain activity is
  downstream.
- `HOST_MISMATCH_WRITE_HELPER=PROVEN` and `HOST_MISMATCH_READ_HELPER=PROVEN`;
  both are wrong callee substitutions in one replacement and must be fixed
  together. Exact `func_80097990/97978` public generated helpers were not
  found; existing host `bainput_enable/isEnabled` are not equivalent.
- A glidx23-only query fix is insufficient. A full faithful replacement is
  **EXPECTED** to restore this chain, but no runtime PASS is claimed.

## Post-Fix Validation Checkpoint — `tooie_bainput_func_800979D8` Faithful Fix (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/` is valid;
`NEW_RAW_CAPTURE_POST_FIX`, manifest SHA is `9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`,
all 16 `SHA256SUMS` entries pass (dual verification).

- **`ROOT_CAUSE_FOUND:`** `YES`
- **`ROOT_CAUSE_FIX_VALIDATED:`** `YES`
- **`FIX_IMPLEMENTED:`** `YES`
- **`FIX_SCOPE:`** `src/game/boot_hw_stubs.cpp` (`tooie_bainput_func_80097990` on `input+4`, `tooie_bainput_func_80097978` on `input+4`, and corrected `tooie_bainput_func_800979D8`)
- **`HOST_WRITE_HELPER_FIXED:`** `YES`
- **`HOST_READ_HELPER_FIXED:`** `YES`

**Validation Results (100% Parity with ANGRYLION_CONTROL):**
- `GLIDX21_CONTROL_PARITY:` `PASS` (held=0, enabled=1, input+4=0xFFFFFFFF, v0=0)
- `GLIDX22_HELD_PARITY:` `PASS` (held=1/1)
- `GLIDX22_ENABLED_PARITY:` `PASS` (enabled=0/0)
- `GLIDX22_INPUT4_OLD_PARITY:` `PASS` (0xFFFFFFFF/0xFFFFFFFF)
- `GLIDX22_OPERATION_PARITY:` `PASS` (func_80097990 CLEAR store)
- `GLIDX22_INPUT4_NEW_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_INPUT4_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_800979D8_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_80097B70_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_800A4928_PARITY:` `PASS` (TAKEN/TAKEN, skipping camera rotation left)
- `GLIDX23_80112D90_CALL_PARITY:` `PASS` (NO/NO, camera rotation left completely collapsed)
- `GLIDX23_P60C_PARITY:` `PASS` (p60+0xC = 0/0)
- `POST_FIX_EEBF0_PARITY:` `PASS` (yaw=270.0 in both runtimes)
- `POST_FIX_EE780_PARITY:` `PASS` (a3=0 in glidx 21..23, 3fa21681 in glidx 24, 3fe2ec4e in glidx 25)
- `BTN13_NEARBY_REGRESSION:` `PASS` (input+4 bit1 SET, v0=0 in both runtimes)

**Status & Next Steps:**
- The entire USER_DEMO_5 locomotion/camera divergence chain is **RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX**.
- P08 remains **ACTIVE / MANUAL_VALIDATION_PENDING** pending broader physical validation gates.
- `NEXT_SINGLE_ACTION:` GPT-5.6 Luna High independently arbitrate the post-fix validation artifact and determine the remaining P08 work.
- `NEXT_RECOMMENDED_MODEL:` GPT-5.6 Luna High. No commit/push.

## Arbitragem final pós-fix P08 / USER_DEMO_5 — 2026-08-26 (GPT-5.6 Luna High)

Este bloco **CORRIGE/SUPERSEDE** o checkpoint anterior somente quanto ao
escopo, proveniência, classificação dos gates e trabalho restante. O histórico
anterior é preservado.

Artifact arbitrado: `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/`
com `MANIFEST_SHA=9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`.
`SHA256SUMS` confere integralmente (16/16 arquivos `OK`).

### Veredicto causal

- `RT_RAW_PROVENANCE: NEW_RAW_CAPTURE_POST_FIX`.
- `AN_RAW_PROVENANCE: REFERENCE_ORACLE_CAPTURE_REUSED`.
- `REFERENCE_ORACLE_REUSE_VALID: YES`: `oracle.log` é byte-identical ao
  `/tmp/oracle_fresh3.log`; os timestamps mostram a captura Oracle depois da
  instrumentação e antes do fix de produção. A reutilização é válida para este
  artifact delimitado; não prova um estado externo ilimitado.
- A implementação atual é semanticamente equivalente ao MIPS: no caminho
  não-held faz SET em `input+4` e retorna zero; no caminho held consulta
  `input+0`, faz CLEAR em `input+4` quando disabled, e consulta/retorna o bit
  de `input+4`. O shift usa os mesmos 5 bits baixos e o retorno preserva a
  extensão de sinal de 32 bits.
- `USER_DEMO5_TARGET_CHAIN_PARITY: PROVEN` — parity bit a bit do chain focal,
  não do jogo inteiro: `800979D8 → 80097B70 → 800A4928 → câmera/80112D90 →
  p60+0xC → yaw/EEBF0/EE780`.
- `GLOBAL_GAME_PARITY: NOT_PROVEN`; não foi feita nova captura nem long
  playthrough nesta rodada.

Gates críticos: `DIRECTLY_SUPPORTED` para held/enabled, old/new `input+4`,
operação CLEAR/SET, retornos de `800979D8`/`80097B70`, branch `800A4928`,
`p60+0xC`, yaw, `EEBF0/EE780` e regressão btn13; `DERIVED_BUT_PROVEN` para a
junção causal normalizada e a ausência de dispatch de rotação esquerda em
`80112D90`; `UNSUPPORTED` somente para parity global, long playthrough e
validação Windows/controle físico (ainda não executados).

### Residuais e status

- `P08_STATUS: ACTIVE / MANUAL_VALIDATION_PENDING`.
- `P09_STATUS: PENDING / NOT_FORMALLY_OPEN`.
- `USER_DEMO5_STATUS: RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`.
- `AUDIO_STARTUP_TICK: P08_RESIDUAL` (observado pelo usuário; não é prova de
  regressão causal deste chain).
- `WIDESCREEN_EDGE_GEOMETRY: POST_P08_ENHANCEMENT` (conteúdo original 4:3,
  não um gate de parity do alvo) is **CORRECTED/SUPERSEDED**. The 2026-08-25
  class-C `func_800CAB70` visual-root claim is also **CORRECTED/SUPERSEDED**:
  user manual CAB70/E3DC0 bypass is `NO_CHANGE`. Current class:
  `G — RT64_G_EX_ASPECT_PIPELINE_BOUNDARY`
  ([[plan/evidence/p08-widescreen-edge-geometry]]).
- `CHARACTER_EDGE_GRAPHICS_GLITCH: P08_RESIDUAL` (observado, ainda sem
  reprodução causal formal).
- `LONG_PLAYTHROUGH_VALIDATION: MANUAL_VALIDATION_PENDING`.
- `WINDOWS_GAMEPAD_VALIDATION: BLOCKING_P08 / MANUAL_VALIDATION_PENDING`.

Remaining P08: live RT64 aspect/scissor/screenPos discriminator for
`WIDESCREEN_EDGE_GEOMETRY` (class G; CAB70 visual-root **REFUTED_FOR_OBSERVED_SYMPTOM**);
checklist formal Windows/controle físico; long playthrough; triagem
separada do tic de áudio e de `CHARACTER_EDGE_GRAPHICS_GLITCH`.

`CURRENT_FIX_READY_FOR_COMMIT: YES` para o fix focal, condicionado à revisão
do worktree: a produção está em `src/game/boot_hw_stubs.cpp`; os cinco docs
de evidência contêm este checkpoint; os artifacts em `/tmp` não são commitáveis.
O worktree ainda mistura instrumentação/diagnóstico e documentação histórica,
portanto não está sendo declarado como commit único limpo. `BUILD_THIS_ROUND:
NOT_RUN`; nenhum PASS de build é inferido. Nenhum commit/push foi feito.

`NEXT_SINGLE_ACTION: Gemini 3.7 Flash High run the live RT64
useWideViewport / converted-scissor / post-aspect screenPos-X discriminator
specified in [[plan/evidence/p08-widescreen-edge-geometry]]. Formal
Windows/gamepad validation remains a separate P08 gate.`

## Current P08 display-list identity & frame-join fix (2026-08-26)

The single discriminator `GUEST_DISPLAY_LIST_IDENTITY_CAPTURE` has been audited,
corrected, and verified end-to-end. The root cause of the previous unjoined capture
was diagnosed: `tooie_ws_capture_graphics_task` in `boot_hw_stubs.cpp` hooked
`0x80014148` and read `task+0x08` as `ucode_boot` and `task+0x0C` as `ucode_boot_size`
with `end < start`, dropping 100% of emissions, while `rt64_interpreter.cpp` opened
an independent log handle that buffered writes until shutdown and hardcoded
`guest_frame=UNKNOWN`.

The fix:
1. Hooked master graphics task submission at `func_80013EF0` (`0x80013EF0`), where
   `$a0` is `GraphicsTask*` (`task+0x08 = start`, `task+0x0C = end`, `size = end - start`).
2. Thread-safe, mutex-protected unified logging directly into `/tmp/tooie-ws-dl-identity.log`.
3. Exported canonical `tooie_get_current_guest_frame()` and `tooie_get_current_capture_id()`
   read by Tooie guest hooks, RSP HLE interpreter, and RT64 rasterizer.
4. Direct C linkage `tooie_ws_log_dl_rsp()` replacing file polling.

`DL_IDENTITY_CAPTURE_READY: YES`; `DL_ADDRESS_SEMANTICS: PROVEN`;
`NESTED_DL_TRACKING: YES`; `NODE_TO_DL_JOIN: NO`;
`DL_TO_RSP_JOIN: YES`; `RSP_TO_RT64_WORKLOAD_JOIN: YES`;
`DL_TO_RT64_DRAW_JOIN: YES`; `END_TO_END_JOIN: YES`;
`PER_FRAME_ACTIVE_DL_SET_READY: YES`; `DL_FIRST_LAST_SEEN_READY: YES`;
`OLD_DL_EVENT_TIMING_BUG_ROOT: PROVEN`; `DL_IDENTITY_FRAME_JOIN_FIX_STATUS: VERIFIED`;
`GUEST_FRAME_JOIN_ACCURACY: 100.0%`; `UNJOINED_DL_EVENT_COUNT: 0`;
`DL_EVENTS_WITH_GUEST_FRAME_UNKNOWN: 0`.

Validation smoke test results:
- `[ws-capture-start]`: 1
- `[ws-frame]`: 648
- `[ws-dl-emit]`: 4,779
- `[ws-dl-rsp]`: 8,908
- `[ws-dl-rt64]`: 8,908
- `[ws-rt64-draw]`: 47,761
- Root ping-pong: `80159600` and `80160cd0` (`DOUBLE_BUFFER`).
- Root join match: 100% matched with ~78µs main-to-render thread latency.

Causal state: `CURRENT_ROOT_CLASS: UNKNOWN / RE-ANCHOR_REQUIRED` (preserved);
`CURRENT_ROOT_BOUNDARY: NOT_PROMOTED` (preserved).
Artifact: `/tmp/p08-widescreen-dl-framejoin-fix-20260826T130423Z/`; manifest
SHA256: `4c650961c173fb7ad7a924fbaaf90abe1393cc3a9d1a53adfe41807d7442cb32`.

`NEXT_SINGLE_ACTION: User triggers one live capture during active gameplay exhibiting the edge geometry glitch (touch /tmp/tooie-ws-capture) to generate authoritative trace in /tmp/tooie-ws-dl-identity.log.`

## P08 FOCAL_BRANCH_TIMELINE setup (2026-08-26)

Implemented diagnostic-only branch capture in `src/game/boot_hw_stubs.cpp`.
The direct generated `80801174` path is observed through caller-frame
reconstruction in wrappers at `800E3E8C → 800E3D0C → 800CAA24`; no generated
file was edited.
Events use the same `capture_id` and `guest_frame` as the existing DL stream.
Stable identity is `section_ptr + section_index + section_entry_ptr` with
entry stride `0x58`.

`CAA24` is a two-group separating-plane predicate over the camera/viewport
volume from `D_8012D500` and two AABB corner vectors (`v0=1` pass, `v0=0`
reject). Its exact retail writer/update cadence remains `UNKNOWN`. Build:
`ninja -C build TooieRecompiled` PASS. The launcher smoke completed 720 frames
and emitted 3,881 DL events, but did not enter map traversal, so branch runtime
evidence remains `UNKNOWN`; no causal claim is made. `FIX_IMPLEMENTED: NO`.

Artifact: `/tmp/p08-widescreen-focal-branch-timeline-setup-20260826T164317Z/`;
manifest SHA `f010ac8a1a33e97494a0d6d5dcb90a237bc8814399c4a01ba1328f21c9de81e3`.
Next single action: one live user capture at the visible residual, then Grok
4.6 Medium arbitration of sphere versus CAA24 versus section state.

## P08 ultrawide revalidation (2026-08-31)

The production T_INTRO projection fix remains valid for the tested 16:9
scenes. CPU homogeneous clipping of the corrected MAP_18B OPA triangle set
under the final source-aspect policy still finds target-only wall-to-empty
terminations at 21:9 and 32:9 in representative Scene A poses; separate
right-side target-only portions are SKY18/background. The 21:9+ authored
extent residual is therefore confirmed under the production policy, while
the exact live Scene B capture remains a semantic-equivalence check through
the shared opening camera/root. No geometry extension was implemented.

Artifact: `/tmp/p08-tintro-ultrawide-revalidation-20260831T042437Z/`.

## P08 authoritative T_INTRO recapture (2026-08-31)

One new CPU-only RT64 graphics-task capture was produced with cached
`TOOIE_DIAG_TINTRO_AUTHORITATIVE_GEOMETRY=1`. Triangle emission records carry
source pass owner (OPA/XLU/OTHER), material state, DL provenance, real
workload vertex/UV/color buffers, matrices, viewport, and scissor; RDP
`fillRect`/`drawTexRect` records provide background-source evidence. No GPU
readback was used. The temporary instrumentation was removed afterward.

Observed rows: Scene A 1,644 camera samples / 118,319 triangles; Scene B
4,227 / 550,829 valid triangles. A and the post-file-select B entry were
observed, but the bounded run ended without an explicit B end marker.
Environment-vs-character asset identity and connected topology remain
insufficient for a safe-aspect failure predicate. `CAP_CONFIDENCE: LOW`; no
numeric production cap is promoted and no production fix is implemented.

Artifact: `/tmp/p08-tintro-authoritative-recapture-20260831T071206Z/`; manifest
SHA `72aa50123e9ed4914bd2c47b71d50e37df35dbe3a463622b36359f89c5dfcde1`.
`NEXT_SINGLE_ACTION`: add a bounded authoritative environment asset/group
discriminator at the same CPU draw seam and rerun to an explicit Scene-B end
marker; do not implement a production cap.

### P08 T_INTRO centered 16:9 presentation candidate (2026-08-31; deferred)

An experimental centered black-pillarbox policy above 16:9 was implemented
and user-tested acceptable, but is explicitly deferred. Its Tooie activation
hook, RT64 presentation extension, and deterministic rectangle test were
removed during the P08 closure cleanup. It is not current production behavior;
the committed dynamic T_INTRO source-aspect projection fix remains unchanged.

Static validation of the removed candidate had passed before deferral:
`TooieRecompiled` build, `window_display_test` 38/38, projection policy test
7/7, and `tintro_presentation_rect_test`. No GPU readback was used. Artifact:
`/tmp/p08-tintro-pillarbox-candidate-20260831T130202Z/` (SHA256SUMS SHA
`320b3cfe86ec3625615dc270f063907f0b838baa2911b7c0222f76da39cbd4de`).

`PILLARBOX_CANDIDATE_FINAL_STATUS: DEFERRED_AND_REMOVED_FROM_WORKTREE`.
`NEXT_SINGLE_ACTION`: investigate animated/swaying model-part glitches.

### P08 closure cleanup (2026-08-31)

The obsolete P08 diagnostic layer in `boot_hw_stubs.cpp`, two unreferenced
P08-only offline probes, the repository-local capture tree, and all
candidate-specific RT64/Tooie code were removed. The reusable AngryLyon
control instrumentation remains because its oracle scripts depend on it.
The permanent T_INTRO projection regression test remains. Confirmed open bug
queue: (1) animated/swaying model parts — root `UNKNOWN`, priority NEXT; (2)
intermittent audio “tic tic” — root `UNKNOWN`, after bug 1; (3) T_INTRO
ultrawide above 16:9 — deferred, proven authored-extent residual.

## P09 backpack flap witness checkpoint (2026-09-01)

The P09 bounded diagnostic is implemented at the Tooie source layer.
`TOOIE_DIAG_BACKPACK_FLAP=1` retains the resolved entry-table probe inside
`babackpack_entrypoint_3` (overlay section 113, entrypoint 3), rather than the
bypassed address-map key `0x80800320`. It now also wraps the proven live caller
`func_80092444` at guest callsite `0x80092710` and observes only its
`LOOKUP_FUNC(0x80084500)` resolution. The source-owned linker wrapper records
the resolved pointer, symbol, `a0`–`a3`, context, `sp`, and `ra`, then delegates
the original caller and lookup unchanged. It is default OFF, CPU-only, and
does not modify animation, interpolation, culling, model data, or generated
code. The static controller fields are `+0x15C`, `+0x160`, `+0x16C`, and
`+0x170`; exact model node, DL, matrix stack, submitted matrix, and RSP
ownership remain `UNKNOWN`. The status file flushes at startup, first caller,
first lookup, every 60 lookups, and shutdown; the bounded CSV/anomaly report
still flushes at sampler events plus anomaly/shutdown. The authoritative
zero-hit capture disproved the prior section-456 filter: overlay 456 is
`chblackeye`; `0x80084500` decodes to overlay 113. The corrected 2026-09-01
capture still had zero hits, with `LAST_FRAME=0` and no handler
dispatch/error witness. Static generated code shows the synthetic
`_babackpack_entrypoint_3` stub calling `recomp_syscall_handler`, while the
live caller emits the direct lookup. Thus 113/3 is proven as the stub identity,
not as an observed handler invocation.

The completed caller-local capture reached `func_80092444` 2,026 times but
recorded zero requests for `0x80084500`: `CALLER_FIRST_HIT=YES`,
`CALLER_HIT_COUNT=2026`, `LOOKUP_HIT_COUNT=0`,
`REAL_TARGET_ARGUMENTS_CAPTURED=NO`. This proves caller entry, not execution
of the internal guest callsite. Static control flow has an early conditional
at `0x800926EC` that skips `0x80092710` when
`MEM_BU(MEM_W(a0, 0x50), 0x11)==0`; earlier `+0xA2`/`+0x18` gates can also
return. The diagnostic now records the caller `a0`, `+0x50` data pointer, and
those gate bytes at entry, with no new global lookup hook.

The stale section-90 `bsbflap_update` hook and the bypassed address-map hook
were removed. The direct probe lives around the resolved
`entry.entrypoints[entry_index]` call, so it survives overlay load/reload and
does not affect other syscall behavior. The caller-local probe is the next
runtime boundary; `babackpack_entrypoint_3` has not yet received a runtime
call or instance. Exact flap transform/interpolation ownership remains
`UNKNOWN` and no production root is proven.

Canonical evidence: [[plan/evidence/p09-backpack-flap-witness]].

`NEXT_SINGLE_ACTION: run one new caller-entry gate capture below; do not repeat
the handler-filter capture or the prior identical caller-local run`.
The next capture writes `/tmp/tooie-backpack-flap-*` directly.

```sh
cd /home/wesley/projetos/N64-Recomp/TooieRecomp && \
rm -f /tmp/tooie-backpack-flap-* && \
TOOIE_DIAG_BACKPACK_FLAP=1 ./build/TooieRecompiled
```

**Updated caller-entry capture result (2026-09-01):** the run reached the
caller wrapper 1,299 times, with `LOOKUP_HIT_COUNT=0`. The status artifact
persisted only `CALLER_FIRST_A0=0x801cb580`, `CALLER_LAST_A0=0x801c6580`,
`CALLER_FIRST_T6=0x801cb94c`, and first gates `+A2=0`, `+18=0`, `+11=1`;
the witness CSV is header-only (`SAMPLE_COUNT=0`, `ANOMALY_COUNT=0`). Thus
the requested all-hit gate distributions, pointer clusters, and dominant
runtime path are not captured. The first recorded branch is
`+A2=0` → `0x80092474` → `func_800A89F8` → `func_800A4C68` /
`func_800A940C` → `+18=0` → return at `0x8009271C`, with no transform or draw
operation reached before the return. The `+0x50` value remains only a
per-object secondary state-pointer candidate; backpack ownership is unknown.
Do not add another hook or treat `0x80084500` as the observed flap path.

### P09 caller population diagnostic repair (2026-09-01)

The existing `func_80092444` caller diagnostic now persists one completed
tuple per hit to `/tmp/tooie-backpack-flap-caller.csv`: hit index/frame, `A0`,
`+0x50`, `+A2`, `+0x18`, `+0x11`, deterministic branch class, lookup
eligibility/execution, and lookup request/resolved target when observed. It
uses the existing caller and lookup wrappers only; generated code and
production rendering are unchanged. The first row flushes immediately,
subsequent rows every 60 hits, and the normal shutdown path flushes/closes the
file. Status now reports bounded unique-pointer counts, gate histograms,
branch counts, skip counts, lookup-eligible/executed counts, and the dominant
branch. Static control flow is captured at
`docs/plan/evidence/func-80092444-control-flow.txt`.

`CALLER_DIAGNOSTIC_REPAIRED: YES`; `BRANCH_CLASSIFICATION_IMPLEMENTED: YES`;
`PER_HIT_CLUSTER_KEYS_CAPTURED: YES`; `CSV_FIRST_HIT_PERSISTENCE: YES`;
`CSV_PERIODIC_PERSISTENCE: YES`; `STATUS_HISTOGRAMS_IMPLEMENTED: YES`;
`STATIC_CONTROL_FLOW_MAP_COMPLETE: YES`; `DIAGNOSTIC_DEFAULT_OFF: YES`.

Validation passed: `TooieRecompiled` build, `window_display_test` 38/38,
`tintro_projection_policy_test` 7/7, and `git diff --check`. The next single
action is one fresh runtime population capture:

```sh
cd /home/wesley/projetos/N64-Recomp/TooieRecomp && \
rm -f /tmp/tooie-backpack-flap-* && \
TOOIE_DIAG_BACKPACK_FLAP=1 ./build/TooieRecompiled
```

After that capture, analyze the generated files directly. Do not add another
hook or infer backpack ownership before clustering the caller rows.

### P09 stage-counter capture — identity observation gap (2026-09-01)

The repaired nested-draw capture reached the generic render stages but not a
backpack-filtered scope. D674C resolved 28,639 assets, all classified as
nonmatching; DE448 and DE498 had 17,139 and 17,955 total hits respectively,
with zero filtered hits. E2AA4 had zero total hits. E2180 had 1,103 total hits,
all rejected as `NO_ACTIVE_MODEL_CONTEXT`; no sample or anomaly was written.
The draw-scope stack reported zero overflow events, so this is not a nesting
regression.

The existing D674C observer was extended with a bounded top-frequency table of
actual nonmatching asset IDs/model pointers and explicit
`DRAW_CONTEXT_MAX_DEPTH`/`DRAW_CONTEXT_STACK_OVERFLOW` fields. This adds no
hook and makes the next capture discriminate wrong presumed IDs from a cached
or unobserved model association. No production fix is authorized.

```text
RUNTIME_WITNESS_CAPTURED: NO
LAST_PROVEN_BACKPACK_STAGE: NONE
FIRST_MISSING_BACKPACK_STAGE: D674C_ASSET_ID_MODEL_BIN_ASSOCIATION
FILTER_PROPAGATION_NOW_WORKING: NOT_RUNTIME_PROVEN
CANDIDATE_IMPLEMENTED: NO
NEXT_SINGLE_ACTION: rerun the short stage-counter capture with the repaired existing observer
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

### P09 repaired live Banjo model witness (2026-09-02)

The repaired player scope is proven and the capture is usable: 2,067 accepted
DE448 samples, all at guest callsite `0x800926D8`, with scope depth 1 and no
overflow/wrong-callsite/other rejects. The accepted model sequence is
`0x62A` / `0x801BEBE0` / 68 bones for frames 486–609, followed by sustained
normal-walk `0x62B` / `0x8020E340` / 66 bones for frames 610–2552. The live
normal-walk Banjo body is therefore `0x62B` (HIGH confidence); `0x61F` is
REFUTED for this capture.

The normal-walk snout is `GEOCMD_02` and uses `func_800E2AA4` →
`func_800E2180`. Its static node is model-relative `0x14960`; bawobble still
produces hip bone 3 and reaches snout bone 42 through the parent chain. The
BanjoRecomp ExtraSegments/CPU-skinning/transform-ID path is a structural
comparison only and is not a proven Tooie root.

A default-off witness is implemented: E0B8C records bone IDs 3/42, and E2180
records the player-scoped `0x62B` snout node's guest previous/current matrices
to `/tmp/tooie-secondary-sway-bones.csv` and
`/tmp/tooie-secondary-sway-snout.csv`. No production fix was made.

```text
CAPTURE_USABLE: YES
LIVE_BANJO_BODY_RESOURCE: 0x62B
LIVE_BANJO_MODEL_BIN: 0x8020E340
BANJO_SNOUT_RENDER_CLASS: GEOCMD_02
BANJO_SNOUT_USES_E2AA4_E2180: YES
CURRENT_ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
SNOUT_TRANSFORM_WITNESS_IMPLEMENTED: YES
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_SINGLE_ACTION: run the new default-off bone/matrix witness once and inspect its two CSVs
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```
## P09 — SINGLE_G_MTX_IDENTITY_WITNESS_V2 (2026-09-03)

The diagnostic repair is complete and statically validated. The V1 capture is
usable only as proof of the old witness bug: `0x8007BCB0` was never the
matrix cursor, and pre-call `mlMtxApply` `ctx->r5` was not its destination.

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V2
PRIMARY_WITNESS: SNOUT_DCF48_SINGLE_G_MTX
MATRIX_CURSOR_SOURCE: D_8007BCD0
FUNC_80019AA0_OUTPUT: post-call D_8007BCD0 (old cursor + 0x40), after write
MLMTXAPPLY_ENTRY_A0: guMtxF2L source
MLMTXAPPLY_REAL_DESTINATION: D_8007BCD0 loaded at wrapper entry
G_MTX_OPERAND: DCF48 node + 4 / mlMtxApply source pointer
TRACE_TOKEN_COUNTERS: implemented
ROOT_CLASS: SINGLE_G_MTX_IDENTITY_UNRESOLVED
ROOT_CONFIDENCE: LOW
MISSING_GROUP_CAUSAL_STATUS: HYPOTHESIS
PRODUCTION_FIX: NOT_JUSTIFIED
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_SINGLE_ACTION: run one V2 capture and correlate guest matrix rows with RT64 identity rows
GPU_READBACK_USED: NO
```

The V1 NaN/Inf result is invalidated by the pointer bug and must not be used
as game evidence. No production fix, commit, or push.

## P09 — V2 runtime arbitration (2026-09-03)

The post-build V2 capture is usable and confirms the RT64 primary key:
`mlMtxApply` source pointer equals the raw `G_MTX` operand for 3,063 of 3,064
enqueued snout tokens. The F2L destination equals that operand in 0 samples.
The captured post-call `D_8007BCD0` composition field is also not the F2L
source and contains a trailing NaN in every persisted composition row. This
is a diagnostic composition-witness/dataflow problem, not guest-animation
evidence. RT64 AUTO identity is observed, but no `MATCH` event or previous
guest matrix was captured, so semantic continuity remains UNKNOWN.

```text
V2_CAPTURE: POSTDATED_AND_USABLE
RT64_PRIMARY_KEY: PROVEN_CORRECT
GUMTXF2L_SOURCE_EQUALS_COMPOSED_OUTPUT: NO
RT64_MATCH_CONTINUITY: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
PRODUCTION_FIX: NOT_JUSTIFIED
NEXT_SINGLE_ACTION: resolve the actual 19AA0/F2L source dataflow before another capture
GPU_READBACK_USED: NO
```

## P09 — V3 real guest matrix / match seam (2026-09-03)

The preserved V3 capture proves the corrected guest matrix path:
`DCF48/ADE2C -> 19AA0 -> D_8007BCB0 -> guMtxF2L -> G_MTX -> RT64`. The
composed source matches in 512/512 bounded rows, the F2L destination equals
the G_MTX operand in 512/512 rows, and all captured 16-float matrices are
finite with stable positive determinant. The V1/V2 NaN observation is
superseded.

The RT64 match artifact is not yet a semantic previous-frame result. Its
`UNMATCHED/NO_MATCHTRANSFORM` rows are emitted by the diagnostic flush with
hard-coded previous sentinels; the actual failed `matchTransform()` callback
arguments are not captured. No RT64 identity/interpolation or missing-group
root is proven.

```text
V3_GUEST_MATRIX: VALID
G_MTX_SUBMISSION: PROVEN_CORRECT
RT64_PREVIOUS_MATCH: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
PRODUCTION_FIX: NOT_JUSTIFIED
NEXT_SINGLE_ACTION: capture actual matchTransform callback arguments and token-predicate rejection reason before flush
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
```

## CURRENT (2026-09-03) — P09_V5_REAL_CALL_MATCH_RESULT

V4's zero CALL/candidate rows were caused by diagnostic workload ownership:
`matrixCommon` carries `state->workloadId`, while `setVertexCommon` supplies a
zero `workload.workloadId`. V5 preserves the non-zero matrix identity,
separates matrix/vertex/match workload fields, and labels finalization-only
rows as synthetic. No graphical root is established and no production fix is
authorized.

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V5
WORKLOAD_BINDING: MATRIX_WORKLOAD_PRESERVED
REAL_UNMATCHED_REQUIRES_REAL_MATCH_DECISION: YES
REAL_MATCHTRANSFORM_WITNESS_RETAINED: YES
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: identify the live Kazooie feather model/bone/transform-owner/G_MTX and reuse the generic V5 match witness
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

The V5 capture is post-executable and usable. Workload binding now reaches
the real RT64 path: 5/5 call matches, 5/5 eligible transform prerequisites,
77 valid candidates, 34 real `matchTransform` matches, and 5 real
`ONE_TO_ONE_PREVIOUS_ALREADY_USED` rejections. The 2,340 finalization rows
are synthetic cleanup. The two strict context-key mismatches are explained by
RT64 call-hash pairing and transform-pair observation identity, not a proven
AUTO identity failure. No snout production fix is justified; switch the next
diagnostic target to Kazooie head feathers while carrying Banjo.

## CURRENT (2026-09-04) — P09_KAZOOIE_FEATHER_V5_FULL_CHAIN

The prior SIMPLE experiment was `ROOT_ONLY`: it changed only the selected
`0x62B/+0x124A0` index-30 / anim-103 match. The user's result was `SAME`, which
refutes that narrow experiment only. It does not test distal bones 104/105 or
the other visible feather identities.

The V5 diagnostic statically recognizes the seven proven live `0x62B`
identities, retains the V4 token/match witness, and emits
`/tmp/tooie-kazooie-feather-postmatch.csv`. The fresh no-interpolation capture
corrected the runtime state: only boneAnimID 103 reached the token/match and
override path. `secondary_sway_note_dcf48()` accepts the three Primary classes
but rejects `Sibling62B`, so 79/100/101/102/104/105 never entered the hook.
The V5 full-chain runtime claim is therefore superseded; no-interpolation was
proven only for 103 and the visual result `SAME` does not close temporal
interpolation.

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V5
FULL_CHAIN_V4_TOKEN_SUPPORT: STATIC_ONLY; RUNTIME_COVERAGE_INCOMPLETE
POST_MATCH_MATRIX_WITNESS_IMPLEMENTED: YES
INTERPOLATION_FRACTION_CPU_OBSERVABLE: YES (curFrameWeight)
FULL_CHAIN_NO_INTERP_AB_IMPLEMENTED: PARTIAL (bone 103 runtime-covered)
IDENTITY_MATCHING_UNCHANGED: YES
GUEST_MATRICES_UNCHANGED: YES
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES (one no-interpolation run only)
NEXT_SINGLE_ACTION: repair the diagnostic DCF48 sibling acceptance gate; do not request an identical capture
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[plan/evidence/p09-kazooie-feather-v5-full-chain]],
[[reverse-engineering/systems/transform-interpolation]],
[[reverse-engineering/characters/kazooie]].

## CURRENT (2026-09-05) — P09_V7_FRESH_CAPTURE_STATE_C

The fresh V7 launch used the current direct build and produced the status
heartbeat, proving enablement and raw hook reachability. It produced no
combined CSV: all six target record counters and all tracked counters are zero,
with `OBSERVED_TARGET_MASK=0x00`. Per the V7 stop rule, this is state C: the
earliest failed gate is target/filter correlation. No append, fullSync slice,
CPU upload, join, render-selection, or renderer root can be classified.

```text
V7_RUNTIME_STATE: C
V7_ACTIVATION_VALID: YES
RAW_APPEND_HOOK_COUNT: 3104629
RAW_FULLSYNC_HOOK_COUNT: 1799
RAW_RENDER_SELECTION_HOOK_COUNT: 3929
TRACKED_APPEND_COUNT: 0
TRACKED_HANDOFF_COUNT: 0
TRACKED_RENDER_SELECTION_COUNT: 0
CSV_PRESENT: NO
CAPTURE_USABLE: NO
CAPTURE_COMPLETE_FOR_CAUSAL_DECISION: NO
MANDATORY_TARGET_COVERAGE_COMPLETE: NO
JOIN_592_PROVENANCE_STATUS: INCOMPLETE
JOIN_54C_PROVENANCE_STATUS: INCOMPLETE
JOIN_564_PROVENANCE_STATUS: INCOMPLETE
CPU_HANDOFF_STATUS: UNKNOWN
FIRST_UNEXPLAINED_LAYER: target/filter correlation gate
ROOT_CLASS: UNKNOWN (no target witness reached)
ROOT_CONFIDENCE: HIGH for gate identification; NONE for rendering root
NEXT_SINGLE_ACTION: resolve why all six target DLs are filtered out, then rerun one bounded V7 capture retaining status and CSV
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[plan/evidence/p09-v7-cpu-handoff-capture-analysis]]. Do not mix
this run with V6/V6B/V6C rows.

## CURRENT (2026-09-05) — V7 EXACT VALIDATION BLOCKED BY MODEL BRIDGE

The first post-repair V7 capture produced both artifacts and reached all three
raw RT64 hook sites. It found 11,740 low-page candidates, but every candidate
was rejected by exact validation because the independent model-bin bridge
reported `model_bin=0`. No candidate entered causal append, handoff, coverage,
or renderer analysis. Existing model evidence shows raw getter rows with
`RESOURCE_ID=0x62A` that are normalized to live `0x62B` after metadata checks;
the V7 bridge currently accepts only raw `0x62B`, which is the next diagnostic
admission blocker.

```text
V7_RUNTIME_STATE: D_EXACT_VALIDATION_BLOCKED
RAW_APPEND_HOOK_COUNT: 4282916
RAW_FULLSYNC_HOOK_COUNT: 2284
RAW_RENDER_SELECTION_HOOK_COUNT: 4890
TARGET_CANDIDATE_COUNT: 11740
TARGET_VALIDATED_COUNT: 0
TARGET_REJECTED_AFTER_VALIDATION_COUNT: 11740
TRACKED_APPEND_COUNT: 0
TRACKED_HANDOFF_COUNT: 0
TRACKED_RENDER_SELECTION_COUNT: 0
OBSERVED_TARGET_MASK: 0x00
EXACT_TARGET_VALIDATION_RUNTIME_WORKING: NO
MANDATORY_TARGET_COVERAGE_COMPLETE: NO
CPU_HANDOFF_STATUS: UNKNOWN
FIRST_UNEXPLAINED_LAYER: exact target validation bridge
ROOT_CLASS: UNKNOWN (no validated target witness)
ROOT_CONFIDENCE: HIGH for gate identification; NONE for rendering root
NEXT_SINGLE_ACTION: repair raw-0x62A-to-live-0x62B model-bin admission, then run one bounded V7 capture
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[plan/evidence/p09-v7-cpu-handoff-capture-analysis]].

## CURRENT (2026-09-05) — V7 ADMISSION REPAIR VALIDATED

The state-C root was repaired in the RT64 target resolver: logical V6C DL
indices remain accepted, and resolved runtime DL addresses are matched through
the V6C model-bin entry layout's low-page offset. The append seam now records
aggregate filter stages and a bounded eight-entry rejection sample, including
explicit model-bin unavailability at that seam. These are diagnostic-only
changes; face/index/matrix/upload/render/synchronization behavior is unchanged.

```text
V7_TARGET_ADMISSION_REPAIRED: YES (static resolver repair)
APPEND_FILTER_STAGE_COUNTERS_IMPLEMENTED: YES
BOUNDED_TARGET_REJECTION_SAMPLE_IMPLEMENTED: YES (8 entries)
NEXT_CAPTURE_CAN_LOCALIZE_TARGET_FILTER_FAILURE: NO (identity collision remains)
V7_TARGET_MATCH_COLLISION_SAFE_FOR_CAUSAL_USE: NO
LOW_PAGE_MATCH_CAN_COLLIDE: YES (12-bit mask; differences of k*0x1000 collide)
FALSE_CANDIDATE_CAN_REACH_CAUSAL_CSV_AS_VALID_TARGET: YES
KNOWN_COLLIDING_NON_FEATHER_DLS_FOUND: UNKNOWN
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
TOOIE_DIFF_CHECK: PASS
RT64_DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: NO (capture blocked pending identity validation)
NEXT_SINGLE_ACTION: add an independent model/DL validation state before authorizing capture
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```
## CURRENT (2026-09-05) — V7 MODEL-BIN BRIDGE REPAIRED

The diagnostic model bridge keeps the proven raw `0x62B` identity and uses the
existing metadata normalization path for raw `0x62A` representations when the
model header proves the 66-bone `0x62B` shape. `ctx->r2` remains full-width through
the bridge; masking occurs only at guest-RDRAM/physical-address access seams.
Bridge counters and bounded first/last accepted raw, normalized, full guest,
and physical-base fields are emitted in the V7 status, and rejected getter
events clear the prior model bin.

```text
V7_EXACT_MODEL_BIN_BRIDGE_REPAIRED: YES
RAW_RESOURCE_0X62A_TO_LIVE_0X62B_NORMALIZATION_PROVEN: YES (metadata classifier)
USES_NORMALIZED_MODEL_IDENTITY: YES
RAW_0X62A_BLINDLY_TREATED_AS_0X62B: NO
MODEL_BIN_POINTER_FULL_WIDTH_PRESERVED: YES
ANY_ZERO_EXTENSION_RISK: NO
TARGET_VALIDATION_EXPRESSION_UNCHANGED: YES
LOW_PAGE_REMAINS_CANDIDATE_ONLY: YES
UNVALIDATED_CANDIDATE_CAN_REACH_CAUSAL_ANALYSIS: NO
MODEL_BRIDGE_COUNTERS_IMPLEMENTED: YES
NEXT_CAPTURE_CAN_DISTINGUISH_NORMALIZATION_FAILURE: YES
TARGET_534_EXACT_VALIDATION_REACHABLE_AFTER_REPAIR: YES
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
TOOIE_DIFF_CHECK: PASS
RT64_DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES (one bounded V7 capture)
NEXT_SINGLE_ACTION: run the authorized bounded V7 capture and retain status plus CSV
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```
## CURRENT (2026-09-05) — V7 FIRST CAPTURE STILL BLOCKED BEFORE MODEL IDENTITY

The bounded capture after the bridge repair produced both V7 artifacts and
postdates the rebuilt executable. It is usable for admission diagnostics, but
`CAPTURE_COMPLETE=0` and no exact target entered the causal pipeline. The CSV
has 47,574 candidate rows across five targets; every row reports
`model_bin=0x0` and `validation_reason=1`. The bridge emitted zero raw
`0x62A`/`0x62B` classifications, zero metadata normalizations, zero accepts,
and 1,786 rejects. This run therefore did not exercise the proven
raw-`0x62A`-to-live-`0x62B` path; the schema cannot distinguish the reject
subcause before classification. No downstream handoff or renderer claim is
authorized.

```text
V7_RUNTIME_STATE: E_EXACT_VALIDATION_BLOCKED_NO_MODEL_IDENTITY
CAPTURE_USABLE: YES for admission diagnostics; NO for causal handoff analysis
CAPTURE_COMPLETE: 0
RAW_APPEND_HOOK_COUNT: 5214884
RAW_FULLSYNC_HOOK_COUNT: 2489
RAW_RENDER_SELECTION_HOOK_COUNT: 5223
TARGET_CANDIDATE_COUNT: 47574
TARGET_VALIDATED_COUNT: 0
TARGET_REJECTED_AFTER_VALIDATION_COUNT: 47574
MODEL_GETTER_RAW_62A_COUNT: 0
MODEL_GETTER_RAW_62B_COUNT: 0
MODEL_GETTER_NORMALIZED_62B_COUNT: 0
MODEL_BIN_BRIDGE_ACCEPT_COUNT: 0
MODEL_BIN_BRIDGE_REJECT_COUNT: 1786
TRACKED_APPEND_COUNT: 0
TRACKED_HANDOFF_COUNT: 0
TRACKED_RENDER_SELECTION_COUNT: 0
OBSERVED_TARGET_MASK: 0x00
EXACT_TARGET_VALIDATION_RUNTIME_WORKING: NO
UNVALIDATED_TARGET_USED_CAUSALLY: NO
MANDATORY_TARGET_COVERAGE_COMPLETE: NO
CPU_HANDOFF_STATUS: UNKNOWN
FIRST_UNEXPLAINED_LAYER: model-bin bridge before exact target validation
ROOT_CLASS: UNKNOWN (no validated target witness)
ROOT_CONFIDENCE: HIGH for bridge-gate failure; NONE for rendering root
NEXT_SINGLE_ACTION: resolve the 1,786 bridge rejects and rerun one bounded V7 capture retaining status plus CSV
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[plan/evidence/p09-v7-cpu-handoff-capture-analysis]].

## P09 V7 durable-bin bridge repair — state F (2026-09-07)

V7 model context is now published at `secondary_sway_register_durable_bin()`
after its existing metadata proof of normalized `0x62B` (magic `0xB`, gtype
flag clear, 66 bones). This is the sole authoritative identity source. The
publisher stores a sign-extended guest `uint64_t`, records the physical base,
increments a context generation, and clears a replaced or evicted matching
context. The `func_80092B04` getter wrapper remains non-authoritative and now
reports explicit argument, zero-return, resource, and metadata rejection
counters; it cannot admit or override V7 context.

RT64 status exposes durable registration, normalized-`0x62B`, publish/clear,
generation, last-context, and getter-reason counters. Exact validation remains
`current_dl == physical(model_bin) + 0x2670 + logical_index * 8`; low-page
matching remains candidate-only and unvalidated candidates cannot reach causal
analysis. No game launch or V7 capture was performed. Build, 38/38 window
tests, 7/7 projection tests, both `git diff --check`, and `graft build` pass.

## P09 V7 fresh capture gate result — state G (2026-09-07)

The user-generated post-repair V7 capture is fresh, enabled, and schema-valid. The durable seam is working: 1,449 registrations, 1,297 normalized `0x62B`, 1,297 context publications, zero clears; last guest bin `0xFFFFFFFF80229070`, physical base `0x229070`. Getter rejection reasons are now visible, with `GETTER_REJECT_ARG_FILTER` first.

Exact target admission remains blocked. There are 5,296 candidate records and zero validated records: 2,526 reason-1 zero-model rejects and 2,770 reason-2 full-address DL mismatches. Candidates cover `OWN_105_0x534` (1,879), `JOIN_DISTAL_0x54C` (1,906), `OWN_104_0x57C` (1,216), and `OWN_103_0x5AA` (295); no `JOIN_DISTAL_0x564` or `JOIN_ROOT_0x592`. `CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO` and `OBSERVED_TARGET_MASK=0x00`; downstream CPU handoff and join/render conclusions remain unknown by the stop rule. Next single action: repair candidate-to-full-address admission, then repeat one bounded V7 capture. No code or GPU changes were made in this analysis turn; no commit or push.
## P09 V7 fresh capture after exact direct-DL admission repair — state H (2026-09-07)

Fresh schema/version artifacts postdate the executable. Exact direct admission is PROVEN: 1485 normalized 0x62B publications and 40124 exact matches, with all six target bits observed (0x3F). Per-target exact counts: 0x534=11464; 0x54C=5732; 0x564=5732; 0x57C=5732; 0x592=5732; 0x5AA=5732.

Recorded bounded pipeline coverage is 448 APPEND, 96 INITIAL_HANDOFF_OK (16 per target), and 32 RENDER_SELECTION. Face range/content and velocity-zero checks pass for sampled handoffs. All handoffs report byte_mismatch=3, whose combined counter lacks a component breakdown; CPU payload equality and first divergence are therefore UNKNOWN. JOIN expected-role provenance remains unresolved, keeping CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO and preventing promotion of a later-renderer root.

NEXT_SINGLE_ACTION: isolate byte-mismatch components and resolve JOIN expected-role provenance, then rerun one bounded V7 capture. Analysis only; no production fix, GPU readback, synchronization, commit, or push.
