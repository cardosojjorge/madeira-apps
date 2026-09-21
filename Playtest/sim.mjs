import { readFileSync } from "node:fs";
import { Narrative, clearStorySave, readStorySave } from "./js/systems.js";

const data = "/workspace/Content/Data";
const order = ["CH01", "CH02", "CH03", "CH04", "CH05", "CH06", "CH07", "CH08", "CH09", "CH10"];
const read = (path) => JSON.parse(readFileSync(path, "utf8"));
const game = read(`${data}/Game.json`);
const characterFile = read(`${data}/Characters.json`);
const chapters = {};
for (const id of order) chapters[id] = read(`${data}/Chapters/${id}.json`);
const characters = Object.fromEntries(characterFile.Characters.map((character) => [character.Id, character]));
const choices = {};
for (const id of order) {
  for (const node of chapters[id].Dialogue || []) {
    for (const choice of node.Choices || []) choices[choice.ChoiceID] = choice;
  }
}
const catalog = { game, characters, chapters, order, choices };

function play(mayaChoice, doorChoice) {
  clearStorySave();
  const gameState = new Narrative(catalog);
  const panels = [];
  const track = () => {
    while (gameState.panelQueue.length) panels.push(gameState.shiftPanel().cardId);
  };
  if (!gameState.newGame("CH01")) throw new Error("new game failed");
  track();
  const act = (id) => {
    const def = chapters.CH01.Interactables.find((item) => item.Id === id);
    if (!def || !gameState.interactableVisible(def)) throw new Error(`missing ${id}`);
    gameState.notifyInspect(def.Id);
    if (def.ClueID) gameState.discoverClue(def.ClueID);
    if (def.DialogueID) gameState.startDialogue(def.DialogueID);
    if (def.MemoryID && !gameState.flashbackActive) {
      gameState.returnPose = { x: 1, y: 2, z: 3, yaw: 0, pitch: 0 };
      const memory = gameState.beginMemory(def.MemoryID);
      if (!memory) throw new Error(`memory ${id}`);
    }
    track();
  };
  const choose = (textIncludes) => {
    const index = gameState.visibleChoices.findIndex((choice) => choice.Text.includes(textIncludes));
    if (index < 0) throw new Error(`choice ${textIncludes} in ${gameState.currentDialogueId}`);
    gameState.choose(index);
    track();
  };

  act("INT_CLOCK");
  act("INT_PHOTO");
  act("INT_NAMEPLATE");
  act("INT_CONTRACT");
  act("INT_WAIVER");
  act("INT_COIN");
  act("INT_PHONE");
  choose(mayaChoice === "help" ? "Play the last" : "Save the message");
  if (gameState.dialogueOpen) gameState.advanceDialogue();
  act("INT_UMBRELLA");
  if (!gameState.flashbackActive) throw new Error("flashback did not start");
  const early = gameState.tryReturn();
  if (early.ok) throw new Error("left memory too soon");
  act("INT_STATEMENT");
  act("INT_MAYA");
  choose("Who told you");
  choose(mayaChoice === "help" ? "Show me where to sign" : "Take them back");
  if (gameState.dialogueOpen) gameState.advanceDialogue();
  const back = gameState.tryReturn();
  if (!back.ok) throw new Error(back.reason || "return failed");
  gameState.notifyTimelineOpened();
  track();
  act("INT_STRANGER");
  const coin = gameState.visibleChoices.find((choice) => choice.ChoiceID === "CHOICE_ASK_COIN");
  if (!coin) throw new Error("coin question missing");
  choose("What door");
  if (gameState.dialogueOpen) gameState.advanceDialogue();
  act("INT_HOURGLASS");
  act("INT_DOOR");
  choose(doorChoice === "open" ? "Open it." : "Step back.");
  if (gameState.dialogueOpen) gameState.advanceDialogue();
  const finalId = {
    helpopen: "INT_FINAL_HO",
    helpshut: "INT_FINAL_HS",
    ignoreopen: "INT_FINAL_IO",
    ignoreshut: "INT_FINAL_IS",
  }[`${mayaChoice}${doorChoice}`];
  act(finalId);
  if (!gameState.story.bChapterComplete) throw new Error("chapter not complete");
  const uniquePanels = [...new Set(panels)];
  if (uniquePanels.length !== 12) throw new Error(`panels ${uniquePanels.length}: ${uniquePanels.join(",")}`);
  return gameState;
}

const helped = play("help", "open");
const body = helped.getBody();
if (!body.includes("DELAYED") || !body.includes("roof door")) throw new Error(`unexpected body ${body}`);
const notebook = helped.buildNotebook().map((entry) => entry.body).join("\n");
for (const banned of ["you died of", "heart", "The Void", "grim reaper", "Soul 11"]) {
  if (notebook.toLowerCase().includes(banned.toLowerCase())) throw new Error(`notebook leaked ${banned}`);
}
const saved = readStorySave();
if (saved.CurrentChapterId !== "CH01" || saved.CurrentCardId !== "CH01_CARD_12") throw new Error("save card");
if (!saved.CluesFound.includes("CLUE_STATEMENT") || saved.ChoiceByBranch.BRANCH_MAYA !== "CHOICE_HELP_MAYA_B") {
  throw new Error(JSON.stringify(saved.ChoiceByBranch));
}
const resumed = new Narrative(catalog);
if (!resumed.continueGame()) throw new Error("continue failed");
if (resumed.story.CurrentCardId !== "CH01_CARD_12" || !resumed.isClueActive("CLUE_FINAL_HO")) throw new Error("resume state");
if (!resumed.commit("CHOICE_IGNORE_MAYA", true)) throw new Error("recommit");
if (!resumed.getBody().includes("EXECUTED")) throw new Error("branch did not rewrite card");
if (resumed.isClueActive("CLUE_FINAL_HO")) throw new Error("old final clue still filed");
const swapped = chapters.CH01.Interactables.find((item) => item.Id === "INT_FINAL_IO");
if (!resumed.interactableVisible(swapped)) throw new Error("replacement desk object missing");
resumed.notifyInspect(swapped.Id);
resumed.discoverClue(swapped.ClueID);
if (!resumed.isClueActive("CLUE_FINAL_IO")) throw new Error("new final clue was not filed");

const shut = play("ignore", "shut");
if (!shut.getBody().includes("EXECUTED") || !shut.isClueActive("CLUE_FINAL_IS")) throw new Error("shut ending");
const blocked = new Narrative(catalog);
if (blocked.newGame("CH02")) throw new Error("chapter 2 started");
console.log("sim ok", helped.story.CluesFound.length, "clues");
