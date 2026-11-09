---
title: "Perfis de qualidade e orçamento da atmosfera"
description: "Configurações propostas, fallback por capacidade e controle do custo combinado sem alterar gameplay."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, quality, performance, accessibility]
---

# Perfis de qualidade e orçamento da atmosfera

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Nomes e valores são proposta de produto, não opções existentes na UI.
Ver [guia](README.md), [roteiro](implementation-roadmap.md) e [validação](validation.md).

## Separar escolhas

Pacote de texturas, direção artística e qualidade técnica são escolhas independentes. Desligar atmosfera não
desinstala texturas; escolher versão leve não força outra resolução, FPS, distância de objetos ou API. Uma falha
de recurso opcional não deve desativar todo o jogo nem sobrescrever preferências salvas de outra plataforma.

| Perfil candidato | Intenção | Limite |
| --- | --- | --- |
| Original | Efeitos novos desligados; pacotes de texturas continuam sob controle separado | Referência de comparação; não significa remover mods escolhidos pelo usuário |
| Leve | Partículas esparsas, filtros simples e fontes locais limitadas nos efeitos já aceitos | Adequado como candidato para hardware restrito; desempenho ainda precisa de medição |
| Atmosférico | Conjunto visual aprovado por região, com densidade e efeitos raster moderados | Só habilita capacidades demonstradas no backend/aparelho |
| Personalizado | Ajuste por efeito, hora fixa/ciclo e intensidade | Preserva escolhas com fallback explícito quando houver limitação |

Durante desenvolvimento, os efeitos novos começam desligados. Um preset de qualidade só pode ativar aquilo que já
foi aceito na combinação e plataforma. Não existe requisito de oferecer um preset “Ultra” com todos os recursos.

## Orçamento antes da produção

Escolher hardware, backend, resolução, taxa-alvo e cenas; medir base sem os efeitos novos. O orçamento de um frame
é `1000 / FPS-alvo` milissegundos: 60 FPS correspondem a aproximadamente 16,67 ms. Isso não é promessa de obter
60 FPS; se a base já exceder o orçamento, primeiro reduzir escopo/qualidade ou registrar ausência de margem.

Fixar limites de tempo CPU/GPU, memória/VRAM e carregamento para o recorte antes de expandir. Considerar custo de
classificar desenhos, consultar superfícies, atualizar partículas e multiplicar passes/viewports. Medir conjunto
real: tempos não podem ser simplesmente somados quando CPU/GPU sobrepõem trabalho ou passam a competir por banda.

Registrar mediana, P95/P99, travadas de compilação/carregamento e estabilidade prolongada. Preparar shaders e
recursos em fronteiras apropriadas quando viável; não esconder travadas mudando seed/cena de teste. Sem consultas
sincronizadas à GPU por partícula ou novo bloqueio do guest para extrair profundidade.

## Escala e fallback

| Custo dominante | Redução candidata antes de desistir do efeito |
| --- | --- |
| Sobreposição/transparência | Menos partículas e menor área coberta; preservar origem e distribuição |
| Composição de tela | Menor resolução do efeito com filtro adequado; sem alterar resolução principal |
| Água/reflexos | Ambiente aproximado e ondulações simples antes de reflexos em tela ou outra câmera |
| Sombras/feixes | Reduzir alcance/fontes ou desligar efeito local sem mudar luz/névoa independentes |
| Texturas | Versão 2× ou subconjunto menor, mipmaps e carregamento apropriado |
| Consultas e classificação | Cadastro explícito/cache por geração validado; sem suprimir trabalho original |

Não remover partículas de forma diferente a cada frame nem fazer qualidade oscilar. No primeiro conjunto, usar
qualidade fixa selecionável. Ajuste automático dinâmico fica adiado até justificar custo e política de histerese.
Desligar efeito, variar qualidade e trocar dispositivo invalidam históricos/recursos de forma controlada.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| QLT-01 | Texturas, atmosfera e opções gráficas existentes independentes; preset não modifica simulação ou preferências sem relação. |
| QLT-02 | Capacidades negociadas por backend/aparelho, com fallback por efeito e preferências persistidas sem corrupção. |
| QLT-03 | Baseline, limites e custo combinado definidos antes de expansão; tempos de frame e memória registrados. |
| QLT-04 | Qualidade fixa e transições estáveis, sem cintilação, alocações repetidas ou bloqueio de GPU/guest por efeito. |
| QLT-05 | Controles independentes de densidade, brilho/flashes, balanço e volume preservam conforto e legibilidade. |

Primeiro recorte: a mesma região com atmosfera desligada, um efeito e a combinação mínima, incluindo duas viewports.
Aceite de qualidade não comprova compatibilidade de formatos/recursos em plataforma diferente.
