// ============================================================
// CENTCOM TACTICAL OPS — Main Application
// ============================================================

(function () {
    "use strict";

    let map;
    let markers = [];
    let events = [];
    let charts = {};
    let packetCount = 0;
    let radarAngle = 0;
    let radarBlips = [];
    const MAX_EVENTS = 60;
    const MAX_ALERTS = 12;
    const MAX_MARKERS = 150;

    // ========================
    // BOOT SEQUENCE
    // ========================
    const bootMessages = [
        "[SYS] Inicializando kernel tático v4.7.2...",
        "[SYS] Carregando módulos de encriptação AES-256...",
        "[NET] Estabelecendo uplink satelital MILSAT-7...",
        "[NET] Handshake seguro confirmado — latência 42ms",
        "[GEO] Carregando dados geoespaciais do Médio Oriente...",
        "[GEO] Mapeamento de zonas de conflito ativo...",
        "[INT] Conectando feeds de inteligência SIGINT/HUMINT...",
        "[INT] Desencriptando fluxos de dados classificados...",
        "[SAT] Imagens satelitais atualizadas — resolução 0.5m",
        "[DEF] Sistemas de defesa anti-aérea operacionais...",
        "[RAD] Radar de longo alcance calibrado — cobertura 360°",
        "[SYS] Todos os subsistemas operacionais — PRONTO"
    ];

    function runBootSequence() {
        const bootText = document.getElementById("boot-text");
        const bootProgress = document.getElementById("boot-progress-bar");
        const bootStatus = document.getElementById("boot-status");
        const bootScreen = document.getElementById("boot-screen");

        let idx = 0;
        const interval = setInterval(() => {
            if (idx < bootMessages.length) {
                bootText.innerHTML += bootMessages[idx] + "\n";
                const progress = ((idx + 1) / bootMessages.length) * 100;
                bootProgress.style.width = progress + "%";
                bootStatus.textContent = bootMessages[idx].split("] ")[1] || "PROCESSANDO...";
                idx++;
            } else {
                clearInterval(interval);
                bootStatus.textContent = "SISTEMA PRONTO — ENTRANDO NO MODO OPERACIONAL";
                setTimeout(() => {
                    bootScreen.classList.add("hidden");
                    setTimeout(() => {
                        bootScreen.remove();
                        initializeApp();
                    }, 800);
                }, 600);
            }
        }, 250);
    }

    // ========================
    // INITIALIZATION
    // ========================
    function initializeApp() {
        initMap();
        initCharts();
        initRadar();
        startClock();
        loadInitialData();
        startRealTimeSimulation();
        startLatencySimulation();
        updateHudTargets();
    }

    // ========================
    // MAP
    // ========================
    function initMap() {
        map = L.map("map", {
            center: [31.0, 38.0],
            zoom: 6,
            zoomControl: true,
            attributionControl: false,
            maxBounds: [[-5, 20], [50, 75]],
            minZoom: 4,
            maxZoom: 14
        });

        L.tileLayer("https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png", {
            subdomains: "abcd",
            maxZoom: 19
        }).addTo(map);

        drawConflictZones();

        map.on("mousemove", function (e) {
            document.getElementById("hud-lat").textContent = e.latlng.lat.toFixed(4);
            document.getElementById("hud-lon").textContent = e.latlng.lng.toFixed(4);
        });

        map.on("zoomend", function () {
            document.getElementById("hud-zoom").textContent = map.getZoom();
        });

        document.getElementById("hud-zoom").textContent = map.getZoom();
    }

    function drawConflictZones() {
        const zones = CONFLICT_DATA.regions;
        Object.values(zones).forEach(zone => {
            L.circle([zone.lat, zone.lon], {
                radius: zone.radius * 111000,
                color: "#ff224433",
                fillColor: "#ff224411",
                fillOpacity: 0.15,
                weight: 1,
                dashArray: "5 5",
                className: "conflict-zone"
            }).addTo(map);

            L.circleMarker([zone.lat, zone.lon], {
                radius: 2,
                color: "#ff2244",
                fillColor: "#ff2244",
                fillOpacity: 0.6,
                weight: 0
            }).addTo(map).bindTooltip(zone.name, {
                permanent: false,
                direction: "top",
                className: "zone-tooltip",
                offset: [0, -5]
            });
        });
    }

    function addAttackMarker(event) {
        const typeClass = `marker-${event.type.id}`;
        const size = 30 + event.severityIndex * 8;

        const icon = L.divIcon({
            className: `attack-marker ${typeClass}`,
            html: `
                <div class="attack-marker-pulse"></div>
                <div class="attack-marker-inner"></div>
                ${event.severityIndex >= 2 ? '<div class="explosion-animation"></div>' : ''}
                ${event.type.id === 'missile' ? '<div class="missile-trail"></div>' : ''}
            `,
            iconSize: [size, size],
            iconAnchor: [size / 2, size / 2]
        });

        const popupContent = `
            <div class="popup-content">
                <div class="popup-header">${event.type.icon} ${event.type.name.toUpperCase()} — ${event.id}</div>
                <div class="popup-row"><span class="popup-label">Região:</span><span class="popup-value">${event.region}</span></div>
                <div class="popup-row"><span class="popup-label">Coordenadas:</span><span class="popup-value">${event.lat.toFixed(4)}, ${event.lon.toFixed(4)}</span></div>
                <div class="popup-row"><span class="popup-label">Severidade:</span><span class="popup-value ${event.severityIndex >= 2 ? 'critical' : 'warning'}">${event.severity}</span></div>
                <div class="popup-row"><span class="popup-label">Facção:</span><span class="popup-value">${event.faction}</span></div>
                <div class="popup-row"><span class="popup-label">Armamento:</span><span class="popup-value">${event.weapon}</span></div>
                <div class="popup-row"><span class="popup-label">Baixas Est.:</span><span class="popup-value critical">${event.casualties}</span></div>
                <div class="popup-row"><span class="popup-label">Confirmado:</span><span class="popup-value">${event.confirmed ? 'SIM' : 'NÃO CONFIRMADO'}</span></div>
                <div class="popup-row"><span class="popup-label">Hora:</span><span class="popup-value">${formatTime(event.timestamp)}</span></div>
            </div>
        `;

        const marker = L.marker([event.lat, event.lon], { icon })
            .addTo(map)
            .bindPopup(popupContent, { maxWidth: 280 });

        marker._eventData = event;
        markers.push(marker);

        if (markers.length > MAX_MARKERS) {
            const oldMarker = markers.shift();
            map.removeLayer(oldMarker);
        }

        if (event.severityIndex >= 2) {
            drawImpactRing(event.lat, event.lon, event.severityIndex);
        }
    }

    function drawImpactRing(lat, lon, severity) {
        const radius = (severity + 1) * 2000;
        const ring = L.circle([lat, lon], {
            radius: radius,
            color: "#ff224466",
            fillColor: "#ff224422",
            fillOpacity: 0.1,
            weight: 1,
            dashArray: "3 6"
        }).addTo(map);

        setTimeout(() => map.removeLayer(ring), 15000);
    }

    function updateHudTargets() {
        document.getElementById("hud-targets").textContent = markers.length;
    }

    // ========================
    // CHARTS
    // ========================
    function initCharts() {
        Chart.defaults.color = "#6b8299";
        Chart.defaults.font.family = "'Share Tech Mono', monospace";
        Chart.defaults.font.size = 10;

        initAttackTypeChart();
        initTimelineChart();
        initRegionChart();
        initIntensityChart();
    }

    function initAttackTypeChart() {
        const ctx = document.getElementById("attack-type-chart").getContext("2d");
        charts.attackType = new Chart(ctx, {
            type: "doughnut",
            data: {
                labels: ["Aéreo", "Míssil", "Terrestre", "Drone", "Naval", "Explosão"],
                datasets: [{
                    data: [30, 25, 20, 15, 5, 5],
                    backgroundColor: [
                        "rgba(255, 34, 68, 0.7)",
                        "rgba(255, 136, 0, 0.7)",
                        "rgba(255, 204, 0, 0.7)",
                        "rgba(170, 68, 255, 0.7)",
                        "rgba(0, 136, 255, 0.7)",
                        "rgba(0, 221, 255, 0.7)"
                    ],
                    borderColor: [
                        "#ff2244", "#ff8800", "#ffcc00",
                        "#aa44ff", "#0088ff", "#00ddff"
                    ],
                    borderWidth: 1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                cutout: "65%",
                plugins: {
                    legend: {
                        position: "bottom",
                        labels: {
                            boxWidth: 8,
                            padding: 8,
                            font: { size: 9 }
                        }
                    }
                }
            }
        });
    }

    function initTimelineChart() {
        const ctx = document.getElementById("timeline-chart").getContext("2d");
        const labels = [];
        const data = [];
        for (let i = 23; i >= 0; i--) {
            labels.push(`${String(i).padStart(2, "0")}h`);
            data.push(Math.floor(Math.random() * 20) + 5);
        }

        charts.timeline = new Chart(ctx, {
            type: "line",
            data: {
                labels: labels,
                datasets: [{
                    label: "Ataques",
                    data: data,
                    borderColor: "#00ff44",
                    backgroundColor: "rgba(0, 255, 68, 0.08)",
                    fill: true,
                    tension: 0.3,
                    borderWidth: 1.5,
                    pointRadius: 0,
                    pointHoverRadius: 4,
                    pointHoverBackgroundColor: "#00ff44"
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    x: {
                        grid: { color: "rgba(26, 58, 42, 0.3)", drawBorder: false },
                        ticks: { maxTicksLimit: 8, font: { size: 8 } }
                    },
                    y: {
                        grid: { color: "rgba(26, 58, 42, 0.3)", drawBorder: false },
                        ticks: { font: { size: 8 } }
                    }
                },
                plugins: {
                    legend: { display: false }
                }
            }
        });
    }

    function initRegionChart() {
        const ctx = document.getElementById("region-chart").getContext("2d");
        charts.region = new Chart(ctx, {
            type: "bar",
            data: {
                labels: ["Gaza", "Líbano", "Síria", "Iémen", "Iraque", "Irão"],
                datasets: [{
                    label: "Incidentes",
                    data: [45, 28, 22, 18, 12, 8],
                    backgroundColor: [
                        "rgba(255, 34, 68, 0.6)",
                        "rgba(255, 136, 0, 0.6)",
                        "rgba(255, 204, 0, 0.6)",
                        "rgba(170, 68, 255, 0.6)",
                        "rgba(0, 136, 255, 0.6)",
                        "rgba(0, 221, 255, 0.6)"
                    ],
                    borderColor: [
                        "#ff2244", "#ff8800", "#ffcc00",
                        "#aa44ff", "#0088ff", "#00ddff"
                    ],
                    borderWidth: 1,
                    borderRadius: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                indexAxis: "y",
                scales: {
                    x: {
                        grid: { color: "rgba(26, 58, 42, 0.3)", drawBorder: false },
                        ticks: { font: { size: 8 } }
                    },
                    y: {
                        grid: { display: false },
                        ticks: { font: { size: 9 } }
                    }
                },
                plugins: {
                    legend: { display: false }
                }
            }
        });
    }

    function initIntensityChart() {
        const ctx = document.getElementById("intensity-chart").getContext("2d");
        const data = [];
        const labels = [];
        for (let i = 11; i >= 0; i--) {
            const h = new Date();
            h.setHours(h.getHours() - i);
            labels.push(`${String(h.getHours()).padStart(2, "0")}:00`);
            data.push(Math.floor(Math.random() * 100));
        }

        charts.intensity = new Chart(ctx, {
            type: "bar",
            data: {
                labels: labels,
                datasets: [{
                    label: "Intensidade",
                    data: data,
                    backgroundColor: data.map(v =>
                        v > 70 ? "rgba(255,34,68,0.7)" :
                        v > 40 ? "rgba(255,136,0,0.7)" :
                        "rgba(0,255,68,0.5)"
                    ),
                    borderWidth: 0,
                    borderRadius: 1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    x: {
                        grid: { color: "rgba(26, 58, 42, 0.3)", drawBorder: false },
                        ticks: { maxTicksLimit: 6, font: { size: 8 } }
                    },
                    y: {
                        grid: { color: "rgba(26, 58, 42, 0.3)", drawBorder: false },
                        ticks: { font: { size: 8 } },
                        max: 100
                    }
                },
                plugins: {
                    legend: { display: false }
                }
            }
        });
    }

    function updateCharts(event) {
        const typeIndex = CONFLICT_DATA.attackTypes.findIndex(t => t.id === event.type.id);
        if (typeIndex >= 0) {
            charts.attackType.data.datasets[0].data[typeIndex]++;
            charts.attackType.update("none");
        }

        const lastIdx = charts.timeline.data.datasets[0].data.length - 1;
        charts.timeline.data.datasets[0].data[lastIdx]++;
        charts.timeline.update("none");

        const regionMap = { "Faixa de Gaza": 0, "Rafah": 0, "Khan Younis": 0, "Jabalia": 0,
            "Beirute": 1, "Sul do Líbano": 1,
            "Damasco": 2, "Alepo": 2, "Idlib": 2, "Deir ez-Zor": 2, "Colinas de Golã": 2,
            "Iémen (Sanaa)": 3, "Hodeida": 3, "Áden": 3,
            "Bagdade": 4, "Tikrit": 4,
            "Teerão": 5
        };

        const rIdx = regionMap[event.region];
        if (rIdx !== undefined) {
            charts.region.data.datasets[0].data[rIdx]++;
            charts.region.update("none");
        }

        const currentHourIdx = charts.intensity.data.datasets[0].data.length - 1;
        const newVal = Math.min(100, charts.intensity.data.datasets[0].data[currentHourIdx] + Math.floor(Math.random() * 5) + 1);
        charts.intensity.data.datasets[0].data[currentHourIdx] = newVal;
        charts.intensity.data.datasets[0].backgroundColor[currentHourIdx] =
            newVal > 70 ? "rgba(255,34,68,0.7)" :
            newVal > 40 ? "rgba(255,136,0,0.7)" :
            "rgba(0,255,68,0.5)";
        charts.intensity.update("none");
    }

    // ========================
    // RADAR
    // ========================
    function initRadar() {
        const canvas = document.getElementById("radar-canvas");
        canvas.width = 170;
        canvas.height = 170;
        drawRadar();
    }

    function drawRadar() {
        const canvas = document.getElementById("radar-canvas");
        const ctx = canvas.getContext("2d");
        const cx = canvas.width / 2;
        const cy = canvas.height / 2;
        const maxR = Math.min(cx, cy) - 5;

        ctx.clearRect(0, 0, canvas.width, canvas.height);

        ctx.fillStyle = "rgba(0, 20, 10, 0.9)";
        ctx.beginPath();
        ctx.arc(cx, cy, maxR, 0, Math.PI * 2);
        ctx.fill();

        for (let i = 1; i <= 4; i++) {
            ctx.strokeStyle = "rgba(0, 255, 68, 0.15)";
            ctx.lineWidth = 0.5;
            ctx.beginPath();
            ctx.arc(cx, cy, (maxR / 4) * i, 0, Math.PI * 2);
            ctx.stroke();
        }

        ctx.strokeStyle = "rgba(0, 255, 68, 0.1)";
        ctx.lineWidth = 0.5;
        for (let a = 0; a < Math.PI * 2; a += Math.PI / 6) {
            ctx.beginPath();
            ctx.moveTo(cx, cy);
            ctx.lineTo(cx + Math.cos(a) * maxR, cy + Math.sin(a) * maxR);
            ctx.stroke();
        }

        const sweepGrad = ctx.createConicalGradient
            ? null
            : (() => {
                const grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, maxR);
                return grad;
            })();

        ctx.save();
        ctx.translate(cx, cy);
        ctx.rotate(radarAngle);

        const gradient = ctx.createLinearGradient(0, 0, maxR, 0);
        gradient.addColorStop(0, "rgba(0, 255, 68, 0.4)");
        gradient.addColorStop(1, "rgba(0, 255, 68, 0)");

        ctx.fillStyle = gradient;
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.arc(0, 0, maxR, -0.4, 0);
        ctx.closePath();
        ctx.fill();

        ctx.strokeStyle = "#00ff44";
        ctx.lineWidth = 1.5;
        ctx.shadowColor = "#00ff44";
        ctx.shadowBlur = 5;
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(maxR, 0);
        ctx.stroke();
        ctx.shadowBlur = 0;

        ctx.restore();

        if (Math.random() < 0.05) {
            radarBlips.push({
                angle: Math.random() * Math.PI * 2,
                dist: 0.2 + Math.random() * 0.7,
                life: 1.0,
                threat: Math.random() > 0.6
            });
        }

        radarBlips = radarBlips.filter(b => b.life > 0);
        radarBlips.forEach(blip => {
            const bx = cx + Math.cos(blip.angle) * blip.dist * maxR;
            const by = cy + Math.sin(blip.angle) * blip.dist * maxR;
            const alpha = blip.life;
            ctx.fillStyle = blip.threat
                ? `rgba(255, 34, 68, ${alpha})`
                : `rgba(0, 255, 68, ${alpha})`;
            ctx.shadowColor = blip.threat ? "#ff2244" : "#00ff44";
            ctx.shadowBlur = 6;
            ctx.beginPath();
            ctx.arc(bx, by, 2.5, 0, Math.PI * 2);
            ctx.fill();
            ctx.shadowBlur = 0;
            blip.life -= 0.008;
        });

        ctx.fillStyle = "#00ff44";
        ctx.shadowColor = "#00ff44";
        ctx.shadowBlur = 8;
        ctx.beginPath();
        ctx.arc(cx, cy, 2, 0, Math.PI * 2);
        ctx.fill();
        ctx.shadowBlur = 0;

        radarAngle += 0.03;
        requestAnimationFrame(drawRadar);
    }

    // ========================
    // EVENT FEED
    // ========================
    function addEventToFeed(event) {
        const feed = document.getElementById("event-feed");
        const item = document.createElement("div");
        item.className = "event-item new-event";
        item.innerHTML = `
            <div class="event-time">${formatTime(event.timestamp)} — ${event.id}</div>
            <span class="event-type ${event.type.id}">${event.type.icon} ${event.type.name.toUpperCase()}</span>
            <div class="event-description">${event.description}</div>
            <div class="event-location">📍 ${event.region} [${event.lat.toFixed(4)}, ${event.lon.toFixed(4)}] — ${event.severity}</div>
        `;

        item.addEventListener("click", () => {
            map.flyTo([event.lat, event.lon], 10, { duration: 1.5 });
        });

        feed.insertBefore(item, feed.firstChild);

        setTimeout(() => item.classList.remove("new-event"), 5000);

        while (feed.children.length > MAX_EVENTS) {
            feed.removeChild(feed.lastChild);
        }
    }

    function addAlert(alert) {
        const container = document.getElementById("alerts-container");
        const item = document.createElement("div");
        item.className = `alert-item ${alert.level}`;
        item.textContent = `[${formatTime(alert.timestamp)}] ${alert.text}`;

        container.insertBefore(item, container.firstChild);

        while (container.children.length > MAX_ALERTS) {
            container.removeChild(container.lastChild);
        }
    }

    // ========================
    // STATS
    // ========================
    function updateStats() {
        const now = Date.now();
        const last24h = events.filter(e => now - e.timestamp.getTime() < 24 * 60 * 60 * 1000);

        const attackCount = last24h.length;
        const totalCasualties = last24h.reduce((sum, e) => sum + e.casualties, 0);
        const activeZones = new Set(last24h.map(e => e.region)).size;
        const missileCount = last24h.filter(e => e.type.id === "missile").length;

        animateNumber("stat-attacks", attackCount);
        animateNumber("stat-casualties", totalCasualties);
        animateNumber("stat-active", activeZones);
        animateNumber("stat-missiles", missileCount);

        document.getElementById("trend-attacks").textContent = `↑ ${Math.floor(Math.random() * 15 + 5)}%`;
        document.getElementById("trend-casualties").textContent = `↑ ${Math.floor(Math.random() * 20 + 3)}%`;
        document.getElementById("trend-active").textContent = `— ${activeZones}`;
        document.getElementById("trend-missiles").textContent = `↑ ${Math.floor(Math.random() * 25 + 8)}%`;
    }

    function animateNumber(elementId, target) {
        const el = document.getElementById(elementId);
        const current = parseInt(el.textContent) || 0;
        if (current === target) return;

        const diff = target - current;
        const steps = 20;
        const increment = diff / steps;
        let step = 0;

        const interval = setInterval(() => {
            step++;
            const value = Math.round(current + increment * step);
            el.textContent = value;
            if (step >= steps) {
                el.textContent = target;
                clearInterval(interval);
            }
        }, 30);
    }

    // ========================
    // CLOCK
    // ========================
    function startClock() {
        function update() {
            const now = new Date();
            const utcNow = new Date(now.toUTCString());

            document.getElementById("date-display").textContent =
                `${String(now.getDate()).padStart(2, "0")}/${String(now.getMonth() + 1).padStart(2, "0")}/${now.getFullYear()}`;

            document.getElementById("time-display").textContent =
                `${String(now.getHours()).padStart(2, "0")}:${String(now.getMinutes()).padStart(2, "0")}:${String(now.getSeconds()).padStart(2, "0")}`;

            document.getElementById("utc-display").textContent =
                `UTC ${String(utcNow.getUTCHours()).padStart(2, "0")}:${String(utcNow.getUTCMinutes()).padStart(2, "0")}:${String(utcNow.getUTCSeconds()).padStart(2, "0")}`;
        }

        update();
        setInterval(update, 1000);
    }

    // ========================
    // LATENCY SIMULATION
    // ========================
    function startLatencySimulation() {
        setInterval(() => {
            const latency = Math.floor(Math.random() * 80 + 20);
            document.getElementById("latency-value").textContent = latency + "ms";

            packetCount += Math.floor(Math.random() * 5 + 1);
            document.getElementById("packets-value").textContent = packetCount.toLocaleString();
        }, 2000);
    }

    // ========================
    // REAL-TIME SIMULATION
    // ========================
    function loadInitialData() {
        const initial = generateInitialEvents(40);
        initial.forEach(event => {
            events.push(event);
            addAttackMarker(event);
            addEventToFeed(event);
        });

        for (let i = 0; i < 3; i++) {
            addAlert(generateAlert());
        }

        updateStats();
        updateHudTargets();
    }

    function startRealTimeSimulation() {
        function scheduleNextEvent() {
            const delay = Math.random() * 4000 + 1500;
            setTimeout(() => {
                const event = generateAttackEvent();
                events.push(event);
                if (events.length > 200) events.shift();

                addAttackMarker(event);
                addEventToFeed(event);
                updateCharts(event);
                updateStats();
                updateHudTargets();

                playEventSound(event);

                if (Math.random() < 0.15) {
                    addAlert(generateAlert());
                }

                scheduleNextEvent();
            }, delay);
        }

        scheduleNextEvent();
    }

    function playEventSound(event) {
        if (event.severityIndex >= 3) {
            flashScreen("critical");
        } else if (event.severityIndex >= 2) {
            flashScreen("warning");
        }
    }

    function flashScreen(type) {
        const overlay = document.createElement("div");
        overlay.style.cssText = `
            position: fixed; top: 0; left: 0; width: 100%; height: 100%;
            pointer-events: none; z-index: 9990;
            background: ${type === "critical" ? "rgba(255,34,68,0.06)" : "rgba(255,136,0,0.04)"};
            animation: screen-flash 0.5s ease-out forwards;
        `;

        const style = document.createElement("style");
        style.textContent = `
            @keyframes screen-flash {
                0% { opacity: 1; }
                100% { opacity: 0; }
            }
        `;

        document.head.appendChild(style);
        document.body.appendChild(overlay);
        setTimeout(() => {
            overlay.remove();
            style.remove();
        }, 500);
    }

    // ========================
    // UTILITIES
    // ========================
    function formatTime(date) {
        return `${String(date.getHours()).padStart(2, "0")}:${String(date.getMinutes()).padStart(2, "0")}:${String(date.getSeconds()).padStart(2, "0")}`;
    }

    // ========================
    // START
    // ========================
    document.addEventListener("DOMContentLoaded", runBootSequence);

})();
