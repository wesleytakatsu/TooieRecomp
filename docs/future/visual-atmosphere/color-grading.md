---
title: "Cores e pós-processamento por ambiente"
description: "Ajustes moderados de paleta e contraste integrados ao clima e à hora visual, preservando HUD e leitura."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, colors, shaders, post-processing]
---

# Cores e pós-processamento por ambiente

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade inicial alta; encaixe raster e separação do HUD ainda precisam
de prova. Ver [guia](README.md), [arquitetura](architecture.md) e [iluminação](lighting.md).

## Melhoria proposta

Aplicar ajustes suaves de exposição, contraste, saturação e balanço de cor por ambiente. Luz quente nos templos,
interiores de mina acolhedores junto às lâmpadas, regiões frias com paleta própria e transições sem saltos. O ciclo
dia/noite fornece parâmetros graduais; chuva pode reduzir contraste solar em regiões expostas.

Não usar um filtro global que faça todos os mapas parecerem iguais. Escuridão artificial não substitui sombras,
e aumentar saturação não recupera detalhe de textura. Não prometer saída HDR só por usar buffers internos de maior precisão.

## Caminho técnico candidato

Identificar o estágio de composição ativo e a representação de cor em cada backend. Definir conversões de espaço
de cor e evitar correção dupla de gamma. Selecionar poucos parâmetros ou LUTs por perfil e interpolar seus pesos.
Máscaras devem separar mundo, céu, transparências e elementos de interface quando necessário.

O menu host e o HUD/legendas desenhados pelo próprio jogo podem seguir caminhos diferentes. Compor o menu depois
de um filtro não garante que o HUD guest ficou preservado. Confirmar ambos antes de escolher o ponto de integração.
Exposição automática não é requisito inicial; se adicionada depois, precisa de limites, adaptação lenta e histórico
por viewport para não oscilar com portas, textos, clarões ou split-screen.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| COL-01 | Perfil por região com referência nativa; cores importantes de pistas, coletáveis e personagens preservadas. |
| COL-02 | HUD, legendas e menus sem alteração indesejada; sem pretos esmagados, brancos cortados ou correção dupla. |
| COL-03 | Transições de região, clima e hora suaves; câmera submersa e J1/J2 tratados conforme seu contexto. |

Primeiro recorte: dois perfis moderados numa transição exterior/interior, com comparação nativa e imagem de referência.
Pré-requisitos: classificação de passes, cor e composição raster verificadas. Perfis em hora fixa bastam para o piloto;
[céu/neblina](sky-and-fog.md) e [ciclo solar](day-night-cycle.md) são integrações artísticas posteriores, não bloqueios.
