---
title: "P08 — Correção de bordas em widescreen: planos de culling nativos e auditoria de volume de visão"
description: "Correção de culling em widescreen (16:9) via ajuste temporário do aspecto do volume de visão nativo (0x8012D500+0xE8) durante a montagem dos vetores horizontais; censo de culling ws zerado; proteção de reprodução do attract demo; testes unitários e de integração PASS."
status: current
type: evidence
project: TooieRecomp
phase: P08
updated: 2026-10-22
tags: [p08, widescreen, frustum, culling, native-planes, view-volume, attract, demo, edge-geometry, census]
---

# Evidência P08: Correção de bordas em widescreen via planos nativos de culling (2026-10-22)

**Data de medição e ambiente:** 2026-10-22, host nativo Windows 11 x64, HEAD `a306a88` com alterações locais de trabalho não commitadas.

---

## 1. Objetivo

Investigar e resolver o desaparecimento de itens e paredes nas bordas laterais da tela quando o jogo é renderizado em proporção 16:9 (widescreen), conforme relatado pelo usuário (`USER_OBSERVED`, 2026-10-22: observado no Gameplay Demo 2 e com destaque na fase dos dinossauros, Terrydactyland).

O objetivo compreende:
1. Mapear estaticamente a arquitetura nativa do volume de visão do jogo, os pontos de teste contra planos de frustum e os efeitos colaterais de culling em estruturas de dados da simulação;
2. Desenvolver uma ferramenta diagnóstica de censo para quantificar rejeições indevidas de frustum em 16:9 ("ws culls");
3. Arbitrar hipóteses de correção no renderizador de modelos versus intervenção no volume de visão nativo;
4. Garantir total determinismo na simulação e preservar a sincronia dos demos gravados do attract mode;
5. Validar a solução em testes unitários e de integração de host.

---

## 2. Mapa estático do volume de visão e testes de plano (`PROVEN`)

A auditoria estática do código recompilado (`RecompiledFuncs/`) e desmontagem da ROM confirmou a arquitetura do volume de visão da engine de Banjo-Tooie (validada também por auditoria independente):

- **Ponteiro global do volume de visão (`0x8012D500`)**: Mantém o ponteiro do volume de visão da viewport ativa atual, espelhado da tabela de viewports `0x80127680 + 12 * idx`.
- **Layout estrutural do volume de visão**:
  - `+0x00`: Posição da câmera no mundo (`Vector3f`, float x, y, z).
  - `+0x0C`: Vetor direcional forward da câmera (`Vector3f`).
  - `+0x24 + i * 0x10`: 4 planos de frustum $\{n, d\}$ (normal tridimensional e distância escalar), onde $i = 0, 1$ são os planos horizontais (esquerdo e direito) e $i = 2, 3$ são os planos verticais (superior e inferior).
  - `+0xE4`: Campo de visão vertical FOV (`40.0f`).
  - `+0xE8`: Proporção de aspecto (padrão $4/3 \approx 1.3333334$, lido de `D_80125970`).
  - `+0xF8` / `+0x11C`: Vetores delimitadores horizontais.
  - `+0x104`: Vetor delimitador vertical.
- **Acessores do volume de visão**: Apenas 41 funções acessoras no intervalo `0x800E3900`–`0x800E4574` leem diretamente este ponteiro global.
- **Testes contra os planos de frustum**:
  - `0x800CAA24`: Teste de AABB float contra os planos de corte (usado para geometria de mapa).
  - `0x800CAB70`: Teste de AABB halfword contra os planos (usado em GeoCmd tipo 13).
  - `0x800CACEC`: Teste de esfera delimitadora contra os planos (modelos, atores e adereços).
  - `0x800CAD9C`: Teste de ponto simples contra planos (código morto na compilação final).
  - `0x800CAEA4`: Teste isolado contra o plano vertical 2.
- **Construção matemática dos vetores e planos**:
  - Os vetores horizontais são derivados de `+0xE8` nas funções `0x800CA1B0`, `0x800CA218` e `0x800CA2A8`, calculando o ângulo como $\text{aspecto} \times \frac{\text{FOV}}{2}$.
  - A função `0x800CAF34` constrói os 4 planos $\{n, d\}$ a cada frame diretamente a partir dos vetores horizontais e verticais, **sem ler** `+0xE8`.
  - A função `0x800CB124` gera a matriz de projeção gráfica a partir de `+0xE8`.
- **Modo wide-screen nativo do jogo (opção de menu)**:
  - A rotina `set_widescreen` (`0x800157E0`) grava o halfword em `0x800799B8`.
  - Leitores conhecidos:
    - `0x80015D14`: Multiplica o aspecto da viewport por constante em `0x80041710` e repassa ao volume via `0x80015CE8` / `0x800CA510`.
    - `0x800E42F0`: Altera os limites da projeção ortográfica do HUD de $\pm 608$ para $\pm 810$.
    - `0x800DF944`: Aplica fator de escala no eixo X aos modelos de elementos de HUD.
    - `gsattract_entrypoint_5`.
  - Esse modo é estritamente **anamórfico**: comprime tanto a cena quanto o HUD no quadro 4:3 para exibição esticada em televisores CRT 16:9.
- **Comportamento do renderizador de modelos (`0x800DE498`)**:
  - Testa a esfera delimitadora do modelo em `0x800DE6EC` via `0x800E3E8C` somente quando `D_8012C828 != 0`.
  - O resultado booleano é armazenado na global `0x8012C940`. Esse valor é consumido posteriormente pela lógica do jogo através de `0x800DF350` (pelos atores e módulos `sukickball`, `chpiranha`, `chcablespark`, `fxrain` e `fxleaves`).
  - Retorna o ponteiro do modelo em caso de visibilidade ou 0 em caso de culling.
  - Invoca callbacks registrados do ator antes (`D_8012C9A8`) e depois (`D_8012C9B0`) da emissão do desenho.
  - A função de limpeza `0x800DE318` zera as configurações a cada chamada, limpando `D_8012C844` (tabela de tags do GeoCmd tipo 10) e `D_8012C848` (zonas do GeoCmd tipo 15).
- **GeoCmd tipo 14 e interceptação via walker (`CORRECTED/SUPERSEDED`)**:
  - Os comandos GeoCmd tipo 14 (`0x800DDDC4`, `0x800DDF14`, `0x800DDFA0`) realizam gates de visibilidade por esfera. Eles são despachados pelo walker `0x800DE2A4` com chamada indireta `jalr` obtida através de `get_function`.
  - Portanto, wrappers registrados **são executados** normalmente. O comentário histórico anterior no repositório afirmando o contrário foi categorizado como `CORRECTED/SUPERSEDED`.
  - As subárvores desses comandos podem conter nós do tipo 10 (posições de tag consumidas por aproximadamente 61 sítios de lógica) e do tipo 15 (flags de zona). Consequentemente, expandir o culling simplesmente forçando o retorno $v_0$ cria o risco real de contaminar a simulação física e lógica. No attract mode, verificou-se que esses comandos tiveram 0 entradas.
- **Risco latente em expansões prévias (`STRONG_INFERENCE`)**:
  - A expansão de AABB `0x800CAA24` usada anteriormente em produção alterava o valor de $v_0$ de forma global. O mesmo canal de efeitos colaterais de tipos 10 e 15 existe sob os gates de GeoCmd tipo 13 (`0x800DDC28` / `0x800DDD14`).

---

## 3. Ferramenta de diagnóstico (`TOOIE_DIAG_WS_CULL_CENSUS`)

Para mensurar e discriminar as ocorrências de culling sem suposições, foi desenvolvida a ferramenta diagnóstica configurável por variável de ambiente (padrão desligada):

```powershell
$env:TOOIE_AUTO_START='1'
$env:TOOIE_DIAG_WS_CULL_CENSUS='C:\caminho\census.txt'
build-win\TooieRecompiled.exe
```

A ferramenta opera na fronteira de frame demarcada por `0x800E9F20` e totaliza por categoria ("kind"):
- `kind 0`: Chamadas sem tag contextual identificada.
- `kind 2`: Escopo `0x8011447C` (atores e adereços dinâmicos).
- `kind 3`: Escopo `0x800B97A4` (desenho puro de modelos e matrizes).
- `kind 10` / `11` / `12`: GeoCmd tipo 14 (gates de esfera).
- `kind 13`: Renderizador de modelos `0x800DE498`.
- `kind 14`: Escopo de ocupação e névoa `0x801143A4`.
- `kind 20`: Teste de AABB de mapa `0x800CAA24`.

Campos computados por frame:
- Contagem de chamadas (`calls`), aceitações (`acc`), rejeições (`rej`), rejeições que seriam aceitas em 16:9 (`ws`), modificações aplicadas (`applied`) e entradas em subárvores (`entries`).
- Linhas `X`: Registram a posição tridimensional $(x, y, z)$ e raio $r$ dos objetos rejeitados em 4:3 que deveriam ser visíveis em 16:9.
- Linhas `S`: Registram o estado do gerador de números pseudoaleatórios (RNG) em `0x8012C7F0` e a posição do jogador no espaço de mundo (sonda de integridade da simulação para testes comparativos A/B).

Componentes de apoio desenvolvidos:
- Cabeçalho puro de cálculo vetorial `src/game/tooie_ws_frustum.h`: Contém a matemática exata de expansão de planos, testes de intersecção com esfera, classificação de AABB e critérios de detecção de corte.
- Suíte de testes `tests/p08_ws_frustum_test.cpp`: 68 asserções cobrindo transformações, planos laterais e estabilidade numérica (desenvolvido por Haiku 5.5, 68/68 PASS).

---

## 4. Medições de linha de base (Attract Mode, 1600×900)

Sob execução do attract mode (`TOOIE_AUTO_START=1`) em janela widescreen 1600×900:
- **Rejeições indevidas de widescreen (`ws`) identificadas na base**:
  - `kind 13` (`0x800DE498`): **1323** eventos de rejeição indevida distribuídos por dezenas de modelos distintos. Esta é a **causa raiz dominante** de itens, Jinjos, portas e adereços desaparecendo nas bordas laterais da tela.
  - Chamadas sem tag (`kind 0`): **348** eventos (inclui o culling de seções de mapa em `gcmapsects`).
  - `kind 14` (`0x801143A4`): **10** eventos.
  - `kind 20` (`0x800CAA24`): **13366** eventos de AABB (já atendidos pela expansão prévia de mapa).
- **Validação da sonda de simulação**:
  - A trajetória do jogador ao longo do attract gravado gerou 1603 posições de coordenadas sem redundâncias consecutivas.
  - Duas execuções consecutivas com a ferramenta configurada em modo neutro (OFF) registraram sequências de coordenadas rigorosamente idênticas (1603/1603 posições coincidentes), comprovando que a sonda `S` é determinística e plenamente confiável para auditoria de divergência.

---

## 5. Tentativa 1: Desenho forçado no renderizador de modelos (`REFUTED`)

A primeira abordagem investigada buscou forçar o desenho em `0x800DE498` quando o objeto caísse na margem 16:9, restaurando o valor original da global `0x8012C940` e o registrador $v_0$ com margem lateral.

Resultados observados:
- **Com callbacks de ator ativos (`D_8012C9A8` / `D_8012C9B0`)**:
  A trajetória da simulação divergiu da linha de base na posição 1348 de 1603 da sonda (um pulo do jogador, diferença inicial de ~0,12 em z que cresce depois).
- **Com callbacks de ator forçados a zero**:
  A trajetória permaneceu idêntica à original na sonda, porém os testes visuais interativos conduzidos pelo usuário revelaram sérios defeitos no gameplay (`USER_OBSERVED`):
  1. Atores e inimigos ficavam congelados nas bordas da tela (a lógica interna continuava tratando-os como fora do campo de visão);
  2. Jinjos, itens colecionáveis e portas tornavam-se visíveis através de paredes sólidas e do chão;
  3. A escotilha de entrada de Jamjars apresentava renderização incorreta e oclusão corrompida.

**Conclusão epistemológica:** A tentativa 1 foi formalmente classificada como **`REFUTED`** como solução de produção. Foi mantida isolada no código como experimento desativado por padrão através da flag `TOOIE_TEST_WS_MODEL_DRAW_ASPECT=1`.

Adicionalmente, comprovou-se que a aplicação isolada da margem lateral (sem forçar o desenho no renderizador) mantém a trajetória do jogador rigorosamente idêntica ao original.

---

## 6. Solução atual: Planos nativos de culling (`PROVEN`)

A solução de produção definitiva atua diretamente no mecanismo matemático em que o jogo deriva seus planos de corte, tornando todos os testes nativos naturalmente conscientes do aspecto 16:9 sem recorrer a desvios artificiais de retorno $v_0$:

1. **Mecanismo de substituição temporária**:
   Durante a execução dos construtores de vetores de corte `0x800CA1B0`, `0x800CA218` e `0x800CA2A8`, o valor da proporção de aspecto armazenado em `+0xE8` do volume é temporariamente ajustado para:
   $$\text{aspecto}_{\text{temp}} = \text{aspecto} \times \left( \frac{\text{aspecto\_alvo}}{\text{aspecto\_origem}} \right) \times \text{margem\_lateral}$$
   O valor original de `+0xE8` é restaurado imediatamente após o término dessas rotinas.
2. **Preservação da matriz de projeção gráfica**:
   A função `0x800CB124` (que constrói a matriz de projeção enviada ao pipeline gráfico) continua lendo a proporção de aspecto base 4:3. Dessa forma, a expansão de campo visual para a apresentação final em tela cheia permanece sob controle limpo do runtime RT64, evitando distorções geométricas.
3. **Consistência em todos os testes nativos**:
   Todos os 41 acessores e todas as rotinas nativas de teste (`0x800CAA24`, `0x800CAB70`, `0x800CACEC`) passam a operar sobre planos de frustum calculados para a proporção real 16:9, exatamente como ocorreria no modo wide-screen nativo do jogo, porém sem o achatamento anamórfico do framebuffer e do HUD.
4. **Desativação de redundância nas expansões por teste**:
   Com os planos nativos ampliados ativos, as expansões isoladas de AABB (`0x800CAA24`) e esfera (`0x800CACEC`) fixam o fator de escala $k = 1.0$, impedindo ampliações duplas dos planos de teste.
5. **Resultado no censo diagnóstico**:
   A contagem de rejeições indevidas de widescreen (`ws`) caiu para **exatamente 0** em todos os tipos (`kind 13`, `kind 0`, `kind 14` e `kind 20`).
6. **Controle de ativação**:
   Ativado por padrão em builds de produção. Pode ser desativado via `TOOIE_TEST_WS_NATIVE_PLANES=0`, restaurando o comportamento de expansão isolada por teste anterior.
7. **Validação interativa em gameplay (`USER_OBSERVED`, 2026-10-22)**:
   O usuário executou a versão final em gameplay interativo (incluindo Terrydactyland e Gameplay Demo 2) e atestou: *"Parece tudo correto agora"*.

---

## 7. Proteção do attract mode (`PROVEN`)

A reprodução dos demos gravados do attract mode armazena entradas de botões e sequências gravadas originalmente na proporção 4:3 da época do Nintendo 64. A utilização de planos de culling largos de 16:9 durante o attract mode causava divergência de simulação na posição 1348 da trajetória gravada (provavelmente devido a pequenas alterações em gatilhos de zona ou ativação de entidades no campo visual ampliado).

Para garantir compatibilidade matemática e reprodutibilidade canônica absoluta:
- Foi adicionada uma guarda no seletor de planos nativos verificando o halfword de estado do demo em `0x80127632 == 5`.
- Quando um demo de attract está em reprodução, o aspecto mantido em `+0xE8` durante a construção dos planos não é expandido, preservando rigorosamente os planos 4:3 originais.
- **Resultado métrico comprovado (`PROVEN`)**: Com esta proteção, a trajetória de 1603 posições do jogador durante o attract mode executou com **0 divergências**, permanecendo 100% idêntica à execução original de referência.

---

## 8. Configurações de margem

O comportamento de expansão lateral é configurável através de variáveis de ambiente de teste, ativas apenas quando a proporção da janela for superior à de origem (em 4:3 o comportamento permanece inalterado):

- `TOOIE_TEST_WS_DRAW_MARGIN`: Fator de margem lateral aplicado à abertura angular dos vetores horizontais do frustum. Valor padrão: `1.15`. Limitado internamente ao intervalo seguro $[1.0, 2.0]$.
- `TOOIE_TEST_WS_DRAW_RADIUS_SCALE`: Fator multiplicador de raio empregado nas rotinas de teste de esfera delimitadora durante expansões manuais. Valor padrão: `1.25`. Limitado ao intervalo $[1.0, 2.0]$.
- Experimento secundário `TOOIE_TEST_WS_GEO_SPHERE_ASPECT=1` (padrão desligado): Realiza uma varredura adicional estritamente visual da subárvore do GeoCmd tipo 14 com as tabelas de tags (tipo 10) e zonas (tipo 15) zeradas. Não foi exercitado no attract mode.

---

## 9. Testes e integridade

A solução foi submetida à verificação completa do pipeline nativo de compilação e teste no Windows 11 x64:

```powershell
tools\windows\Build-Tooie.ps1 -Stage Test -AllTests
```

**Resultado dos testes:**
- Testes obrigatórios do host: **9/9 PASS**
- Testes extras estendidos: **17/17 PASS** (incluindo a nova suíte `p08_ws_frustum_test`)
- Testes pulados: 5 (alvos dependentes de driver gráfico físico SDL/D3D12/Vulkan, pulados intencionalmente por desenho de arquitetura)

**Identificador de integridade do executável:**
- Binário gerado: `build-win\TooieRecompiled.exe`
- Hash SHA-256: `0D44B01E52223B9579C19852CEA06EEEC7457913F14F33DA1A5DC63292C72B02`

---

## 10. Limitações e riscos latentes

1. **Ciclo de recálculo dos planos de visão (`HYPOTHESIS`)**:
   Os planos de frustum são recalculados pelo jogo no momento em que a câmera é montada ou atualizada. Redimensionar dinamicamente a janela do jogo durante a execução de uma fase só terá efeito visível no volume de corte na próxima montagem de câmera da engine.
2. **Paridade em Linux (`NOT_RUN`)**:
   As modificações foram desenvolvidas em C++ padrão sem dependências específicas de sistema operacional ou `#ifdef` de plataforma. O código é estritamente comum ao Linux e ao Windows, com expectativa de comportamento idêntico (`STRONG_INFERENCE`), porém a execução e build no host Linux não foram executadas nesta etapa (`NOT_RUN`, pendente de validação pelo usuário no PC Linux).
3. **Comportamento lógico de atores na borda**:
   O comportamento dos atores e objetos situados nas bordas do campo visual ampliado agora segue as rotinas do modo wide-screen nativo do código original do Banjo-Tooie, podendo diferir sutilmente do comportamento em 4:3 estrito (por exemplo, ativação de animações e lógica ao entrar no campo 16:9).
4. **Glitch de interpolação em cutscene (relato P09)**:
   Durante os testes visuais de gameplay, o usuário observou uma anomalia visual residual de interpolação em cutscenes com close-up de Banjo e Kazooie. Este comportamento não possui relação com os planos de culling aqui tratados e pertence ao escopo investigativo da fase P09.

---

## 11. Próxima ação singular (NEXT_SINGLE_ACTION)

`NEXT_SINGLE_ACTION`: Investigar o glitch de interpolação em cutscene relatado pelo usuário (close de Banjo/Kazooie, escopo P09), após a realização do commit autorizado da presente correção de bordas em widescreen.
