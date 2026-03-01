/* ═══════════════════════════════════════════════════════════════════════════
   CENTRO DE COMANDO MILITAR — CLIENT APPLICATION
   ═══════════════════════════════════════════════════════════════════════════ */

'use strict';

// ─── ESTADO LOCAL ──────────────────────────────────────────────────────────
const state = {
  attacks: [],
  stats: {},
  zones: [],
  attackTypes: [],
  mapMarkers: new Map(),
  trajectoryLines: [],
  criticalQueue: [],
  criticalTimer: null,
  tickerMessages: [],
  typeChart: null,
  countryChart: null,
  timelineChart: null,
  radarBlips: [],
  radarAngle: 0,
  radarAnimId: null,
};

// ─── CONSTANTES ────────────────────────────────────────────────────────────
const SEV_COLOR = {
  CRITICAL: '#ff1a1a',
  HIGH:     '#ff6600',
  MEDIUM:   '#ffd700',
  LOW:      '#44ff88',
};
const TYPE_ICONS = {
  airstrike:  '✈',
  missile:    '🚀',
  drone:      '⬡',
  artillery:  '💥',
  ground:     '⚔',
  naval:      '⚓',
  cyber:      '⚡',
};

// ─── UTILITÁRIOS ───────────────────────────────────────────────────────────
function fmt(n) {
  if (n === undefined || n === null) return '0';
  return Number(n).toLocaleString('pt-PT');
}
function fmtTime(iso) {
  return new Date(iso).toLocaleTimeString('pt-PT', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
}
function lerp(a, b, t) { return a + (b - a) * t; }

// ─── RELÓGIO ───────────────────────────────────────────────────────────────
function startClock() {
  function tick() {
    const now = new Date();
    const dateStr = now.toLocaleDateString('pt-PT', { day: '2-digit', month: '2-digit', year: 'numeric' });
    const timeStr = now.toLocaleTimeString('pt-PT', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    document.getElementById('hdr-date').textContent = dateStr;
    document.getElementById('hdr-time').textContent = timeStr;
    document.getElementById('footer-time').textContent = timeStr;
  }
  tick();
  setInterval(tick, 1000);
}

// ─── MAPA LEAFLET ──────────────────────────────────────────────────────────
let map;

function initMap() {
  map = L.map('map', {
    center: [29.5, 39.0],
    zoom: 5,
    zoomControl: true,
    attributionControl: false,
  });

  // Tile layer escuro estilo militar
  L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', {
    maxZoom: 18,
    subdomains: 'abcd',
  }).addTo(map);

  // Overlay de grid estilo tático (canvas layer)
  const GridLayer = L.GridLayer.extend({
    createTile(coords) {
      const tile = document.createElement('canvas');
      const size = this.getTileSize();
      tile.width = size.x;
      tile.height = size.y;
      const ctx = tile.getContext('2d');
      ctx.strokeStyle = 'rgba(0,212,255,0.06)';
      ctx.lineWidth = 0.5;
      for (let x = 0; x <= size.x; x += 64) {
        ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, size.y); ctx.stroke();
      }
      for (let y = 0; y <= size.y; y += 64) {
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(size.x, y); ctx.stroke();
      }
      return tile;
    }
  });
  new GridLayer({ opacity: 1, zIndex: 2 }).addTo(map);

  // Coordenadas do rato
  map.on('mousemove', e => {
    const lat = e.latlng.lat.toFixed(4);
    const lng = e.latlng.lng.toFixed(4);
    document.getElementById('map-coords-disp').textContent = `LAT: ${lat}  LNG: ${lng}`;
    document.getElementById('radar-coords').textContent = `LAT: ${lat}  LNG: ${lng}`;
  });

  map.on('zoomend', () => {
    document.getElementById('mo-zoom').textContent = `ZOOM: ${map.getZoom()}`;
  });
  document.getElementById('mo-zoom').textContent = `ZOOM: ${map.getZoom()}`;
}

function addAttackMarker(attack) {
  const color = attack.intercepted ? '#0088ff' : (SEV_COLOR[attack.severity] || '#ffffff');
  const r = attack.severity === 'CRITICAL' ? 10
          : attack.severity === 'HIGH'     ? 8
          : attack.severity === 'MEDIUM'   ? 6
          : 5;

  // Círculo principal
  const circle = L.circleMarker([attack.lat, attack.lng], {
    radius: r,
    color: color,
    fillColor: color,
    fillOpacity: attack.intercepted ? 0.3 : 0.7,
    weight: 2,
    opacity: 1,
  });

  // Anel de pulso (adicional)
  const pulse = L.circleMarker([attack.lat, attack.lng], {
    radius: r + 4,
    color: color,
    fillColor: 'transparent',
    fillOpacity: 0,
    weight: 1,
    opacity: 0.5,
    className: 'pulse-marker',
  });

  // Trajetória para mísseis/drones com origem conhecida
  let trajLine = null;
  if (attack.origin && !attack.intercepted) {
    trajLine = L.polyline(
      [[attack.origin.lat, attack.origin.lng], [attack.lat, attack.lng]],
      {
        color: color,
        weight: 1,
        opacity: 0.45,
        dashArray: '4 6',
      }
    ).addTo(map);

    // Marcador de origem
    L.circleMarker([attack.origin.lat, attack.origin.lng], {
      radius: 3,
      color: color,
      fillColor: color,
      fillOpacity: 0.6,
      weight: 1,
    }).addTo(map);

    state.trajectoryLines.push(trajLine);
  }

  // Popup
  const sevColors = { CRITICAL: '#ff1a1a', HIGH: '#ff6600', MEDIUM: '#ffd700', LOW: '#44ff88' };
  const sevColor = sevColors[attack.severity] || '#ffffff';
  const icon = TYPE_ICONS[attack.type] || '●';

  circle.bindPopup(`
    <div class="popup-content">
      <div class="popup-id">${attack.id} — ${fmtTime(attack.timestamp)}</div>
      <div class="popup-type">${icon} ${attack.typeName || attack.type.toUpperCase()}</div>
      <div class="popup-desc">${attack.description}</div>
      <div class="popup-row">
        <span class="popup-label">REGIÃO</span>
        <span class="popup-value">${attack.region}, ${attack.country}</span>
      </div>
      <div class="popup-row">
        <span class="popup-label">ALVO</span>
        <span class="popup-value">${attack.target}</span>
      </div>
      <div class="popup-row">
        <span class="popup-label">BAIXAS EST.</span>
        <span class="popup-value">${attack.intercepted ? '0 (intercetado)' : attack.casualties}</span>
      </div>
      <div class="popup-row">
        <span class="popup-label">GRAVIDADE</span>
        <span class="popup-sev" style="color:${sevColor}">${attack.severity}</span>
      </div>
      ${attack.origin ? `<div class="popup-row"><span class="popup-label">ORIGEM</span><span class="popup-value">${attack.origin.name}</span></div>` : ''}
      ${attack.intercepted ? '<div class="popup-intercepted">✓ INTERCETADO / DESTRUÍDO</div>' : ''}
    </div>
  `, { maxWidth: 260 });

  circle.addTo(map);
  pulse.addTo(map);

  // Timeout para remover marcadores antigos (mantém últimos 80)
  state.mapMarkers.set(attack.id, { circle, pulse, trajLine });

  if (state.mapMarkers.size > 80) {
    const firstKey = state.mapMarkers.keys().next().value;
    const old = state.mapMarkers.get(firstKey);
    if (old.circle) map.removeLayer(old.circle);
    if (old.pulse)  map.removeLayer(old.pulse);
    if (old.trajLine) map.removeLayer(old.trajLine);
    state.mapMarkers.delete(firstKey);
  }

  document.getElementById('mo-active-count').textContent = `MARCADORES: ${state.mapMarkers.size}`;

  // Pulsar anel e remover
  let pulseOpacity = 0.5;
  let pulseRadius = r + 4;
  const pulseFn = setInterval(() => {
    pulseOpacity -= 0.025;
    pulseRadius  += 0.5;
    if (pulseOpacity <= 0 || !map.hasLayer(pulse)) {
      clearInterval(pulseFn);
      if (map.hasLayer(pulse)) map.removeLayer(pulse);
      return;
    }
    pulse.setStyle({ opacity: pulseOpacity });
    pulse.setRadius(pulseRadius);
  }, 60);

  // Para ataques críticos, adicionar flash de tela
  if (attack.severity === 'CRITICAL' && !attack.intercepted) {
    state.criticalQueue.push(attack);
    processCriticalQueue();
  }

  // Adicionar blip ao radar
  addRadarBlip(attack);
}

// ─── ALERTA CRÍTICO ────────────────────────────────────────────────────────
function processCriticalQueue() {
  if (state.criticalTimer || state.criticalQueue.length === 0) return;
  const attack = state.criticalQueue.shift();
  const overlay = document.getElementById('critical-alert-overlay');
  document.getElementById('ca-desc').textContent = attack.description;
  overlay.classList.remove('hidden');
  state.criticalTimer = setTimeout(() => {
    overlay.classList.add('hidden');
    state.criticalTimer = null;
    if (state.criticalQueue.length > 0) processCriticalQueue();
  }, 3500);
}

// ─── RADAR CANVAS ──────────────────────────────────────────────────────────
function initRadar() {
  const canvas = document.getElementById('radarCanvas');
  const ctx = canvas.getContext('2d');
  const W = canvas.width;
  const H = canvas.height;
  const cx = W / 2;
  const cy = H / 2;
  const R = Math.min(W, H) / 2 - 4;

  function drawRadar(ts) {
    ctx.clearRect(0, 0, W, H);

    // Fundo
    ctx.fillStyle = '#010d18';
    ctx.beginPath();
    ctx.arc(cx, cy, R, 0, Math.PI * 2);
    ctx.fill();

    // Rings
    ctx.strokeStyle = 'rgba(0,212,255,0.2)';
    ctx.lineWidth = 0.5;
    for (let i = 1; i <= 4; i++) {
      ctx.beginPath();
      ctx.arc(cx, cy, R * i / 4, 0, Math.PI * 2);
      ctx.stroke();
    }

    // Linhas cruzadas
    ctx.strokeStyle = 'rgba(0,212,255,0.15)';
    ctx.beginPath();
    ctx.moveTo(cx - R, cy); ctx.lineTo(cx + R, cy);
    ctx.moveTo(cx, cy - R); ctx.lineTo(cx, cy + R);
    ctx.stroke();

    // Linha diagonal
    ctx.strokeStyle = 'rgba(0,212,255,0.08)';
    ctx.beginPath();
    ctx.moveTo(cx - R * 0.707, cy - R * 0.707);
    ctx.lineTo(cx + R * 0.707, cy + R * 0.707);
    ctx.moveTo(cx + R * 0.707, cy - R * 0.707);
    ctx.lineTo(cx - R * 0.707, cy + R * 0.707);
    ctx.stroke();

    // Sweep
    state.radarAngle = (state.radarAngle + 0.025) % (Math.PI * 2);
    const sweepGrad = ctx.createConicalGradient
      ? ctx.createConicalGradient(cx, cy, state.radarAngle)
      : null;

    // Traço de varrimento
    ctx.save();
    ctx.beginPath();
    ctx.arc(cx, cy, R, 0, Math.PI * 2);
    ctx.clip();

    // Gradiente do sweep
    const sweepLen = Math.PI * 0.6;
    for (let i = 0; i < 30; i++) {
      const a = state.radarAngle - (i / 30) * sweepLen;
      const alpha = (1 - i / 30) * 0.25;
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.arc(cx, cy, R, a, a - 0.02);
      ctx.closePath();
      ctx.fillStyle = `rgba(0,255,159,${alpha})`;
      ctx.fill();
    }

    // Linha de varrimento
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(cx + R * Math.cos(state.radarAngle), cy + R * Math.sin(state.radarAngle));
    ctx.strokeStyle = 'rgba(0,255,159,0.9)';
    ctx.lineWidth = 1.5;
    ctx.stroke();
    ctx.restore();

    // Blips
    const now = Date.now();
    state.radarBlips = state.radarBlips.filter(b => now - b.born < 6000);
    state.radarBlips.forEach(b => {
      const age = (now - b.born) / 6000;
      const alpha = 1 - age;
      const pulse = Math.sin(now * 0.005 + b.phase) * 0.3 + 0.7;
      ctx.beginPath();
      ctx.arc(b.x, b.y, 3, 0, Math.PI * 2);
      ctx.fillStyle = b.color + Math.floor(alpha * pulse * 255).toString(16).padStart(2, '0');
      ctx.fill();
    });

    // Borda
    const borderGrad = ctx.createRadialGradient(cx, cy, R - 2, cx, cy, R + 2);
    borderGrad.addColorStop(0, 'rgba(0,212,255,0.6)');
    borderGrad.addColorStop(1, 'rgba(0,212,255,0)');
    ctx.beginPath();
    ctx.arc(cx, cy, R, 0, Math.PI * 2);
    ctx.strokeStyle = 'rgba(0,212,255,0.5)';
    ctx.lineWidth = 1.5;
    ctx.stroke();

    // Label centro
    ctx.fillStyle = 'rgba(0,212,255,0.4)';
    ctx.font = '7px Share Tech Mono';
    ctx.textAlign = 'center';
    ctx.fillText('ME-OPS', cx, cy + 2);

    state.radarAnimId = requestAnimationFrame(drawRadar);
  }

  state.radarAnimId = requestAnimationFrame(drawRadar);
}

// Converte coordenadas geográficas em posição no canvas do radar
function geoToRadar(lat, lng) {
  const canvas = document.getElementById('radarCanvas');
  const cx = canvas.width / 2;
  const cy = canvas.height / 2;
  const R = Math.min(canvas.width, canvas.height) / 2 - 4;

  // Centro do radar: lat 30, lng 40 (Médio Oriente)
  const cLat = 29.5; const cLng = 39.0;
  const scale = 3.5; // graus por raio

  const dx = ((lng - cLng) / scale) * R;
  const dy = -((lat - cLat) / scale) * R;

  // Clamp dentro do raio
  const dist = Math.sqrt(dx * dx + dy * dy);
  if (dist > R) {
    const factor = (R - 4) / dist;
    return { x: cx + dx * factor, y: cy + dy * factor };
  }
  return { x: cx + dx, y: cy + dy };
}

function addRadarBlip(attack) {
  const pos = geoToRadar(attack.lat, attack.lng);
  const color = attack.intercepted ? '#0088ff' : (SEV_COLOR[attack.severity] || '#ffffff');
  state.radarBlips.push({
    x: pos.x, y: pos.y,
    color: color.slice(0, 7),
    born: Date.now(),
    phase: Math.random() * Math.PI * 2,
  });
}

// ─── GRÁFICOS CHART.JS ─────────────────────────────────────────────────────
const CHART_DEFAULTS = {
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: {
      labels: {
        color: '#5a8fa0',
        font: { family: 'Share Tech Mono', size: 10 },
        boxWidth: 10,
        padding: 8,
      }
    },
    tooltip: {
      backgroundColor: 'rgba(2,11,20,0.95)',
      borderColor: '#0d4a6e',
      borderWidth: 1,
      titleColor: '#00d4ff',
      bodyColor: '#c8e8f0',
      titleFont: { family: 'Orbitron', size: 10 },
      bodyFont: { family: 'Share Tech Mono', size: 11 },
    }
  }
};

function initTypeChart(byType, types) {
  const ctx = document.getElementById('typeChart').getContext('2d');
  const labels = types.map(t => t.name.toUpperCase());
  const data   = types.map(t => byType[t.id] || 0);
  const colors = types.map(t => t.color);

  state.typeChart = new Chart(ctx, {
    type: 'doughnut',
    data: {
      labels,
      datasets: [{
        data,
        backgroundColor: colors.map(c => c + '99'),
        borderColor: colors,
        borderWidth: 1.5,
        hoverBorderWidth: 2,
        hoverOffset: 4,
      }]
    },
    options: {
      ...CHART_DEFAULTS,
      cutout: '60%',
      plugins: {
        ...CHART_DEFAULTS.plugins,
        legend: {
          ...CHART_DEFAULTS.plugins.legend,
          position: 'right',
          labels: {
            ...CHART_DEFAULTS.plugins.legend.labels,
            font: { family: 'Share Tech Mono', size: 9 },
            boxWidth: 8,
          }
        }
      }
    }
  });
}

function updateTypeChart(byType, types) {
  if (!state.typeChart) return;
  state.typeChart.data.datasets[0].data = types.map(t => byType[t.id] || 0);
  state.typeChart.update('none');
}

function initTimelineChart(byHour) {
  const ctx = document.getElementById('timelineChart').getContext('2d');
  const hours = Array.from({ length: 24 }, (_, i) => `${String(i).padStart(2, '0')}:00`);
  const colors = byHour.map(v => {
    const max = Math.max(...byHour, 1);
    const ratio = v / max;
    if (ratio > 0.75) return '#ff1a1a';
    if (ratio > 0.5)  return '#ff6600';
    if (ratio > 0.25) return '#ffd700';
    return '#00ff9f';
  });

  state.timelineChart = new Chart(ctx, {
    type: 'bar',
    data: {
      labels: hours,
      datasets: [{
        label: 'ATAQUES/HORA',
        data: byHour,
        backgroundColor: colors.map(c => c + 'aa'),
        borderColor: colors,
        borderWidth: 1,
        borderRadius: 2,
      }]
    },
    options: {
      ...CHART_DEFAULTS,
      scales: {
        x: {
          ticks: { color: '#3a6f80', font: { family: 'Share Tech Mono', size: 8 }, maxRotation: 0 },
          grid: { color: 'rgba(0,212,255,0.05)', drawBorder: false },
        },
        y: {
          ticks: { color: '#3a6f80', font: { family: 'Share Tech Mono', size: 9 } },
          grid: { color: 'rgba(0,212,255,0.07)', drawBorder: false },
          beginAtZero: true,
        }
      },
      plugins: { ...CHART_DEFAULTS.plugins, legend: { display: false } },
    }
  });
}

function updateTimelineChart(byHour) {
  if (!state.timelineChart) return;
  const max = Math.max(...byHour, 1);
  const colors = byHour.map(v => {
    const ratio = v / max;
    if (ratio > 0.75) return '#ff1a1a';
    if (ratio > 0.5)  return '#ff6600';
    if (ratio > 0.25) return '#ffd700';
    return '#00ff9f';
  });
  state.timelineChart.data.datasets[0].data = byHour;
  state.timelineChart.data.datasets[0].backgroundColor = colors.map(c => c + 'aa');
  state.timelineChart.data.datasets[0].borderColor = colors;
  state.timelineChart.update('none');
}

function initCountryChart(byRegion, zones) {
  const ctx = document.getElementById('countryChart').getContext('2d');

  // Agregar por país
  const countryMap = {};
  zones.forEach(z => {
    if (!countryMap[z.country]) countryMap[z.country] = 0;
    countryMap[z.country] += (byRegion[z.id] || 0);
  });

  const sorted = Object.entries(countryMap).sort((a, b) => b[1] - a[1]).slice(0, 8);
  const labels = sorted.map(([k]) => k);
  const data   = sorted.map(([, v]) => v);

  const barColors = [
    '#ff1a1a','#ff4400','#ff6600','#ff9900',
    '#ffd700','#aaff00','#00ff9f','#00d4ff',
  ];

  state.countryChart = new Chart(ctx, {
    type: 'bar',
    data: {
      labels,
      datasets: [{
        label: 'ATAQUES',
        data,
        backgroundColor: barColors.slice(0, data.length).map(c => c + '99'),
        borderColor: barColors.slice(0, data.length),
        borderWidth: 1,
        borderRadius: 2,
      }]
    },
    options: {
      ...CHART_DEFAULTS,
      indexAxis: 'y',
      scales: {
        x: {
          ticks: { color: '#3a6f80', font: { family: 'Share Tech Mono', size: 9 } },
          grid: { color: 'rgba(0,212,255,0.06)', drawBorder: false },
          beginAtZero: true,
        },
        y: {
          ticks: { color: '#c8e8f0', font: { family: 'Share Tech Mono', size: 9 } },
          grid: { display: false },
        }
      },
      plugins: { ...CHART_DEFAULTS.plugins, legend: { display: false } },
    }
  });
}

function updateCountryChart(byRegion, zones) {
  if (!state.countryChart) return;
  const countryMap = {};
  zones.forEach(z => {
    if (!countryMap[z.country]) countryMap[z.country] = 0;
    countryMap[z.country] += (byRegion[z.id] || 0);
  });
  const sorted = Object.entries(countryMap).sort((a, b) => b[1] - a[1]).slice(0, 8);
  state.countryChart.data.labels = sorted.map(([k]) => k);
  state.countryChart.data.datasets[0].data = sorted.map(([, v]) => v);
  state.countryChart.update('none');
}

// ─── AMEAÇAS POR REGIÃO ────────────────────────────────────────────────────
function renderRegionThreats(threatLevels, zones) {
  if (!zones || !threatLevels) return;
  const container = document.getElementById('region-threat-list');

  const sorted = [...zones].sort((a, b) => {
    return (threatLevels[b.id] || 0) - (threatLevels[a.id] || 0);
  });

  container.innerHTML = '';
  sorted.slice(0, 12).forEach(zone => {
    const pct = Math.min(100, threatLevels[zone.id] || zone.threatBase || 0);
    const tag = pct >= 80 ? 'CRÍTICO' : pct >= 60 ? 'ALTO' : pct >= 40 ? 'MÉDIO' : 'BAIXO';
    const tagClass = pct >= 80 ? 'tag-critical' : pct >= 60 ? 'tag-high' : pct >= 40 ? 'tag-medium' : 'tag-low';
    const color = pct >= 80 ? '#ff1a1a' : pct >= 60 ? '#ff6600' : pct >= 40 ? '#ffd700' : '#44ff88';

    const row = document.createElement('div');
    row.className = 'region-row';
    row.innerHTML = `
      <div class="region-row-header">
        <span class="region-name">${zone.name.toUpperCase()}</span>
        <div class="region-value-wrap">
          <span class="region-pct" style="color:${color}">${pct}%</span>
          <span class="region-tag ${tagClass}">${tag}</span>
        </div>
      </div>
      <div class="region-bar-bg">
        <div class="region-bar-fill" style="width:${pct}%; background:linear-gradient(90deg, ${color}88, ${color})"></div>
      </div>
    `;
    container.appendChild(row);
  });
}

// ─── ATTACK LOG ────────────────────────────────────────────────────────────
function addLogEntry(attack, prepend = true) {
  const log = document.getElementById('attack-log');
  const icon = TYPE_ICONS[attack.type] || '●';
  const entry = document.createElement('div');
  entry.className = `log-entry sev-${attack.severity}${attack.intercepted ? ' intercepted' : ''}`;
  entry.dataset.id = attack.id;
  entry.innerHTML = `
    <div class="le-header">
      <span class="le-id">${attack.id}</span>
      <span class="le-time">${fmtTime(attack.timestamp)}</span>
    </div>
    <div class="le-header">
      <span class="le-type" style="color:${attack.typeColor || '#ccc'}">${icon} ${attack.typeName || attack.type}</span>
      <span class="le-sev sev-${attack.severity}">${attack.severity}</span>
    </div>
    <div class="le-desc">${attack.description}</div>
    <div class="le-meta">
      <span class="le-region">📍 ${attack.region}</span>
      <span class="le-target">🎯 ${attack.target}</span>
      ${attack.intercepted ? '<span class="intercepted-badge">✓ INTERCETADO</span>' : ''}
    </div>
  `;

  // Ao clicar, centralizar mapa
  entry.addEventListener('click', () => {
    map.setView([attack.lat, attack.lng], 7, { animate: true });
    const m = state.mapMarkers.get(attack.id);
    if (m && m.circle) m.circle.openPopup();
  });

  if (prepend && log.firstChild) {
    log.insertBefore(entry, log.firstChild);
  } else {
    log.appendChild(entry);
  }

  // Limitar entradas no log
  while (log.children.length > 60) {
    log.removeChild(log.lastChild);
  }
}

// ─── ESTATÍSTICAS ──────────────────────────────────────────────────────────
function updateStats(s) {
  const set = (id, val) => {
    const el = document.getElementById(id);
    if (el) el.textContent = fmt(val);
  };
  set('st-total', s.totalAttacks);
  set('st-casualties', s.totalCasualties);
  set('st-zones', s.activeZones || 0);
  set('st-critical', s.criticalAlerts);
  set('st-intercepted', s.intercepted);
  document.getElementById('footer-atk-count').textContent = `ATQ: ${fmt(s.totalAttacks)}`;

  // Ataques na última hora
  const h = new Date().getHours();
  set('st-lasthour', s.byHour ? s.byHour[h] : 0);

  // Barra ameaça global
  const maxThreat = s.threatLevels
    ? Math.max(...Object.values(s.threatLevels), 0)
    : 85;
  const barPct = Math.min(100, maxThreat);
  document.getElementById('gt-bar-fill').style.width = barPct + '%';

  const threatEl = document.getElementById('global-threat-value');
  if (barPct >= 80) {
    threatEl.textContent = 'CRÍTICO';
    threatEl.style.color = '#ff1a1a';
  } else if (barPct >= 60) {
    threatEl.textContent = 'ALTO';
    threatEl.style.color = '#ff6600';
  } else if (barPct >= 40) {
    threatEl.textContent = 'MÉDIO';
    threatEl.style.color = '#ffd700';
  } else {
    threatEl.textContent = 'BAIXO';
    threatEl.style.color = '#44ff88';
  }
}

// ─── TICKER ────────────────────────────────────────────────────────────────
function addTickerMessage(msg, severity = null) {
  const track = document.getElementById('ticker-track');
  const span = document.createElement('span');
  span.className = `ticker-item${severity === 'CRITICAL' ? ' critical' : severity === 'HIGH' ? ' high' : ''}`;
  span.textContent = `[${new Date().toLocaleTimeString('pt-PT', { hour: '2-digit', minute: '2-digit' })}] ${msg}`;
  track.appendChild(span);

  // Manter apenas últimas 15 mensagens
  while (track.children.length > 15) {
    track.removeChild(track.firstChild);
  }
}

function resetTickerAnimation() {
  const track = document.getElementById('ticker-track');
  track.style.animation = 'none';
  track.offsetHeight; // reflow
  track.style.animation = '';
}

// ─── SOCKET.IO ─────────────────────────────────────────────────────────────
function initSocket() {
  const socket = io();

  socket.on('connect', () => {
    document.getElementById('status-dot').className = 'status-dot green';
    document.getElementById('status-text').textContent = 'LIGADO';
  });

  socket.on('disconnect', () => {
    document.getElementById('status-dot').className = 'status-dot red';
    document.getElementById('status-text').textContent = 'DESLIGADO';
  });

  socket.on('initial_data', data => {
    state.attacks = data.attacks || [];
    state.stats = data.stats || {};
    state.zones = data.zones || [];
    state.attackTypes = data.attackTypes || [];

    // Inicializar gráficos
    initTypeChart(state.stats.byType || {}, state.attackTypes);
    initTimelineChart(state.stats.byHour || Array(24).fill(0));
    initCountryChart(state.stats.byRegion || {}, state.zones);

    // Preencher log com historial recente
    state.attacks.slice(0, 40).reverse().forEach(a => addLogEntry(a, false));

    // Adicionar marcadores históricos (últimos 50)
    state.attacks.slice(0, 50).forEach(a => addAttackMarker(a));

    // Render ameaças por região
    renderRegionThreats(state.stats.threatLevels || {}, state.zones);

    // Estatísticas
    updateStats(state.stats);

    // Mensagens ticker iniciais
    state.attacks.slice(0, 5).forEach(a => addTickerMessage(a.description, a.severity));
  });

  socket.on('new_attack', attack => {
    state.attacks.unshift(attack);

    // Mapa
    addAttackMarker(attack);

    // Log
    addLogEntry(attack, true);

    // Ticker
    addTickerMessage(attack.description, attack.severity);
    resetTickerAnimation();
  });

  socket.on('stats_update', stats => {
    state.stats = stats;
    updateStats(stats);
    updateTypeChart(stats.byType || {}, state.attackTypes);
    updateTimelineChart(stats.byHour || Array(24).fill(0));
    updateCountryChart(stats.byRegion || {}, state.zones);
    renderRegionThreats(stats.threatLevels || {}, state.zones);
  });

  socket.on('intel_message', ({ text, ts }) => {
    addTickerMessage(`⚡ ${text}`);
  });
}

// ─── BOOT ───────────────────────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', () => {
  startClock();
  initMap();
  initRadar();
  initSocket();

  // Mensagem ticker de arranque
  setTimeout(() => {
    addTickerMessage('Sistema de rastreamento ativo — Monitorização do teatro de operações do Médio Oriente iniciada');
  }, 1500);
});
