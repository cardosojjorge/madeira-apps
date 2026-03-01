# SimOps Tracker (Simulado)

Este repositório contém um **dashboard estilo “wargame”** (HUD verde, mapa escuro, camadas de densidade + pontos) para **visualizar eventos simulados** ou **carregar um ficheiro local** (CSV/JSON).

Nota importante: **não** é um sistema de rastreamento em tempo real de ataques.

## Como executar

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
streamlit run app/app.py
```

## Formato do ficheiro (CSV/JSON)

Colunas obrigatórias:

- `event_id` (string)
- `timestamp` (ISO 8601; recomendado UTC, ex: `2026-03-01T12:34:56Z`)
- `lat` (float)
- `lon` (float)
- `kind` (string)
- `severity` (0..1)
- `confidence` (0..1)
- `source` (string)

Colunas opcionais:

- `region` (string)