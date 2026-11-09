---
title: "Materiais e relevo aparente das superfícies"
description: "Pedra, madeira, metal e gelo com resposta de luz controlada e extensão explícita do contrato raster."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, materials, normal-maps, rasterization]
---

# Materiais e relevo aparente das superfícies

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade média, com trabalho alto de integração. Ver [guia](README.md).

## Objetivo

Fazer madeira parecer madeira, pedra parecer pedra e metal/gelo refletirem luz de maneira contida. O desenho da
[textura restaurada](texture-restoration.md) continua a referência. Normal maps podem sugerir relevo sem aumentar
geometria; rugosidade controla dispersão do brilho. Nenhum dos dois altera silhueta ou colisão nesta proposta.

Evitar transformar todo piso em plástico brilhante. Materiais pintados e luz original exigem parâmetros moderados.
O molhado da [chuva](rain.md) só deve afetar materiais expostos e cadastrados.

## Estado do contrato e caminho candidato

A estrutura atual [ReplacementTexture](../../../lib/rt64/src/common/rt64_replacement_database.h#L71) define imagem,
hashes e opções de operação/alinhamento. Não define canais próprios para normal e rugosidade. A menção a PBR na
spec não prova que colocar imagens adicionais num ZIP habilita esses materiais.

Projetar associação versionada de material e canais ao desenho/contexto, compatível com packs apenas de cor.
Validar normais, coordenadas e orientação antes de escolher a base tangente para normal mapping. Texturas podem
ser repetidas, espelhadas, recortadas ou compartilhadas entre superfícies diferentes.

Mapas derivados de imagem por IA ou filtros são candidatos sujeitos a revisão: sombra pintada não é necessariamente
um buraco, linha escura não é necessariamente altura e cor clara não determina rugosidade. Confirmar relevo com luz
em movimento e comparar com original. Não prometer reconstrução física correta a partir de uma única imagem.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| MAT-01 | Fluxo de materiais demonstrado no shader ativo; pacote de cor antigo continua funcionando sem canais adicionais. |
| MAT-02 | UVs, espelhamento, normais e orientação corretos, sem relevo invertido ou emendas de iluminação. |
| MAT-03 | Brilho/relevo aprovados por material, sem plástico universal ou duplicação da luz já desenhada. |
| MAT-04 | Sem mudança de colisão/silhueta; fallback por backend e custo/formatos medidos. |

Primeiro recorte: uma parede de pedra e uma superfície de metal, cada qual com resposta distinta e comparáveis sob
uma fonte de luz controlada. Dependências: [iluminação](lighting.md), [texturas](texture-restoration.md) e
[validação](validation.md). Parallax/displacement não são requisitos do piloto.
