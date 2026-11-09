---
title: "Roteiro revisado de protótipos visuais"
description: "Etapas independentes, dependências reais e critérios para continuar, simplificar ou adiar cada melhoria."
status: proposed
type: implementation-roadmap
project: TooieRecomp
updated: 2026-11-07
tags: [future, visuals, roadmap, feasibility]
---

# Roteiro revisado de protótipos visuais

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Revisão do [guia](README.md) em 2026-11-07. A revisão melhora o planejamento;
não comprova viabilidade no executável nem autoriza iniciar código nesta entrega. Ray tracing continua excluído.

## Correções da revisão

| Lacuna anterior | Ajuste adotado | Consequência |
| --- | --- | --- |
| Começar diretamente com 20–30 texturas antes de provar decodificação e fidelidade | Amostra de 3–5 texturas, substituição sem IA e comparação de métodos antes de ampliar o lote | Menor retrabalho e diagnóstico separado de problemas de extração, mapeamento e IA |
| Sequência única podia fazer água/partículas esperar dia/noite | Três frentes com dependências técnicas explícitas | Um ciclo solar complexo não bloqueia um pacote de texturas ou emissor simples |
| “Viabilidade alta” podia soar como integração já pronta | Separar técnica conhecida, ponto de integração e custo comprovado | Todo efeito novo precisa de prova no caminho ativo |
| Dependências cruzadas entre chuva, vento, água e folhas | Campo de vento compartilhado é base; deformação de árvores é consumidor opcional | Chuva pode usar vento fixo; não precisa esperar árvores animadas |
| Muitos efeitos competiam pelo mesmo orçamento | Perfis de qualidade, orçamento total e teste de combinação | A soma dos efeitos precisa caber no alvo, não só cada efeito isolado |
| Faltavam critérios claros de simplificação | Decisão por protótipo: continuar, simplificar ou adiar | Recurso avançado não vira condição para concluir o programa mínimo |

## Primeiras provas

| Etapa | Artefato verificável futuro | O que precisa ser demonstrado | Falha e alternativa |
| --- | --- | --- | --- |
| T0 | Catálogo de 3–5 texturas representativas de chão/paredes | Original decodificado e correspondência de hash/usos, com cobertura dos mods conferida | Sem correspondência confiável, resolver extração/mapeamento; não alimentar IA com captura da cena |
| T1 | Pacote de teste com imagem original sem ampliação | Mesmo desenho, alinhamento, alfa e uso esperado em jogo | Corrigir contrato de substituição; não tentar esconder erro por aumento de resolução |
| T2 | Comparação convencional e IA em 2×/4× nas mesmas amostras | Fidelidade, emendas e qualidade em movimento; escolher parâmetros por família de material | Preferir método convencional/2× ou preservar original nos casos piores |
| T3 | Piloto de 20–30 texturas de Mayahem Temple | Padrão artístico consistente, custo, carregamento e compatibilidade comprovados | Reduzir resolução/subconjunto antes de expandir para outra fase |
| A0 | Uma região identificada e visualização de máscara em desenvolvimento | Mapa, sala, geração, contexto e fronteira de frame confiáveis; desligado nativo | Sem identidade estável, manter cadastro menor ou adiar efeito contextual |
| A1 | Um emissor simples sob teto da mina | Profundidade, término, limite, pausa, J1/J2 e descarte corretos | Usar volume autoral limitado; se atravessar salas, não expandir |
| A2 | Folhas com campo de vento simples | Origem e trajetória coerentes, sem duplicação entre viewports | Movimento constante inicial é aceitável; árvore deformada não é pré-requisito |
| R0 | Prova no raster ativo com uma alteração visual reversível e isolada | Cor/profundidade/contexto alinhados, HUD guest/host preservados e custo medido | Se separação de passes falhar, adiar pós-processo seletivo; textura/partículas não dependem dele |

T0–T3 formam a frente de texturas. A0–A2 formam a base de atmosfera. R0 verifica a integração dos efeitos raster.
As frentes podem ser planejadas independentemente; uma implementação futura trabalha um objetivo delimitado por vez.
Não iniciar três mudanças técnicas simultâneas para cumprir esta tabela.

## Dependências para as melhorias seguintes

| Melhoria | Pré-requisito real | O que pode ficar para depois |
| --- | --- | --- |
| [Cores](color-grading.md) | R0 e contexto por região; hora fixa basta | Ciclo solar, chuva e água moderna |
| [Poeira e folhas](local-particles.md) | A0/A1, origem e limites; campo cosmético simples para folhas | Luz moderna, materiais PBR e áudio adicional |
| [Chuva](rain.md) | Base de partículas e exposição/abrigo; profundidade coerente | Molhado, árvores deformadas, água avançada e áudio novo |
| [Vento em árvores](wind-and-vegetation.md) | Instância/parte estável, máscara, pose e apresentação temporal corretas | Chuva, folhas e ciclo dia/noite completos |
| [Água inicial](water.md) | Identificação da superfície, parâmetros e caminho raster apropriado | SSR, segunda câmera, ciclo contínuo e chuva |
| [Luz e céu em hora fixa](lighting.md) | Componentes nativos identificados e paletas de referência | Luz por pixel, sol visível e sombras projetadas |
| [Dia/noite](day-night-cycle.md) | Dois ou mais estados de luz/céu legíveis e relógio cosmético testado | Água avançada e áudio novo; canais existentes precisam continuar coerentes |
| [Bloom](bloom.md) | R0, emissores identificados e máscara de oclusão válida | Normal maps, dia/noite e luz lançada pela fonte |
| [Sombras de contato](contact-shadows.md) | R0 e profundidade/projeção corretas por viewport | Filtragem temporal e sombras de luzes |
| [Materiais](surface-materials.md) | Contrato de canais, normais/UVs e resposta de luz raster demonstrados | Pacote completo de texturas, chuva e reflexos avançados |
| [Áudio ambiente](ambient-audio.md) | Rota de efeitos/vozes e contexto comprovados | Não bloqueia validar uma partícula inicialmente silenciosa |
| [Reações ao contato](surface-interactions.md) | Contato/evento nativo identificado, material e emissor | Pegadas persistentes, novo sistema de física ou mod de personagem externo |
| [Feixes de luz](light-shafts.md) | Fonte/abertura e oclusão identificadas; composição raster aprovada | Volume físico completo e sombras modernas de toda a fase |

Uma relação artística não é bloqueio técnico automático. Inicialmente usar parâmetros fixos de vento, hora e clima.
Quando dois subsistemas estiverem disponíveis, seu teste de combinação passa a ser obrigatório antes de ativá-los juntos.

## Programa mínimo e expansão

Primeiro conjunto utilizável proposto: piloto de texturas T3, uma região de poeira A1 e uma de folhas A2, com
configuração e desligamento testados. Pode ser entregue sem dia/noite, água moderna, normal maps ou novos sons.
Isso não reduz o escopo desejado: estabelece uma base pequena para expansão verificável.

Depois escolher uma entrega independente: chuva sob céu/abrigo, água inicial ou árvore com vento. Fechar cada uma
com comparação nativa e resultado por backend antes de combinar. Somente após duas atmosferas em hora fixa
funcionarem, estudar ciclo contínuo. Materiais avançados, reflexos caros e novas ideias ficam em prioridade posterior.

## Decisão ao terminar cada protótipo

- **Continuar:** identidade, aparência, legibilidade, regressões e orçamento demonstrados no recorte.
- **Simplificar:** a versão complexa falhou, mas uma alternativa menor preserva intenção e passa nos mesmos critérios.
- **Adiar:** classificação, dados gráficos, custo ou coerência visual não permitem um resultado aceitável. Registrar
  bloqueio exato e seguir outra frente; não forçar culling, dados guest ou dependência de ray tracing.

Registrar escolha, artefato e limite de cobertura no fluxo do projeto. Não transformar um teste local em promessa
para todas as fases. [Validação](validation.md) define provas e [qualidade](quality-presets.md) define orçamento/fallback.

`NEXT_SINGLE_ACTION`: catalogar 3–5 texturas representativas de chão/paredes de Mayahem Temple, com originais
decodificados, hashes e usos conferidos, preparando a prova T1 de substituição sem IA antes das variantes ampliadas.
