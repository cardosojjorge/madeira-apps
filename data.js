const CONFLICT_DATA = {
    regions: {
        gaza: { name: "Faixa de Gaza", lat: 31.3547, lon: 34.3088, radius: 0.15 },
        westbank: { name: "Cisjordânia", lat: 31.9522, lon: 35.2332, radius: 0.3 },
        beirut: { name: "Beirute", lat: 33.8938, lon: 35.5018, radius: 0.1 },
        southLebanon: { name: "Sul do Líbano", lat: 33.2721, lon: 35.2033, radius: 0.2 },
        damascus: { name: "Damasco", lat: 33.5138, lon: 36.2765, radius: 0.15 },
        aleppo: { name: "Alepo", lat: 36.2021, lon: 37.1343, radius: 0.2 },
        tehran: { name: "Teerão", lat: 35.6892, lon: 51.3890, radius: 0.1 },
        bagdad: { name: "Bagdade", lat: 33.3152, lon: 44.3661, radius: 0.15 },
        yemen: { name: "Iémen (Sanaa)", lat: 15.3694, lon: 44.1910, radius: 0.2 },
        rafah: { name: "Rafah", lat: 31.2969, lon: 34.2455, radius: 0.05 },
        khanYounis: { name: "Khan Younis", lat: 31.3462, lon: 34.3032, radius: 0.06 },
        jabalia: { name: "Jabalia", lat: 31.5280, lon: 34.4831, radius: 0.04 },
        golan: { name: "Colinas de Golã", lat: 33.0000, lon: 35.7500, radius: 0.15 },
        idlib: { name: "Idlib", lat: 35.9306, lon: 36.6339, radius: 0.2 },
        hodeida: { name: "Hodeida", lat: 14.7979, lon: 42.9541, radius: 0.1 },
        aden: { name: "Áden", lat: 12.7855, lon: 45.0187, radius: 0.1 },
        tikrit: { name: "Tikrit", lat: 34.6122, lon: 43.6793, radius: 0.1 },
        deirEzZor: { name: "Deir ez-Zor", lat: 35.3359, lon: 40.1408, radius: 0.15 },
        sinai: { name: "Sinai Norte", lat: 31.0, lon: 33.8, radius: 0.2 },
        strait: { name: "Estreito de Hormuz", lat: 26.5667, lon: 56.2500, radius: 0.2 },
    },

    attackTypes: [
        { id: "airstrike", name: "Ataque Aéreo", icon: "✈", weight: 30 },
        { id: "missile", name: "Míssil", icon: "🚀", weight: 25 },
        { id: "ground", name: "Terrestre", icon: "⚔", weight: 20 },
        { id: "drone", name: "Drone", icon: "◉", weight: 15 },
        { id: "naval", name: "Naval", icon: "⚓", weight: 5 },
        { id: "explosion", name: "Explosão/IED", icon: "💥", weight: 5 },
    ],

    factions: [
        "IDF", "Hamas", "Hezbollah", "Houthis", "IRGC",
        "Milícias Iraquianas", "Forças Sírias", "Forças Coligação",
        "PKK/YPG", "ISIS Remanescentes", "Forças Russas", "Não Identificado"
    ],

    weapons: [
        "JDAM GBU-31", "AGM-114 Hellfire", "M270 MLRS", "Qassam-3",
        "Fajr-5", "Fateh-110", "Shahed-136", "Bayraktar TB2",
        "RPG-7", "IED Improvisado", "Kornet ATGM", "S-300 SAM",
        "Burkan-2H", "Tomahawk", "Iron Dome Intercept", "AK-47/Armas Ligeiras",
        "Artilharia M109", "Morteiro 120mm", "Zelzal-2", "Toophan ATGM"
    ],

    severityLevels: ["BAIXO", "MÉDIO", "ALTO", "CRÍTICO"],

    descriptions: [
        "Bombardeamento aéreo detectado na zona urbana",
        "Lançamento de mísseis balísticos de curto alcance",
        "Intercetação de projéteis sobre zona civil",
        "Operação terrestre em curso — combate urbano",
        "Drone de reconhecimento abatido sobre zona restrita",
        "Explosão reportada próxima a infraestrutura militar",
        "Movimento de forças blindadas detectado por satélite",
        "Disparo de artilharia pesada contra posições defensivas",
        "Emboscada a comboio logístico na rota principal",
        "Ataque com drone suicida contra base operacional",
        "Incursão naval em águas territoriais disputadas",
        "Sabotagem de infraestrutura energética confirmada",
        "Activação de defesa anti-aérea — múltiplos alvos",
        "Confronto entre milícias rivais na fronteira",
        "Evacuação médica em curso após ataque intenso",
        "Detecção de túnel transfronteiriço — explosivos encontrados",
        "Infiltração de forças especiais reportada",
        "Bombardeamento de retaliação após provocação fronteiriça",
        "Intercetação de comunicações inimigas — alerta elevado",
        "Impacto em hospital de campanha — baixas civis elevadas"
    ],

    alerts: [
        { text: "ALERTA: Lançamento de mísseis balísticos detectado — origem: Teerão", level: "critical" },
        { text: "ALERTA: Movimentação massiva de tropas na fronteira norte", level: "critical" },
        { text: "AVISO: Sistema de defesa aérea ativado — setor 7", level: "warning" },
        { text: "INFO: Satélite MILSAT-7 reposicionado para cobertura ampliada", level: "info" },
        { text: "ALERTA: Comunicações inimigas intercetadas — possível ataque iminente", level: "critical" },
        { text: "AVISO: Drone não identificado em espaço aéreo restrito", level: "warning" },
        { text: "INFO: Reforços aliados a caminho — ETA 45 min", level: "info" },
        { text: "ALERTA: Explosão em depósito de munições — causa em investigação", level: "critical" },
        { text: "AVISO: Nível de ameaça elevado para zona costeira", level: "warning" },
        { text: "INFO: Actualização de inteligência recebida — análise em curso", level: "info" },
        { text: "ALERTA: Ataque coordenado multi-vector em progresso", level: "critical" },
        { text: "AVISO: Interferência electromagnética detectada — possível jamming", level: "warning" },
        { text: "ALERTA: Violação de cessar-fogo confirmada — setor 12", level: "critical" },
        { text: "AVISO: Forças hostis cruzaram linha de demarcação", level: "warning" },
        { text: "INFO: Missão de reconhecimento completada — dados processados", level: "info" }
    ]
};

function getRandomRegion() {
    const keys = Object.keys(CONFLICT_DATA.regions);
    return CONFLICT_DATA.regions[keys[Math.floor(Math.random() * keys.length)]];
}

function getWeightedAttackType() {
    const types = CONFLICT_DATA.attackTypes;
    const totalWeight = types.reduce((sum, t) => sum + t.weight, 0);
    let rand = Math.random() * totalWeight;
    for (const type of types) {
        rand -= type.weight;
        if (rand <= 0) return type;
    }
    return types[0];
}

function randomInRange(min, max) {
    return min + Math.random() * (max - min);
}

function generateAttackEvent() {
    const region = getRandomRegion();
    const type = getWeightedAttackType();
    const lat = region.lat + (Math.random() - 0.5) * region.radius * 2;
    const lon = region.lon + (Math.random() - 0.5) * region.radius * 2;
    const severity = Math.floor(Math.random() * 4);
    const now = new Date();

    return {
        id: `ATK-${Date.now()}-${Math.random().toString(36).substr(2, 6).toUpperCase()}`,
        timestamp: now,
        lat: lat,
        lon: lon,
        region: region.name,
        type: type,
        severity: CONFLICT_DATA.severityLevels[severity],
        severityIndex: severity,
        faction: CONFLICT_DATA.factions[Math.floor(Math.random() * CONFLICT_DATA.factions.length)],
        weapon: CONFLICT_DATA.weapons[Math.floor(Math.random() * CONFLICT_DATA.weapons.length)],
        description: CONFLICT_DATA.descriptions[Math.floor(Math.random() * CONFLICT_DATA.descriptions.length)],
        casualties: severity >= 2 ? Math.floor(Math.random() * 50) + 1 : Math.floor(Math.random() * 10),
        confirmed: Math.random() > 0.3
    };
}

function generateAlert() {
    const alert = CONFLICT_DATA.alerts[Math.floor(Math.random() * CONFLICT_DATA.alerts.length)];
    return {
        ...alert,
        timestamp: new Date(),
        id: `ALR-${Date.now()}-${Math.random().toString(36).substr(2, 4).toUpperCase()}`
    };
}

function generateInitialEvents(count) {
    const events = [];
    for (let i = 0; i < count; i++) {
        const event = generateAttackEvent();
        event.timestamp = new Date(Date.now() - Math.random() * 24 * 60 * 60 * 1000);
        events.push(event);
    }
    return events.sort((a, b) => b.timestamp - a.timestamp);
}
