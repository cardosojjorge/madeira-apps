from __future__ import annotations

import math
from dataclasses import dataclass
from datetime import UTC, datetime, timedelta

import numpy as np
import pandas as pd


@dataclass(frozen=True)
class Region:
    name: str
    center_lat: float
    center_lon: float
    sigma_km: float


REGIONS: list[Region] = [
    Region("Levant", 33.5, 36.3, 120.0),
    Region("Mesopotamia", 33.3, 44.4, 140.0),
    Region("Gulf", 26.2, 50.6, 160.0),
    Region("Red Sea", 19.6, 40.7, 220.0),
    Region("Sinai", 29.2, 33.8, 140.0),
]

KINDS = ["strike", "intercept", "drone", "rocket", "maritime", "airspace"]
SOURCES = ["simulated"]


def _km_to_deg_lat(km: float) -> float:
    return km / 111.0


def _km_to_deg_lon(km: float, at_lat: float) -> float:
    return km / (111.320 * math.cos(math.radians(at_lat)) + 1e-9)


def generate_events(
    *,
    n: int = 800,
    days: int = 14,
    seed: int = 42,
    now: datetime | None = None,
) -> pd.DataFrame:
    rng = np.random.default_rng(seed)
    if now is None:
        now = datetime.now(UTC)

    region_idx = rng.integers(0, len(REGIONS), size=n)

    # Timestamp distribution: more recent heavier weight
    t0 = now - timedelta(days=days)
    # Use beta to bias towards recent
    u = rng.beta(2.5, 1.2, size=n)
    ts = pd.to_datetime(t0 + (now - t0) * u)

    kinds = rng.choice(KINDS, size=n, p=[0.22, 0.10, 0.18, 0.20, 0.14, 0.16])
    severity = np.clip(rng.normal(0.55, 0.22, size=n), 0.05, 0.98)
    confidence = np.clip(rng.normal(0.72, 0.18, size=n), 0.10, 0.99)

    lat = np.empty(n, dtype=float)
    lon = np.empty(n, dtype=float)
    for i, ridx in enumerate(region_idx):
        r = REGIONS[int(ridx)]
        # gaussian scatter in km around region center
        dx_km = rng.normal(0.0, r.sigma_km)
        dy_km = rng.normal(0.0, r.sigma_km)
        lat[i] = r.center_lat + _km_to_deg_lat(dy_km)
        lon[i] = r.center_lon + _km_to_deg_lon(dx_km, r.center_lat)

    df = pd.DataFrame(
        {
            "event_id": [f"sim-{seed}-{i:05d}" for i in range(n)],
            "timestamp": ts,
            "lat": lat,
            "lon": lon,
            "region": [REGIONS[int(i)].name for i in region_idx],
            "kind": kinds,
            "severity": severity,
            "confidence": confidence,
            "source": rng.choice(SOURCES, size=n),
        }
    )

    df["timestamp"] = pd.to_datetime(df["timestamp"], utc=True)
    df = df.sort_values("timestamp", ascending=False).reset_index(drop=True)
    return df


def to_geojson(df: pd.DataFrame) -> dict:
    features: list[dict] = []
    for _, r in df.iterrows():
        features.append(
            {
                "type": "Feature",
                "geometry": {"type": "Point", "coordinates": [float(r["lon"]), float(r["lat"])]},
                "properties": {
                    "event_id": str(r["event_id"]),
                    "timestamp": str(r["timestamp"]),
                    "region": str(r["region"]),
                    "kind": str(r["kind"]),
                    "severity": float(r["severity"]),
                    "confidence": float(r["confidence"]),
                    "source": str(r["source"]),
                },
            }
        )
    return {"type": "FeatureCollection", "features": features}

