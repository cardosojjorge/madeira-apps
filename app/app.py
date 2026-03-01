from __future__ import annotations

from datetime import UTC, datetime, timedelta

import pandas as pd
import plotly.express as px
import pydeck as pdk
import streamlit as st

from app.data_io import load_events
from app.simulate_events import generate_events


st.set_page_config(page_title="SimOps Tracker (Simulado)", layout="wide")


def _wargame_theme():
    st.markdown(
        """
        <style>
          :root { --hud: #8dff4a; --bg: #07110a; --panel: #0b1c10; --muted: #93b99b; }
          .stApp { background: radial-gradient(80% 80% at 20% 10%, #0b2112 0%, var(--bg) 55%, #040a06 100%); color: #e7ffe2; }
          h1,h2,h3 { color: var(--hud); letter-spacing: 0.5px; }
          .stMarkdown, .stText, p, span, label { color: #dfffe0; }
          div[data-testid="stMetric"] { background: color-mix(in srgb, var(--panel) 85%, transparent); border: 1px solid rgba(141,255,74,0.22); padding: 12px 14px; border-radius: 10px; }
          section[data-testid="stSidebar"] { background: linear-gradient(180deg, rgba(11,28,16,0.92), rgba(7,17,10,0.92)); border-right: 1px solid rgba(141,255,74,0.18); }
          .stButton button { border: 1px solid rgba(141,255,74,0.35); background: rgba(11,28,16,0.85); }
          .stButton button:hover { border: 1px solid rgba(141,255,74,0.7); }
          .stSelectbox, .stMultiSelect, .stSlider { color: #e7ffe2; }
        </style>
        """,
        unsafe_allow_html=True,
    )


@st.cache_data(show_spinner=False)
def _sim_df(n: int, days: int, seed: int) -> pd.DataFrame:
    return generate_events(n=n, days=days, seed=seed)


def _filter_df(df: pd.DataFrame, *, start: datetime, end: datetime, kinds: list[str], min_conf: float):
    out = df[(df["timestamp"] >= start) & (df["timestamp"] <= end)]
    if kinds:
        out = out[out["kind"].isin(kinds)]
    out = out[out["confidence"] >= min_conf]
    return out


def _make_map(df: pd.DataFrame):
    if df.empty:
        return None

    view = pdk.ViewState(
        latitude=float(df["lat"].mean()),
        longitude=float(df["lon"].mean()),
        zoom=4.3,
        pitch=48,
        bearing=12,
    )

    # “Radar/HUD” color ramp by severity
    df = df.copy()
    df["sev"] = (df["severity"] * 255).astype(int)
    df["color"] = df["sev"].apply(lambda v: [min(255, 40 + v), min(255, 255), min(255, 80 + v // 3), 185])
    df["radius"] = (1800 + df["severity"] * 6200).astype(int)

    scatter = pdk.Layer(
        "ScatterplotLayer",
        data=df,
        get_position="[lon, lat]",
        get_radius="radius",
        get_fill_color="color",
        pickable=True,
        opacity=0.78,
        stroked=False,
    )

    hexagon = pdk.Layer(
        "HexagonLayer",
        data=df,
        get_position="[lon, lat]",
        radius=22000,
        elevation_scale=45,
        elevation_range=[0, 3500],
        pickable=False,
        extruded=True,
        coverage=0.75,
    )

    # Dark basemap; no external key required
    deck = pdk.Deck(
        layers=[hexagon, scatter],
        initial_view_state=view,
        map_style="mapbox://styles/mapbox/dark-v10",
        tooltip={
            "html": "<b>{kind}</b> | sev={severity:.2f} conf={confidence:.2f}<br/>{timestamp}<br/>{region}<br/>{event_id}",
            "style": {"backgroundColor": "#06130b", "color": "#cfffbd", "border": "1px solid rgba(141,255,74,0.35)"},
        },
    )
    return deck


def _charts(df: pd.DataFrame):
    if df.empty:
        st.info("Sem eventos para os filtros atuais.")
        return

    # Timeline (counts)
    by_hour = (
        df.assign(bucket=df["timestamp"].dt.floor("H"))
        .groupby(["bucket", "kind"], as_index=False)
        .size()
        .rename(columns={"size": "count"})
    )
    fig_t = px.area(
        by_hour,
        x="bucket",
        y="count",
        color="kind",
        title="Volume temporal (por hora)",
        template="plotly_dark",
    )
    fig_t.update_layout(legend_title_text="Tipo", height=320, margin=dict(l=10, r=10, t=45, b=10))
    st.plotly_chart(fig_t, use_container_width=True)

    # Severity vs confidence scatter
    fig_sc = px.scatter(
        df,
        x="confidence",
        y="severity",
        color="kind",
        hover_data=["timestamp", "region", "event_id"],
        title="Severidade vs Confiança",
        template="plotly_dark",
    )
    fig_sc.update_layout(height=320, margin=dict(l=10, r=10, t=45, b=10))
    st.plotly_chart(fig_sc, use_container_width=True)

    # Kind breakdown
    fig_k = px.bar(
        df.groupby("kind", as_index=False).size().rename(columns={"size": "count"}),
        x="kind",
        y="count",
        title="Distribuição por tipo",
        template="plotly_dark",
    )
    fig_k.update_layout(height=320, margin=dict(l=10, r=10, t=45, b=10))
    st.plotly_chart(fig_k, use_container_width=True)


def main():
    _wargame_theme()

    st.title("SimOps Tracker — Visualização Simulada")
    st.caption(
        "Este painel usa dados simulados (ou ficheiros locais) e **não** faz rastreamento em tempo real de ataques."
    )

    with st.sidebar:
        st.subheader("Fonte de dados")
        mode = st.radio("Modo", ["Simulado", "Carregar CSV/JSON"], index=0)

        if mode == "Simulado":
            n = st.slider("Nº de eventos", 200, 5000, 1200, step=100)
            days = st.slider("Janela (dias)", 1, 90, 14, step=1)
            seed = st.number_input("Seed", min_value=0, max_value=999999, value=42, step=1)
            df = _sim_df(n=n, days=days, seed=int(seed))
        else:
            up = st.file_uploader("Ficheiro (.csv ou .json)", type=["csv", "json"])
            if up is None:
                st.info("Carrega um ficheiro para continuar.")
                return
            try:
                df = load_events(up)
            except Exception as e:
                st.error(f"Erro ao carregar: {e}")
                return

        st.divider()
        st.subheader("Filtros")
        end = datetime.now(UTC)
        start_default = end - timedelta(days=7)
        start = st.date_input("Início (UTC)", value=start_default.date())
        end_d = st.date_input("Fim (UTC)", value=end.date())
        start_dt = datetime.combine(start, datetime.min.time(), tzinfo=UTC)
        end_dt = datetime.combine(end_d, datetime.max.time(), tzinfo=UTC)

        kinds = sorted(df["kind"].unique().tolist())
        sel_kinds = st.multiselect("Tipos", kinds, default=kinds)
        min_conf = st.slider("Confiança mínima", 0.0, 1.0, 0.35, step=0.05)

    fdf = _filter_df(df, start=start_dt, end=end_dt, kinds=sel_kinds, min_conf=min_conf)

    c1, c2, c3, c4 = st.columns(4)
    c1.metric("Eventos (filtro)", f"{len(fdf):,}".replace(",", "."))
    c2.metric("Confiança média", f"{fdf['confidence'].mean():.2f}" if not fdf.empty else "—")
    c3.metric("Severidade média", f"{fdf['severity'].mean():.2f}" if not fdf.empty else "—")
    c4.metric("Janela", f"{start_dt.date()} → {end_dt.date()}")

    left, right = st.columns([1.25, 1.0], gap="large")
    with left:
        st.subheader("Mapa")
        deck = _make_map(fdf)
        if deck is None:
            st.info("Sem dados para renderizar no mapa.")
        else:
            st.pydeck_chart(deck, use_container_width=True)

    with right:
        st.subheader("Gráficos")
        _charts(fdf)

    st.subheader("Eventos (amostra)")
    st.dataframe(
        fdf.head(200)[
            ["timestamp", "kind", "severity", "confidence", "region", "lat", "lon", "event_id", "source"]
        ],
        use_container_width=True,
        height=360,
    )


if __name__ == "__main__":
    main()

