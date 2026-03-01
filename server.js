const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = socketIo(server, {
  cors: { origin: '*' }
});

app.use(express.static(path.join(__dirname, 'public')));

// ─── DADOS GEOGRÁFICOS DO MÉDIO ORIENTE ───────────────────────────────────
const ZONES = [
  { id: 'gaza',       name: 'Gaza',            country: 'Palestina',     lat: 31.35,  lng: 34.33,  baseFreq: 0.30, threatBase: 95 },
  { id: 'west_bank',  name: 'Cisjordânia',     country: 'Palestina',     lat: 32.05,  lng: 35.24,  baseFreq: 0.12, threatBase: 70 },
  { id: 'israel_s',   name: 'Israel Sul',      country: 'Israel',        lat: 31.25,  lng: 34.80,  baseFreq: 0.10, threatBase: 60 },
  { id: 'israel_n',   name: 'Israel Norte',    country: 'Israel',        lat: 32.90,  lng: 35.30,  baseFreq: 0.12, threatBase: 65 },
  { id: 'lebanon_s',  name: 'Sul do Líbano',   country: 'Líbano',        lat: 33.20,  lng: 35.35,  baseFreq: 0.15, threatBase: 80 },
  { id: 'beirut',     name: 'Beirute',          country: 'Líbano',        lat: 33.89,  lng: 35.50,  baseFreq: 0.06, threatBase: 45 },
  { id: 'damascus',   name: 'Damasco',          country: 'Síria',         lat: 33.51,  lng: 36.29,  baseFreq: 0.08, threatBase: 55 },
  { id: 'aleppo',     name: 'Alepo',            country: 'Síria',         lat: 36.20,  lng: 37.16,  baseFreq: 0.06, threatBase: 50 },
  { id: 'homs',       name: 'Homs',             country: 'Síria',         lat: 34.73,  lng: 36.72,  baseFreq: 0.05, threatBase: 45 },
  { id: 'golan',      name: 'Colinas do Golan', country: 'Síria',        lat: 33.05,  lng: 35.80,  baseFreq: 0.08, threatBase: 60 },
  { id: 'baghdad',    name: 'Bagdade',          country: 'Iraque',        lat: 33.34,  lng: 44.40,  baseFreq: 0.07, threatBase: 50 },
  { id: 'mosul',      name: 'Mossul',           country: 'Iraque',        lat: 36.34,  lng: 43.13,  baseFreq: 0.05, threatBase: 40 },
  { id: 'basra',      name: 'Basra',            country: 'Iraque',        lat: 30.50,  lng: 47.78,  baseFreq: 0.04, threatBase: 35 },
  { id: 'sanaa',      name: 'Sanaa',            country: 'Iémen',         lat: 15.37,  lng: 44.19,  baseFreq: 0.10, threatBase: 75 },
  { id: 'hodeidah',   name: 'Hodeidah',         country: 'Iémen',         lat: 14.80,  lng: 42.95,  baseFreq: 0.08, threatBase: 65 },
  { id: 'aden',       name: 'Aden',             country: 'Iémen',         lat: 12.78,  lng: 45.03,  baseFreq: 0.05, threatBase: 55 },
  { id: 'tehran',     name: 'Teerão',           country: 'Irão',          lat: 35.69,  lng: 51.39,  baseFreq: 0.04, threatBase: 30 },
  { id: 'red_sea',    name: 'Mar Vermelho',     country: 'Águas Internacionais', lat: 15.0, lng: 42.5, baseFreq: 0.10, threatBase: 70 },
  { id: 'sinai',      name: 'Sinai',            country: 'Egito',         lat: 30.00,  lng: 34.00,  baseFreq: 0.04, threatBase: 30 },
];

const ATTACK_TYPES = [
  { id: 'airstrike',  name: 'Ataque Aéreo',    icon: '✈', color: '#ff4444', weight: 20 },
  { id: 'missile',    name: 'Míssil',          icon: '🚀', color: '#ff6600', weight: 25 },
  { id: 'drone',      name: 'Drone/UAV',       icon: '⬡',  color: '#ff9900', weight: 20 },
  { id: 'artillery',  name: 'Artilharia',      icon: '💥', color: '#ffcc00', weight: 15 },
  { id: 'ground',     name: 'Assalto Terrestre',icon: '⚔', color: '#66ff66', weight: 10 },
  { id: 'naval',      name: 'Naval',           icon: '⚓', color: '#00ccff', weight: 5  },
  { id: 'cyber',      name: 'Cibernético',     icon: '⚡', color: '#cc00ff', weight: 5  },
];

const SEVERITIES = [
  { id: 'LOW',      label: 'BAIXO',    color: '#44ff88', weight: 25 },
  { id: 'MEDIUM',   label: 'MÉDIO',    color: '#ffcc00', weight: 35 },
  { id: 'HIGH',     label: 'ALTO',     color: '#ff6600', weight: 30 },
  { id: 'CRITICAL', label: 'CRÍTICO',  color: '#ff0000', weight: 10 },
];

const TARGETS = [
  'Infraestrutura Militar', 'Base Aérea', 'Armazém de Armamento', 'Centro de Comando',
  'Posto de Controlo', 'Instalação Nuclear', 'Porto Marítimo', 'Refinaria de Petróleo',
  'Central Elétrica', 'Ponte Estratégica', 'Túnel Subterrâneo', 'Campo de Treino',
  'Radar Militar', 'Bateria Antiaérea', 'Navio de Guerra', 'Veículo Blindado',
  'Zona Residencial', 'Hospital (colateral)', 'Aeroporto', 'Sistema de Comunicações',
];

const MISSILE_ORIGINS = [
  { name: 'Gaza', lat: 31.35, lng: 34.33 },
  { name: 'Sul do Líbano', lat: 33.05, lng: 35.60 },
  { name: 'Iémen', lat: 14.80, lng: 43.00 },
  { name: 'Síria', lat: 33.50, lng: 36.30 },
  { name: 'Iraque', lat: 32.00, lng: 44.00 },
  { name: 'Irão', lat: 34.00, lng: 50.00 },
];

const INTEL_MESSAGES = [
  'ALERTA: Lançamento de múltiplos foguetes detetado no norte de Gaza — Sistema Iron Dome ativado',
  'RELATÓRIO: Explosões reportadas nos arredores de Beirute — origem desconhecida',
  'INTERCEPÇÃO: 8 mísseis balísticos interceptados sobre o Mar Vermelho por destróier USS Carney',
  'INTEL: Movimentação de tropas blindadas detetada na fronteira norte de Israel',
  'ALERTA: Ataque de drone Shahed-136 detetado na rota de Bagdade-Basra',
  'RELATÓRIO: Forças do Hezbollah disparam salva de Katyusha — impacto em Kiryat Shmona',
  'INTERCEPÇÃO: Sistema Arrow 3 abate míssil balístico sobre o espaço aéreo israelense',
  'INTEL: Satélite deteta coluna de fumo em instalação militar de Damasco',
  'ALERTA: Navio cargueiro atacado por drone naval Houthi a 40nm de Hodeidah',
  'RELATÓRIO: Ataque cibernético à infraestrutura elétrica de Sanaa — origem rastreada',
  'INTERCEPÇÃO: Comunicações inimigas bloqueadas — operação SIGINT ativa',
  'INTEL: Concentração de forças no Vale do Jordão — monitorização aumentada',
  'ALERTA: Plataforma petrolífera no Golfo Pérsico sob ameaça — força naval deslocada',
  'RELATÓRIO: Sistema de defesa aérea S-300 sírio ativado na região de Damasco',
  'INTERCEPÇÃO: 3 VANTs hostis abatidos pela Força Aérea Israelense sobre o Golan',
  'INTEL: Movimentação de submarino iraniano detetada no Golfo de Omã',
  'ALERTA: Ataque terrestre em curso na Cisjordânia — unidades de resposta rápida mobilizadas',
  'RELATÓRIO: Explosão em refinaria de petróleo em Aden — fogo ainda ativo',
  'INTERCEPÇÃO: Frequências de comunicação Hamas encriptadas — análise em andamento',
  'INTEL: Reforços de milícias pró-iranianas chegam à fronteira sírio-iraquiana',
];

// ─── ESTADO GLOBAL ─────────────────────────────────────────────────────────
let attackHistory = [];
let attackId = 1000;
let stats = {
  totalAttacks: 0,
  totalCasualties: 0,
  criticalAlerts: 0,
  intercepted: 0,
  byType: {},
  byRegion: {},
  byHour: Array(24).fill(0),
};

ATTACK_TYPES.forEach(t => { stats.byType[t.id] = 0; });
ZONES.forEach(z => { stats.byRegion[z.id] = 0; });

const threatLevels = {};
ZONES.forEach(z => { threatLevels[z.id] = z.threatBase + Math.floor(Math.random() * 5); });

// ─── FUNÇÕES AUXILIARES ────────────────────────────────────────────────────
function weightedRandom(items) {
  const total = items.reduce((s, i) => s + (i.weight || 1), 0);
  let r = Math.random() * total;
  for (const item of items) {
    r -= (item.weight || 1);
    if (r <= 0) return item;
  }
  return items[items.length - 1];
}

function randOffset(range) {
  return (Math.random() - 0.5) * range;
}

function generateDescription(type, zone, severity, target) {
  const typeMap = {
    airstrike:  [`Caças F-35 realizaram ataque aéreo em ${zone.name}`, `Bombardeiros atacaram ${target} em ${zone.name}`, `Ataques aéreos de precisão em ${zone.name}`],
    missile:    [`Salva de mísseis balísticos lançada contra ${zone.name}`, `Foguetes Qassam atingem ${zone.name}`, `Ataque com míssil Fateh-110 em ${zone.name}`],
    drone:      [`Enxame de drones UAV detetado sobre ${zone.name}`, `Drone Shahed-136 intercetado perto de ${zone.name}`, `Ataque com VANT kamikaze em ${zone.name}`],
    artillery:  [`Artilharia pesada em ${zone.name}`, `Bombardeamento de morteiros em ${zone.name}`, `Salva de obuses em ${zone.name}`],
    ground:     [`Avanço de forças terrestres em ${zone.name}`, `Conflito de infantaria em ${zone.name}`, `Coluna blindada avança em ${zone.name}`],
    naval:      [`Bombardeamento naval próximo a ${zone.name}`, `Navio de guerra abre fogo em ${zone.name}`, `Drone naval ataca embarcação perto de ${zone.name}`],
    cyber:      [`Ataque cibernético à infraestrutura de ${zone.name}`, `Sistema elétrico comprometido em ${zone.name}`, `Falha nas comunicações militares em ${zone.name}`],
  };
  const opts = typeMap[type.id] || [`Incidente em ${zone.name}`];
  return opts[Math.floor(Math.random() * opts.length)];
}

function generateAttack() {
  const zone = ZONES[Math.floor(Math.random() * ZONES.length)];
  const type = weightedRandom(ATTACK_TYPES);
  const severity = weightedRandom(SEVERITIES);
  const target = TARGETS[Math.floor(Math.random() * TARGETS.length)];
  const casualties = severity.id === 'CRITICAL' ? Math.floor(Math.random() * 80 + 20)
                   : severity.id === 'HIGH'     ? Math.floor(Math.random() * 40 + 5)
                   : severity.id === 'MEDIUM'   ? Math.floor(Math.random() * 15)
                   :                               Math.floor(Math.random() * 5);

  const intercepted = type.id === 'missile' && Math.random() < 0.45;
  const hasOrigin = ['missile', 'drone'].includes(type.id) && Math.random() > 0.4;
  const origin = hasOrigin ? MISSILE_ORIGINS[Math.floor(Math.random() * MISSILE_ORIGINS.length)] : null;

  const attack = {
    id: `ATK-${String(++attackId).padStart(5, '0')}`,
    timestamp: new Date().toISOString(),
    type: type.id,
    typeName: type.name,
    typeColor: type.color,
    typeIcon: type.icon,
    region: zone.name,
    regionId: zone.id,
    country: zone.country,
    lat: zone.lat + randOffset(0.4),
    lng: zone.lng + randOffset(0.4),
    target,
    casualties,
    severity: severity.id,
    severityLabel: severity.label,
    severityColor: severity.color,
    intercepted,
    origin,
    description: generateDescription(type, zone, severity, target),
  };

  return attack;
}

function updateStats(attack) {
  stats.totalAttacks++;
  if (!attack.intercepted) stats.totalCasualties += attack.casualties;
  if (attack.severity === 'CRITICAL') stats.criticalAlerts++;
  if (attack.intercepted) stats.intercepted++;
  stats.byType[attack.type] = (stats.byType[attack.type] || 0) + 1;
  stats.byRegion[attack.regionId] = (stats.byRegion[attack.regionId] || 0) + 1;

  const hour = new Date(attack.timestamp).getHours();
  stats.byHour[hour]++;

  // Actualizar nível de ameaça da zona
  const zone = ZONES.find(z => z.id === attack.regionId);
  if (zone) {
    const delta = attack.severity === 'CRITICAL' ? 3
                : attack.severity === 'HIGH'     ? 2
                : attack.severity === 'MEDIUM'   ? 1
                : 0;
    threatLevels[zone.id] = Math.min(100, (threatLevels[zone.id] || zone.threatBase) + delta);
  }
}

// ─── SIMULAÇÃO DE ATAQUES ──────────────────────────────────────────────────
function scheduleNextAttack() {
  const baseInterval = 2500;
  const jitter = Math.random() * 4000;
  setTimeout(() => {
    const attack = generateAttack();
    updateStats(attack);
    attackHistory.unshift(attack);
    if (attackHistory.length > 200) attackHistory.pop();

    io.emit('new_attack', attack);
    io.emit('stats_update', {
      ...stats,
      threatLevels,
      activeZones: [...new Set(attackHistory.slice(0, 30).map(a => a.regionId))].length,
    });

    // Mensagem Intel aleatória (30% chance)
    if (Math.random() < 0.30) {
      const msg = INTEL_MESSAGES[Math.floor(Math.random() * INTEL_MESSAGES.length)];
      const ts = new Date().toLocaleTimeString('pt-PT', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
      io.emit('intel_message', { text: msg, ts });
    }

    scheduleNextAttack();
  }, baseInterval + jitter);
}

// Pré-popular com historial de 4 horas
function populateHistory() {
  const now = Date.now();
  for (let i = 240; i >= 0; i--) {
    const a = generateAttack();
    a.timestamp = new Date(now - i * 60000 - Math.random() * 60000).toISOString();
    updateStats(a);
    attackHistory.push(a);
  }
}

populateHistory();
scheduleNextAttack();

// ─── SOCKET.IO ─────────────────────────────────────────────────────────────
io.on('connection', (socket) => {
  console.log(`[CONECTADO] Cliente ${socket.id}`);

  socket.emit('initial_data', {
    attacks: attackHistory.slice(0, 80),
    stats: {
      ...stats,
      threatLevels,
      activeZones: [...new Set(attackHistory.slice(0, 30).map(a => a.regionId))].length,
    },
    zones: ZONES,
    attackTypes: ATTACK_TYPES,
  });

  socket.on('disconnect', () => {
    console.log(`[DESCONECTADO] Cliente ${socket.id}`);
  });
});

// ─── SERVIDOR ──────────────────────────────────────────────────────────────
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`\n╔══════════════════════════════════════════════════╗`);
  console.log(`║   CENTRO DE COMANDO MILITAR - SERVIDOR ATIVO    ║`);
  console.log(`║   Porta: ${PORT}   Status: OPERACIONAL              ║`);
  console.log(`╚══════════════════════════════════════════════════╝\n`);
});
