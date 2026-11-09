---
title: "Água estilizada com reflexos e refração"
description: "Ondas, profundidade, cáusticas e transição submersa por shaders raster, preservando a física nativa."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, water, shaders, rasterization]
---

# Água estilizada com reflexos e refração

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade média/alta para água estilizada; classificação das superfícies,
profundidade e encaixe dos passes ainda são `UNKNOWN`. Ver [guia](README.md) e [arquitetura](architecture.md).

## Melhoria proposta

Água mais viva, com duas escalas de pequenas ondas, brilho angular, transparência coerente, cor conforme profundidade
e refração suave do que está atrás. Acrescentar ondulações visuais de contato e cáusticas discretas no fundo quando
o ambiente permitir. Jolly Roger's Lagoon é candidata ao primeiro recorte, sem aprovar automaticamente todas as salas.

Preservar altura, correnteza física, colisão, nado, entrada/saída, fôlego, dano e eventos nativos. Ondas cosméticas
começam com amplitude que não crie diferença perceptível entre superfície visual e contato físico.

## Caminho incremental

| Nível | Técnica candidata | Limite |
| --- | --- | --- |
| Inicial | Normais animadas e mistura angular entre transparência e reflexo de ambiente coerente | Reflexo aproximado não reproduz todos os objetos dinâmicos |
| Intermediário | Cor por espessura/profundidade, refração e cáusticas em receptores escolhidos | Precisa de profundidade útil e mascaramento de superfície/fundo |
| Avançado | Reflexos em tela com fallback de ambiente | Conteúdo fora da tela e desoclusões não podem ser reconstruídos só pela imagem |
| Estudo separado | Segunda câmera para reflexo planar em superfícies selecionadas | Exige nova passagem, clipping e submissão de geometria adequada; custo maior |

Todas as opções são raster. Normais em camadas, Fresnel e composição de refração são técnicas descritas na
[referência NVIDIA](https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-19-generic-refraction-simulation).
A existência da técnica não prova disponibilidade imediata desses buffers no Tooie.

Identificar água por recurso, passe e contexto comprovados; não selecionar toda textura azul ou transparente.
Capturar fundo sem amostrar o próprio resultado, respeitar transparências e evitar refratar objetos que estão à
frente da superfície. Cáusticas precisam de receptores submersos corretos, não de um filtro aplicado ao HUD.
Interiores, água escura, poluída ou especial recebem perfis próprios; nem toda água deve ficar turquesa ou cristalina.

Ao mergulhar, trocar neblina, distorção e áudio conforme a câmera e o meio, com transição estável na linha d'água.
Uma câmera submersa e outra fora da água exigem composição independente no split-screen. Chuva pode gerar pequenas
ondulações na superfície exposta, sem transformar colisão de partículas em simulação física de fluidos.

As [reações de contato](surface-interactions.md) definem emissão única para passos, entrada e impactos adicionais.
Água inicial pode usar hora/ambiente fixos e efeitos de contato nativos; não depende de IA, dia/noite, chuva ou
novos callbacks de passo. Fundo e profundidade precisam pertencer à mesma viewport/amostra de apresentação,
inclusive em frames interpolados; dados guest de outra idade não são substituto automático.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| WAT-01 | Água identificada e mascarada corretamente; objetos transparentes comuns não recebem shader aquático. |
| WAT-02 | Brilho, refração e cor aprovados acima/abaixo da superfície, com margem e objetos à frente/atrás corretos. |
| WAT-03 | Reflexo declara aproximações e possui fallback estável nas bordas, em cortes e em conteúdo fora da tela. |
| WAT-04 | Nado, altura física, eventos e efeitos nativos preservados; sem água duplicada ou cáustica fora do meio. |
| WAT-05 | Estado por viewport, limpeza em transições e custo/qualidade por backend demonstrados. |

Primeiro recorte: uma superfície, um fundo visível e uma entrada submersa em Jolly Roger's Lagoon. Começar pelo nível
inicial antes de reflexos em tela ou segunda câmera. Ver [validação](validation.md), [chuva](rain.md) e
[céu/neblina](sky-and-fog.md).
