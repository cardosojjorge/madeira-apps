# Playtest do Capítulo 01

Versão de browser para jogar o Capítulo 01 (Adrian Vale) sem o Unreal Engine. O projecto C++ fica como está. O texto do jogo continua em inglês e vem de `Content/Data` e de `Docs/StoryBible.md`.

## Abrir

Na raiz do repositório:

```bash
python3 -m http.server 8765 --bind 0.0.0.0
```

Abra [http://127.0.0.1:8765/Playtest/](http://127.0.0.1:8765/Playtest/).

## Controlos

- WASD anda, Shift corre, rato olha (clique no ecrã para capturar o rato)
- E ou clique esquerdo interage; clique direito examina
- 1–4 escolhe diálogo; Espaço continua
- Tab caderno, Q linha do tempo, F memória
- Esc pausa. No menu, as setas navegam e Enter confirma

Quit não fecha o separador. Volta ao título.

## O que se joga

Menu (New Game, Continue, Chapters, Evidence, Settings, Quit), cidade à chuva, relógio 23:47. Só o Capítulo 01 abre. O greybox, as doze cartas, o flashback, o ramo da linha do tempo e a gravação seguem os JSON. Continue repõe capítulo, carta, pistas e escolhas. O caderno guarda provas e não conclui a morte.
