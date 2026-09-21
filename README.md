# HOW DID I DIE?

Mistério narrativo em terceira pessoa, film noir. Esta fatia cobre os passos 1 a 12: o Capítulo 01 jogável e os sistemas reutilizáveis. Os capítulos 02 a 10, a revelação, The Void como sistema, o julgamento final e Soul 11 estão só em dados.

O texto dentro do jogo está em inglês.

## Abrir no Unreal Engine 5.5

1. Instale o Unreal Engine 5.5 e o Enhanced Input (já vem com o motor).
2. Abra `HowDidIDie.uproject` na raiz deste repositório. A associação do motor é `5.5`.
3. Deixe o editor gerar os ficheiros de projeto e compilar os módulos `HowDidIDie` e `HowDidIDieEditor`.
4. O mapa por defeito é o mapa de motor `/Engine/Maps/Entry`. Não há `.umap` do jogo. Carregue em Play. O modo de jogo constrói o greybox a partir de `Content/Data`.

### Se só tiver o Unreal Engine 5.4

Em `HowDidIDie.uproject`, mude `EngineAssociation` para `5.4`. A ordem de includes dos targets já é `Unreal5_4`. Volte a gerar os ficheiros de projeto e compile. O código foi escrito contra as APIs públicas do 5.5, com esse fallback em mente.

Este ambiente de desenvolvimento não tem o Unreal Engine instalado. O projeto não foi compilado aqui. Não há binários `.uasset` nem `.umap` de autor: a geometria, o input, a interface e o pós-processo nascem em C++ em runtime.

## O que se pode jogar

Capítulo 01, Adrian Vale, tema Ambition, relógio 23:47.

- Menu: New Game, Continue, Chapters, Evidence, Settings, Quit. Chuva, cidade, silhueta, relógio.
- New Game substitui a gravação, com confirmação se já existir uma hora guardada.
- Movimento em terceira pessoa, aceleração e travagem, andar e correr, câmara cinematográfica, colisão.
- Prompt de interação no formato `E — INSPECT`.
- Diálogo com escolhas que não produzem o mesmo resultado.
- Caderno: People, Locations, Objects, Clues, Memories, Events, Timeline, Connections. Guarda provas. Não escreve a conclusão.
- Doze cartas de história.
- Um flashback jogável que revela informação e atualiza a linha do tempo.
- Pelo menos um ramo da linha do tempo que muda o estado narrativo seguinte (texto, pistas e objetos), e não apenas um corte.
- Gravação e Continue no mesmo sítio.
- Valores de alma e dedicação escondidos. No jogo, a consola `HDIDDumpSoul` mostra-os. Não aparecem no caderno.
- Noir a cerca de 90% monocromático. Dentro da memória a cor sobe, porque cor é memória a voltar.
- Definições: volumes, legendas e tamanho, resolução (1080p, 1440p, 4K), janela / borderless / ecrã inteiro, sensibilidades, vibração, redução de movimento, alto contraste, preset gráfico e rebind de todas as ações.

## O que é só dados

`Content/Data/Chapters/CH02.json` até `CH10.json` usam os mesmos sistemas, com `Playable` a falso. O menu lista-os como DATA ONLY. `Content/Data/Locked/late_game.json` fecha a identidade, The Void, os finais e Soul 11. O código recusa começar esses sistemas mesmo que alguém mude a flag no JSON.

Um capítulo novo é um JSON em `Content/Data/Chapters`. O controlador do jogador não tem ramos por capítulo.

## Controlos

Teclado: WASD, rato, E e botão esquerdo interagem, botão direito inspeciona, Shift corre, Espaço avança o diálogo, Esc pausa, Tab caderno, Q linha do tempo, F memória, R recuo contextual, 1–4 escolhas, M local, setas navegam menus.

Comando: stick esquerdo move, stick direito olha, A interage, X inspeciona, L3 corre, Start pausa, Y caderno, RT linha do tempo, LT memória, B volta, D-pad navega.

Todas as ações são remapeáveis em Settings.

## Validar a narrativa

No editor: Tools → HOW DID I DIE? → Validate Narrative.

Em jogo, na consola: `HDID.ValidateNarrative` ou o exec `HDIDValidate`.

Sem o motor:

```bash
python3 Tools/narrative_data.py validate
```

O script também regenera o JSON se for chamado sem `validate`. O JSON em `Content/Data` é o que o jogo lê.

## Limites honestos

- Não há malhas, materiais, MetaSounds nem níveis de autor. As formas vêm de `/Engine/BasicShapes`. O áudio é um misturador de volumes e camadas nomeadas, sem ondas gravadas.
- Onde a arquitetura pede um `BP_`, a classe C++ equivalente está comentada em `Source/HowDidIDie/HowDidIDie.cpp`.
- A bíblia está em `Docs/StoryBible.md`.
