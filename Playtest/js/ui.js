/** English interface. Story strings are passed in from the narrative data. */

export class Shell {
  constructor(root) {
    this.root = root;
    root.innerHTML = `
      <canvas id="view"></canvas>
      <div class="grain"></div>
      <div class="vignette"></div>
      <div class="notice" id="notice" style="display:none"></div>
      <section class="screen" id="menu">
        <canvas id="menu-canvas"></canvas>
        <div class="menu-copy">
          <p class="kicker" id="tagline"></p>
          <h1 id="title"></h1>
          <p class="clock" id="menu-clock"></p>
          <div class="menu-list" id="menu-list"></div>
          <p class="footer" id="menu-footer"></p>
        </div>
      </section>
      <section class="screen" id="sheet">
        <div class="sheet">
          <p class="kicker" id="sheet-kicker"></p>
          <h2 id="sheet-title"></h2>
          <div id="sheet-body"></div>
          <p class="footer" id="sheet-footer"></p>
        </div>
      </section>
      <section class="hud" id="hud">
        <div class="hud-top"><strong>HOW DID I DIE?</strong><span class="clock" id="hud-clock"></span></div>
        <div class="crosshair"></div>
        <div class="prompt" id="prompt" style="display:none"></div>
        <div class="caption"><span id="card-index"></span><b id="card-title"></b></div>
        <div class="hud-bottom" id="location-line"></div>
        <div class="memory-hint" id="memory-hint"></div>
      </section>
      <section class="panel" id="panel">
        <p class="kicker" id="panel-index"></p>
        <div class="panel-frame">
          <canvas id="panel-art" width="640" height="800"></canvas>
          <div>
            <p class="meta" id="panel-meta"></p>
            <h2 id="panel-title"></h2>
            <p class="body" id="panel-body"></p>
          </div>
        </div>
        <p class="continue-hint hint">SPACE — CONTINUE</p>
      </section>
      <section class="dialogue" id="dialogue">
        <div class="speaker" id="speaker"></div>
        <div class="line" id="line"></div>
        <div class="choices" id="choices"></div>
        <p class="hint" id="dialogue-hint"></p>
      </section>
      <section class="examine" id="examine">
        <h3 id="examine-title"></h3>
        <p id="examine-body"></p>
      </section>
      <section class="end-card screen" id="ending">
        <p class="kicker">CHAPTER COMPLETE</p>
        <h2 id="end-title"></h2>
        <p class="footer">The hour has closed.</p>
        <div class="menu-list" id="end-list"></div>
      </section>
      <section class="screen" id="quit">
        <canvas id="quit-canvas"></canvas>
        <div class="quit-copy">
          <h2 id="quit-title"></h2>
          <p class="footer">The hour stops here.</p>
          <div class="menu-list" id="quit-list"></div>
        </div>
      </section>
    `;
    this.view = root.querySelector("#view");
    this.menuCanvas = root.querySelector("#menu-canvas");
    this.quitCanvas = root.querySelector("#quit-canvas");
    this.panelArt = root.querySelector("#panel-art");
    this.selected = 0;
    this.sheetItems = [];
    this.menuItems = [];
    this.timelineBranch = 0;
    this.timelineIndex = 0;
    this.settingsIndex = 0;
    this.noticeTimer = 0;
    this.mode = "menu";
    this.rainTime = 0;
  }

  canvas() {
    return this.view;
  }

  setContrast(on) {
    document.body.classList.toggle("contrast", !!on);
  }

  setStill(on) {
    document.body.classList.toggle("still", !!on);
  }

  showOnly(ids) {
    for (const el of this.root.querySelectorAll(".screen, .panel, .dialogue, .examine, .hud")) {
      el.classList.remove("on");
    }
    for (const id of ids) this.root.querySelector(`#${id}`)?.classList.add("on");
  }

  showMenu(model) {
    this.mode = "menu";
    this.menuItems = model.items;
    this.selected = Math.min(this.selected, model.items.length - 1);
    this.root.querySelector("#title").textContent = model.title;
    this.root.querySelector("#tagline").textContent = model.tagline;
    this.root.querySelector("#menu-clock").textContent = model.clock;
    this.root.querySelector("#menu-footer").textContent = model.footer;
    this.root.querySelector("#quit-title").textContent = model.title;
    const list = this.root.querySelector("#menu-list");
    list.innerHTML = "";
    model.items.forEach((item, index) => {
      const button = document.createElement("button");
      button.textContent = item.label;
      button.disabled = !!item.disabled;
      button.className = index === this.selected ? "on" : "";
      button.addEventListener("click", () => {
        if (item.disabled) return;
        this.selected = index;
        model.onChoose(item);
      });
      list.appendChild(button);
    });
    this.showOnly(["menu"]);
    this.paintCity(this.menuCanvas, model.clock);
  }

  showSheet(model) {
    this.mode = "sheet";
    this.sheetItems = model.items;
    this.selected = Math.min(this.selected, Math.max(0, model.items.length - 1));
    this.root.querySelector("#sheet-kicker").textContent = model.kicker || "";
    this.root.querySelector("#sheet-title").textContent = model.title;
    this.root.querySelector("#sheet-footer").textContent = model.footer || "";
    const body = this.root.querySelector("#sheet-body");
    body.innerHTML = "";
    if (model.kind === "settings") {
      model.items.forEach((item, index) => {
        const row = document.createElement("div");
        row.className = `settings-row${index === this.selected ? " on" : ""}`;
        row.innerHTML = `<span>${item.label}</span><span>${item.value}</span>`;
        body.appendChild(row);
      });
    } else if (model.kind === "detail") {
      const p = document.createElement("p");
      p.className = "entry-body";
      p.textContent = model.detail || "";
      body.appendChild(p);
    } else {
      const list = document.createElement("div");
      list.className = "sheet-list";
      model.items.forEach((item, index) => {
        const button = document.createElement("button");
        button.className = index === this.selected ? "on" : "";
        button.disabled = !!item.disabled;
        button.innerHTML = item.detail && model.kind !== "timeline" ? `${item.label}<small>${item.detail}</small>` : item.label;
        if (model.kind === "timeline") button.textContent = item.label;
        button.addEventListener("click", () => {
          if (item.disabled) return;
          this.selected = index;
          model.onChoose(item, index);
        });
        list.appendChild(button);
      });
      body.appendChild(list);
      if (model.extra) {
        const extra = document.createElement("p");
        extra.className = "entry-body";
        extra.textContent = model.extra;
        body.appendChild(extra);
      }
    }
    this.showOnly(["sheet"]);
  }

  showPlay() {
    this.mode = "play";
    this.showOnly(["hud"]);
  }

  showPanel(model) {
    this.mode = "panel";
    this.root.querySelector("#panel-index").textContent = model.index;
    this.root.querySelector("#panel-meta").textContent = `${model.time}  ·  ${model.scene}`;
    this.root.querySelector("#panel-title").textContent = model.title;
    this.root.querySelector("#panel-body").textContent = model.body;
    this.drawPanel(model);
    this.showOnly(["hud", "panel"]);
  }

  showDialogue(model) {
    this.mode = "dialogue";
    this.selected = Math.min(this.selected, Math.max(0, model.choices.length - 1));
    this.root.querySelector("#speaker").textContent = model.speaker;
    this.root.querySelector("#line").textContent = model.text;
    this.root.querySelector("#dialogue-hint").textContent = model.choices.length ? "1–4 CHOOSE" : "SPACE — CONTINUE";
    const box = this.root.querySelector("#choices");
    box.innerHTML = "";
    model.choices.forEach((choice, index) => {
      const button = document.createElement("button");
      button.className = `choice${index === this.selected ? " on" : ""}`;
      button.textContent = `${index + 1}   ${choice.Text}`;
      button.addEventListener("click", () => model.onChoose(index));
      box.appendChild(button);
    });
    this.showOnly(["hud", "dialogue"]);
  }

  showExamine(title, body) {
    if (!title) {
      this.root.querySelector("#examine").classList.remove("on");
      return;
    }
    this.root.querySelector("#examine-title").textContent = title;
    this.root.querySelector("#examine-body").textContent = body;
    this.root.querySelector("#examine").classList.add("on");
  }

  showEnding(model) {
    this.mode = "ending";
    this.selected = 0;
    this.menuItems = model.items;
    this.root.querySelector("#end-title").textContent = model.title;
    const list = this.root.querySelector("#end-list");
    list.innerHTML = "";
    model.items.forEach((item, index) => {
      const button = document.createElement("button");
      button.textContent = item.label;
      button.className = index === 0 ? "on" : "";
      button.addEventListener("click", () => model.onChoose(item));
      list.appendChild(button);
    });
    this.showOnly(["ending"]);
  }

  showQuit(title) {
    this.mode = "quit";
    this.selected = 0;
    this.root.querySelector("#quit-title").textContent = title;
    const list = this.root.querySelector("#quit-list");
    list.innerHTML = "";
    const button = document.createElement("button");
    button.textContent = "RETURN TO TITLE";
    button.className = "on";
    button.addEventListener("click", () => this.onQuitReturn?.());
    list.appendChild(button);
    this.showOnly(["quit"]);
    this.paintCity(this.quitCanvas, "");
  }

  setHud(hud) {
    this.root.querySelector("#hud-clock").textContent = hud.clock || "";
    this.root.querySelector("#card-index").textContent = hud.index || "";
    this.root.querySelector("#card-title").textContent = hud.title || "";
    this.root.querySelector("#location-line").textContent = hud.location || "";
    this.root.querySelector("#memory-hint").textContent = hud.memory || "";
    const prompt = this.root.querySelector("#prompt");
    if (!hud.prompt) {
      prompt.style.display = "none";
    } else {
      prompt.style.display = "block";
      prompt.textContent = hud.prompt;
      prompt.style.left = `${hud.promptX}px`;
      prompt.style.top = `${hud.promptY}px`;
    }
  }

  flashNotice(text) {
    const el = this.root.querySelector("#notice");
    el.textContent = text;
    el.style.display = "block";
    this.noticeTimer = 3.2;
  }

  tickNotice(dt) {
    if (this.noticeTimer <= 0) return;
    this.noticeTimer -= dt;
    if (this.noticeTimer <= 0) this.root.querySelector("#notice").style.display = "none";
  }

  moveSelection(delta, count) {
    if (count <= 0) return;
    this.selected = (this.selected + delta + count) % count;
  }

  paintCity(canvas, clock) {
    const ctx = canvas.getContext("2d");
    const w = (canvas.width = window.innerWidth);
    const h = (canvas.height = window.innerHeight);
    ctx.fillStyle = "#07080c";
    ctx.fillRect(0, 0, w, h);
    const base = h * 0.72;
    for (let i = 0; i < 26; i += 1) {
      const bw = w * (0.03 + ((i * 17) % 7) * 0.012);
      const bh = h * (0.18 + ((i * 29) % 10) * 0.045);
      const x = (i / 26) * w;
      ctx.fillStyle = i % 2 === 0 ? "#12151b" : "#0c0e13";
      ctx.fillRect(x, base - bh, bw, bh + 40);
      if (i % 4 === 1) {
        ctx.fillStyle = i % 8 === 1 ? "#c4a574" : "#3d6d78";
        ctx.fillRect(x + bw * 0.35, base - bh + 18, 4, 4);
      }
    }
    ctx.fillStyle = "#050506";
    ctx.beginPath();
    ctx.ellipse(w * 0.78, base - 10, 16, 70, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillRect(w * 0.78 - 18, base - 8, 36, 8);
    if (clock) {
      ctx.fillStyle = "#c4a574";
      ctx.font = "28px Barlow Condensed, Arial Narrow, sans-serif";
      ctx.fillText(clock, w * 0.78 - 28, 48);
    }
    this.cityCtx = ctx;
    this.cityW = w;
    this.cityH = h;
    this.cityBase = base;
  }

  tickCity(dt) {
    if (this.mode !== "menu" && this.mode !== "quit") return;
    const canvas = this.mode === "menu" ? this.menuCanvas : this.quitCanvas;
    const ctx = canvas.getContext("2d");
    this.rainTime += dt;
    ctx.strokeStyle = "rgba(190, 198, 206, 0.28)";
    ctx.lineWidth = 1;
    for (let i = 0; i < 80; i += 1) {
      const x = (i * 97 + this.rainTime * 180) % this.cityW;
      const y = (i * 53 + this.rainTime * 320) % this.cityH;
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.lineTo(x - 6, y + 16);
      ctx.stroke();
    }
  }

  drawPanel(model) {
    const canvas = this.panelArt;
    const ctx = canvas.getContext("2d");
    ctx.fillStyle = "#101218";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.strokeStyle = "rgba(230,225,214,0.35)";
    ctx.strokeRect(18, 18, canvas.width - 36, canvas.height - 36);
    const id = model.cardId;
    const ink = "#d9d3c7";
    const dim = "#6e6a62";
    ctx.fillStyle = "#0c0e12";
    ctx.fillRect(48, 80, 544, 520);
    ctx.strokeStyle = dim;
    ctx.strokeRect(48, 80, 544, 520);
    const accent = id === "CH01_CARD_04" || id === "CH01_CARD_10" ? "#c4a574" : id === "CH01_CARD_06" ? "#3d6d78" : ink;
    ctx.strokeStyle = ink;
    ctx.lineWidth = 2;
    if (id === "CH01_CARD_01") {
      ctx.strokeRect(250, 160, 140, 220);
      ctx.beginPath();
      ctx.moveTo(320, 190);
      ctx.lineTo(320, 250);
      ctx.lineTo(360, 250);
      ctx.stroke();
    } else if (id === "CH01_CARD_02") {
      ctx.strokeRect(120, 150, 180, 140);
      ctx.fillStyle = "#1a1c20";
      ctx.fillRect(160, 180, 90, 80);
      ctx.strokeRect(400, 360, 120, 160);
    } else if (id === "CH01_CARD_03" || id === "CH01_CARD_12") {
      ctx.strokeRect(140, 300, 360, 180);
      ctx.fillStyle = ink;
      for (let i = 0; i < 5; i += 1) ctx.fillRect(170, 330 + i * 22, 280, 2);
    } else if (id === "CH01_CARD_04") {
      ctx.beginPath();
      ctx.arc(320, 340, 54, 0, Math.PI * 2);
      ctx.strokeStyle = accent;
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(320, 310);
      ctx.lineTo(300, 340);
      ctx.lineTo(320, 370);
      ctx.lineTo(340, 340);
      ctx.closePath();
      ctx.stroke();
    } else if (id === "CH01_CARD_05") {
      ctx.strokeRect(230, 240, 180, 80);
      ctx.beginPath();
      ctx.arc(430, 280, 18, 0, Math.PI * 2);
      ctx.stroke();
    } else if (id === "CH01_CARD_06") {
      ctx.strokeRect(80, 140, 480, 360);
      ctx.beginPath();
      ctx.moveTo(320, 180);
      ctx.lineTo(250, 280);
      ctx.lineTo(390, 280);
      ctx.closePath();
      ctx.strokeStyle = accent;
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(320, 280);
      ctx.lineTo(320, 430);
      ctx.stroke();
    } else if (id === "CH01_CARD_07" || id === "CH01_CARD_08") {
      ctx.strokeRect(160, 220, 70, 220);
      ctx.strokeRect(360, 250, 60, 190);
      ctx.fillStyle = "#e4dcce";
      ctx.fillRect(400, 300, 40, 28);
    } else if (id === "CH01_CARD_09") {
      ctx.fillStyle = "#050506";
      ctx.fillRect(250, 160, 120, 320);
      ctx.fillStyle = ink;
    } else if (id === "CH01_CARD_10") {
      ctx.beginPath();
      ctx.moveTo(320, 180);
      ctx.lineTo(270, 280);
      ctx.lineTo(370, 280);
      ctx.closePath();
      ctx.moveTo(320, 460);
      ctx.lineTo(270, 360);
      ctx.lineTo(370, 360);
      ctx.closePath();
      ctx.strokeStyle = accent;
      ctx.stroke();
    } else if (id === "CH01_CARD_11") {
      ctx.strokeRect(200, 140, 240, 420);
      ctx.beginPath();
      ctx.moveTo(250, 340);
      ctx.lineTo(310, 340);
      ctx.stroke();
    }
    ctx.fillStyle = dim;
    ctx.font = "18px Barlow Condensed, sans-serif";
    ctx.fillText(model.time || "", 60, 70);
    ctx.strokeStyle = "rgba(190,198,206,0.25)";
    ctx.lineWidth = 1;
    for (let i = 0; i < 16; i += 1) {
      ctx.beginPath();
      ctx.moveTo(70 + i * 28, 90);
      ctx.lineTo(60 + i * 28, 150);
      ctx.stroke();
    }
  }
}
