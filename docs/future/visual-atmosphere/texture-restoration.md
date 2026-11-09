---
title: "Restauração fiel de texturas com IA"
description: "Plano de inventário, extração privada, super-resolução, continuidade e empacotamento de chão e paredes."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, textures, ai, restoration]
---

# Restauração fiel de texturas com IA

Estado do novo pacote e da produção com IA: `HYPOTHESIS` / `NOT_IMPLEMENTED`. O carregamento de texturas já existe,
com validação Windows registrada na [spec de modding](../../specs/Tooie/modding-and-extensions.md). Nenhuma textura
foi extraída, gerada ou aprovada como parte desta documentação. Voltar ao [guia](README.md).

## Objetivo e fidelidade

Melhorar principalmente chão, paredes, rochas, madeira, tijolos, areia e grama preservando o desenho Rare.
Super-resolução parte da imagem original e infere detalhes adicionais; não recupera informação perdida com certeza.
O resultado precisa manter composição, paleta, símbolos, quantidade e posição de formas importantes e escala visual
do material. Não substituir pedra pintada por fotografia nem adicionar rachaduras ou motivos sem revisão.

UI e materiais já atendidos pelos pacotes existentes ficam fora do complemento inicial. Nem o rótulo “UI only” do
BT Reloaded nem a presença de uma pasta `Areas` bastam para decidir cobertura: conferir entradas, imagens e usos.
Uma textura compartilhada pode alterar diversos mapas; nenhuma substituição é exclusiva de uma sala só pelo nome
do arquivo. Se o mesmo hash exigir duas direções artísticas incompatíveis, preservar uma variante comum ou estudar
substituição contextual separadamente, sem assumir que o pacote atual oferece isso.

Começar com 3–5 amostras representativas de chão/paredes, antes de produzir o lote de 20–30. Incluir repetição,
emenda e variante de paleta/uso compartilhado quando existirem entre os candidatos. Casos difíceis precisam de
alternativa definida, não de aprovação por uma amostra única mais fácil. Ver [roteiro](implementation-roadmap.md).

## 1. Catalogar cobertura e fontes

Para cada candidato, registrar os seguintes campos num catálogo de trabalho privado ou metadados sem imagens:

| Campo | Finalidade |
| --- | --- |
| ID editorial e material | Nome humano para localizar a textura sem trocar sua identidade técnica |
| Hash RT64 e versão | Associação ao conteúdo original usado pelo renderizador; versão atual documentada: 5 |
| Dimensões, formato, paleta e alfa | Decodificação correta, proporção e transparência |
| Mapa/sala e usos compartilhados | Descobrir impacto além do piloto e estados alternativos |
| Repetição, espelhamento, recortes e atlas | Definir bordas e reconstrução de conjuntos |
| Animação ou variantes de paleta | Processar a família com consistência |
| Cobertura por pacote existente | Complementar, preservar ou rejeitar conflito conscientemente |
| Fonte privada e checksum | Reproduzir a extração e nunca sobrescrever o original |
| Modelo, pesos/checksum e parâmetros de IA | Reproduzir cada variante candidata |
| Artefato, escala e aprovação | Saber qual imagem entrou no pacote e qual foi rejeitada |

Fontes principais são os dados originais N64 do usuário. Extrações Xbox já presentes localmente podem ser
referências de desenho ou candidatas comparativas, mas seus arquivos numerados não possuem associação automática
com hashes N64 demonstrada nesta análise. Não começar ampliando uma versão já ampliada de procedência incerta.

## 2. Separar identificação de exportação de imagens

O dump RT64 serve para identificar textura e metadados de uso, mas o
[guia vendorizado](../../../lib/rt64/TEXTURE-PACKS.md) descreve dumps de TMEM/RDRAM e informa que o dumper não exporta
automaticamente PNGs decodificados. Portanto, o primeiro marco inclui obter imagens originais corretas, não apenas
clicar em exportar e assumir um conjunto pronto para IA.

Caminhos candidatos: extração privada da ROM com decodificação validada contra o desenho em jogo, ou captura da
textura decodificada no renderizador por ferramenta de desenvolvimento específica. Escolher após verificar formatos,
paletas, tiles e disponibilidade de ferramentas existentes. Captura de tela da cena não substitui textura original:
ela mistura perspectiva, luz, neblina e filtragem.

Conferir cada imagem decodificada contra a textura usada em jogo. Conteúdo dinâmico ou renderizado em framebuffer
exige estudo próprio; não o tratar como textura estática. O mapeamento RT64 deve acompanhar as variantes reais de
paleta/uso. Hash Rice ou nome numerado isolado não garante correspondência.

Antes da IA, carregar uma cópia sem ampliação da imagem original pelo mesmo caminho de substituição proposto.
Conferir alinhamento, orientação, transparência, cor e usos adicionais. O resultado deve reproduzir a referência
nativa dentro dos limites conhecidos do caminho de substituição; diferença sem explicação bloqueia aquela amostra.
Esse controle separa erro de extração/decodificação de erro introduzido pela ampliação.

## 3. Produzir variantes conservadoras

1. Preservar originais imutáveis e separar saídas por experimento.
2. Criar baseline sem IA por ampliação convencional, além dos candidatos com IA.
3. Testar 2× e 4× nas dimensões; selecionar por material e distância observada, não por um tamanho universal.
4. Comparar modelos de super-resolução, como `RealESRGAN_x2plus`, `RealESRGAN_x4plus`, `RealESRNet_x4plus` e
   `RealESRGAN_x4plus_anime_6B`, sem presumir que um único modelo é melhor para todas as texturas. O candidato
   `RealESRNet_x4plus` usa objetivo MSE e pode ficar mais suave; incluí-lo como comparação conservadora, sem garantia de fidelidade.
5. Manter nitidez e redução de ruído contidas. Desabilitar reconstrução de rostos para texturas de chão e paredes.
6. Ajustar ou retocar detalhes que alteraram símbolos, contornos, manchas ou estilo. Rejeitar resultados inadequados.

Os modelos e suas finalidades estão no [catálogo oficial Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN/blob/master/docs/model_zoo.md).
O parâmetro de redução de ruído `-dn` é específico de `realesr-general-x4v3` no
[script oficial](https://github.com/xinntao/Real-ESRGAN/blob/master/inference_realesrgan.py); não presumir que funciona
em qualquer modelo. Versões, dependências e pesos devem ser fixados quando começar o piloto, antes de produção em lote.

Configurar e executar ferramenta de super-resolução faz parte da produção futura pelo agente. Registrar ambiente,
pesos, checksum, escala nativa do modelo e escala final: gerar em 4× e reduzir para 2× não é o mesmo experimento
que usar modelo nativo 2×. Modelos generativos gerais para edição de imagens não substituem automaticamente esse
processo em lote, reproduzível e com tratamento de bordas.

Escolher um preset de produção por família de material, com exceções justificadas e reversíveis. Guardar estado
por textura para retomar lotes, evitar processamento repetido e manter variantes rejeitadas fora do pacote.
Se IA piorar o resultado, usar ampliação convencional, escala menor ou original. A meta é qualidade fiel, não
percentual de imagens obrigatoriamente processadas por IA.

Geração livre por prompt e redesenho por difusão não são o caminho padrão de preservação. Eventual reparo assistido
deve ser localizado e comparado com o original. Não há promessa automática de “sem perder originalidade”.

## 4. Preservar emendas, conjuntos e transparência

- Textura repetível precisa manter continuidade horizontal/vertical conforme seu uso. Testar mosaico 3×3 e paredes
  com orientação/espelhamento reais. Padding periódico e correção das bordas são candidatos, não garantia automática.
- O processamento por blocos da ferramenta de IA reduz memória de inferência, mas não torna uma textura repetível.
- Se a arte foi dividida em peças ou atlas, reconstruir o conjunto quando houver layout comprovado, ampliar de forma
  coerente e recortar nas coordenadas corretas. Não ampliar cada pedaço cegamente.
- Processar famílias animadas com parâmetros consistentes e inspecionar em sequência: detalhes inventados por quadro
  podem piscar. Variantes de paleta precisam preservar o propósito da mudança de cor.
- Tratar canal alfa separadamente quando necessário, manter bordas corretas e evitar halos em mipmaps. Não transformar
  transparência parcial em recorte opaco. Respeitar orientação, espaço de cor e opções de alinhamento do pacote.
- Gerar mipmaps a partir do conjunto final com política compatível de repetição, cor e alfa. Validar recortes
  próximos e distantes: corrigir a borda da imagem principal não garante que os níveis menores fiquem sem emenda.
- Não gravar nova sombra/luz forte dentro da imagem se ela conflitar com iluminação nativa ou futuro ciclo solar.
  Remover sombreamento original também exige curadoria: nem toda mancha escura é uma sombra a ser apagada.

## 5. Revisar dentro do jogo

Comparar original, baseline convencional, 2× e 4× com mesma câmera, hora e luz. Conferir de perto e em movimento,
paredes oblíquas, repetição de chão, emendas entre peças e mapas que compartilham o recurso. Uma textura impressionante
isolada pode ficar ruidosa quando cobre toda a parede.

Reduzir novamente a versão ampliada ajuda a detectar mudanças grandes em formas e cores, mas não prova fidelidade.
SSIM/PSNR, se usados, são auxiliares; aprovação visual e legibilidade de símbolos continuam necessárias. Registrar
motivo de aceitar 2× em uma textura e 4× em outra. O piloto deve definir exemplos aprovados e rejeitados para o lote.

## 6. Empacotar e controlar custo

Criar complemento separado com somente as substituições aprovadas e sua base RT64. Não editar nem reempacotar o ZIP
original do mod instalado. Resolver colisões de hashes explicitamente; reordenação entre packs ainda precisa de
validação conforme o contrato atual. Preparar fontes privadas, imagens de trabalho e pacote final em locais separados.

No desktop, avaliar DDS com compressão adequada, mipmaps e cache de mipmaps pequenos para carregamento assíncrono.
O [guia RT64](https://github.com/rt64/rt64/blob/main/TEXTURE-PACKS.md) descreve o fluxo e as ferramentas. PNG é útil na
produção, mas não é a escolha inicial para distribuição otimizada. Formato, compressão e qualidade de alfa precisam
de verificação no alvo; uma versão de portátil não pode presumir suporte a BC7.

Escala 4× em largura/altura significa 16× mais pixels antes de compressão; 2× significa 4×. Usar resolução proporcional
ao desenho e à tela, sem impor 4K a cada tile pequeno. Medir VRAM, memória, carregamento frio/quente, pop-in e
cintilação a distância. Preservar mipmaps e filtragem evita que detalhe adicional vire ruído em movimento.

ROM, dumps, imagens proprietárias, pesos temporários e pacotes privados não entram no Git nem no release do jogo.
Metadados reprodutíveis e ferramentas próprias podem ser versionados conforme as regras do projeto. Distribuição de
um futuro pacote artístico exige avaliação própria de fontes/licenças; esta documentação não publica assets.

## Requisitos e primeiro marco

| ID | Requisito e aceite |
| --- | --- |
| TEX-01 | Catálogo distingue original, hash, variantes e cobertura; nenhum recurso entra só por semelhança de nome. |
| TEX-02 | Imagem decodificada conferida em jogo antes da IA; exportação de imagem é um marco explícito. |
| TEX-03 | Comparação convencional/2×/4× mantém composição, símbolos e paleta aprovados. |
| TEX-04 | Repetição, atlas, alfa e animações passam inspeção em mosaico e em movimento, sem halos ou cintilação indevida. |
| TEX-05 | Complemento não sobrescreve pacotes privados originais nem substitui a UI fora do escopo. |
| TEX-06 | Formatos, mipmaps, cache e custo medidos por plataforma; originais e produção permanecem reproduzíveis e privados. |
| TEX-07 | Substituição da imagem original sem IA reproduz referência antes da comparação de ampliadores; divergências têm causa identificada. |
| TEX-08 | Escala nativa/final, preset por material e resultado de cada textura registrados; lote pode ser retomado e usar fallback sem IA. |

Primeiro marco: catalogar 3–5 texturas representativas de chão/paredes de Mayahem Temple, obter originais e provar
substituição sem IA. Depois comparar convencional/IA em 2×/4×, escolher padrão artístico e ampliar para o piloto
de 20–30. Produção completa depende de qualidade e custo desse piloto; nova tecnologia não bloqueia preservar um original melhor.
