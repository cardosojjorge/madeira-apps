/**
 * Chapter 01 narrative rules for the browser playtest.
 * Text, ids, flags, and objectives come from Content/Data. Do not author story here.
 */

const SAVE_KEY = "HDID_Story";
const SETTINGS_KEY = "HDID_Settings";
const memoryStore = new Map();

function storeGet(key) {
  try {
    if (typeof localStorage !== "undefined") return localStorage.getItem(key);
  } catch {
    /* private mode */
  }
  return memoryStore.has(key) ? memoryStore.get(key) : null;
}

function storeSet(key, value) {
  try {
    if (typeof localStorage !== "undefined") {
      localStorage.setItem(key, value);
      return;
    }
  } catch {
    /* private mode */
  }
  memoryStore.set(key, value);
}

function storeDel(key) {
  try {
    if (typeof localStorage !== "undefined") localStorage.removeItem(key);
  } catch {
    /* private mode */
  }
  memoryStore.delete(key);
}

export const NOTEBOOK_CATEGORIES = [
  "People",
  "Locations",
  "Objects",
  "Clues",
  "Memories",
  "Events",
  "Timeline",
  "Connections",
];

const SOUL_KEYS = [
  "Truth",
  "Empathy",
  "Courage",
  "Sacrifice",
  "Curiosity",
  "Honesty",
  "Compassion",
  "Understanding",
  "Dedication",
];

export function defaultSettings() {
  return {
    MasterVolume: 0.8,
    MusicVolume: 0.7,
    EffectsVolume: 0.8,
    DialogueVolume: 1,
    Subtitles: true,
    SubtitleScale: 1,
    GraphicsPreset: "High",
    MouseSensitivity: 1,
    Vibration: true,
    MotionReduction: false,
    HighContrast: false,
  };
}

export function loadSettings() {
  try {
    const raw = storeGet(SETTINGS_KEY);
    if (!raw) return defaultSettings();
    return { ...defaultSettings(), ...JSON.parse(raw) };
  } catch {
    return defaultSettings();
  }
}

export function writeSettings(settings) {
  storeSet(SETTINGS_KEY, JSON.stringify(settings));
}

export function hasStorySave() {
  return !!storeGet(SAVE_KEY);
}

export function readStorySave() {
  try {
    const raw = storeGet(SAVE_KEY);
    return raw ? JSON.parse(raw) : null;
  } catch {
    return null;
  }
}

export function writeStorySave(story) {
  storeSet(SAVE_KEY, JSON.stringify(story));
}

export function clearStorySave() {
  storeDel(SAVE_KEY);
}

function emptySoul() {
  const soul = {};
  for (const key of SOUL_KEYS) soul[key] = 0;
  return soul;
}

function emptyDedication() {
  return {
    CluesFound: 0,
    OptionalCluesFound: 0,
    HiddenObjects: 0,
    FlashbacksDiscovered: 0,
    AlternateTimelines: 0,
    NPCsHelped: 0,
    OptionalDialogue: 0,
    CrossChapterConnections: 0,
  };
}

function conditionsMet(conditions, flags) {
  if (!conditions || conditions.length === 0) return true;
  for (const condition of conditions) {
    if (flags[condition.Flag] !== condition.Equals) return false;
  }
  return true;
}

function anyConditionMet(conditions, flags) {
  if (!conditions || conditions.length === 0) return false;
  for (const condition of conditions) {
    if (flags[condition.Flag] === condition.Equals) return true;
  }
  return false;
}

export class Narrative {
  constructor(catalog) {
    this.catalog = catalog;
    this.story = null;
    this.derivedFlags = {};
    this.dialogueOpen = false;
    this.currentDialogueId = "";
    this.visibleChoices = [];
    this.flashbackActive = false;
    this.returnPose = null;
    this.completionAnnounced = false;
    this.panelQueue = [];
    this.panelsSeen = new Set();
    this.lastBody = "";
    this.listeners = new Set();
  }

  onChange(fn) {
    this.listeners.add(fn);
    return () => this.listeners.delete(fn);
  }

  emit() {
    for (const fn of this.listeners) fn();
  }

  chapter() {
    if (!this.story) return null;
    return this.catalog.chapters[this.story.CurrentChapterId] || null;
  }

  cardById(id) {
    const chapter = this.chapter();
    if (!chapter) return null;
    return chapter.Cards.find((card) => card.CardID === id) || null;
  }

  currentCard() {
    return this.story ? this.cardById(this.story.CurrentCardId) : null;
  }

  cardIndex(id) {
    const chapter = this.chapter();
    if (!chapter) return -1;
    return chapter.Cards.findIndex((card) => card.CardID === id);
  }

  indexLabel() {
    const chapter = this.chapter();
    const index = this.cardIndex(this.story?.CurrentCardId);
    if (!chapter || index < 0) return "";
    const n = String(index + 1).padStart(2, "0");
    const count = String(chapter.Cards.length).padStart(2, "0");
    return `CARD ${n} / ${count}`;
  }

  character(id) {
    return this.catalog.characters[id] || null;
  }

  findChoice(id) {
    return this.catalog.choices[id] || null;
  }

  findDialogue(id) {
    const chapter = this.chapter();
    if (!chapter) return null;
    return chapter.Dialogue.find((node) => node.DialogueID === id) || null;
  }

  findClue(id) {
    const chapter = this.chapter();
    if (!chapter) return null;
    return chapter.Clues.find((clue) => clue.ClueID === id) || null;
  }

  findConsequence(id) {
    const chapter = this.chapter();
    if (!chapter) return null;
    return chapter.Consequences.find((item) => item.Id === id) || null;
  }

  findMemory(id) {
    const chapter = this.chapter();
    if (!chapter) return null;
    return chapter.Memories.find((item) => item.MemoryID === id) || null;
  }

  findEvent(id) {
    const chapter = this.chapter();
    if (!chapter) return null;
    return chapter.Timeline.find((item) => item.EventID === id) || null;
  }

  isPlayable(chapterId) {
    const chapter = this.catalog.chapters[chapterId];
    return !!(chapter && chapter.Playable);
  }

  menuClock() {
    const playable = this.catalog.order.find((id) => this.isPlayable(id));
    const chapter = playable ? this.catalog.chapters[playable] : null;
    return chapter?.Clock || this.catalog.game.MenuClockFallback || "23:47";
  }

  blankStory(chapterId) {
    const chapter = this.catalog.chapters[chapterId];
    const story = {
      CurrentChapterId: chapterId,
      CurrentCardId: chapter.Cards[0].CardID,
      ProtagonistId: chapter.ProtagonistId,
      TimelineVisited: [],
      TimelineUnlocked: chapter.Timeline.filter((event) => !event.StartsLocked).map((event) => event.EventID),
      ChoiceByBranch: {},
      OneShotChoices: [],
      CluesFound: [],
      MemoriesFound: [],
      Relationships: {},
      CrossChapterDiscoveries: [],
      Soul: emptySoul(),
      Dedication: emptyDedication(),
      EndingsUnlocked: [],
      UnlockedContent: [],
      SavedLocation: null,
      SavedRotation: null,
      bHasSavedTransform: false,
      ActiveMemoryId: "",
      BaseFlags: {},
      CardsReached: [chapter.Cards[0].CardID],
      SpeakersHeard: [],
      LocationsEntered: [],
      InspectedIds: [],
      DialoguesCompleted: [],
      bTimelineOpened: false,
      BranchRevisionCount: 0,
      bChapterComplete: false,
      ReturnPose: null,
    };
    return story;
  }

  newGame(chapterId) {
    if (!this.isPlayable(chapterId)) return false;
    this.story = this.blankStory(chapterId);
    this.derivedFlags = {};
    this.dialogueOpen = false;
    this.currentDialogueId = "";
    this.visibleChoices = [];
    this.flashbackActive = false;
    this.returnPose = null;
    this.completionAnnounced = false;
    this.panelQueue = [];
    this.panelsSeen = new Set();
    this.lastBody = "";
    this.rebuild();
    this.queuePanel(this.story.CurrentCardId, true);
    this.autosave();
    this.emit();
    return true;
  }

  continueGame() {
    const loaded = readStorySave();
    if (!loaded || !loaded.CurrentChapterId) return false;
    if (!this.isPlayable(loaded.CurrentChapterId)) return false;
    this.story = loaded;
    this.story.Soul = { ...emptySoul(), ...(loaded.Soul || {}) };
    this.story.Dedication = { ...emptyDedication(), ...(loaded.Dedication || {}) };
    this.story.ChoiceByBranch = { ...(loaded.ChoiceByBranch || {}) };
    this.story.BaseFlags = { ...(loaded.BaseFlags || {}) };
    this.story.Relationships = { ...(loaded.Relationships || {}) };
    this.derivedFlags = {};
    this.dialogueOpen = false;
    this.visibleChoices = [];
    this.currentDialogueId = "";
    this.flashbackActive = !!loaded.ActiveMemoryId;
    this.returnPose = loaded.ReturnPose || null;
    this.completionAnnounced = !!loaded.bChapterComplete;
    this.panelQueue = [];
    this.rebuild();
    this.panelsSeen = new Set();
    for (const id of this.story.CardsReached || []) {
      const card = this.cardById(id);
      if (!card) continue;
      const body = id === this.story.CurrentCardId ? this.getBody(card) : card.Body;
      this.panelsSeen.add(`${id}::${body}`);
    }
    this.lastBody = this.getBody();
    this.emit();
    return true;
  }

  autosave() {
    if (!this.story) return;
    this.story.ActiveMemoryId = this.flashbackActive ? this.story.ActiveMemoryId : "";
    this.story.ReturnPose = this.returnPose;
    writeStorySave(this.story);
  }

  rememberPose(pose) {
    if (!this.story) return;
    this.story.SavedLocation = { X: pose.x, Y: pose.y, Z: pose.z };
    this.story.SavedRotation = { Yaw: pose.yaw, Pitch: pose.pitch };
    this.story.bHasSavedTransform = true;
  }

  getFlags() {
    return { ...(this.story?.BaseFlags || {}), ...this.derivedFlags };
  }

  requirementsMet(choice) {
    return conditionsMet(choice.Requirements || [], this.getFlags());
  }

  applyConsequence(consequence, relationships) {
    for (const [flag, value] of Object.entries(consequence.SetFlags || {})) {
      this.derivedFlags[flag] = value;
    }
    for (const [who, delta] of Object.entries(consequence.Relationship || {})) {
      relationships[who] = (relationships[who] || 0) + delta;
    }
  }

  activeChoiceIds() {
    const ids = [...(this.story.OneShotChoices || [])];
    for (const value of Object.values(this.story.ChoiceByBranch || {})) ids.push(value);
    return ids;
  }

  getActiveConsequences() {
    const out = [];
    for (const choiceId of this.activeChoiceIds()) {
      const choice = this.findChoice(choiceId);
      if (!choice) continue;
      const consequence = this.findConsequence(choice.ConsequenceID);
      if (consequence) out.push(consequence);
    }
    return out;
  }

  rebuild() {
    if (!this.story) return;
    this.derivedFlags = {};
    const relationships = {};
    for (const dialogueId of this.story.DialoguesCompleted) {
      const node = this.findDialogue(dialogueId);
      if (!node) continue;
      for (const [who, delta] of Object.entries(node.RelationshipChanges || {})) {
        relationships[who] = (relationships[who] || 0) + delta;
      }
    }
    const reveals = [];
    for (const consequence of this.getActiveConsequences()) {
      this.applyConsequence(consequence, relationships);
      reveals.push(...(consequence.RevealClues || []));
      for (const eventId of consequence.UnlockTimeline || []) this.unlockEvent(eventId);
    }
    this.story.Relationships = relationships;
    for (const clueId of reveals) this.discoverClue(clueId, true);
    this.recomputeSoul();
  }

  recomputeSoul() {
    if (!this.story) return;
    const soul = emptySoul();
    const dedication = emptyDedication();
    dedication.AlternateTimelines = this.story.BranchRevisionCount || 0;
    dedication.FlashbacksDiscovered = (this.story.MemoriesFound || []).length;
    dedication.CrossChapterConnections = (this.story.CrossChapterDiscoveries || []).length;
    for (const clueId of this.story.CluesFound) {
      const clue = this.findClue(clueId);
      if (!clue) continue;
      const active = this.isClueActive(clueId);
      if (!active && clue.BranchExclusive) continue;
      dedication.CluesFound += 1;
      if (clue.Optional) dedication.OptionalCluesFound += 1;
      if (clue.Hidden) dedication.HiddenObjects += 1;
      for (const [key, value] of Object.entries(clue.SoulOnDiscover || {})) {
        if (key in soul) soul[key] += value;
      }
    }
    for (const consequence of this.getActiveConsequences()) {
      for (const [key, value] of Object.entries(consequence.Soul || {})) {
        if (key in soul) soul[key] += value;
      }
      if (consequence.HelpedNpc) dedication.NPCsHelped += 1;
      if (consequence.OptionalDialogue) dedication.OptionalDialogue += 1;
    }
    this.story.Soul = soul;
    this.story.Dedication = dedication;
  }

  discoverClue(clueId, fromRebuild = false) {
    if (!clueId || !this.story || this.story.CluesFound.includes(clueId)) return false;
    const clue = this.findClue(clueId);
    if (!clue) return false;
    this.story.CluesFound.push(clueId);
    if (clue.GrantFlag) this.story.BaseFlags[clue.GrantFlag] = clue.GrantValue;
    if (clue.CrossChapter) {
      const mark = `${clueId}@${clue.CrossTarget}`;
      if (!this.story.CrossChapterDiscoveries.includes(mark)) this.story.CrossChapterDiscoveries.push(mark);
    }
    for (const speaker of clue.RelatedCharacters || []) {
      if (!this.story.SpeakersHeard.includes(speaker)) this.story.SpeakersHeard.push(speaker);
    }
    for (const eventId of clue.RelatedEvents || []) this.unlockEvent(eventId);
    this.unlockFromClue(clueId);
    if (!fromRebuild) {
      this.recomputeSoul();
      this.tryAdvance();
      this.autosave();
    }
    return true;
  }

  isClueDiscovered(clueId) {
    return !!this.story && this.story.CluesFound.includes(clueId);
  }

  isClueActive(clueId) {
    if (!this.isClueDiscovered(clueId)) return false;
    const clue = this.findClue(clueId);
    if (!clue) return false;
    if (!clue.BranchExclusive) return true;
    return conditionsMet(clue.ActiveIf || [], this.getFlags());
  }

  notifySpeaker(id) {
    if (id && this.story && !this.story.SpeakersHeard.includes(id)) this.story.SpeakersHeard.push(id);
  }

  notifyLocation(locationId) {
    if (!locationId || !this.story) return;
    if (!this.story.LocationsEntered.includes(locationId)) {
      this.story.LocationsEntered.push(locationId);
      this.tryAdvance();
    }
  }

  isCardReached(cardId) {
    if (!cardId) return true;
    return !!this.story && this.story.CardsReached.includes(cardId);
  }

  isObjectiveMet(objective) {
    if (!this.story) return false;
    const parts = String(objective).split("|");
    for (const part of parts) {
      const split = part.indexOf(":");
      if (split < 0) continue;
      const key = part.slice(0, split);
      const value = part.slice(split + 1);
      if (key === "discover" && this.isClueActive(value)) return true;
      if (key === "inspect" && this.story.InspectedIds.includes(value)) return true;
      if (key === "dialogue" && this.story.DialoguesCompleted.includes(value)) return true;
      if (key === "commit" && Object.prototype.hasOwnProperty.call(this.story.ChoiceByBranch, value)) return true;
      if (key === "flashback" && this.story.MemoriesFound.includes(value)) return true;
      if (key === "open" && value === "TIMELINE" && this.story.bTimelineOpened) return true;
      if (key === "enter" && this.story.LocationsEntered.includes(value)) return true;
    }
    return false;
  }

  areObjectivesMet(card) {
    for (const objective of card.Objectives || []) {
      if (!this.isObjectiveMet(objective)) return false;
    }
    return true;
  }

  getBody(card = this.currentCard()) {
    if (!card) return "";
    const flags = this.getFlags();
    for (const variant of card.Variants || []) {
      if (conditionsMet(variant.IfAll || [], flags)) return variant.Body;
    }
    return card.Body;
  }

  queuePanel(cardId, force = false) {
    const card = this.cardById(cardId);
    if (!card) return;
    const body = this.getBody(card);
    const token = `${cardId}::${body}`;
    if (!force && this.panelsSeen.has(token)) return;
    this.panelsSeen.add(token);
    this.panelQueue.push({ cardId, body });
    this.lastBody = body;
  }

  shiftPanel() {
    return this.panelQueue.shift() || null;
  }

  tryAdvance() {
    if (!this.story) return;
    for (let guard = 0; guard < 16; guard += 1) {
      const card = this.currentCard();
      if (!card) return;
      if (!this.areObjectivesMet(card)) return;
      if (!card.NextCards || card.NextCards.length === 0) {
        this.completeChapter();
        return;
      }
      this.story.CurrentCardId = card.NextCards[0];
      if (!this.story.CardsReached.includes(this.story.CurrentCardId)) {
        this.story.CardsReached.push(this.story.CurrentCardId);
      }
      this.queuePanel(this.story.CurrentCardId);
    }
  }

  completeChapter() {
    if (!this.story || this.story.bChapterComplete) return;
    this.story.bChapterComplete = true;
    const mark = `${this.story.CurrentChapterId}_COMPLETE`;
    if (!this.story.UnlockedContent.includes(mark)) this.story.UnlockedContent.push(mark);
    this.completionAnnounced = false;
    this.autosave();
  }

  consumeChapterComplete() {
    if (!this.story?.bChapterComplete || this.completionAnnounced) return false;
    this.completionAnnounced = true;
    return true;
  }

  notifyInspect(id) {
    if (!this.story || !id) return;
    if (!this.story.InspectedIds.includes(id)) this.story.InspectedIds.push(id);
    this.tryAdvance();
  }

  commit(choiceId, advanceCard) {
    const choice = this.findChoice(choiceId);
    if (!choice || !this.story || !this.requirementsMet(choice)) return false;
    if (choice.BranchGroup) {
      const previous = this.story.ChoiceByBranch[choice.BranchGroup];
      if (previous !== choiceId) {
        if (previous) this.story.BranchRevisionCount += 1;
        this.story.ChoiceByBranch[choice.BranchGroup] = choiceId;
      }
    } else if (!this.story.OneShotChoices.includes(choiceId)) {
      this.story.OneShotChoices.push(choiceId);
    }
    this.rebuild();
    if (advanceCard) this.tryAdvance();
    const body = this.getBody();
    if (body !== this.lastBody) this.queuePanel(this.story.CurrentCardId, true);
    this.autosave();
    this.emit();
    return true;
  }

  startDialogue(dialogueId) {
    const node = this.findDialogue(dialogueId);
    if (!node) return false;
    const lower = (node.Text || "").toLowerCase();
    if (node.Reveal || lower.includes("grim reaper") || lower.includes("the void") || lower.includes("soul 11") || lower.includes("you died of")) {
      return false;
    }
    this.openNode(node);
    return true;
  }

  openNode(node) {
    this.dialogueOpen = true;
    this.currentDialogueId = node.DialogueID;
    const first = !this.story.DialoguesCompleted.includes(node.DialogueID);
    if (!this.story.DialoguesCompleted.includes(node.DialogueID)) {
      this.story.DialoguesCompleted.push(node.DialogueID);
    }
    if (node.Speaker) this.notifySpeaker(node.Speaker);
    if (first) {
      for (const clueId of node.ClueUnlocks || []) this.discoverClue(clueId);
      this.rebuild();
    }
    this.visibleChoices = (node.Choices || []).filter((choice) => this.requirementsMet(choice));
    this.tryAdvance();
    this.autosave();
    this.emit();
  }

  choose(index) {
    const choice = this.visibleChoices[index];
    if (!choice) return;
    this.commit(choice.ChoiceID, false);
    if (choice.NextNode) this.startDialogue(choice.NextNode);
    else {
      this.closeDialogue();
      this.tryAdvance();
      this.emit();
    }
  }

  advanceDialogue() {
    if (!this.dialogueOpen || this.visibleChoices.length > 0) return;
    const node = this.findDialogue(this.currentDialogueId);
    if (node?.NextNode) this.startDialogue(node.NextNode);
    else this.closeDialogue();
    this.emit();
  }

  closeDialogue() {
    this.dialogueOpen = false;
    this.visibleChoices = [];
    this.currentDialogueId = "";
  }

  currentNode() {
    return this.findDialogue(this.currentDialogueId);
  }

  unlockEvent(eventId) {
    if (!eventId || !this.story) return;
    if (!this.story.TimelineUnlocked.includes(eventId)) this.story.TimelineUnlocked.push(eventId);
  }

  unlockFromClue(clueId) {
    const chapter = this.chapter();
    if (!chapter) return;
    for (const event of chapter.Timeline) {
      if (event.UnlockOnClue === clueId) this.unlockEvent(event.EventID);
    }
  }

  isEventUnlocked(eventId) {
    return !!this.story && this.story.TimelineUnlocked.includes(eventId);
  }

  travelTo(eventId) {
    if (!this.isEventUnlocked(eventId)) return false;
    const event = this.findEvent(eventId);
    if (!event || !this.story) return false;
    if (!this.story.TimelineVisited.includes(eventId)) this.story.TimelineVisited.push(eventId);
    this.autosave();
    return event;
  }

  notifyTimelineOpened() {
    if (!this.story) return;
    this.story.bTimelineOpened = true;
    this.tryAdvance();
    this.autosave();
    this.emit();
  }

  beginMemory(memoryId) {
    if (this.flashbackActive || !memoryId) return false;
    const memory = this.findMemory(memoryId);
    if (!memory || !this.story) return false;
    this.flashbackActive = true;
    this.story.ActiveMemoryId = memoryId;
    if (!this.story.MemoriesFound.includes(memoryId)) this.story.MemoriesFound.push(memoryId);
    this.unlockEvent(memory.TimelineEventID);
    this.travelTo(memory.TimelineEventID);
    this.tryAdvance();
    this.recomputeSoul();
    this.autosave();
    this.emit();
    return memory;
  }

  canLeaveMemory() {
    const memory = this.findMemory(this.story?.ActiveMemoryId);
    if (!memory) return true;
    if (!memory.RequiredClue) return true;
    return this.isClueDiscovered(memory.RequiredClue);
  }

  holdLine() {
    const memory = this.findMemory(this.story?.ActiveMemoryId);
    return memory?.HoldLine || "The memory is still holding something.";
  }

  tryReturn() {
    if (!this.flashbackActive) return { ok: false, reason: "" };
    if (!this.canLeaveMemory()) return { ok: false, reason: this.holdLine() };
    this.flashbackActive = false;
    if (this.story) this.story.ActiveMemoryId = "";
    const pose = this.returnPose;
    this.returnPose = null;
    this.autosave();
    this.emit();
    return { ok: true, pose };
  }

  interactableVisible(def) {
    if (!this.story) return false;
    if (def.RequiredCard && !this.isCardReached(def.RequiredCard)) return false;
    const flags = this.getFlags();
    if ((def.PresentIf || []).length > 0 && !conditionsMet(def.PresentIf, flags)) return false;
    if (anyConditionMet(def.AbsentIf || [], flags)) return false;
    if (def.OnlyInFlashback && def.FlashbackMemoryID !== (this.flashbackActive ? this.story.ActiveMemoryId : "")) return false;
    return true;
  }

  visibleInteractables() {
    const chapter = this.chapter();
    if (!chapter) return [];
    return chapter.Interactables.filter((def) => this.interactableVisible(def));
  }

  locationAt(x, y) {
    const chapter = this.chapter();
    if (!chapter) return null;
    let best = null;
    let bestArea = Infinity;
    for (const location of chapter.Locations) {
      const inside = x >= location.Origin.X && x <= location.Origin.X + location.SizeX && y >= location.Origin.Y && y <= location.Origin.Y + location.SizeY;
      const area = location.SizeX * location.SizeY;
      if (inside && area < bestArea) {
        best = location;
        bestArea = area;
      }
    }
    return best;
  }

  buildNotebook() {
    if (!this.story) return [];
    const entries = [];
    const add = (category, title, body, source) => entries.push({ category, title, body, source });
    const people = new Set(this.story.SpeakersHeard || []);
    for (const clueId of this.story.CluesFound) {
      if (!this.isClueActive(clueId)) continue;
      const clue = this.findClue(clueId);
      for (const who of clue?.RelatedCharacters || []) people.add(who);
    }
    for (const personId of people) {
      const character = this.character(personId);
      if (!character) continue;
      let body = character.NotebookIntro;
      for (const clueId of this.story.CluesFound) {
        if (!this.isClueActive(clueId)) continue;
        const clue = this.findClue(clueId);
        if (clue?.RelatedCharacters?.includes(personId)) body += `\nFiled beside: ${clue.Title}`;
      }
      add("People", character.DisplayName, body, personId);
    }
    for (const locationId of this.story.LocationsEntered) {
      const location = this.chapter()?.Locations.find((item) => item.LocationId === locationId);
      if (location) add("Locations", location.Name, location.Description, locationId);
    }
    for (const clueId of this.story.CluesFound) {
      if (!this.isClueActive(clueId)) continue;
      const clue = this.findClue(clueId);
      if (!clue) continue;
      add("Clues", clue.Title, `${clue.Description}\n${clue.DiscoveryMethod} · ${clue.Importance}`, clueId);
      if ((clue.Categories || []).includes("Objects")) add("Objects", clue.Title, clue.Description, clueId);
    }
    for (const memoryId of this.story.MemoriesFound) {
      const memory = this.findMemory(memoryId);
      if (!memory) continue;
      add("Memories", memory.MemoryID, `${memory.QuestionAnswered}\n\n${memory.QuestionOpened}`, memoryId);
    }
    for (const cardId of this.story.CardsReached) {
      const card = this.cardById(cardId);
      if (!card) continue;
      const body = cardId === this.story.CurrentCardId ? this.getBody(card) : card.Body;
      add("Events", `${card.Time}  ${card.Title}`, body, cardId);
    }
    const chapter = this.chapter();
    if (chapter) {
      for (const event of chapter.Timeline) {
        if (!this.story.TimelineUnlocked.includes(event.EventID)) continue;
        let body = event.Title;
        if (this.story.TimelineVisited.includes(event.EventID)) body += "\nVisited.";
        for (const branch of event.Branches || []) {
          body += `\n${branch.Label}`;
          const choice = this.findChoice(branch.ChoiceID);
          if (choice && choice.BranchGroup) {
            const chosen = this.story.ChoiceByBranch[choice.BranchGroup];
            if (chosen && (chosen === branch.ChoiceID || chosen.startsWith(branch.ChoiceID) || branch.ChoiceID.startsWith(chosen))) {
              body += "  — standing";
            }
          }
        }
        add("Timeline", event.Time, body, event.EventID);
        add("Events", `${event.Time}  ${event.Title}`, event.Title, event.EventID);
      }
    }
    if ((this.story.BranchRevisionCount || 0) > 0) {
      add("Timeline", "Revision", "An earlier version of this hour was recorded. The room now matches the choice that stands.", "REVISION");
    }
    const activeClues = this.story.CluesFound.filter((id) => this.isClueActive(id)).sort();
    for (const clueId of activeClues) {
      const clue = this.findClue(clueId);
      if (!clue) continue;
      for (const relatedId of clue.RelatedClues || []) {
        if (relatedId <= clueId || !this.isClueActive(relatedId)) continue;
        const related = this.findClue(relatedId);
        if (!related) continue;
        add("Connections", "Filed together", `${clue.Title}\n${related.Title}`, `${clueId}|${relatedId}`);
      }
    }
    return entries;
  }
}

export async function fetchCatalog() {
  const order = ["CH01", "CH02", "CH03", "CH04", "CH05", "CH06", "CH07", "CH08", "CH09", "CH10"];
  const [game, characterFile, ...chapterFiles] = await Promise.all([
    fetch("/Content/Data/Game.json").then((response) => response.json()),
    fetch("/Content/Data/Characters.json").then((response) => response.json()),
    ...order.map((id) => fetch(`/Content/Data/Chapters/${id}.json`).then((response) => response.json())),
  ]);
  const chapters = {};
  order.forEach((id, index) => {
    chapters[id] = chapterFiles[index];
  });
  const characters = {};
  for (const character of characterFile.Characters) characters[character.Id] = character;
  const choices = {};
  for (const id of order) {
    for (const node of chapters[id].Dialogue || []) {
      for (const choice of node.Choices || []) choices[choice.ChoiceID] = choice;
    }
  }
  return { game, characters, chapters, order, choices };
}
