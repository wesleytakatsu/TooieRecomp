---
title: "Áudio ambiente integrado ao clima e aos cenários"
description: "Chuva, vento, folhas, terra e máquinas com mixagem contextual e preservação da música e cues nativos."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, audio, atmosphere, immersion]
---

# Áudio ambiente integrado ao clima e aos cenários

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Complemento de imersão proposto; integração de novos efeitos, espacialização
e mixagem ainda precisam de estudo. Ver [guia](README.md), [arquitetura](architecture.md) e
[áudio existente](../../specs/Tooie/audio-system.md).

## Atmosfera proposta

Chuva com atenuação ao entrar sob teto, vento que cresce nas rajadas, farfalhar ocasional junto de vegetação,
grãos caindo na mina, goteiras e vapor/máquinas em fontes identificadas. Regiões aprovadas podem ter ambiência
noturna discreta. Intensidade acompanha o contexto, sem manter vento forte numa sala fechada.

O alvo é preencher o ambiente sem mascarar passos, inimigos, diálogo, coletáveis e sinais de puzzle. Inventariar
sons nativos antes de adicionar camadas; uma chuva visual nova pode aproveitar som existente quando houver
correspondência confirmada e controle adequado.

## Caminho técnico e propriedade

O conteúdo `audio.json` documentado substitui música por sequência/canais; não demonstra uma API genérica para
novos efeitos de chuva ou folhas. Estudar criação de fontes nativas ou um barramento host separado com ciclo de
vida, ganho e sincronização próprios. Não reutilizar uma sequência musical arbitrária para tocar ambiência.

Associar eventos ao mesmo contexto das partículas; não criar uma voz por gota/grão. Loops têm emenda revisada,
fade de entrada/saída e limites de vozes. Desligar efeito ou trocar de mapa encerra suas fontes, sem som órfão.
Pausa e retomada seguem a política cosmética; efeitos não podem duplicar depois de reentrada ou troca de dispositivo.

Para split-screen, duas câmeras não criam duas cópias integrais do mesmo loop. Definir uma política explícita de
ouvintes: combinar contribuições por exposição/distância com normalização e limite de ganho. J1 abrigado e J2 sob
chuva é um teste específico; a escolha de mixagem precisa de escuta antes de virar contrato.

Fontes novas devem ser próprias ou possuir licença adequada. Manter áudios privados, extrações e pacotes existentes
fora do Git/release, conforme o fluxo atual. Controle de ambiência é independente do volume da BGM e dos efeitos
nativos; ativar atmosfera não deve silenciar música nem aumentar ganho total sem limite.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| AUD-01 | Fonte/região e exposição coerentes com efeito visual, com transição suave sob teto e entre meios. |
| AUD-02 | Sem duplicação por câmera/partícula, clipping, loops com cliques ou vozes órfãs em transições. |
| AUD-03 | Música, diálogo e cues nativos preservados; volume próprio e configuração mestre respeitados. |
| AUD-04 | Mixagem, pausa, dispositivo, limites de vozes e origem dos arquivos validados por plataforma. |

Primeiro recorte: loop discreto de chuva ou evento curto de terra na mina, escolhendo apenas um. Comparar com áudio
nativo, inclusive com dois ouvintes em ambientes distintos. Relacionados: [chuva](rain.md), [poeira](mine-dust.md),
[folhas](airborne-leaves.md) e [validação](validation.md).
