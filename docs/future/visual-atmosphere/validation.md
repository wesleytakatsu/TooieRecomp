---
title: "Matriz de aceite das melhorias visuais e atmosfera"
description: "Provas futuras por melhoria, cenário, plataforma e modo, sem promover planejamento a teste executado."
status: proposed
type: validation-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, visuals, atmosphere, validation]
---

# Matriz de aceite das melhorias visuais e atmosfera

Esta matriz pertence ao [plano visual](README.md), não à cobertura atual do executável. Requisitos de cada melhoria
estão em seus documentos. **Todos os casos abaixo estão `NOT_IMPLEMENTED`; nenhum teste de runtime foi executado
para esta proposta.** Auditoria de Markdown não valida efeito gráfico, fidelidade artística ou desempenho.

## Requisitos e discriminadores por melhoria

| Família de requisitos | Documento | Comparação mínima e resultado esperado | Estado |
| --- | --- | --- | --- |
| ATM-01–12 | [Arquitetura](architecture.md) | Desligado, contexto ausente, estados nativos, pausas, gerações e duas câmeras; snapshot temporal e descarte corretos | NOT_IMPLEMENTED |
| TEX-01–08 | [Texturas](texture-restoration.md) | Original pelo caminho de substituição antes de convencional/IA em 2×/4×; desenho, emendas, alfa, hash e retomada de lote corretos | NOT_IMPLEMENTED |
| COL-01–03 | [Cores](color-grading.md) | Interior/exterior, água e HUD; paleta coerente sem modificar UI nem destruir detalhes escuros | NOT_IMPLEMENTED |
| LIT-01–04 | [Iluminação](lighting.md) | J1/J2 e duas instâncias; luz localizada sem dupla aplicação ou vazamento entre salas/buffers | NOT_IMPLEMENTED |
| BLM-01–03 | [Bloom](bloom.md) | Fogo/lâmpada versus neve, texto branco e coletável; apenas emissores autorizados ganham halo | NOT_IMPLEMENTED |
| SHD-01–03 | [Sombras](contact-shadows.md) | Parede/chão, sprite, água, objetos móveis e corte de câmera; sem halo de HUD nem trilhas temporais | NOT_IMPLEMENTED |
| MAT-01–04 | [Materiais](surface-materials.md) | Pedra, madeira, metal e gelo; relevo e brilho compatíveis sem modificar silhueta/colisão | NOT_IMPLEMENTED |
| WAT-01–05 | [Água](water.md) | Acima/abaixo da superfície, margem, transparência e duas câmeras; sem inversão de profundidade ou reflexo indevido | NOT_IMPLEMENTED |
| RAI-01–05 | [Chuva](rain.md) | Exterior, teto, abrigo parcial e interior; gotas/som/molhado respeitam exposição e término | NOT_IMPLEMENTED |
| DAY-01–06 | [Dia/noite](day-night-cycle.md) | Horas fixas antes do ciclo; duração independente de FPS, céu/luz sem contradizer sombras incorporadas e interiores preservados | NOT_IMPLEMENTED |
| WND-01–06 | [Vento](wind-and-vegetation.md) | Tronco/copa, duas instâncias, árvore estática e animada; base firme e pose interpolada sem deformação acumulada/dupla | NOT_IMPLEMENTED |
| DST-01–04 | [Poeira da mina](mine-dust.md) | Teto, túnel adjacente, piso e saída da região; origem correta, emissão limitada e limpeza completa | NOT_IMPLEMENTED |
| LEF-01–04 | [Folhas](airborne-leaves.md) | Bosque, interior vizinho e duas câmeras; trajetória coerente com vento, sem duplicação ou coleta falsa | NOT_IMPLEMENTED |
| PAR-01–04 | [Partículas locais](local-particles.md) | Neve, cinza, vapor e submerso; fonte e meio corretos, sem duplicar efeitos nativos | NOT_IMPLEMENTED |
| SKY-01–04 | [Céu/neblina](sky-and-fog.md) | Céu claro/noturno, sala fechada e transição; sem emendas, revelação de limites ou mudança de culling | NOT_IMPLEMENTED |
| AUD-01–04 | [Áudio ambiente](ambient-audio.md) | Chuva sob teto, poeira e pausa; loops sem cliques, sem duplicação J1/J2, BGM/cues preservados | NOT_IMPLEMENTED |
| QLT-01–05 | [Qualidade e orçamento](quality-presets.md) | Efeito isolado e conjunto, capacidade ausente e mudança de preset; preferências e orçamento preservados | NOT_IMPLEMENTED |
| INT-01–05 | [Reações ao contato](surface-interactions.md) | Passo/aterrissagem nativos em terra versus metal/voo, duas formas e duas câmeras; resposta única no material correto | NOT_IMPLEMENTED |
| LFX-01–04 | [Feixes de luz](light-shafts.md) | Fonte visível/oculta, parede, giro de câmera e poeira; volume limitado sem vazamentos ou duplicação de luz | NOT_IMPLEMENTED |

## Casos transversais obrigatórios

| Caso | Critério proposto | Estado |
| --- | --- | --- |
| Baseline | Capturar mesma cena com pacote/efeito desligado e ligado, com hora, seed, câmera e build registrados | NOT_IMPLEMENTED |
| Simulação nativa | Comparar timers, colisão, flags e eventos nos recortes relevantes; aparência não altera regras do jogo | NOT_IMPLEMENTED |
| Tempo | Comparar duração real a 30/60/120 FPS, com pausa e retorno; sem aceleração nem emissão acumulada | NOT_IMPLEMENTED |
| Trocas | Porta, Warp, morte, respawn, mapa, ativação/desativação e resize; recursos sem vazamento ou uso após descarte | NOT_IMPLEMENTED |
| Câmera | 4:3, 16:9, ultrawide e corte; profundidade, viewport, histórico e bordas corretos | NOT_IMPLEMENTED |
| Split-screen | J1 exposto/J2 abrigado, um submerso e duas instâncias iguais; clima único com composição por câmera | NOT_IMPLEMENTED |
| Modos especiais | Demos, minigames, bosses, cutscenes e menus; nativo até aceite específico | NOT_IMPLEMENTED |
| Composição | Combinar chuva + vento + água + ciclo, depois bloom/sombras; ordem, limites e custo não podem ser validados só isoladamente | NOT_IMPLEMENTED |
| Mods | BT Reloaded ligado/desligado e complemento; conflitos identificados, UI existente preservada | NOT_IMPLEMENTED |
| Capacidades | Recurso opcional ausente não impede início do jogo; fallback por efeito com configuração consistente | NOT_IMPLEMENTED |
| Legibilidade | Plataformas, perigos, coletáveis, mira, legendas e cores de pistas identificáveis em todas as horas aprovadas | NOT_IMPLEMENTED |
| Conforto | Reduzir partículas, balanço e flashes sem obrigar desligar melhorias independentes | NOT_IMPLEMENTED |
| Prioridade nativa | Estado roteirizado, efeito nativo equivalente e variante local do mesmo mapa; nenhuma duplicação ou sobrescrita de gameplay | NOT_IMPLEMENTED |
| Amostra gráfica | Cor/profundidade/pose/máscara do mesmo quadro e viewport, com interpolação ligada/desligada; sem usar dado guest atrasado como equivalente | NOT_IMPLEMENTED |
| Streaming | Carregamento frio/quente e região nova com pacote de texturas; shader/IO sem travadas ocultadas por cache anterior | NOT_IMPLEMENTED |
| Integração de contato | Mesmo evento em 30/60/120 FPS gera uma resposta, com vida e dono corretos e sem duplicar efeito nativo | NOT_IMPLEMENTED |

## Provas de entrada e decisões

As etapas T0–T3, A0–A2 e R0 do [roteiro](implementation-roadmap.md) têm aceite próprio. Não iniciar lote completo
sem provar decodificação e substituição original; não declarar efeito em tela seletivo aceito sem demonstrar
mascaramento/contexto na integração ativa. Protótipo aprovado fecha seu recorte, não todas as melhorias relacionadas.

Para cada resultado futuro, registrar decisão de continuar, simplificar ou adiar e seu motivo concreto. Se a versão
leve resolver o objetivo com menos custo, ela pode ser a entrega aceita. Nenhum teste exige que IA, reflexo em tela,
segunda câmera ou material avançado seja usado quando a alternativa preserva melhor desenho e desempenho.

Comparações mantêm resolução, backend, driver, interpolação, distância de objetos, mods, hora, seed cosmética e
câmera controlados. Para testar preservação de simulação, usar reprodução/fixture adequada ao recorte e comparar
no mesmo ponto de atualização. Movimento manual diferente e alterações na opção de distância de objetos não
são evidência de que uma partícula alterou IA/timers. Não prometer igualdade binária de sessões não determinísticas.

## Plataformas e desempenho

Criar resultado separado por backend efetivamente suportado e aparelho. Uma execução Windows não é prova Linux,
e desktop não é prova OpenGL ES. Incluir o fallback sem efeitos avançados e uma combinação com texturas menores no
portátil. Não exigir BC7 onde o hardware/driver não o suporta.

Antes de medir, definir hardware, resolução, apresentação, configuração, cena e meta. Registrar tempos de frame
CPU/GPU quando disponíveis, mediana e percentis, memória/VRAM, carregamento frio/quente e estabilidade prolongada.
Comparar base, efeitos isolados e combinação aprovada; contagem média de FPS sozinha não detecta travadas.
Não fixar meta universal sem baseline. Degradar qualidade com histerese, se adotado, sem partículas piscando.

O primeiro conjunto usa qualidade fixa. Controle automático de qualidade é posterior, não requisito implícito para
fechar este plano. Os [perfis de qualidade](quality-presets.md) separam capacidade, direção artística, texturas e
preferências existentes. Medir também duas viewports, leituras/esperas de GPU e o custo de consultas por emissor,
em vez de considerar apenas número de partículas ou resolução de texturas.

## Evidência e conclusão

Cada execução futura registra comando, exit code, HEAD, estado de trabalho, hashes de patches/pacotes, backend,
driver/aparelho, região, configurações, resultado observado e limite do aceite. Capturas e vídeos de validação ficam
fora do repositório quando forem transitórios ou contiverem assets privados; a proveniência durável segue o fluxo
do projeto. Não criar uma nova pasta de evidências neste plano.

Usar `PASS` somente para caso executado com critério atendido; `FAIL` para falha observada; `NOT_RUN` para caso
implementado não executado; `NOT_IMPLEMENTED` para teste/efeito ainda inexistente. Aceite artístico exige inspeção
em movimento e aprovação do piloto; métrica de imagem não prova preservação do desenho.

Ao implementar uma melhoria, atualizar o [HANDOFF](../../HANDOFF.md), a spec dona do comportamento e o RE somente
quando houver conhecimento nativo comprovado. Esta pasta mantém propostas e pendências, sem duplicar contratos
concluídos. Os resultados existentes continuam em [testes atuais](../../specs/Tooie/testing-and-oracle.md).
