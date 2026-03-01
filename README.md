# Centro de Comando Militar — Rastreamento de Conflitos em Tempo Real

Sistema de rastreamento e visualização de conflitos militares no Médio Oriente, estilo sala de guerra / simulação tática.

## Funcionalidades

- **Mapa Tático ao Vivo** — Leaflet.js com tiles escuros e marcadores de ataque animados com trajetórias de mísseis
- **Radar Tático** — Canvas animado com varrimento e blips de posição
- **Dados em Tempo Real** — Socket.io com atualizações automáticas a cada 2–6 segundos
- **Gráficos Militares** — Chart.js com linha do tempo por hora, tipos de ataque e distribuição regional
- **Ameaças por Região** — Barras de progresso com nível de ameaça dinâmico para 19 zonas do Médio Oriente
- **Registo de Ataques ao Vivo** — Log em tempo real com filtro por gravidade e clique para centrar no mapa
- **Alerta Crítico** — Overlay automático para eventos de nível CRÍTICO
- **Ticker de Intel** — Rodapé com mensagens de inteligência scrolling
- **Tema HUD Militar** — Design inspirado em jogos de simulação de guerra com scanlines, glow e efeitos CRT

## Tecnologia

- **Backend:** Node.js + Express + Socket.io
- **Frontend:** Vanilla JS + Leaflet.js + Chart.js
- **Tipografia:** Orbitron + Share Tech Mono (Google Fonts)

## Instalação e Execução

```bash
npm install
npm start
```

O servidor inicia na porta `3000`. Aceder em: http://localhost:3000

## Regiões Monitorizadas

Gaza, Cisjordânia, Israel Norte/Sul, Sul do Líbano, Beirute, Damasco, Alepo, Homs, Colinas do Golan, Bagdade, Mossul, Basra, Sanaa, Hodeidah, Aden, Teerão, Mar Vermelho, Sinai

## Tipos de Ataque Simulados

- Ataque Aéreo (caças F-35, bombardeiros)
- Míssil Balístico (Qassam, Fateh-110)
- Drone/UAV (Shahed-136, enxames)
- Artilharia (morteiros, obuses)
- Assalto Terrestre (infantaria, blindados)
- Naval (corvetas, drones navais)
- Cibernético (infraestruturas, comunicações)
