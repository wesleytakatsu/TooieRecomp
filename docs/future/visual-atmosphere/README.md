---
title: "Guia de melhorias visuais e atmosfera imersiva"
description: "Mapa dos planos de texturas, iluminação, água, clima e ambientação por cenário, sem ray tracing."
status: proposed
type: design-guide
project: TooieRecomp
updated: 2026-11-07
tags: [future, visuals, atmosphere, design]
---

# Guia de melhorias visuais e atmosfera imersiva

## Escopo e estado

Planejamento solicitado em 2026-11-07: deixar Banjo-Tooie mais bonito e imersivo, preservando o desenho original,
com atenção a chão, paredes e áreas não atendidas pelos mods existentes. Inclui chuva, passagem visual do tempo,
vento na vegetação, terra caindo dos tetos da mina, folhas ao vento e efeitos adequados a cada ambiente.

**Ray tracing está excluído.** As propostas usam rasterização, shaders, texturas, partículas e áudio ambiente.
Nenhuma melhoria nova desta pasta foi implementada, construída ou validada em jogo. Esta entrega autoriza e registra
documentação; não inicia implementação. As páginas são propostas de desenho, não contratos atuais do motor.

Classificação: arquitetura e técnicas candidatas são `HYPOTHESIS`; pontos de integração, custo e cobertura ainda não
demonstrados são `UNKNOWN`. `PROVEN` aplica-se somente aos fatos existentes com fonte indicada. Todos os novos
requisitos e testes de runtime começam em `NOT_IMPLEMENTED`.

## Direção artística

O alvo é uma restauração do estilo Rare: formas, paleta, símbolos e materiais reconhecíveis, com mais definição,
volume e vida. Cada cenário tem identidade própria. Evitar superfícies plásticas, excesso de brilho, escuridão que
prejudique navegação e densidade de partículas que esconda personagens ou coletáveis. Efeitos precisam ser opcionais.

## Guia das melhorias

Viabilidade é avaliação de engenharia, não comprovação por protótipo. “Alta” descreve a técnica e o recorte candidato;
não significa que já haja ponto de integração seguro ou desempenho suficiente no Tooie. Esses dois pontos precisam
de prova no caminho ativo. Trabalho inclui produção artística e validação.

| ID | Melhoria e documento | Viabilidade inicial | Trabalho previsto |
| --- | --- | --- | --- |
| TEX | [Restauração de texturas com IA](texture-restoration.md) | Alta; carregamento existente | Médio, com curadoria por material |
| COL | [Cores e pós-processamento](color-grading.md) | Alta | Médio |
| LIT | [Iluminação de cenários e personagens](lighting.md) | Média/alta para luz nativa; média para luz por pixel | Médio/alto |
| BLM | [Bloom seletivo](bloom.md) | Alta após identificar emissores | Médio |
| SHD | [Sombras de contato e oclusão ambiente](contact-shadows.md) | Média | Alto |
| MAT | [Materiais e relevo aparente](surface-materials.md) | Média; extensão necessária | Alto |
| WAT | [Água, reflexos e refração](water.md) | Média/alta para versão estilizada | Alto |
| RAI | [Chuva e superfícies molhadas](rain.md) | Média/alta com áreas de exposição | Médio/alto |
| DAY | [Passagem do tempo, dia e noite](day-night-cycle.md) | Média; exige coerência do cenário | Alto |
| WND | [Vento e movimento de árvores e objetos](wind-and-vegetation.md) | Média; classificação por instância pendente | Alto |
| DST | [Terra e poeira caindo do teto das minas](mine-dust.md) | Alta para emissores locais | Médio |
| LEF | [Folhas voando nas florestas](airborne-leaves.md) | Alta para partículas locais | Médio |
| PAR | [Neve, cinzas, vapor e outras partículas locais](local-particles.md) | Alta para variantes simples | Médio |
| SKY | [Céu, neblina e profundidade atmosférica](sky-and-fog.md) | Média/alta | Médio/alto |
| AUD | [Áudio ambiente integrado aos efeitos](ambient-audio.md) | Média; rota própria de efeitos a investigar | Médio/alto |
| INT | [Reações visuais ao contato com o ambiente](surface-interactions.md) | Média; contatos e materiais precisam ser identificados | Médio/alto, prioridade posterior |
| LFX | [Feixes de luz localizados](light-shafts.md) | Média em volumes delimitados | Médio/alto, prioridade posterior |

## Documentos compartilhados

- [Arquitetura e compatibilidade](architecture.md): relógio, perfis, integração gráfica, propriedade e plataformas.
- [Perfis e compatibilidade por cenário](scenario-profiles.md): onde cada efeito faz sentido e onde deve ser vetado.
- [Matriz de validação](validation.md): requisitos, discriminações e provas necessárias para aceitar uma melhoria.
- [Roteiro revisado de protótipos](implementation-roadmap.md): dependências reais, entregas mínimas e decisões de continuar, simplificar ou adiar.
- [Perfis de qualidade e orçamento](quality-presets.md): separar escolha artística, capacidade técnica e custo combinado.
- [Cogitações gerais do projeto](../implementation-ideas.md): outras propostas, independentes deste programa visual.

## O que já existe e o que falta demonstrar

| Base observada | Estado e limite |
| --- | --- |
| Pacotes de texturas `.rtz` e ZIP com raiz RT64 | `PROVEN` no contrato e validação Windows documentada; não prova suporte aos novos efeitos. Ver [modding](../../specs/Tooie/modding-and-extensions.md). |
| Iluminação dos modelos de jogador por cores nos vértices | `PROVEN` no [RE de iluminação](../../reverse-engineering/systems/player-lighting.md). Não equivale a iluminação moderna por pixel. |
| Interpolação, apresentação e widescreen | Contratos existentes em [gráficos](../../specs/Tooie/graphics-and-presentation.md). Novos efeitos devem respeitá-los. |
| Suporte pronto a normal maps e mapas de rugosidade em pacotes | `UNKNOWN` como integração completa; a estrutura [ReplacementTexture](../../../lib/rt64/src/common/rt64_replacement_database.h#L71) não define esses canais. A menção a PBR na spec não comprova um fluxo funcional. |
| Dump de texturas já convertido automaticamente para PNG | Não assumir: o [guia vendorizado](../../../lib/rt64/TEXTURE-PACKS.md) descreve dumps brutos e a limitação de decodificação. |
| Clima, calendário visual e classificações de árvores/tetos | `UNKNOWN` como pontos de integração; esta pasta não prova suporte existente nem ausência de efeitos nativos equivalentes. |

BT Reloaded declara cobertura “UI only”, mas o inventário local consultado também contém arquivos nas categorias
`Areas`, `Skybox` e `Objects`. É necessário auditar hashes e usos reais antes de selecionar lacunas. As extrações
Xbox locais são referências candidatas; não há correspondência automática comprovada com os materiais N64.

## Prioridades e frentes de execução futura

O [roteiro revisado](implementation-roadmap.md) substitui a sequência única anterior. Trabalhar um protótipo por
checkpoint; as frentes abaixo são independentes no planejamento e não exigem iniciar mudanças simultâneas.

| Frente | Sequência mínima | Expansão condicionada |
| --- | --- | --- |
| Texturas | 3–5 amostras catalogadas → substituição original sem IA → convencional/IA em 2×/4× → piloto de 20–30 | Outras famílias e mapas após padrão artístico, custo e cobertura aprovados |
| Atmosfera local | Uma região com identidade/máscara → poeira sob um teto → folhas com vento simples | Chuva com abrigo, árvore deformada e áudio, cada qual em recorte próprio |
| Render raster | Prova isolada de cor/profundidade/contexto/HUD → escolher um efeito de cada vez | Água inicial, cores, luz e céu em hora fixa; depois ciclo contínuo, materiais e sombras |
| Ideias adicionais | Contatos de superfície e feixes de luz ficam em prioridade posterior | Só iniciar após eventos, emissores, máscaras e orçamento necessários funcionarem |

Primeiro conjunto utilizável proposto: texturas piloto, uma região de poeira e uma de folhas, com configuração e
desligamento testados. Dia/noite, água avançada e materiais não são condições para concluir esse primeiro conjunto.
Hora, vento e clima fixos permitem validar efeitos isolados; integração entre canais é testada quando coexistirem.

Metas de FPS/memória e [perfis de qualidade](quality-presets.md) dependem de baseline no hardware alvo. Se a técnica
complexa falhar, simplificar ou adiar conforme o roteiro. Nenhuma estimativa de desempenho está comprovada aqui.

`NEXT_SINGLE_ACTION`: catalogar 3–5 texturas representativas de chão/paredes de Mayahem Temple, com imagem original
decodificada, hashes, usos e cobertura conferidos, preparando a substituição sem IA antes das variantes ampliadas.
