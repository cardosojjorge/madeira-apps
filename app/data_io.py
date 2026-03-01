from __future__ import annotations

from datetime import UTC

import pandas as pd


REQUIRED_COLUMNS = {
    "event_id",
    "timestamp",
    "lat",
    "lon",
    "kind",
    "severity",
    "confidence",
    "source",
}

OPTIONAL_COLUMNS = {"region"}


def load_events(file) -> pd.DataFrame:
    name = getattr(file, "name", "") or ""
    lower = name.lower()
    if lower.endswith(".csv"):
        df = pd.read_csv(file)
    elif lower.endswith(".json"):
        df = pd.read_json(file)
    else:
        # try csv then json
        try:
            df = pd.read_csv(file)
        except Exception:
            file.seek(0)
            df = pd.read_json(file)

    missing = REQUIRED_COLUMNS - set(df.columns)
    if missing:
        raise ValueError(f"Faltam colunas obrigatórias: {sorted(missing)}")

    df = df.copy()
    df["timestamp"] = pd.to_datetime(df["timestamp"], utc=True, errors="coerce")
    if df["timestamp"].isna().any():
        raise ValueError("Há timestamps inválidos na coluna 'timestamp'.")

    df["lat"] = pd.to_numeric(df["lat"], errors="coerce")
    df["lon"] = pd.to_numeric(df["lon"], errors="coerce")
    if df["lat"].isna().any() or df["lon"].isna().any():
        raise ValueError("Há coordenadas inválidas em 'lat'/'lon'.")

    df["severity"] = pd.to_numeric(df["severity"], errors="coerce")
    df["confidence"] = pd.to_numeric(df["confidence"], errors="coerce")
    if df["severity"].isna().any() or df["confidence"].isna().any():
        raise ValueError("Há valores inválidos em 'severity'/'confidence'.")

    df["severity"] = df["severity"].clip(0.0, 1.0)
    df["confidence"] = df["confidence"].clip(0.0, 1.0)

    if "region" not in df.columns:
        df["region"] = "unknown"

    df = df.sort_values("timestamp", ascending=False).reset_index(drop=True)
    df["timestamp"] = df["timestamp"].dt.tz_convert(UTC)
    return df

