import { Greybox } from "./world.js";
import { Shell } from "./ui.js";
import {
  NOTEBOOK_CATEGORIES,
  Narrative,
  fetchCatalog,
  hasStorySave,
  loadSettings,
  writeSettings,
} from "./systems.js";

const settings = loadSettings();
const shell = new Shell(document.querySelector("#app"));
const keys = new Set();
let catalog = null;
let narrative = null;
let world = null;
let audio = null;
let screen = "boot";
let stack = [];
let replaceConfirm = false;
let locationId = "";
let examine = null;
let lookX = 0;
let lookY = 0;
let lockExitAt = 0;
let saveTimer = 0;
let last = performance.now();

const MENU = [
  { id: "new", label: "NEW GAME" },
  { id: "continue", label: "CONTINUE" },
  { id: "chapters", label: "CHAPTERS" },
  { id: "evidence", label: "EVIDENCE" },
  { id: "settings", label: "SETTINGS" },
  { id: "quit", label: "QUIT" },
];

function top() {
  return stack[stack.length - 1] || "";
}

function push(name) {
  stack.push(name);
}

function pop() {
  return stack.pop();
}

function poseFromStory() {
  const story = narrative.story;
  if (!story?.bHasSavedTransform || !story.SavedLocation) return null;
  return {
    x: story.SavedLocation.X,
    y: story.SavedLocation.Y,
    z: story.SavedLocation.Z,
    yaw: story.SavedRotation?.Yaw || 0,
    pitch: story.SavedRotation?.Pitch || 0,
  };
}

function remember() {
  if (!narrative.story || !world) return;
  narrative.rememberPose(world.poseUe());
}

function refreshWorld() {
  const chapter = narrative.chapter();
  if (!chapter || !world) return;
  world.refreshInteractables(chapter, narrative.visibleInteractables());
  world.locationId = locationId;
  const rain = narrative.flashbackActive || locationId === "LOC_STAIR" || locationId === "LOC_LOBBY";
  world.setRainVisible(rain);
  const node = narrative.dialogueOpen ? narrative.currentNode() : null;
  const mode = node?.Camera || (narrative.flashbackActive ? "Flashback" : narrative.currentCard()?.Camera);
  world.setCameraMode(mode || "OverShoulder");
  world.setSaturation(narrative.flashbackActive ? 0.22 : chapter.ColourSaturation ?? 0.1);
}

function reader() {
  if (narrative.story && screen !== "menu" && top() !== "main") return narrative;
  const copy = new Narrative(catalog);
  if (!copy.continueGame()) return null;
  return copy;
}

function showMain() {
  screen = "menu";
  stack = [];
  examine = null;
  shell.selected = Math.min(shell.selected, MENU.length - 1);
  shell.showMenu({
    title: catalog.game.Title,
    tagline: catalog.game.Tagline,
    clock: narrative.menuClock(),
    footer: replaceConfirm ? "NEW GAME replaces the saved hour." : `${catalog.game.Tagline}\n${narrative.menuClock()}`,
    items: MENU,
    onChoose: (item) => chooseMenu(item.id),
  });
  if (audio) audio.setLayers(["rain", "city"]);
}

function chooseMenu(id) {
  if (id === "new") {
    if (hasStorySave() && !replaceConfirm) {
      replaceConfirm = true;
      showMain();
      return;
    }
    replaceConfirm = false;
    startChapter("CH01", true);
    return;
  }
  replaceConfirm = false;
  if (id === "continue") {
    if (!narrative.continueGame()) {
      shell.flashNotice("No hour is saved.");
      showMain();
      return;
    }
    beginSession(true);
    return;
  }
  if (id === "chapters") {
    shell.selected = 0;
    showChapters();
    return;
  }
  if (id === "evidence") {
    shell.selected = 0;
    showEvidenceCategories();
    return;
  }
  if (id === "settings") {
    shell.selected = 0;
    showSettings(false);
    return;
  }
  if (id === "quit") showQuit();
}

function showChapters() {
  screen = "chapters";
  const items = catalog.order.map((id) => {
    const chapter = catalog.chapters[id];
    return {
      id,
      label: `${id.slice(-2)}    ${chapter.Title}`,
      detail: chapter.Playable ? "OPEN" : "DATA ONLY",
    };
  });
  shell.showSheet({
    kicker: "CHAPTERS",
    title: "CHAPTERS",
    footer: "Only an open chapter can be played.",
    items,
    onChoose: (item) => {
      if (!narrative.isPlayable(item.id)) {
        shell.flashNotice("This chapter is recorded. It is not open.");
        showChapters();
        return;
      }
      if (hasStorySave() && !replaceConfirm) {
        replaceConfirm = true;
        shell.flashNotice("NEW GAME replaces the saved hour.");
        return;
      }
      replaceConfirm = false;
      startChapter(item.id, true);
    },
  });
}

function showEvidenceCategories() {
  screen = "evidence";
  shell.showSheet({
    kicker: "EVIDENCE",
    title: "EVIDENCE",
    footer: "Evidence is filed. Conclusions are not.",
    items: NOTEBOOK_CATEGORIES.map((name) => ({ id: name, label: name })),
    onChoose: (item) => {
      shell.selected = 0;
      showEntries(item.id, false);
    },
  });
}

function showNotebookCategories() {
  screen = "notebook";
  if (top() !== "notebook" && top() !== "notebook-entry") push("notebook");
  shell.selected = 0;
  shell.showSheet({
    kicker: "NOTEBOOK",
    title: "NOTEBOOK",
    footer: "Evidence is filed. Conclusions are not.",
    items: NOTEBOOK_CATEGORIES.map((name) => ({ id: name, label: name })),
    onChoose: (item) => {
      shell.selected = 0;
      showEntries(item.id, true);
    },
  });
}

function showEntries(category, inNotebook) {
  screen = inNotebook ? "notebook-entry" : "evidence-entry";
  if (inNotebook && top() !== "notebook-entry") push("notebook-entry");
  const source = inNotebook ? narrative : reader();
  const entries = source ? source.buildNotebook().filter((entry) => entry.category === category) : [];
  const items = entries.length
    ? entries.map((entry) => ({ id: entry.source, label: entry.title, detail: entry.body }))
    : [{ id: "empty", label: "Nothing filed yet.", disabled: true, detail: "" }];
  shell.showSheet({
    kicker: category.toUpperCase(),
    title: category.toUpperCase(),
    footer: "Filed, not solved.",
    items,
    kind: "list",
    onChoose: (item) => {
      if (item.disabled) return;
      shell.showSheet({
        kicker: category.toUpperCase(),
        title: item.label,
        footer: "Filed, not solved.",
        kind: "detail",
        detail: item.detail,
        items: [],
      });
      screen = inNotebook ? "notebook-detail" : "evidence-detail";
    },
  });
}

function settingsRows() {
  const preset = settings.GraphicsPreset;
  return [
    { id: "MasterVolume", label: "Master", value: Math.round(settings.MasterVolume * 100) + "%" },
    { id: "MusicVolume", label: "Music", value: Math.round(settings.MusicVolume * 100) + "%" },
    { id: "EffectsVolume", label: "Effects", value: Math.round(settings.EffectsVolume * 100) + "%" },
    { id: "DialogueVolume", label: "Dialogue", value: Math.round(settings.DialogueVolume * 100) + "%" },
    { id: "Subtitles", label: "Subtitles", value: settings.Subtitles ? "ON" : "OFF" },
    { id: "SubtitleScale", label: "Subtitle size", value: settings.SubtitleScale.toFixed(1) },
    { id: "MouseSensitivity", label: "Mouse", value: settings.MouseSensitivity.toFixed(1) },
    { id: "MotionReduction", label: "Motion reduction", value: settings.MotionReduction ? "ON" : "OFF" },
    { id: "HighContrast", label: "High contrast", value: settings.HighContrast ? "ON" : "OFF" },
    { id: "GraphicsPreset", label: "Graphics", value: preset },
  ];
}

function showSettings(inGame) {
  screen = "settings";
  if (inGame && top() !== "settings") push("settings");
  shell.showSheet({
    kicker: "SETTINGS",
    title: "SETTINGS",
    footer: "Left and right change a value.",
    kind: "settings",
    items: settingsRows(),
    onChoose: () => nudgeSetting(1),
  });
}

function nudgeSetting(dir) {
  const rows = settingsRows();
  const row = rows[shell.selected];
  if (!row) return;
  const id = row.id;
  if (id.endsWith("Volume")) settings[id] = Math.min(1, Math.max(0, settings[id] + dir * 0.05));
  else if (id === "Subtitles" || id === "MotionReduction" || id === "HighContrast") settings[id] = dir ? !settings[id] : settings[id];
  else if (id === "SubtitleScale") settings[id] = Math.min(1.8, Math.max(0.8, +(settings[id] + dir * 0.1).toFixed(1)));
  else if (id === "MouseSensitivity") settings[id] = Math.min(2.5, Math.max(0.4, +(settings[id] + dir * 0.1).toFixed(1)));
  else if (id === "GraphicsPreset") {
    const order = ["Low", "Medium", "High", "Epic"];
    const index = order.indexOf(settings.GraphicsPreset);
    settings.GraphicsPreset = order[(index + (dir >= 0 ? 1 : order.length - 1)) % order.length];
  }
  writeSettings(settings);
  shell.setContrast(settings.HighContrast);
  shell.setStill(settings.MotionReduction);
  world?.setPreset(settings.GraphicsPreset);
  audio?.apply();
  showSettings(top() === "settings");
}

function showPause() {
  if (top() !== "pause") {
    remember();
    narrative.autosave();
    push("pause");
  }
  screen = "pause";
  shell.selected = 0;
  if (document.pointerLockElement) document.exitPointerLock();
  shell.showSheet({
    kicker: "PAUSED",
    title: "PAUSED",
    footer: "The hour is waiting.",
    items: [
      { id: "resume", label: "RESUME" },
      { id: "notebook", label: "NOTEBOOK" },
      { id: "settings", label: "SETTINGS" },
      { id: "menu", label: "MAIN MENU" },
      { id: "quit", label: "QUIT" },
    ],
    onChoose: (item) => {
      if (item.id === "resume") {
        pop();
        sync();
      } else if (item.id === "notebook") {
        shell.selected = 0;
        showNotebookCategories();
      } else if (item.id === "settings") {
        shell.selected = 0;
        showSettings(true);
      } else if (item.id === "menu") showMain();
      else if (item.id === "quit") showQuit();
    },
  });
}

function showTimeline() {
  if (narrative.dialogueOpen || screen === "ending") return;
  if (top() === "timeline") {
    pop();
    sync();
    return;
  }
  narrative.notifyTimelineOpened();
  push("timeline");
  screen = "timeline";
  shell.timelineIndex = shell.timelineIndex || 0;
  renderTimeline();
}

function timelineStanding(branch, story) {
  const choice = narrative.findChoice(branch.ChoiceID);
  if (!choice?.BranchGroup) return false;
  const chosen = story.ChoiceByBranch[choice.BranchGroup];
  if (!chosen) return false;
  return chosen === branch.ChoiceID || chosen.startsWith(branch.ChoiceID) || branch.ChoiceID.startsWith(chosen);
}

function renderTimeline() {
  const chapter = narrative.chapter();
  const events = chapter?.Timeline || [];
  if (shell.timelineIndex >= events.length) shell.timelineIndex = 0;
  const items = events.map((event, index) => {
    const open = narrative.isEventUnlocked(event.EventID);
    let label = open ? `${event.Time}    ${event.Title}` : `——    ${event.Title}`;
    if (index === shell.timelineIndex && event.Branches?.length) {
      const branch = Math.max(0, Math.min(shell.timelineBranch, event.Branches.length - 1));
      shell.timelineBranch = branch;
      label += "\n";
      label += event.Branches.map((item, branchIndex) => {
        const standing = timelineStanding(item, narrative.story);
        const mark = branchIndex === branch ? `[ ${standing ? "* " : ""}${item.Label} ]` : `${standing ? "* " : ""}${item.Label}`;
        return mark;
      }).join("      ");
    }
    return { id: event.EventID, label, disabled: false };
  });
  const selected = events[shell.timelineIndex];
  shell.selected = shell.timelineIndex;
  shell.showSheet({
    kicker: "TIMELINE",
    title: "TIMELINE",
    footer: "Left and right revise a branch. Confirm stands in that hour.",
    kind: "timeline",
    items,
    extra: selected && narrative.story.TimelineVisited.includes(selected.EventID) ? "Visited." : "",
    onChoose: () => confirmTimeline(),
  });
}

function confirmTimeline() {
  const events = narrative.chapter()?.Timeline || [];
  const event = events[shell.timelineIndex];
  if (!event) return;
  if (!narrative.isEventUnlocked(event.EventID)) {
    shell.flashNotice("This hour is not open.");
    renderTimeline();
    return;
  }
  if (event.Branches?.length) {
    const index = Math.max(0, Math.min(shell.timelineBranch, event.Branches.length - 1));
    const choiceId = event.Branches[index].ChoiceID;
    if (!narrative.commit(choiceId, true)) shell.flashNotice("That version of the hour will not hold.");
  }
  const travelled = narrative.travelTo(event.EventID);
  if (travelled && !narrative.flashbackActive) world.teleportUe(travelled.Position, 0);
  refreshWorld();
  renderTimeline();
}

function showDialogue() {
  const node = narrative.currentNode();
  if (!node) return;
  screen = "dialogue";
  const speaker = narrative.character(node.Speaker);
  shell.showDialogue({
    speaker: speaker?.DisplayName || node.Speaker || "",
    text: settings.Subtitles ? node.Text : node.Text,
    choices: narrative.visibleChoices,
    onChoose: (index) => {
      shell.selected = 0;
      narrative.choose(index);
      refreshWorld();
      sync();
    },
  });
  const line = document.querySelector("#line");
  if (line) line.style.fontSize = `${28 * (settings.SubtitleScale || 1)}px`;
  if (document.pointerLockElement) document.exitPointerLock();
}

function showPanel() {
  const item = narrative.panelQueue[0];
  const card = narrative.cardById(item.cardId);
  if (!card) {
    narrative.shiftPanel();
    sync();
    return;
  }
  screen = "panel";
  if (document.pointerLockElement) document.exitPointerLock();
    shell.showPanel({
    cardId: card.CardID,
    index: labelFor(card),
    time: card.Time,
    scene: card.Scene,
    title: card.Title,
    body: item.body,
  });
}

function labelFor(card) {
  const index = narrative.cardIndex(card.CardID);
  const count = narrative.chapter().Cards.length;
  return `CARD ${String(index + 1).padStart(2, "0")} / ${String(count).padStart(2, "0")}`;
}

function showEnding() {
  screen = "ending";
  if (document.pointerLockElement) document.exitPointerLock();
  shell.selected = 0;
  shell.showEnding({
    title: narrative.currentCard()?.Title || "THE HOUR",
    items: [
      { id: "remain", label: "REMAIN" },
      { id: "menu", label: "MAIN MENU" },
    ],
    onChoose: (item) => {
      if (item.id === "menu") showMain();
      else sync();
    },
  });
}

function showQuit() {
  remember();
  narrative.story && narrative.autosave();
  screen = "quit";
  stack = [];
  if (document.pointerLockElement) document.exitPointerLock();
  shell.onQuitReturn = () => showMain();
  shell.showQuit(catalog.game.Title);
}

function startChapter(id, fresh) {
  if (fresh) {
    if (!narrative.newGame(id)) {
      shell.flashNotice("This chapter is recorded. It is not open.");
      return;
    }
  }
  beginSession(false);
}

function beginSession(continued) {
  const chapter = narrative.chapter();
  world.buildChapter(chapter);
  world.setPreset(settings.GraphicsPreset);
  if (continued && poseFromStory()) world.applyPose(poseFromStory());
  else {
    const card = narrative.currentCard();
    const location = chapter.Locations.find((item) => item.LocationId === card.Location) || chapter.Locations[0];
    world.spawnPose(location);
    if (narrative.flashbackActive && narrative.story.ActiveMemoryId) {
      const memory = narrative.findMemory(narrative.story.ActiveMemoryId);
      if (memory) world.teleportUe(memory.EntryPosition, 0);
    }
  }
  locationId = "";
  stack = [];
  examine = null;
  screen = "play";
  noteLocation();
  refreshWorld();
  remember();
  narrative.autosave();
  sync();
}

function noteLocation() {
  const x = world.position.x / 0.01;
  const y = world.position.z / 0.01;
  const location = narrative.locationAt(x, y);
  if (!location || location.LocationId === locationId) return;
  locationId = location.LocationId;
  world.locationId = locationId;
  narrative.notifyLocation(locationId);
}

function enterMemory(memory) {
  world.teleportUe(memory.EntryPosition, 0);
  world.setSaturation(0.22);
  world.setCameraMode("Flashback");
  world.setRainVisible(true);
  locationId = "";
  noteLocation();
  refreshWorld();
}

function leaveMemory(result) {
  if (result.pose) world.applyPose(result.pose);
  locationId = "";
  noteLocation();
  refreshWorld();
}

function tryMemory() {
  if (narrative.flashbackActive) {
    const result = narrative.tryReturn();
    if (!result.ok) {
      if (result.reason) shell.flashNotice(result.reason);
      return;
    }
    leaveMemory(result);
    sync();
    return;
  }
  const focused = world.focus;
  if (focused?.def.MemoryID) {
    interact();
    return;
  }
  const card = narrative.currentCard();
  const memoryId = card?.Flashbacks?.[0];
  if (!memoryId) return;
  narrative.returnPose = world.poseUe();
  const memory = narrative.beginMemory(memoryId);
  if (!memory) return;
  enterMemory(memory);
  sync();
}

function interact() {
  const focused = world.focus;
  if (!focused || screen !== "play") return;
  const def = focused.def;
  narrative.notifyInspect(def.Id);
  if (def.ClueID) narrative.discoverClue(def.ClueID);
  if (def.Action === "ReturnMemory") {
    const result = narrative.tryReturn();
    if (!result.ok) {
      if (result.reason) shell.flashNotice(result.reason);
    } else leaveMemory(result);
    refreshWorld();
    sync();
    return;
  }
  if (def.DialogueID) narrative.startDialogue(def.DialogueID);
  if (def.MemoryID && !narrative.flashbackActive) {
    narrative.returnPose = world.poseUe();
    const memory = narrative.beginMemory(def.MemoryID);
    if (memory) enterMemory(memory);
  }
  examine = null;
  refreshWorld();
  sync();
}

function blocksPlay() {
  return screen !== "play";
}

function sync() {
  if (screen === "menu" || screen === "quit" || screen === "chapters" || screen === "evidence" || screen === "evidence-entry" || screen === "evidence-detail") {
    return;
  }
  if (narrative?.story) refreshWorld();
  if (screen === "ending") {
    if (narrative.panelQueue.length) showPanel();
    return;
  }
  refreshHud();
  if (narrative.dialogueOpen) {
    showDialogue();
    return;
  }
  const overlay = top();
  if (overlay === "pause") {
    if (screen !== "pause") showPause();
    return;
  }
  if (overlay === "settings") {
    screen = "settings";
    return;
  }
  if (overlay === "timeline") {
    screen = "timeline";
    return;
  }
  if (overlay === "notebook" || overlay === "notebook-entry") {
    return;
  }
  if (screen === "notebook" || screen === "notebook-entry" || screen === "notebook-detail" || screen === "settings" || screen === "pause" || screen === "timeline") {
    return;
  }
  if (narrative.panelQueue.length) {
    showPanel();
    refreshHud();
    return;
  }
  if (narrative.consumeChapterComplete()) {
    showEnding();
    return;
  }
  screen = "play";
  shell.showPlay();
  if (examine) shell.showExamine(examine.title, examine.body);
  refreshHud();
  const layers = narrative.currentCard()?.Audio || ["rain"];
  audio?.setLayers(narrative.flashbackActive ? ["rain", "whispers"] : layers);
}

function refreshHud() {
  const card = narrative.currentCard();
  const location = narrative.chapter()?.Locations.find((item) => item.LocationId === locationId);
  const focused = screen === "play" ? world.focus : null;
  let promptX = window.innerWidth / 2;
  let promptY = window.innerHeight * 0.72;
  if (focused) {
    const point = world.project(focused.mesh.position.clone().setY(focused.mesh.position.y + 0.35));
    if (!point.behind) {
      promptX = point.x;
      promptY = point.y;
    }
  }
  let memory = "";
  if (narrative.flashbackActive) memory = narrative.canLeaveMemory() ? "F — RETURN" : "F — THE MEMORY HOLDS";
  else if (narrative.currentCard()?.Flashbacks?.length) memory = "F — REMEMBER";
  const look = document.pointerLockElement ? "" : "Click to look.";
  shell.setHud({
    clock: card?.Time || narrative.menuClock(),
    index: narrative.indexLabel(),
    title: card?.Title || "",
    location: `${location ? location.Name : ""}\nWASD · Shift run · E interact · Tab notebook · Q timeline${look ? "\n" + look : ""}`,
    memory,
    prompt: focused?.def.Prompt || "",
    promptX,
    promptY,
  });
  shell.tickNotice(0);
}

function back() {
  if (screen === "dialogue") {
    narrative.closeDialogue();
    refreshWorld();
    sync();
    return;
  }
  if (screen === "panel") {
    narrative.shiftPanel();
    sync();
    return;
  }
  if (screen === "ending") return;
  if (screen === "quit") {
    showMain();
    return;
  }
  if (screen === "notebook-detail" || screen === "evidence-detail") {
    const notebook = screen.startsWith("notebook");
    screen = notebook ? "notebook-entry" : "evidence";
    if (notebook && top() === "notebook-entry") pop();
    if (notebook) showNotebookCategories();
    else showEvidenceCategories();
    return;
  }
  if (screen === "notebook-entry") {
    if (top() === "notebook-entry") pop();
    showNotebookCategories();
    return;
  }
  if (screen === "notebook") {
    stack = stack.filter((name) => name !== "notebook" && name !== "notebook-entry");
    screen = "play";
    sync();
    return;
  }
  if (screen === "evidence-entry") {
    showEvidenceCategories();
    return;
  }
  if (screen === "evidence" || screen === "chapters" || screen === "settings" && top() !== "settings") {
    shell.selected = 0;
    showMain();
    return;
  }
  if (screen === "settings" && top() === "settings") {
    pop();
    screen = "play";
    sync();
    return;
  }
  if (screen === "timeline") {
    if (top() === "timeline") pop();
    screen = "play";
    sync();
    return;
  }
  if (screen === "pause") {
    if (top() === "pause") pop();
    screen = "play";
    sync();
    return;
  }
  if (screen === "menu") {
    replaceConfirm = false;
    showMain();
    return;
  }
  if (screen === "play") showPause();
}

function confirm() {
  if (screen === "menu") {
    const item = MENU[shell.selected];
    if (item) chooseMenu(item.id);
    return;
  }
  if (screen === "panel") {
    narrative.shiftPanel();
    sync();
    return;
  }
  if (screen === "dialogue") {
    if (narrative.visibleChoices.length) narrative.choose(shell.selected);
    else narrative.advanceDialogue();
    shell.selected = 0;
    refreshWorld();
    sync();
    return;
  }
  if (screen === "timeline") {
    confirmTimeline();
    return;
  }
  if (screen === "settings") {
    nudgeSetting(1);
    return;
  }
  if (screen === "ending") {
    const id = shell.selected === 0 ? "remain" : "menu";
    if (id === "menu") showMain();
    else {
      screen = "play";
      sync();
    }
    return;
  }
  if (screen === "quit") showMain();
  else if (screen === "chapters" || screen === "evidence" || screen === "evidence-entry" || screen === "notebook" || screen === "notebook-entry" || screen === "pause") {
    document.querySelector("#sheet .sheet-list button.on")?.click();
  }
}

function createAudio() {
  const AudioCtx = window.AudioContext || window.webkitAudioContext;
  if (!AudioCtx) return null;
  const ctx = new AudioCtx();
  const master = ctx.createGain();
  const music = ctx.createGain();
  const effects = ctx.createGain();
  music.connect(master);
  effects.connect(master);
  master.connect(ctx.destination);
  const buffer = ctx.createBuffer(1, ctx.sampleRate * 2, ctx.sampleRate);
  const data = buffer.getChannelData(0);
  for (let i = 0; i < data.length; i += 1) data[i] = Math.random() * 2 - 1;
  const rain = ctx.createBufferSource();
  rain.buffer = buffer;
  rain.loop = true;
  const filter = ctx.createBiquadFilter();
  filter.type = "bandpass";
  filter.frequency.value = 1200;
  filter.Q.value = 0.4;
  const rainGain = ctx.createGain();
  rain.connect(filter);
  filter.connect(rainGain);
  rainGain.connect(effects);
  rain.start();
  const city = ctx.createOscillator();
  city.type = "sine";
  city.frequency.value = 46;
  const cityGain = ctx.createGain();
  cityGain.gain.value = 0.03;
  city.connect(cityGain);
  cityGain.connect(effects);
  city.start();
  const piano = ctx.createOscillator();
  piano.type = "triangle";
  piano.frequency.value = 98;
  const pianoGain = ctx.createGain();
  pianoGain.gain.value = 0.02;
  piano.connect(pianoGain);
  pianoGain.connect(music);
  piano.start();
  const api = {
    ctx,
    setLayers(layers) {
      const on = new Set(layers || []);
      rainGain.gain.value = on.has("rain") ? 0.18 : 0.04;
      cityGain.gain.value = on.has("city") ? 0.04 : 0.01;
      pianoGain.gain.value = on.has("piano") || on.has("clocks") ? 0.035 : 0.008;
    },
    apply() {
      master.gain.value = settings.MasterVolume;
      music.gain.value = settings.MusicVolume;
      effects.gain.value = settings.EffectsVolume;
    },
  };
  api.apply();
  api.setLayers(["rain", "city"]);
  return api;
}

function frame(now) {
  const dt = Math.min(0.05, (now - last) / 1000);
  last = now;
  if (world && narrative?.story && screen !== "menu" && screen !== "quit" && screen !== "boot") {
    const blocked = blocksPlay();
    const forward = (keys.has("KeyW") ? 1 : 0) - (keys.has("KeyS") ? 1 : 0);
    const strafe = (keys.has("KeyD") ? 1 : 0) - (keys.has("KeyA") ? 1 : 0);
    world.update(dt, {
      forward,
      strafe,
      sprint: keys.has("ShiftLeft") || keys.has("ShiftRight"),
      lookX,
      lookY,
    }, blocked);
    lookX = 0;
    lookY = 0;
    if (!blocked) {
      const before = narrative.story.CurrentCardId;
      noteLocation();
      if (narrative.story.CurrentCardId !== before || narrative.panelQueue.length) {
        refreshWorld();
        sync();
      }
      saveTimer += dt;
      if (saveTimer > 2) {
        saveTimer = 0;
        remember();
        narrative.autosave();
      }
    }
    refreshHud();
    shell.tickNotice(dt);
  }
  shell.tickCity(settings.MotionReduction ? 0 : dt);
  if ((screen === "menu" || screen === "quit") && !settings.MotionReduction) {
    shell.paintCity(screen === "menu" ? shell.menuCanvas : shell.quitCanvas, screen === "menu" ? narrative.menuClock() : "");
    shell.tickCity(dt);
  }
  requestAnimationFrame(frame);
}

function activeRows() {
  if (screen === "menu") return [...document.querySelectorAll("#menu-list button")];
  if (screen === "quit") return [...document.querySelectorAll("#quit-list button")];
  if (screen === "ending") return [...document.querySelectorAll("#end-list button")];
  if (screen === "settings") return [...document.querySelectorAll("#sheet .settings-row")];
  return [...document.querySelectorAll("#sheet .sheet-list button")];
}

function onKey(event) {
  if (event.repeat && ["Space", "Enter"].includes(event.code)) return;
  keys.add(event.code);
  const menuish = ["menu", "chapters", "evidence", "evidence-entry", "evidence-detail", "notebook", "notebook-entry", "notebook-detail", "settings", "pause", "timeline", "ending", "quit"].includes(screen);
  if (["ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", "Space", "Tab", "KeyQ", "KeyE"].includes(event.code)) event.preventDefault();
  if (event.code === "Escape") {
    event.preventDefault();
    if (performance.now() - lockExitAt < 250) return;
    back();
    return;
  }
  if (event.code === "Tab" && (screen === "play" || screen.startsWith("notebook"))) {
    if (screen.startsWith("notebook")) {
      stack = stack.filter((name) => name !== "notebook" && name !== "notebook-entry");
      sync();
    } else if (screen === "play") showNotebookCategories();
    return;
  }
  if (event.code === "KeyQ" && (screen === "play" || screen === "timeline")) {
    showTimeline();
    return;
  }
  if (event.code === "KeyF" && (screen === "play" || narrative.flashbackActive && screen === "play")) {
    tryMemory();
    return;
  }
  if ((event.code === "KeyE" || event.code === "Digit0") && screen === "play") {
    interact();
    return;
  }
  if (screen === "play" && event.code === "KeyE") return;
  if (screen === "dialogue") {
    const digit = ["Digit1", "Digit2", "Digit3", "Digit4"].indexOf(event.code);
    if (digit >= 0 && narrative.visibleChoices[digit]) {
      shell.selected = 0;
      narrative.choose(digit);
      refreshWorld();
      sync();
      return;
    }
    if (event.code === "Space" || event.code === "Enter") confirm();
    if (event.code === "ArrowUp" || event.code === "ArrowDown") {
      shell.moveSelection(event.code === "ArrowDown" ? 1 : -1, narrative.visibleChoices.length);
      showDialogue();
    }
    return;
  }
  if (screen === "panel" && (event.code === "Space" || event.code === "Enter" || event.code === "KeyE")) {
    narrative.shiftPanel();
    sync();
    return;
  }
  if (screen === "timeline") {
    const events = narrative.chapter()?.Timeline || [];
    if (event.code === "ArrowUp" || event.code === "ArrowDown") {
      const count = events.length;
      shell.timelineIndex = (shell.timelineIndex + (event.code === "ArrowDown" ? 1 : -1) + count) % count;
      shell.timelineBranch = 0;
      renderTimeline();
    }
    if (event.code === "ArrowLeft" || event.code === "ArrowRight") {
      const eventDef = events[shell.timelineIndex];
      const count = eventDef?.Branches?.length || 0;
      if (count) {
        shell.timelineBranch = (shell.timelineBranch + (event.code === "ArrowRight" ? 1 : -1) + count) % count;
        renderTimeline();
      }
    }
    if (event.code === "Enter" || event.code === "Space") confirmTimeline();
    return;
  }
  if (screen === "settings" && (event.code === "ArrowLeft" || event.code === "ArrowRight")) {
    nudgeSetting(event.code === "ArrowRight" ? 1 : -1);
    return;
  }
  if (menuish && (event.code === "ArrowUp" || event.code === "ArrowDown")) {
    const nodes = activeRows();
    shell.moveSelection(event.code === "ArrowDown" ? 1 : -1, nodes.length || 1);
    nodes.forEach((node, index) => node.classList.toggle("on", index === shell.selected));
    return;
  }
  if (menuish && (event.code === "Enter" || event.code === "Space")) confirm();
}

function rerender() {
  if (screen === "menu") showMain();
  else if (screen === "chapters") showChapters();
  else if (screen === "evidence") showEvidenceCategories();
  else if (screen === "pause") showPause();
  else if (screen === "settings") showSettings(top() === "settings");
  else if (screen === "timeline") renderTimeline();
  else if (screen === "ending") showEnding();
}

window.addEventListener("keydown", onKey);
window.addEventListener("keyup", (event) => keys.delete(event.code));
window.addEventListener("resize", () => world?.resize());
document.addEventListener("pointerlockchange", () => {
  if (!document.pointerLockElement && screen === "play") {
    lockExitAt = performance.now();
    showPause();
  }
});

document.addEventListener("mousedown", (event) => {
  audio?.ctx.resume();
  if (event.button === 2 && screen === "play" && world.focus) {
    event.preventDefault();
    examine = { title: world.focus.def.Title, body: world.focus.def.InspectText };
    shell.showExamine(examine.title, examine.body);
    return;
  }
  if (event.button === 0 && screen === "play") {
    const canvas = shell.canvas();
    if (document.pointerLockElement !== canvas) canvas.requestPointerLock();
    else interact();
  }
  if (event.button === 0 && screen === "panel") {
    narrative.shiftPanel();
    sync();
  }
});
window.addEventListener("contextmenu", (event) => event.preventDefault());

async function boot() {
  shell.setContrast(settings.HighContrast);
  shell.setStill(settings.MotionReduction);
  try {
    catalog = await fetchCatalog();
  } catch (error) {
    document.body.textContent = "The narrative files could not be loaded.";
    console.error(error);
    return;
  }
  narrative = new Narrative(catalog);
  world = new Greybox(shell.canvas());
  world.setPreset(settings.GraphicsPreset);
  world.resize();
  audio = createAudio();
  window.HDID = {
    narrative,
    world,
    shell,
    settings,
    get screen() {
      return screen;
    },
    newGame() {
      narrative.newGame("CH01");
      beginSession(false);
    },
    hold(code, down) {
      if (down) keys.add(code);
      else keys.delete(code);
    },
    aim(dx, dy) {
      lookX += dx;
      lookY += dy;
    },
    interact,
    tryMemory,
    dismissPanel() {
      if (screen === "panel") {
        narrative.shiftPanel();
        sync();
      }
    },
    openTimeline() {
      showTimeline();
    },
    closeTop() {
      back();
    },
    snapshot() {
      return {
        screen,
        card: narrative.story?.CurrentCardId || "",
        clues: [...(narrative.story?.CluesFound || [])],
        choices: { ...(narrative.story?.ChoiceByBranch || {}) },
        flags: narrative.story ? narrative.getFlags() : {},
        complete: !!narrative.story?.bChapterComplete,
        memory: narrative.story?.ActiveMemoryId || "",
        queue: narrative.panelQueue.map((item) => item.cardId),
      };
    },
  };
  showMain();
  requestAnimationFrame(frame);
}

boot();
