#!/usr/bin/env python3
"""Build Content/Data JSON and validate it against the narrative rules.

The game loads the JSON. This script is the authoring source for that data
and a mirror of the in-engine validator (HDID.ValidateNarrative).
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Content" / "Data"

PROTAGONISTS = [
    ("CH_ADRIAN", "Adrian Vale", 42, "CEO", "Ambition", "CH01"),
    ("CH_MAYA", "Maya", 21, "Student", "Trust", "CH02"),
    ("CH_DANIEL", "Daniel Cross", 51, "Detective", "Truth", "CH03"),
    ("CH_ELENA", "Elena Moretti", 39, "Doctor", "Responsibility", "CH04"),
    ("CH_LUCAS", "Lucas", 34, "Photographer / Traveler", "Escape", "CH05"),
    ("CH_MARCO", "Marco", 47, "Chef", "Ambition / Sacrifice", "CH06"),
    ("CH_SOFIA", "Sofia", 27, "Athlete", "Sacrifice", "CH07"),
    ("CH_NINA", "Nina", 29, "Influencer", "Identity", "CH08"),
    ("CH_ELIAS", "Elias Hart", 56, "Scientist", "Control", "CH09"),
    ("CH_CLARA", "Clara", 63, "Artist", "Memory", "CH10"),
]

COIN_BEATS = {
    "CH01": "Adrian discovers the coin.",
    "CH02": "Maya possesses the coin.",
    "CH03": "Daniel identifies the coin.",
    "CH04": "Elena sees the coin in an old photograph.",
    "CH05": "Lucas photographs the coin.",
    "CH06": "Marco receives the coin.",
    "CH07": "Sofia finds the coin.",
    "CH08": "Nina sees the coin in an old recording.",
    "CH09": "Elias investigates the coin.",
    "CH10": "Clara paints the coin.",
}

HOURGLASS_BEATS = {
    "CH01": "An hourglass in the boardroom is nearly empty, and still only atmospheric.",
    "CH02": "A cheap hourglass timer sits on Maya's desk, borrowed from a lab class.",
    "CH03": "The hourglass appears in a crime-scene photograph Daniel cannot date.",
    "CH04": "A patient's hourglass was left in Elena's office after visiting hours.",
    "CH05": "The station clock is shaped like an hourglass; Lucas frames it and walks away.",
    "CH06": "Marco's kitchen timer is an hourglass he never turns.",
    "CH07": "The stadium clock is painted as an hourglass over the final lap.",
    "CH08": "Nina's stream overlay is an hourglass she does not remember adding.",
    "CH09": "Elias draws the hourglass as a function and the labels fall off the axes.",
    "CH10": "Clara paints the hourglass behind the coin, sand in both bulbs at once.",
}

STRANGER_STAGES = {
    "CH01": "Silhouette",
    "CH02": "Hands",
    "CH03": "Voice",
    "CH04": "PartialFace",
    "CH05": "Photograph",
    "CH06": "HistoricalReference",
    "CH07": "DirectInteraction",
    "CH08": "Ageless",
    "CH09": "IdentityNearlyConfirmed",
    "CH10": "FullRevealLocked",
}

SATURATION = {
    "CH01": 0.10,
    "CH02": 0.14,
    "CH03": 0.18,
    "CH04": 0.22,
    "CH05": 0.26,
    "CH06": 0.30,
    "CH07": 0.34,
    "CH08": 0.38,
    "CH09": 0.44,
    "CH10": 0.50,
}

DISCOVERY = {
    "physical",
    "visual",
    "audio",
    "temporal",
    "dialogue",
    "documentary",
    "photographic",
    "environmental",
}
IMPORTANCE = {"critical", "major", "minor"}
CROSS_KINDS = {
    "coin",
    "hourglass",
    "phrase",
    "figure",
    "photograph",
    "newspaper",
    "painting",
    "location",
    "motif",
    "symbol",
}
CLUE_KEYS = [
    "ClueID",
    "ChapterID",
    "CardID",
    "Description",
    "Location",
    "DiscoveryMethod",
    "RelatedCharacters",
    "RelatedEvents",
    "RelatedClues",
    "MemoryReference",
    "Importance",
    "Optional",
]
CARD_KEYS = [
    "CardID",
    "ChapterID",
    "Scene",
    "Location",
    "Time",
    "Characters",
    "Objectives",
    "Clues",
    "Dialogue",
    "Choices",
    "Flashbacks",
    "Consequences",
    "Audio",
    "Camera",
    "NextCards",
]
DIALOGUE_KEYS = [
    "DialogueID",
    "Speaker",
    "Text",
    "Choices",
    "Requirements",
    "Consequences",
    "ClueUnlocks",
    "RelationshipChanges",
    "SoulChanges",
    "NextNode",
]
SAVE_PROPS = [
    "CurrentChapterId",
    "CurrentCardId",
    "ProtagonistId",
    "TimelineVisited",
    "TimelineUnlocked",
    "ChoiceByBranch",
    "CluesFound",
    "MemoriesFound",
    "Relationships",
    "CrossChapterDiscoveries",
    "Soul",
    "Dedication",
    "EndingsUnlocked",
    "UnlockedContent",
]
SOUL_FIELDS = [
    "Truth",
    "Empathy",
    "Courage",
    "Sacrifice",
    "Curiosity",
    "Honesty",
    "Compassion",
    "Understanding",
    "Dedication",
]
DEDICATION_FIELDS = [
    "CluesFound",
    "OptionalCluesFound",
    "FlashbacksDiscovered",
    "AlternateTimelines",
    "NPCsHelped",
    "OptionalDialogue",
    "HiddenObjects",
    "CrossChapterConnections",
]
FORBIDDEN_CH01 = [
    "grim reaper",
    "the void",
    "soul 11",
    "you died of",
    "heart attack",
    "cardiac",
]


def V(x, y, z):
    return {"X": x, "Y": y, "Z": z}


def cond(flag, equals):
    return {"Flag": flag, "Equals": equals}


def clue(**kwargs):
    data = {
        "ClueID": "",
        "ChapterID": "CH01",
        "CardID": "",
        "Title": "",
        "Description": "",
        "Location": "",
        "DiscoveryMethod": "physical",
        "RelatedCharacters": [],
        "RelatedEvents": [],
        "RelatedClues": [],
        "MemoryReference": "",
        "Importance": "major",
        "Optional": False,
        "Hidden": False,
        "BranchExclusive": False,
        "CrossChapter": False,
        "CrossTarget": "",
        "ActiveIf": [],
        "GrantFlag": "",
        "GrantValue": "",
        "Categories": ["Clues"],
        "SoulOnDiscover": {},
    }
    data.update(kwargs)
    return data


def node(did, speaker, text, choices=None, next_node="", clue_unlocks=None, soul=None, camera=""):
    return {
        "DialogueID": did,
        "Speaker": speaker,
        "Text": text,
        "Choices": choices or [],
        "Requirements": [],
        "Consequences": [],
        "ClueUnlocks": clue_unlocks or [],
        "RelationshipChanges": {},
        "SoulChanges": soul or {},
        "NextNode": next_node,
        "Camera": camera,
        "Reveal": False,
    }


def choice(cid, text, consequence, next_node="", branch="", requirements=None):
    return {
        "ChoiceID": cid,
        "Text": text,
        "BranchGroup": branch,
        "ConsequenceID": consequence,
        "NextNode": next_node,
        "Requirements": requirements or [],
    }


def consequence(cid, flags=None, soul=None, reveal=None, rel=None, helped=False, optional=False):
    return {
        "Id": cid,
        "SetFlags": flags or {},
        "Soul": soul or {},
        "RevealClues": reveal or [],
        "UnlockTimeline": [],
        "Relationship": rel or {},
        "HelpedNpc": helped,
        "OptionalDialogue": optional,
        "NotebookFact": "",
    }


def card(cid, title, scene, location, time, body, objectives, nxt=None, **kwargs):
    return {
        "CardID": cid,
        "ChapterID": kwargs.get("chapter", "CH01"),
        "Title": title,
        "Scene": scene,
        "Location": location,
        "Time": time,
        "Characters": kwargs.get("characters", ["CH_ADRIAN"]),
        "Objectives": objectives,
        "Clues": kwargs.get("clues", []),
        "Dialogue": kwargs.get("dialogue", []),
        "Choices": kwargs.get("choices", []),
        "Flashbacks": kwargs.get("flashbacks", []),
        "Consequences": kwargs.get("consequences", []),
        "Audio": kwargs.get("audio", ["rain", "city"]),
        "Camera": kwargs.get("camera", "OverShoulder"),
        "NextCards": [nxt] if nxt else [],
        "Body": body,
        "Variants": kwargs.get("variants", []),
    }


def interact(**kwargs):
    data = {
        "Id": "",
        "LocationId": "",
        "Kind": "Inspectable",
        "Prompt": "E \u2014 INSPECT",
        "Title": "",
        "InspectText": "",
        "ClueID": "",
        "DialogueID": "",
        "MemoryID": "",
        "TimelineEventID": "",
        "Position": V(0, 0, 0),
        "Scale": V(0.3, 0.3, 0.3),
        "Mesh": "Cube",
        "Color": "1A1C22",
        "PresentIf": [],
        "AbsentIf": [],
        "RequiredCard": "",
        "Hidden": False,
        "BlocksPlayer": False,
        "OnlyInFlashback": False,
        "FlashbackMemoryID": "",
        "TraceStyle": "",
        "Action": "",
    }
    data.update(kwargs)
    return data


def prop(mesh, pos, scale, color, blocks=True):
    return {"Mesh": mesh, "Position": pos, "Scale": scale, "Color": color, "BlocksPlayer": blocks}


def light(pos, color, intensity, radius):
    return {"Position": pos, "Color": color, "Intensity": intensity, "Radius": radius}


def write_json(path: Path, payload) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def build_characters():
    intros = {
        "CH_ADRIAN": "Chief executive of Vale Tower. The nameplate says so. Nothing on it says where the hour went.",
        "CH_MAYA": "A student. She was in the lobby before midnight with papers that needed a signature.",
        "CH_DANIEL": "A detective. His byline is on the evening paper in the boardroom.",
        "CH_ELENA": "A doctor. Her name is on a waiver in the desk.",
        "CH_LUCAS": "A photographer. His name is not in this office.",
        "CH_MARCO": "A chef. His name is not in this office.",
        "CH_SOFIA": "An athlete. Her name is not in this office.",
        "CH_NINA": "A public voice. Her name is not in this office.",
        "CH_ELIAS": "A scientist. His name is not in this office.",
        "CH_CLARA": "The name on the back of an unfinished canvas.",
        "CH_STRANGER": "A silhouette. No face. No name. He does not explain himself.",
    }
    voices = {
        "CH_ADRIAN": "Low, controlled, used to being the last person to speak.",
        "CH_MAYA": "Young, careful, out of breath when she is afraid of being late.",
        "CH_DANIEL": "Dry, observational, tired of lies that waste the night.",
        "CH_ELENA": "Precise, quiet, the voice of someone who has already given the warning.",
        "CH_LUCAS": "Light, already halfway out of the room.",
        "CH_MARCO": "Warm, crowded, talking over the kitchen.",
        "CH_SOFIA": "Short sentences. Breath kept for the next effort.",
        "CH_NINA": "Bright, performed, a half-second delay as if she is hearing herself.",
        "CH_ELIAS": "Measured. He corrects his own units.",
        "CH_CLARA": "Slow, certain about pictures and uncertain about dates.",
        "CH_STRANGER": "Deep. Calm. Restrained. Almost compassionate. Never cartoonishly evil.",
    }
    characters = []
    for cid, name, age, job, theme, _chapter in PROTAGONISTS:
        characters.append(
            {
                "Id": cid,
                "DisplayName": name,
                "Age": age,
                "Profession": job,
                "Theme": theme,
                "NotebookIntro": intros[cid],
                "VoiceNote": voices[cid],
            }
        )
    characters.append(
        {
            "Id": "CH_STRANGER",
            "DisplayName": "Stranger",
            "Age": 0,
            "Profession": "",
            "Theme": "",
            "NotebookIntro": intros["CH_STRANGER"],
            "VoiceNote": voices["CH_STRANGER"],
        }
    )
    return {"Characters": characters}


def build_chapter_one():
    c = "CH01"
    cards = []
    ids = [f"CH01_CARD_{i:02d}" for i in range(1, 13)]
    titles = [
        "THE AWAKENING",
        "THE ROOM",
        "THE DESK",
        "THE COIN",
        "THE MESSAGE",
        "THE CORRIDOR",
        "THE FLASHBACK",
        "THE BRANCH",
        "THE STRANGER",
        "THE HOURGLASS",
        "THE DOOR",
        "THE HOUR",
    ]
    bodies = [
        "The carpet is wet at the knees. The office has kept the rain out and the hour in. A clock on the shelf is not blinking. It is simply stopped.",
        "The room knows his name better than he does. Glass, a desk, a photograph that has lost a face.",
        "Papers for a night that was supposed to end in a signature. Some of them have already been handled. Some of them have not.",
        "Something small has been waiting under the blotter. It is not a key. He is the one who finds it.",
        "The phone has one message and the patience of a machine. The voice is young. The time is 23:12.",
        "The corridor is narrower than the decision that filled it. Water has been walked in and not walked back.",
        "Colour leaves, then a little of it returns. The lobby is earlier than this room. She is still there.",
        "23:15 can be visited. What he does with her papers will still be on the desk when the hour is late.",
        "A man with no face stands where the light fails. He speaks like a threat. He speaks like a warning.",
        "The boardroom is set for no one. The sand in the glass has almost finished agreeing with the clock.",
        "He was told not to open it. The bar is cold. The rain is on the other side, and so is whatever he wants.",
        "The clock moves once, to 23:55. The office keeps what the hour made of him.",
    ]
    variants_12 = [
        {
            "Id": "V_HELP_SHUT",
            "IfAll": [cond("MayaOutcome", "Helped"), cond("DoorOutcome", "Shut")],
            "Body": "The contract is stamped DELAYED. A student card lies in the ink. The roof door is still shut. The clock says 23:55, and the memo has nowhere to put his name.",
        },
        {
            "Id": "V_HELP_OPEN",
            "IfAll": [cond("MayaOutcome", "Helped"), cond("DoorOutcome", "Open")],
            "Body": "The contract is stamped DELAYED. The roof door has been opened onto the rain. A wet print crosses the memo. The clock says 23:55.",
        },
        {
            "Id": "V_IGNORE_SHUT",
            "IfAll": [cond("MayaOutcome", "Ignored"), cond("DoorOutcome", "Shut")],
            "Body": "The contract is stamped EXECUTED, 23:16. The phone holds a sentence that ends in blank paper. The roof door is shut. The clock says 23:55.",
        },
        {
            "Id": "V_IGNORE_OPEN",
            "IfAll": [cond("MayaOutcome", "Ignored"), cond("DoorOutcome", "Open")],
            "Body": "The contract is stamped EXECUTED, 23:16. The roof door stands open. One shoe is on the landing. The clock says 23:55, and the second glass was never filled.",
        },
    ]
    objectives = [
        ["inspect:INT_CLOCK"],
        ["discover:CLUE_PHOTO", "discover:CLUE_NAMEPLATE"],
        ["discover:CLUE_CONTRACT"],
        ["discover:CLUE_COIN"],
        ["dialogue:DLG_RESP_REPLAY|dialogue:DLG_RESP_SAVE"],
        ["discover:CLUE_UMBRELLA"],
        ["flashback:MEM_LOBBY", "discover:CLUE_STATEMENT"],
        ["commit:BRANCH_MAYA", "open:TIMELINE"],
        ["dialogue:DLG_RESP_ACCUSE|dialogue:DLG_RESP_HOW|dialogue:DLG_RESP_DOOR|dialogue:DLG_RESP_COIN"],
        ["discover:CLUE_HOURGLASS"],
        ["commit:BRANCH_DOOR"],
        ["discover:CLUE_FINAL_HS|discover:CLUE_FINAL_HO|discover:CLUE_FINAL_IS|discover:CLUE_FINAL_IO"],
    ]
    scenes = [
        "Office floor",
        "Office",
        "Desk",
        "Desk blotter",
        "Telephone",
        "Corridor",
        "Lobby, earlier",
        "Timeline",
        "Corridor end",
        "Boardroom",
        "Roof door",
        "Office, 23:55",
    ]
    locations = [
        "LOC_OFFICE",
        "LOC_OFFICE",
        "LOC_OFFICE",
        "LOC_OFFICE",
        "LOC_OFFICE",
        "LOC_CORRIDOR",
        "LOC_LOBBY",
        "LOC_CORRIDOR",
        "LOC_CORRIDOR",
        "LOC_BOARDROOM",
        "LOC_STAIR",
        "LOC_OFFICE",
    ]
    times = ["23:47", "23:47", "23:47", "23:47", "23:47", "23:47", "23:15", "23:15", "23:47", "23:47", "23:47", "23:55"]
    cameras = [
        "OverShoulder",
        "OverShoulder",
        "Close",
        "Close",
        "Close",
        "Corridor",
        "Flashback",
        "OverShoulder",
        "Conversation",
        "OverShoulder",
        "Low",
        "Close",
    ]
    for i in range(12):
        extra = {}
        if i == 6:
            extra = {
                "flashbacks": ["MEM_LOBBY"],
                "clues": ["CLUE_STATEMENT"],
                "characters": ["CH_ADRIAN", "CH_MAYA"],
                "audio": ["rain", "whispers"],
            }
        if i == 4:
            extra = {"dialogue": ["DLG_PHONE"], "audio": ["phones", "rain"]}
        if i == 7:
            extra = {"choices": ["CHOICE_HELP_MAYA", "CHOICE_IGNORE_MAYA"]}
        if i == 8:
            extra = {
                "dialogue": ["DLG_STRANGER"],
                "characters": ["CH_ADRIAN", "CH_STRANGER"],
                "audio": ["whispers", "rain"],
            }
        if i == 10:
            extra = {"choices": ["CHOICE_OPEN_DOOR", "CHOICE_LEAVE_DOOR"], "dialogue": ["DLG_DOOR"]}
        if i == 11:
            extra = {"variants": variants_12, "audio": ["clocks", "rain", "piano"]}
        cards.append(
            card(
                ids[i],
                titles[i],
                scenes[i],
                locations[i],
                times[i],
                bodies[i],
                objectives[i],
                ids[i + 1] if i < 11 else None,
                camera=cameras[i],
                **extra,
            )
        )

    clues = [
        clue(
            ClueID="CLUE_CLOCK",
            CardID="CH01_CARD_01",
            Title="Stopped clock",
            Description="The shelf clock reads 23:47. The second hand is not trying. Rain ticks louder than it does.",
            Location="LOC_OFFICE",
            DiscoveryMethod="temporal",
            RelatedEvents=["TL_2347"],
            Importance="critical",
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_PHOTO",
            CardID="CH01_CARD_02",
            Title="Photograph without a face",
            Description="A framed photograph on the north wall. The room, the suit, the rain in the glass are still there. The face has been eaten down to a pale blank.",
            Location="LOC_OFFICE",
            DiscoveryMethod="photographic",
            Importance="major",
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_NAMEPLATE",
            CardID="CH01_CARD_02",
            Title="Nameplate",
            Description="ADRIAN VALE. CHIEF EXECUTIVE. The letters are clean. A second line under the title has been scraped until the metal shows.",
            Location="LOC_OFFICE",
            DiscoveryMethod="physical",
            RelatedCharacters=["CH_ADRIAN"],
            Importance="major",
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_RAIN",
            CardID="CH01_CARD_02",
            Title="Rain on the glass",
            Description="The city is a black strip under the window. A few signs keep a little colour. The rest of the night has given it up.",
            Location="LOC_OFFICE",
            DiscoveryMethod="environmental",
            Importance="minor",
            Optional=True,
            SoulOnDiscover={"Curiosity": 1},
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_CONTRACT",
            CardID="CH01_CARD_03",
            Title="Northline contract",
            Description="A consolidation agreement for Northline Transit. The signature line is still open. A time has been pencilled in the margin: before midnight.",
            Location="LOC_OFFICE",
            DiscoveryMethod="documentary",
            RelatedClues=["CLUE_STATEMENT"],
            Importance="critical",
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_WAIVER",
            CardID="CH01_CARD_03",
            Title="Waiver",
            Description="A medical waiver clipped behind the contract. Dr. Elena Moretti advises the patient named on the first line against strain after dusk. The second name has been scraped away.",
            Location="LOC_OFFICE",
            DiscoveryMethod="documentary",
            RelatedCharacters=["CH_ELENA", "CH_ADRIAN"],
            RelatedClues=["CLUE_CONTRACT"],
            Importance="major",
            Optional=True,
            Hidden=True,
            CrossChapter=True,
            CrossTarget="CH04",
            SoulOnDiscover={"Curiosity": 1},
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_TORN_CARD",
            CardID="CH01_CARD_03",
            Title="Torn card",
            Description="A story card torn through the title. The visible print reads CARD 0 and then nothing. The rest of the line is a blank bite.",
            Location="LOC_OFFICE",
            DiscoveryMethod="physical",
            Importance="minor",
            Optional=True,
            Hidden=True,
            SoulOnDiscover={"Curiosity": 1},
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_MISSING_NAME",
            CardID="CH01_CARD_03",
            Title="Plaque with no name",
            Description="A small plaque by the door. The screw holes are there. The name is not. Someone took the time to lift the letters and not replace them.",
            Location="LOC_OFFICE",
            DiscoveryMethod="visual",
            Importance="minor",
            Optional=True,
            Hidden=True,
            SoulOnDiscover={"Curiosity": 1},
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_COIN",
            CardID="CH01_CARD_04",
            Title="The coin",
            Description="A coin under the blotter. One face is blank. The other is worn into the shape of an hourglass. Nobody put it in his hand. He found it.",
            Location="LOC_OFFICE",
            DiscoveryMethod="physical",
            RelatedClues=["CLUE_HOURGLASS"],
            RelatedEvents=["TL_2330"],
            Importance="critical",
            CrossChapter=True,
            CrossTarget="ALL",
            GrantFlag="HasCoin",
            GrantValue="Found",
            SoulOnDiscover={"Curiosity": 1},
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_VOICEMAIL",
            CardID="CH01_CARD_05",
            Title="Voicemail, 23:12",
            Description="Maya's voice, careful, out of breath. She is in the lobby. She says the papers have to be signed before midnight or the page goes blank.",
            Location="LOC_OFFICE",
            DiscoveryMethod="audio",
            RelatedCharacters=["CH_MAYA"],
            RelatedEvents=["TL_2312"],
            Importance="critical",
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_BREATH",
            CardID="CH01_CARD_05",
            Title="Second voice",
            Description="Under the last three seconds of the voicemail, rain, and a second voice, too low to belong to Maya. It says she does not have much time. It does not say what to do.",
            Location="LOC_OFFICE",
            DiscoveryMethod="audio",
            RelatedCharacters=["CH_MAYA", "CH_STRANGER"],
            RelatedClues=["CLUE_VOICEMAIL", "CLUE_SILHOUETTE"],
            MemoryReference="",
            Importance="minor",
            Optional=True,
            SoulOnDiscover={"Curiosity": 1},
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_COAT",
            CardID="CH01_CARD_02",
            Title="Wet coat",
            Description="The coat on the chair is still wet at the shoulders. A lobby receipt in the pocket is timed 23:00.",
            Location="LOC_OFFICE",
            DiscoveryMethod="physical",
            RelatedEvents=["TL_2300"],
            Importance="minor",
            Optional=True,
            SoulOnDiscover={"Curiosity": 1},
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_UMBRELLA",
            CardID="CH01_CARD_06",
            Title="Umbrella",
            Description="A student umbrella, still open enough to drip. The water points back toward the elevator, not toward the office.",
            Location="LOC_CORRIDOR",
            DiscoveryMethod="physical",
            RelatedCharacters=["CH_MAYA"],
            Importance="major",
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_STATEMENT",
            CardID="CH01_CARD_07",
            Title="Witness statement",
            Description="The pages are a witness statement about Northline's night routes. Maya says they have to be signed before midnight or the text disappears. She did not write them.",
            Location="LOC_LOBBY",
            DiscoveryMethod="documentary",
            RelatedCharacters=["CH_MAYA", "CH_ADRIAN"],
            RelatedEvents=["TL_2315"],
            RelatedClues=["CLUE_CONTRACT", "CLUE_VOICEMAIL"],
            MemoryReference="MEM_LOBBY",
            Importance="critical",
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_SILHOUETTE",
            CardID="CH01_CARD_07",
            Title="The warning in the lobby",
            Description="Maya says a silhouette told her she did not have much time, and that she should not trust the quiet. She did not see a face. She still needed a signature.",
            Location="LOC_LOBBY",
            DiscoveryMethod="dialogue",
            RelatedCharacters=["CH_MAYA", "CH_STRANGER"],
            RelatedClues=["CLUE_BREATH"],
            MemoryReference="MEM_LOBBY",
            Importance="major",
            Optional=True,
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_HOURGLASS",
            CardID="CH01_CARD_10",
            Title="The hourglass",
            Description="An hourglass on the boardroom table, sand almost gone. No meeting is using it. It is simply running out.",
            Location="LOC_BOARDROOM",
            DiscoveryMethod="physical",
            RelatedClues=["CLUE_COIN"],
            Importance="major",
            CrossChapter=True,
            CrossTarget="ALL",
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_NEWSPAPER",
            CardID="CH01_CARD_10",
            Title="Evening paper",
            Description="The evening paper, folded to a column by Daniel Cross. The piece is about a man who missed the last train. One name in the second paragraph is a black bar.",
            Location="LOC_BOARDROOM",
            DiscoveryMethod="documentary",
            RelatedCharacters=["CH_DANIEL"],
            Importance="minor",
            Optional=True,
            CrossChapter=True,
            CrossTarget="CH03",
            SoulOnDiscover={"Curiosity": 1},
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_PAINTING",
            CardID="CH01_CARD_10",
            Title="Unfinished canvas",
            Description="A canvas leans on the boardroom wall. Rain, a tower, a small coin in the corner that the paint has not finished. On the back, a name: Clara. The date is scratched out.",
            Location="LOC_BOARDROOM",
            DiscoveryMethod="visual",
            RelatedCharacters=["CH_CLARA"],
            RelatedClues=["CLUE_COIN"],
            Importance="minor",
            Optional=True,
            CrossChapter=True,
            CrossTarget="CH10",
            SoulOnDiscover={"Curiosity": 1},
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_DOOR_TOUCH",
            CardID="CH01_CARD_11",
            Title="Cold bar",
            Description="The roof-door bar is cold. Rain presses the seam from the other side. Nothing on the door says what the landing is for.",
            Location="LOC_STAIR",
            DiscoveryMethod="physical",
            RelatedEvents=["TL_2341"],
            Importance="major",
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_SHOE",
            CardID="CH01_CARD_11",
            Title="One shoe",
            Description="One black shoe on the wet landing. The railing bolt beside it is sheared. The rain has been coming in longer than the shoe has been here.",
            Location="LOC_STAIR",
            DiscoveryMethod="physical",
            RelatedEvents=["TL_2341"],
            Importance="major",
            BranchExclusive=True,
            ActiveIf=[cond("DoorOutcome", "Open")],
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_UNOPENED",
            CardID="CH01_CARD_11",
            Title="Untouched bar",
            Description="The bar is still latched. A dry oval on the metal shows where a hand almost settled and then did not.",
            Location="LOC_STAIR",
            DiscoveryMethod="visual",
            RelatedEvents=["TL_2341"],
            Importance="major",
            BranchExclusive=True,
            ActiveIf=[cond("DoorOutcome", "Shut")],
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_FINAL_HS",
            CardID="CH01_CARD_12",
            Title="Delayed stamp",
            Description="The contract is stamped DELAYED. A student card lies in the ink, Maya's, still warm. The memo under it has a blank where a name should be printed in the morning.",
            Location="LOC_OFFICE",
            DiscoveryMethod="documentary",
            RelatedCharacters=["CH_MAYA", "CH_ADRIAN"],
            RelatedEvents=["TL_2355"],
            RelatedClues=["CLUE_CONTRACT", "CLUE_STATEMENT"],
            Importance="critical",
            BranchExclusive=True,
            ActiveIf=[cond("MayaOutcome", "Helped"), cond("DoorOutcome", "Shut")],
            Categories=["Clues", "Objects"],
        ),
        clue(
            ClueID="CLUE_FINAL_HO",
            CardID="CH01_CARD_12",
            Title="Delayed, and the door",
            Description="The contract is stamped DELAYED. A wet print crosses the memo from the direction of the roof door. The student card is in the ink. The landing was still visited.",
            Location="LOC_OFFICE",
            DiscoveryMethod="documentary",
            RelatedCharacters=["CH_MAYA"],
            RelatedEvents=["TL_2355"],
            RelatedClues=["CLUE_SHOE", "CLUE_CONTRACT"],
            Importance="critical",
            BranchExclusive=True,
            ActiveIf=[cond("MayaOutcome", "Helped"), cond("DoorOutcome", "Open")],
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_FINAL_IS",
            CardID="CH01_CARD_12",
            Title="Executed stamp",
            Description="The contract is stamped EXECUTED, 23:16. The phone shows a message that ends mid-word. The rest of the line is blank paper. The roof door is shut.",
            Location="LOC_OFFICE",
            DiscoveryMethod="documentary",
            RelatedEvents=["TL_2355"],
            RelatedClues=["CLUE_VOICEMAIL", "CLUE_CONTRACT"],
            Importance="critical",
            BranchExclusive=True,
            ActiveIf=[cond("MayaOutcome", "Ignored"), cond("DoorOutcome", "Shut")],
            Categories=["Clues"],
        ),
        clue(
            ClueID="CLUE_FINAL_IO",
            CardID="CH01_CARD_12",
            Title="Executed, and the landing",
            Description="The contract is stamped EXECUTED, 23:16. The second glass on the desk was never filled. The roof door was opened. The phone's last sentence stops in a blank.",
            Location="LOC_OFFICE",
            DiscoveryMethod="documentary",
            RelatedEvents=["TL_2355"],
            RelatedClues=["CLUE_SHOE", "CLUE_CONTRACT"],
            Importance="critical",
            BranchExclusive=True,
            ActiveIf=[cond("MayaOutcome", "Ignored"), cond("DoorOutcome", "Open")],
            Categories=["Clues"],
        ),
    ]

    dialogues = [
        node(
            "DLG_PHONE",
            "CH_MAYA",
            "A voicemail, 23:12. A young woman's voice, careful, out of breath. \"Mr. Vale, it's Maya. I'm in the lobby. If the papers aren't signed by midnight they said the page goes blank. Please.\"",
            choices=[
                choice("CHOICE_REPLAY", "Play the last three seconds again.", "CONS_REPLAY", "DLG_RESP_REPLAY"),
                choice("CHOICE_SAVE_MSG", "Save the message and put the phone down.", "CONS_SAVE_MSG", "DLG_RESP_SAVE"),
            ],
            clue_unlocks=["CLUE_VOICEMAIL"],
            camera="Close",
        ),
        node(
            "DLG_RESP_REPLAY",
            "CH_MAYA",
            "The last three seconds are mostly rain. Under it, a second voice, almost kind, tells her she does not have much time.",
            camera="Close",
        ),
        node(
            "DLG_RESP_SAVE",
            "CH_ADRIAN",
            "He saves it. The phone goes dark. The lobby does not.",
            camera="Close",
        ),
        node(
            "DLG_MAYA",
            "CH_MAYA",
            "Mr. Vale? I need you to sign this before midnight. They said the papers vanish at twelve. I came straight up. I wasn't sure you'd still be here.",
            choices=[
                choice("CHOICE_HELP_MAYA", "Show me where to sign.", "CONS_HELP", "DLG_RESP_HELP", "BRANCH_MAYA"),
                choice("CHOICE_IGNORE_MAYA", "The deal is tonight. Take them back down.", "CONS_IGNORE", "DLG_RESP_IGNORE", "BRANCH_MAYA"),
                choice("CHOICE_ASK_MAYA", "Who told you that you don't have much time?", "CONS_ASK", "DLG_MAYA_DECIDE"),
            ],
            camera="Conversation",
        ),
        node(
            "DLG_MAYA_DECIDE",
            "CH_MAYA",
            "I didn't see a face. He stood where the light stops. He said I didn't have much time, and that I shouldn't trust the quiet. Will you sign them?",
            choices=[
                choice("CHOICE_HELP_MAYA_B", "Show me where to sign.", "CONS_HELP", "DLG_RESP_HELP", "BRANCH_MAYA"),
                choice("CHOICE_IGNORE_MAYA_B", "The deal is tonight. Take them back down.", "CONS_IGNORE", "DLG_RESP_IGNORE", "BRANCH_MAYA"),
            ],
            camera="Conversation",
        ),
        node(
            "DLG_RESP_HELP",
            "CH_MAYA",
            "Then here. Before the ink decides to leave. I'll wait while you read. I can wait.",
            camera="Conversation",
        ),
        node(
            "DLG_RESP_IGNORE",
            "CH_MAYA",
            "Then I'll take them back down. I hope the lobby still has a door.",
            camera="Conversation",
        ),
        node(
            "DLG_STRANGER",
            "CH_STRANGER",
            "You don't have much time.",
            choices=[
                choice("CHOICE_ACCUSE", "Are you threatening me?", "CONS_ACCUSE", "DLG_RESP_ACCUSE"),
                choice("CHOICE_ASK_DEATH", "Then tell me how I died.", "CONS_ASK_DEATH", "DLG_RESP_HOW"),
                choice("CHOICE_WHICH_DOOR", "What door?", "CONS_WHICH_DOOR", "DLG_RESP_DOOR"),
                choice(
                    "CHOICE_ASK_COIN",
                    "What is this coin?",
                    "CONS_ASK_COIN",
                    "DLG_RESP_COIN",
                    requirements=[cond("HasCoin", "Found")],
                ),
            ],
            camera="Conversation",
        ),
        node("DLG_RESP_ACCUSE", "CH_STRANGER", "If a warning frightens you, keep it.", camera="Conversation"),
        node(
            "DLG_RESP_HOW",
            "CH_STRANGER",
            "That is the wrong question. You already walked past the answer.",
            camera="Close",
        ),
        node(
            "DLG_RESP_DOOR",
            "CH_STRANGER",
            "You shouldn't open that door. You will know it by how badly you want it open.",
            camera="Conversation",
        ),
        node("DLG_RESP_COIN", "CH_STRANGER", "You found it. Don't ask me what it wants.", camera="Close"),
        node(
            "DLG_DOOR",
            "CH_ADRIAN",
            "The roof door. The bar is cold. Rain pushes at the seam.",
            choices=[
                choice("CHOICE_OPEN_DOOR", "Open it.", "CONS_OPEN_DOOR", "DLG_RESP_OPEN", "BRANCH_DOOR"),
                choice("CHOICE_LEAVE_DOOR", "Step back.", "CONS_LEAVE_DOOR", "DLG_RESP_SHUT", "BRANCH_DOOR"),
            ],
            clue_unlocks=["CLUE_DOOR_TOUCH"],
            camera="Low",
        ),
        node("DLG_RESP_OPEN", "CH_ADRIAN", "The latch gives. Rain comes in and does not explain itself.", camera="Low"),
        node("DLG_RESP_SHUT", "CH_ADRIAN", "He takes his hand off the bar. The dry mark of it stays.", camera="Low"),
    ]

    consequences = [
        consequence("CONS_REPLAY", {"HeardVoicemail": "Replayed"}, {"Curiosity": 1}, ["CLUE_BREATH"], optional=True),
        consequence("CONS_SAVE_MSG", {"HeardVoicemail": "Saved"}, {"Honesty": 1}),
        consequence(
            "CONS_HELP",
            {"MayaOutcome": "Helped"},
            {"Empathy": 2, "Sacrifice": 1, "Compassion": 1},
            rel={"CH_MAYA": 2},
            helped=True,
        ),
        consequence("CONS_IGNORE", {"MayaOutcome": "Ignored"}, {"Empathy": -2}, rel={"CH_MAYA": -2}),
        consequence("CONS_ASK", {}, {"Curiosity": 1}, ["CLUE_SILHOUETTE"], optional=True),
        consequence("CONS_ACCUSE", {"StrangerTone": "Accused"}, {"Courage": 1}, rel={"CH_STRANGER": -1}),
        consequence("CONS_ASK_DEATH", {"StrangerTone": "Asked"}, {"Curiosity": 1, "Honesty": 1}),
        consequence("CONS_WHICH_DOOR", {"StrangerTone": "Warned"}, {"Understanding": 1}),
        consequence("CONS_ASK_COIN", {"StrangerTone": "AskedCoin"}, {"Curiosity": 1, "Understanding": 1}),
        consequence("CONS_OPEN_DOOR", {"DoorOutcome": "Open"}, {"Courage": 1}, ["CLUE_SHOE"]),
        consequence("CONS_LEAVE_DOOR", {"DoorOutcome": "Shut"}, {"Understanding": 1}, ["CLUE_UNOPENED"]),
    ]

    memory = {
        "MemoryID": "MEM_LOBBY",
        "ChapterID": c,
        "CardID": "CH01_CARD_07",
        "QuestionAnswered": "At 23:15 Maya was in the lobby with a witness statement about Northline, and she needed a signature before midnight.",
        "QuestionOpened": "A silhouette told her she did not have much time. The statement does not say what happens if the door upstairs is opened.",
        "RequiredClue": "CLUE_STATEMENT",
        "TimelineEventID": "TL_2315",
        "EntryLocationId": "LOC_LOBBY",
        "EntryPosition": V(420, 3900, 110),
        "HoldLine": "The memory is still holding something.",
        "Playable": True,
    }

    timeline = [
        {
            "EventID": "TL_2300",
            "ChapterID": c,
            "Time": "23:00",
            "Title": "The lobby receipt",
            "LocationId": "LOC_OFFICE",
            "Position": V(1100, 420, 110),
            "StartsLocked": True,
            "Branches": [],
            "UnlockOnClue": "CLUE_COAT",
        },
        {
            "EventID": "TL_2312",
            "ChapterID": c,
            "Time": "23:12",
            "Title": "Maya's voicemail",
            "LocationId": "LOC_OFFICE",
            "Position": V(900, 700, 110),
            "StartsLocked": True,
            "Branches": [],
            "UnlockOnClue": "CLUE_VOICEMAIL",
        },
        {
            "EventID": "TL_2315",
            "ChapterID": c,
            "Time": "23:15",
            "Title": "The lobby",
            "LocationId": "LOC_LOBBY",
            "Position": V(420, 3900, 110),
            "StartsLocked": True,
            "Branches": [
                {"ChoiceID": "CHOICE_HELP_MAYA", "Label": "HELP MAYA"},
                {"ChoiceID": "CHOICE_IGNORE_MAYA", "Label": "IGNORE MAYA"},
            ],
            "UnlockOnClue": "CLUE_STATEMENT",
        },
        {
            "EventID": "TL_2330",
            "ChapterID": c,
            "Time": "23:30",
            "Title": "The coin",
            "LocationId": "LOC_OFFICE",
            "Position": V(700, 740, 110),
            "StartsLocked": True,
            "Branches": [],
            "UnlockOnClue": "CLUE_COIN",
        },
        {
            "EventID": "TL_2341",
            "ChapterID": c,
            "Time": "23:41",
            "Title": "The roof door",
            "LocationId": "LOC_STAIR",
            "Position": V(4000, 700, 110),
            "StartsLocked": True,
            "Branches": [
                {"ChoiceID": "CHOICE_OPEN_DOOR", "Label": "OPEN THE DOOR"},
                {"ChoiceID": "CHOICE_LEAVE_DOOR", "Label": "LEAVE IT SHUT"},
            ],
            "UnlockOnClue": "CLUE_DOOR_TOUCH",
        },
        {
            "EventID": "TL_2347",
            "ChapterID": c,
            "Time": "23:47",
            "Title": "The awakening",
            "LocationId": "LOC_OFFICE",
            "Position": V(380, 700, 110),
            "StartsLocked": False,
            "Branches": [],
            "UnlockOnClue": "",
        },
        {
            "EventID": "TL_2355",
            "ChapterID": c,
            "Time": "23:55",
            "Title": "The hour",
            "LocationId": "LOC_OFFICE",
            "Position": V(740, 780, 110),
            "StartsLocked": True,
            "Branches": [],
            "UnlockOnClue": "",
        },
    ]

    def final_obj(iid, clue_id, title, text, flags):
        return interact(
            Id=iid,
            LocationId="LOC_OFFICE",
            Kind="Clue",
            Prompt="E \u2014 INSPECT",
            Title=title,
            InspectText=text,
            ClueID=clue_id,
            Position=V(760, 800, 92),
            Scale=V(0.42, 0.28, 0.04),
            Mesh="Cube",
            Color="C4B8A4",
            PresentIf=[cond(a, b) for a, b in flags],
            RequiredCard="CH01_CARD_12",
        )

    interactables = [
        interact(
            Id="INT_CLOCK",
            LocationId="LOC_OFFICE",
            Kind="Inspectable",
            Title="Clock",
            InspectText="23:47. The second hand has stopped mid-accusation.",
            ClueID="CLUE_CLOCK",
            Position=V(520, 1180, 150),
            Scale=V(0.16, 0.16, 0.28),
            Mesh="Cylinder",
            Color="2A3038",
            RequiredCard="CH01_CARD_01",
        ),
        interact(
            Id="INT_PHOTO",
            LocationId="LOC_OFFICE",
            Kind="VoidTrace",
            Title="Photograph",
            InspectText="The face is gone. The suit is still standing in the rain.",
            ClueID="CLUE_PHOTO",
            Position=V(280, 1320, 190),
            Scale=V(0.55, 0.04, 0.4),
            Mesh="Cube",
            Color="6A645C",
            RequiredCard="CH01_CARD_02",
            TraceStyle="Photo",
        ),
        interact(
            Id="INT_NAMEPLATE",
            LocationId="LOC_OFFICE",
            Kind="Inspectable",
            Title="Nameplate",
            InspectText="ADRIAN VALE. CHIEF EXECUTIVE. The line under it has been scraped clean.",
            ClueID="CLUE_NAMEPLATE",
            Position=V(700, 860, 88),
            Scale=V(0.46, 0.12, 0.06),
            Mesh="Cube",
            Color="8A8478",
            RequiredCard="CH01_CARD_02",
        ),
        interact(
            Id="INT_WINDOW",
            LocationId="LOC_OFFICE",
            Kind="Inspectable",
            Title="Window",
            InspectText="Rain on the glass. The city keeps a little colour and no explanations.",
            ClueID="CLUE_RAIN",
            Position=V(800, 80, 180),
            Scale=V(1.4, 0.04, 0.9),
            Mesh="Cube",
            Color="142028",
            RequiredCard="CH01_CARD_02",
        ),
        interact(
            Id="INT_CONTRACT",
            LocationId="LOC_OFFICE",
            Kind="Clue",
            Title="Contract",
            InspectText="Northline Transit. The signature line is open. Midnight is written in the margin like a dare.",
            ClueID="CLUE_CONTRACT",
            Position=V(820, 760, 88),
            Scale=V(0.38, 0.26, 0.03),
            Mesh="Cube",
            Color="D9D3C7",
            RequiredCard="CH01_CARD_03",
        ),
        interact(
            Id="INT_WAIVER",
            LocationId="LOC_OFFICE",
            Kind="Clue",
            Prompt="E \u2014 INSPECT",
            Title="Waiver",
            InspectText="Elena Moretti's waiver. Strain after dusk. A second name, removed.",
            ClueID="CLUE_WAIVER",
            Position=V(900, 820, 86),
            Scale=V(0.22, 0.16, 0.02),
            Mesh="Cube",
            Color="E6E0D4",
            RequiredCard="CH01_CARD_03",
            Hidden=True,
        ),
        interact(
            Id="INT_TORN_CARD",
            LocationId="LOC_OFFICE",
            Kind="VoidTrace",
            Title="Torn card",
            InspectText="CARD 0 — and then a tear where the title should be.",
            ClueID="CLUE_TORN_CARD",
            Position=V(620, 690, 84),
            Scale=V(0.18, 0.12, 0.02),
            Mesh="Cube",
            Color="1B1A18",
            RequiredCard="CH01_CARD_03",
            Hidden=True,
            TraceStyle="TornCard",
        ),
        interact(
            Id="INT_MISSING_NAME",
            LocationId="LOC_OFFICE",
            Kind="VoidTrace",
            Title="Plaque",
            InspectText="The letters were lifted out. The name did not grow back.",
            ClueID="CLUE_MISSING_NAME",
            Position=V(180, 240, 160),
            Scale=V(0.34, 0.06, 0.16),
            Mesh="Cube",
            Color="3A3E44",
            RequiredCard="CH01_CARD_03",
            Hidden=True,
            TraceStyle="MissingName",
        ),
        interact(
            Id="INT_COIN",
            LocationId="LOC_OFFICE",
            Kind="StoryObject",
            Prompt="E \u2014 TAKE",
            Title="Coin",
            InspectText="Blank on one face. An hourglass worn into the other. He is the one who finds it.",
            ClueID="CLUE_COIN",
            Position=V(680, 730, 90),
            Scale=V(0.1, 0.1, 0.025),
            Mesh="Cylinder",
            Color="C4A574",
            RequiredCard="CH01_CARD_04",
        ),
        interact(
            Id="INT_PHONE",
            LocationId="LOC_OFFICE",
            Kind="DialogueTrigger",
            Prompt="E \u2014 LISTEN",
            Title="Phone",
            InspectText="One message. 23:12.",
            DialogueID="DLG_PHONE",
            Position=V(960, 700, 88),
            Scale=V(0.16, 0.08, 0.04),
            Mesh="Cube",
            Color="22262C",
            RequiredCard="CH01_CARD_05",
        ),
        interact(
            Id="INT_COAT",
            LocationId="LOC_OFFICE",
            Kind="Clue",
            Title="Coat",
            InspectText="Wet at the shoulders. A lobby receipt timed 23:00.",
            ClueID="CLUE_COAT",
            Position=V(1180, 420, 70),
            Scale=V(0.35, 0.28, 0.7),
            Mesh="Cube",
            Color="1A1C20",
            RequiredCard="CH01_CARD_02",
        ),
        interact(
            Id="INT_UMBRELLA",
            LocationId="LOC_CORRIDOR",
            Kind="MemoryTrigger",
            Prompt="E \u2014 REMEMBER",
            Title="Umbrella",
            InspectText="Maya's umbrella, still dripping toward the elevator.",
            ClueID="CLUE_UMBRELLA",
            MemoryID="MEM_LOBBY",
            Position=V(2100, 700, 40),
            Scale=V(0.12, 0.12, 0.7),
            Mesh="Cylinder",
            Color="1A3A3A",
            RequiredCard="CH01_CARD_06",
        ),
        interact(
            Id="INT_STRANGER",
            LocationId="LOC_CORRIDOR",
            Kind="GrimReaper",
            Prompt="E \u2014 SPEAK",
            Title="Stranger",
            InspectText="A silhouette. No face to inspect.",
            DialogueID="DLG_STRANGER",
            Position=V(3400, 700, 120),
            Scale=V(0.45, 0.28, 1.85),
            Mesh="Cube",
            Color="050506",
            RequiredCard="CH01_CARD_09",
        ),
        interact(
            Id="INT_HOURGLASS",
            LocationId="LOC_BOARDROOM",
            Kind="Inspectable",
            Title="Hourglass",
            InspectText="Sand, almost finished. Nobody called a meeting.",
            ClueID="CLUE_HOURGLASS",
            Position=V(2700, 1280, 96),
            Scale=V(0.12, 0.12, 0.28),
            Mesh="Cylinder",
            Color="8A8378",
            RequiredCard="CH01_CARD_10",
        ),
        interact(
            Id="INT_NEWSPAPER",
            LocationId="LOC_BOARDROOM",
            Kind="Clue",
            Title="Newspaper",
            InspectText="Daniel Cross. A man who missed the last train. A name barred out in black.",
            ClueID="CLUE_NEWSPAPER",
            Position=V(2860, 1240, 90),
            Scale=V(0.32, 0.22, 0.03),
            Mesh="Cube",
            Color="C8C2B6",
            RequiredCard="CH01_CARD_10",
        ),
        interact(
            Id="INT_PAINTING",
            LocationId="LOC_BOARDROOM",
            Kind="Clue",
            Title="Canvas",
            InspectText="Rain, the tower, a coin not finished. Clara, on the back. The date is gone.",
            ClueID="CLUE_PAINTING",
            Position=V(2500, 1680, 190),
            Scale=V(0.7, 0.05, 0.9),
            Mesh="Cube",
            Color="2C3338",
            RequiredCard="CH01_CARD_10",
        ),
        interact(
            Id="INT_DOOR",
            LocationId="LOC_STAIR",
            Kind="DialogueTrigger",
            Prompt="E \u2014 OPEN",
            Title="Roof door",
            InspectText="Cold metal. Rain on the far side.",
            DialogueID="DLG_DOOR",
            Position=V(4120, 700, 130),
            Scale=V(0.16, 1.1, 2.1),
            Mesh="Cube",
            Color="121418",
            RequiredCard="CH01_CARD_11",
            BlocksPlayer=True,
        ),
        interact(
            Id="INT_SHOE",
            LocationId="LOC_STAIR",
            Kind="Clue",
            Title="Shoe",
            InspectText="One shoe. A sheared bolt. Rain older than the shoe.",
            ClueID="CLUE_SHOE",
            Position=V(4380, 760, 20),
            Scale=V(0.22, 0.1, 0.08),
            Mesh="Cube",
            Color="0C0C0E",
            PresentIf=[cond("DoorOutcome", "Open")],
            RequiredCard="CH01_CARD_11",
        ),
        interact(
            Id="INT_UNOPENED",
            LocationId="LOC_STAIR",
            Kind="Clue",
            Title="Latch",
            InspectText="Still latched. The dry oval of a hand that changed its mind.",
            ClueID="CLUE_UNOPENED",
            Position=V(4080, 700, 120),
            Scale=V(0.2, 0.08, 0.08),
            Mesh="Cube",
            Color="5A5348",
            PresentIf=[cond("DoorOutcome", "Shut")],
            RequiredCard="CH01_CARD_11",
        ),
        interact(
            Id="INT_MAYA",
            LocationId="LOC_LOBBY",
            Kind="CharacterInteraction",
            Prompt="E \u2014 SPEAK",
            Title="Maya",
            InspectText="A student with a wet coat and a folder she will not put down.",
            DialogueID="DLG_MAYA",
            Position=V(980, 4000, 110),
            Scale=V(0.42, 0.28, 1.65),
            Mesh="Cylinder",
            Color="243038",
            OnlyInFlashback=True,
            FlashbackMemoryID="MEM_LOBBY",
        ),
        interact(
            Id="INT_STATEMENT",
            LocationId="LOC_LOBBY",
            Kind="Clue",
            Prompt="E \u2014 READ",
            Title="Statement",
            InspectText="Northline's night routes. A signature line. Midnight.",
            ClueID="CLUE_STATEMENT",
            Position=V(1080, 3960, 90),
            Scale=V(0.28, 0.2, 0.03),
            Mesh="Cube",
            Color="E4DCCE",
            OnlyInFlashback=True,
            FlashbackMemoryID="MEM_LOBBY",
        ),
        interact(
            Id="INT_RETURN",
            LocationId="LOC_LOBBY",
            Kind="StoryObject",
            Prompt="F \u2014 RETURN",
            Title="Return",
            InspectText="The memory can be left once it has given up the page.",
            Position=V(420, 3500, 110),
            Scale=V(0.4, 0.4, 1.2),
            Mesh="Cube",
            Color="10141A",
            OnlyInFlashback=True,
            FlashbackMemoryID="MEM_LOBBY",
            Action="ReturnMemory",
        ),
        final_obj(
            "INT_FINAL_HS",
            "CLUE_FINAL_HS",
            "Desk, delayed",
            "DELAYED. A student card in the ink. A blank where the morning memo wants a name.",
            [("MayaOutcome", "Helped"), ("DoorOutcome", "Shut")],
        ),
        final_obj(
            "INT_FINAL_HO",
            "CLUE_FINAL_HO",
            "Desk, delayed",
            "DELAYED. A wet print from the roof door crosses the memo.",
            [("MayaOutcome", "Helped"), ("DoorOutcome", "Open")],
        ),
        final_obj(
            "INT_FINAL_IS",
            "CLUE_FINAL_IS",
            "Desk, executed",
            "EXECUTED, 23:16. The phone's last sentence ends in blank paper.",
            [("MayaOutcome", "Ignored"), ("DoorOutcome", "Shut")],
        ),
        final_obj(
            "INT_FINAL_IO",
            "CLUE_FINAL_IO",
            "Desk, executed",
            "EXECUTED, 23:16. The second glass was never filled. The door was.",
            [("MayaOutcome", "Ignored"), ("DoorOutcome", "Open")],
        ),
    ]

    locations_out = [
        {
            "LocationId": "LOC_OFFICE",
            "Name": "Vale Tower — office",
            "Description": "A corner office above a rain-soaked city. The carpet remembers someone's knees.",
            "Origin": V(0, 0, 0),
            "SizeX": 1600,
            "SizeY": 1400,
            "Height": 420,
            "Corridor": False,
            "OpenEast": True,
            "OpenWest": False,
            "OpenNorth": False,
            "OpenSouth": False,
            "Spawn": V(380, 700, 120),
            "SpawnYaw": 0,
            "Props": [
                prop("Cube", V(760, 780, 40), V(1.8, 0.9, 0.8), "14161C", True),
                prop("Cube", V(1180, 400, 45), V(0.5, 0.5, 0.9), "101216", True),
                prop("Cube", V(500, 1240, 110), V(1.6, 0.35, 0.9), "12141A", True),
                prop("Cube", V(200, -80, 160), V(2.2, 0.4, 3.2), "07080C", False),
                prop("Cube", V(700, -160, 200), V(1.2, 0.4, 4.2), "05060A", False),
                prop("Cube", V(1200, -40, 120), V(1.4, 0.3, 2.2), "0A1014", False),
            ],
            "Lights": [
                light(V(760, 780, 230), "FFD2A1", 1400, 520),
                light(V(400, 200, 280), "7F97A8", 900, 900),
            ],
        },
        {
            "LocationId": "LOC_CORRIDOR",
            "Name": "East corridor",
            "Description": "A narrow corridor. Wet footprints point both ways and agree on nothing.",
            "Origin": V(1600, 560, 0),
            "SizeX": 2200,
            "SizeY": 280,
            "Height": 360,
            "Corridor": True,
            "OpenEast": True,
            "OpenWest": True,
            "OpenNorth": True,
            "OpenSouth": False,
            "Spawn": V(1800, 700, 120),
            "SpawnYaw": 0,
            "Props": [prop("Cube", V(2500, 620, 20), V(2.0, 0.35, 0.04), "0E1218", False)],
            "Lights": [light(V(2500, 700, 300), "6E8EA8", 700, 700)],
        },
        {
            "LocationId": "LOC_BOARDROOM",
            "Name": "Boardroom",
            "Description": "A glass room set for an announcement nobody stayed to make.",
            "Origin": V(2150, 840, 0),
            "SizeX": 1100,
            "SizeY": 900,
            "Height": 420,
            "Corridor": False,
            "OpenEast": False,
            "OpenWest": False,
            "OpenNorth": False,
            "OpenSouth": True,
            "Spawn": V(2700, 1100, 120),
            "SpawnYaw": 90,
            "Props": [prop("Cube", V(2700, 1240, 40), V(2.2, 0.9, 0.8), "12141A", True)],
            "Lights": [light(V(2700, 1300, 260), "C4A574", 600, 500)],
        },
        {
            "LocationId": "LOC_STAIR",
            "Name": "Roof landing",
            "Description": "The last door on the floor. Rain has opinions about the seam.",
            "Origin": V(3800, 560, 0),
            "SizeX": 800,
            "SizeY": 280,
            "Height": 360,
            "Corridor": True,
            "OpenEast": False,
            "OpenWest": True,
            "OpenNorth": False,
            "OpenSouth": False,
            "Spawn": V(3950, 700, 120),
            "SpawnYaw": 0,
            "Props": [],
            "Lights": [light(V(4200, 700, 260), "5A6A78", 500, 400)],
        },
        {
            "LocationId": "LOC_LOBBY",
            "Name": "Lobby, 23:15",
            "Description": "The tower lobby, earlier than the office. The rain is inside the light.",
            "Origin": V(0, 3200, 0),
            "SizeX": 1800,
            "SizeY": 1400,
            "Height": 480,
            "Corridor": False,
            "OpenEast": False,
            "OpenWest": False,
            "OpenNorth": False,
            "OpenSouth": False,
            "Spawn": V(420, 3900, 120),
            "SpawnYaw": 0,
            "Props": [
                prop("Cube", V(900, 4100, 40), V(1.4, 0.5, 0.8), "161A20", True),
                prop("Cube", V(200, 4300, 180), V(0.4, 0.4, 3.6), "101820", False),
            ],
            "Lights": [light(V(900, 3900, 300), "D7C4A4", 1100, 700)],
        },
    ]

    return {
        "ChapterID": c,
        "Title": "Ambition",
        "Logline": "A man wakes in his office at 23:47 and cannot remember the hour he lost.",
        "Playable": True,
        "ProtagonistId": "CH_ADRIAN",
        "Theme": "Ambition",
        "ColourSaturation": 0.10,
        "Clock": "23:47",
        "CoinBeat": COIN_BEATS[c],
        "HourglassBeat": HOURGLASS_BEATS[c],
        "StrangerStage": STRANGER_STAGES[c],
        "CrossChapterRefs": [
            {
                "Id": "CH01_REF_COIN",
                "TargetChapter": "ALL",
                "Kind": "coin",
                "Note": "Adrian discovers the coin under the blotter. Every later chapter meets the same object.",
            },
            {
                "Id": "CH01_REF_GLASS",
                "TargetChapter": "ALL",
                "Kind": "hourglass",
                "Note": "The boardroom hourglass is borrowed time, still only atmospheric.",
            },
            {
                "Id": "CH01_REF_MAYA",
                "TargetChapter": "CH02",
                "Kind": "figure",
                "Note": "Maya, the student in the lobby, is the protagonist of Chapter 02.",
            },
            {
                "Id": "CH01_REF_ELENA",
                "TargetChapter": "CH04",
                "Kind": "symbol",
                "Note": "Elena Moretti's waiver is already in the desk.",
            },
            {
                "Id": "CH01_REF_DANIEL",
                "TargetChapter": "CH03",
                "Kind": "newspaper",
                "Note": "Daniel Cross's evening column is in the boardroom.",
            },
            {
                "Id": "CH01_REF_CLARA",
                "TargetChapter": "CH10",
                "Kind": "painting",
                "Note": "Clara's unfinished canvas already contains the coin.",
            },
            {
                "Id": "CH01_REF_PHRASE",
                "TargetChapter": "ALL",
                "Kind": "phrase",
                "Note": "You don't have much time. You shouldn't open that door.",
            },
        ],
        "Locations": locations_out,
        "Cards": cards,
        "Clues": clues,
        "Dialogue": dialogues,
        "Consequences": consequences,
        "Memories": [memory],
        "Timeline": timeline,
        "Interactables": interactables,
        "MusicIdentity": "piano_low",
        "MusicLayers": ["rain", "city", "clocks", "piano"],
    }


def build_stub(index, protagonist_id, name, theme, logline, refs):
    cid = f"CH{index:02d}"
    loc = f"LOC_{cid}"
    cards = []
    titles = [
        "The Hour",
        "The Room",
        "The Paper",
        "The Coin",
        "The Voice",
        "The Passage",
        "The Memory",
        "The Branch",
        "The Figure",
        "The Glass",
        "The Threshold",
        "The Record",
    ]
    for n in range(1, 13):
        card_id = f"{cid}_CARD_{n:02d}"
        nxt = f"{cid}_CARD_{n+1:02d}" if n < 12 else None
        body = logline
        variants = []
        objectives = [f"enter:{loc}"]
        extra_choices = []
        extra_dialogue = []
        if n == 4:
            body = COIN_BEATS[cid]
        elif n == 8:
            body = "The earlier choice is still in the room. Changing it changes what the record holds."
            extra_choices = [f"CHOICE_{cid}_A", f"CHOICE_{cid}_B"]
            variants = []
        elif n == 9:
            body = f"The stranger is present as {STRANGER_STAGES[cid]}. He does not solve the night."
            extra_dialogue = [f"DLG_{cid}_FIGURE"]
        elif n == 10:
            body = HOURGLASS_BEATS[cid]
        elif n == 12:
            body = "The chapter is recorded. It is not open."
            variants = [
                {
                    "Id": f"V_{cid}_A",
                    "IfAll": [cond("Outcome", "A")],
                    "Body": "The record keeps outcome A. The other page is no longer the one on the table.",
                },
                {
                    "Id": f"V_{cid}_B",
                    "IfAll": [cond("Outcome", "B")],
                    "Body": "The record keeps outcome B. What was signed, trusted, or refused has changed the page.",
                },
            ]
            objectives = [f"discover:{cid}_CLUE_A|{cid}_CLUE_B"]
        cards.append(
            card(
                card_id,
                titles[n - 1],
                titles[n - 1],
                loc,
                f"23:{10 + n:02d}" if n < 12 else "23:55",
                body,
                objectives,
                nxt,
                chapter=cid,
                characters=[protagonist_id],
                choices=extra_choices,
                dialogue=extra_dialogue,
                variants=variants,
            )
        )
    # fix times to be valid HH:MM — 23:10+n for n=1 is 23:11. n=11 is 23:21. Card 12 is 23:55. Good.
    # Wait n=1 uses 23:11. That's fine and sorted for cards but timeline is separate.
    dialogue = [
        node(
            f"DLG_{cid}_FIGURE",
            "CH_STRANGER",
            "You don't have much time.",
            choices=[
                choice(f"CHOICE_{cid}_A", "Stay with what was asked.", f"CONS_{cid}_A", f"DLG_{cid}_RESP_A", "BRANCH_OUTCOME"),
                choice(f"CHOICE_{cid}_B", "Leave it for later.", f"CONS_{cid}_B", f"DLG_{cid}_RESP_B", "BRANCH_OUTCOME"),
            ],
        ),
        node(f"DLG_{cid}_RESP_A", "CH_STRANGER", "Then the page will keep your hand.", next_node=""),
        node(f"DLG_{cid}_RESP_B", "CH_STRANGER", "Later is how a name goes missing.", next_node=""),
    ]
    consequences = [
        consequence(f"CONS_{cid}_A", {"Outcome": "A"}, {"Empathy": 1}, [f"{cid}_CLUE_A"], helped=True),
        consequence(f"CONS_{cid}_B", {"Outcome": "B"}, {"Courage": 1}, [f"{cid}_CLUE_B"]),
    ]
    clues = [
        clue(
            ClueID=f"{cid}_CLUE_MAIN",
            ChapterID=cid,
            CardID=f"{cid}_CARD_01",
            Title="Recorded trace",
            Description=logline,
            Location=loc,
            DiscoveryMethod="documentary",
            RelatedCharacters=[protagonist_id],
            RelatedEvents=[f"TL_{cid}_1"],
            RelatedClues=[],
            MemoryReference=f"MEM_{cid}",
            Importance="major",
            Categories=["Clues"],
        ),
        clue(
            ClueID=f"{cid}_CLUE_A",
            ChapterID=cid,
            CardID=f"{cid}_CARD_12",
            Title="Outcome A",
            Description="The page that remains when the first choice stands.",
            Location=loc,
            DiscoveryMethod="documentary",
            RelatedCharacters=[protagonist_id],
            RelatedEvents=[f"TL_{cid}_2"],
            Importance="critical",
            BranchExclusive=True,
            ActiveIf=[cond("Outcome", "A")],
            Categories=["Clues"],
        ),
        clue(
            ClueID=f"{cid}_CLUE_B",
            ChapterID=cid,
            CardID=f"{cid}_CARD_12",
            Title="Outcome B",
            Description="The page that remains when the later choice stands.",
            Location=loc,
            DiscoveryMethod="documentary",
            RelatedCharacters=[protagonist_id],
            RelatedEvents=[f"TL_{cid}_2"],
            Importance="critical",
            BranchExclusive=True,
            ActiveIf=[cond("Outcome", "B")],
            Categories=["Clues"],
        ),
    ]
    return {
        "ChapterID": cid,
        "Title": theme,
        "Logline": logline,
        "Playable": False,
        "ProtagonistId": protagonist_id,
        "Theme": theme,
        "ColourSaturation": SATURATION[cid],
        "Clock": "23:47",
        "CoinBeat": COIN_BEATS[cid],
        "HourglassBeat": HOURGLASS_BEATS[cid],
        "StrangerStage": STRANGER_STAGES[cid],
        "CrossChapterRefs": refs,
        "Locations": [
            {
                "LocationId": loc,
                "Name": f"{name} — recorded room",
                "Description": logline,
                "Origin": V(0, 0, 0),
                "SizeX": 1200,
                "SizeY": 900,
                "Height": 400,
                "Corridor": False,
                "OpenEast": False,
                "OpenWest": False,
                "OpenNorth": False,
                "OpenSouth": False,
                "Spawn": V(300, 400, 120),
                "SpawnYaw": 0,
                "Props": [],
                "Lights": [light(V(400, 400, 250), "7F97A8", 800, 600)],
            }
        ],
        "Cards": cards,
        "Clues": clues,
        "Dialogue": dialogue,
        "Consequences": consequences,
        "Memories": [
            {
                "MemoryID": f"MEM_{cid}",
                "ChapterID": cid,
                "CardID": f"{cid}_CARD_07",
                "QuestionAnswered": f"{name} was already holding the thread this chapter exists to remember.",
                "QuestionOpened": "What the coin was doing in this room is not yet playable.",
                "RequiredClue": f"{cid}_CLUE_MAIN",
                "TimelineEventID": f"TL_{cid}_1",
                "EntryLocationId": loc,
                "EntryPosition": V(300, 400, 120),
                "HoldLine": "The memory is still holding something.",
                "Playable": False,
            }
        ],
        "Timeline": [
            {
                "EventID": f"TL_{cid}_1",
                "ChapterID": cid,
                "Time": "23:10",
                "Title": "First mark",
                "LocationId": loc,
                "Position": V(300, 400, 120),
                "StartsLocked": True,
                "Branches": [],
                "UnlockOnClue": f"{cid}_CLUE_MAIN",
            },
            {
                "EventID": f"TL_{cid}_2",
                "ChapterID": cid,
                "Time": "23:30",
                "Title": "The branch",
                "LocationId": loc,
                "Position": V(500, 400, 120),
                "StartsLocked": True,
                "Branches": [
                    {"ChoiceID": f"CHOICE_{cid}_A", "Label": "STAY"},
                    {"ChoiceID": f"CHOICE_{cid}_B", "Label": "LEAVE"},
                ],
                "UnlockOnClue": "",
            },
            {
                "EventID": f"TL_{cid}_3",
                "ChapterID": cid,
                "Time": "23:55",
                "Title": "The record",
                "LocationId": loc,
                "Position": V(700, 400, 120),
                "StartsLocked": False,
                "Branches": [],
                "UnlockOnClue": "",
            },
        ],
        "Interactables": [
            interact(
                Id=f"INT_{cid}_NOTE",
                LocationId=loc,
                Kind="Clue",
                Title="Recorded note",
                InspectText=logline,
                ClueID=f"{cid}_CLUE_MAIN",
                Position=V(500, 450, 90),
                RequiredCard=f"{cid}_CARD_01",
            )
        ],
        "MusicIdentity": "stub",
        "MusicLayers": ["rain", "piano"],
    }


STUBS = [
    (
        2,
        "CH_MAYA",
        "Maya",
        "Trust",
        "A student is trusted with a page that does not want to stay written.",
        [
            {"Id": "CH02_COIN", "TargetChapter": "CH01", "Kind": "coin", "Note": "Maya possesses the coin Adrian discovered."},
            {"Id": "CH02_GLASS", "TargetChapter": "ALL", "Kind": "hourglass", "Note": "A borrowed lab timer, sand almost spent."},
            {"Id": "CH02_SIGN", "TargetChapter": "CH01", "Kind": "symbol", "Note": "Adrian's signature is the one she trusted."},
            {"Id": "CH02_FILE", "TargetChapter": "CH03", "Kind": "phrase", "Note": "The statement is on its way to Daniel Cross."},
        ],
    ),
    (
        3,
        "CH_DANIEL",
        "Daniel Cross",
        "Truth",
        "A detective lays ten nights on one table and refuses the easy name.",
        [
            {"Id": "CH03_COIN", "TargetChapter": "ALL", "Kind": "coin", "Note": "Daniel identifies the coin as the same object in unrelated files."},
            {"Id": "CH03_GLASS", "TargetChapter": "ALL", "Kind": "hourglass", "Note": "The hourglass is in a photograph he cannot date."},
            {"Id": "CH03_MAYA", "TargetChapter": "CH02", "Kind": "newspaper", "Note": "Maya's statement is in the file, and his own column is the paper Adrian read."},
            {"Id": "CH03_ELENA", "TargetChapter": "CH04", "Kind": "symbol", "Note": "Elena's waiver is exhibit C."},
        ],
    ),
    (
        4,
        "CH_ELENA",
        "Elena Moretti",
        "Responsibility",
        "A doctor finds her own warning in a file she meant to close.",
        [
            {"Id": "CH04_COIN", "TargetChapter": "ALL", "Kind": "coin", "Note": "Elena sees the coin in an old photograph of a patient."},
            {"Id": "CH04_GLASS", "TargetChapter": "ALL", "Kind": "hourglass", "Note": "A patient left an hourglass after visiting hours."},
            {"Id": "CH04_ADRIAN", "TargetChapter": "CH01", "Kind": "photograph", "Note": "Adrian is the patient named on the waiver."},
            {"Id": "CH04_CLARA", "TargetChapter": "CH10", "Kind": "painting", "Note": "A print of Clara's canvas hangs in the waiting room."},
        ],
    ),
    (
        5,
        "CH_LUCAS",
        "Lucas",
        "Escape",
        "A photographer frames the thing he should have stood beside.",
        [
            {"Id": "CH05_COIN", "TargetChapter": "ALL", "Kind": "coin", "Note": "Lucas photographs the coin and leaves."},
            {"Id": "CH05_GLASS", "TargetChapter": "ALL", "Kind": "hourglass", "Note": "The station clock is an hourglass he frames instead of a person."},
            {"Id": "CH05_PAPER", "TargetChapter": "CH03", "Kind": "newspaper", "Note": "Daniel's paper is in the edge of the frame."},
            {"Id": "CH05_NINA", "TargetChapter": "CH08", "Kind": "photograph", "Note": "Nina will publish the photograph."},
        ],
    ),
    (
        6,
        "CH_MARCO",
        "Marco",
        "Ambition / Sacrifice",
        "A chef serves the room and misses the chair that mattered.",
        [
            {"Id": "CH06_COIN", "TargetChapter": "ALL", "Kind": "coin", "Note": "Marco receives the coin as a tip."},
            {"Id": "CH06_GLASS", "TargetChapter": "ALL", "Kind": "hourglass", "Note": "The kitchen timer is an hourglass he never turns."},
            {"Id": "CH06_SOFIA", "TargetChapter": "CH07", "Kind": "symbol", "Note": "Sofia's banquet ticket is on the pass."},
            {"Id": "CH06_ELIAS", "TargetChapter": "CH09", "Kind": "figure", "Note": "Elias has a standing table and does not eat."},
            {"Id": "CH06_HIST", "TargetChapter": "ALL", "Kind": "phrase", "Note": "A 1920s clipping mentions a server who does not age."},
        ],
    ),
    (
        7,
        "CH_SOFIA",
        "Sofia",
        "Sacrifice",
        "An athlete spends a body the note already asked her to keep.",
        [
            {"Id": "CH07_COIN", "TargetChapter": "ALL", "Kind": "coin", "Note": "Sofia finds the coin in her locker."},
            {"Id": "CH07_GLASS", "TargetChapter": "ALL", "Kind": "hourglass", "Note": "The stadium clock is painted as an hourglass."},
            {"Id": "CH07_ELENA", "TargetChapter": "CH04", "Kind": "symbol", "Note": "Elena's note is taped inside the locker."},
            {"Id": "CH07_MARCO", "TargetChapter": "CH06", "Kind": "location", "Note": "The fundraiser dinner was Marco's."},
        ],
    ),
    (
        8,
        "CH_NINA",
        "Nina",
        "Identity",
        "An influencer watches a recording of a woman she cannot claim.",
        [
            {"Id": "CH08_COIN", "TargetChapter": "ALL", "Kind": "coin", "Note": "Nina sees the coin in an old recording she does not remember filming."},
            {"Id": "CH08_GLASS", "TargetChapter": "ALL", "Kind": "hourglass", "Note": "An hourglass overlay is on the stream and not in her edit."},
            {"Id": "CH08_LUCAS", "TargetChapter": "CH05", "Kind": "photograph", "Note": "Lucas's photograph is the thumbnail."},
            {"Id": "CH08_CLARA", "TargetChapter": "CH10", "Kind": "painting", "Note": "Clara's canvas is the backdrop."},
        ],
    ),
    (
        9,
        "CH_ELIAS",
        "Elias Hart",
        "Control",
        "A scientist measures a coin that will not sit inside his dates.",
        [
            {"Id": "CH09_COIN", "TargetChapter": "ALL", "Kind": "coin", "Note": "Elias investigates the coin. The alloy predates his chart."},
            {"Id": "CH09_GLASS", "TargetChapter": "ALL", "Kind": "hourglass", "Note": "He draws the hourglass and the axis labels fall off."},
            {"Id": "CH09_DANIEL", "TargetChapter": "CH03", "Kind": "symbol", "Note": "Daniel's case number is on the sample bag."},
            {"Id": "CH09_MARCO", "TargetChapter": "CH06", "Kind": "figure", "Note": "Marco's book lists a guest who does not eat."},
        ],
    ),
    (
        10,
        "CH_CLARA",
        "Clara",
        "Memory",
        "An artist paints a coin she has already painted, in a room that forgot her name.",
        [
            {"Id": "CH10_COIN", "TargetChapter": "ALL", "Kind": "coin", "Note": "Clara paints the coin."},
            {"Id": "CH10_GLASS", "TargetChapter": "ALL", "Kind": "hourglass", "Note": "She paints the hourglass with sand in both bulbs."},
            {"Id": "CH10_CANVAS", "TargetChapter": "CH01", "Kind": "painting", "Note": "The canvas in Adrian's boardroom is hers, unfinished."},
            {"Id": "CH10_NINA", "TargetChapter": "CH08", "Kind": "photograph", "Note": "Nina used the painting as a backdrop."},
            {"Id": "CH10_LATE", "TargetChapter": "LATE", "Kind": "figure", "Note": "The full sitting is locked. It is not playable in this slice."},
        ],
    ),
]


def build_late():
    return {
        "Playable": False,
        "ImplementSteps": [16, 17, 18, 19],
        "GrimReaperIdentity": "The Grim Reaper",
        "GrimReaperIsVillain": False,
        "GrimReaperLines": [
            "You thought you were solving their deaths.",
            "You were learning who they were.",
            "And I was learning who you are.",
            "I don't decide who deserves another life.",
            "You showed me.",
            "There is one soul left.",
        ],
        "VoidName": "The Void",
        "VoidPolicy": "Unnamed until late game. Early traces only: missing names, corrupted photographs, broken cards, blank dialogue.",
        "Endings": [
            {"Id": "THE_RETURN", "Title": "THE RETURN", "Meaning": "The soul returns."},
            {"Id": "THE_RELEASE", "Title": "THE RELEASE", "Meaning": "The soul accepts its journey."},
            {"Id": "THE_LOST", "Title": "THE LOST", "Meaning": "The soul disappears into The Void."},
            {"Id": "THE_SACRIFICE", "Title": "THE SACRIFICE", "Meaning": "One soul helps another."},
            {"Id": "THE_AWAKENING", "Title": "THE AWAKENING", "Meaning": "The soul understands the cycle."},
        ],
        "Soul11Title": "SOUL 11",
        "Soul11Then": "YOU",
        "Soul11Chairs": 11,
        "Soul11CoinPlacedBy": "The Grim Reaper",
        "FinalMessage": [
            "You cannot change what happened.",
            "But you can change what it means.",
        ],
        "AwakeningLine": "Now we begin.",
    }


def build_music():
    identities = {
        "CH_ADRIAN": "piano_low",
        "CH_MAYA": "soft_keys",
        "CH_DANIEL": "muted_brass",
        "CH_ELENA": "single_cello",
        "CH_LUCAS": "travelling_guitar",
        "CH_MARCO": "kitchen_jazz",
        "CH_SOFIA": "pulse_drums",
        "CH_NINA": "processed_voice",
        "CH_ELIAS": "thin_oscillator",
        "CH_CLARA": "worn_strings",
        "CH_STRANGER": "motif_fragment",
    }
    return {
        "Layers": [
            "rain",
            "wind",
            "footsteps",
            "city",
            "traffic",
            "clocks",
            "doors",
            "electrical_hum",
            "phones",
            "paper",
            "whispers",
            "jazz",
            "piano",
            "strings",
        ],
        "Identities": identities,
        "StrangerMotifFull": "LOCKED_STEP_16",
    }


def build_all() -> None:
    write_json(
        DATA / "Game.json",
        {
            "Title": "HOW DID I DIE?",
            "Tagline": "EVERY CHOICE HAS A MEMORY.",
            "MenuClockFallback": "23:47",
        },
    )
    write_json(DATA / "Characters.json", build_characters())
    write_json(DATA / "Music.json", build_music())
    write_json(DATA / "Locked" / "late_game.json", build_late())
    write_json(DATA / "Chapters" / "CH01.json", build_chapter_one())
    for spec in STUBS:
        chapter = build_stub(*spec)
        write_json(DATA / "Chapters" / f"{chapter['ChapterID']}.json", chapter)


def parse_clock(text: str):
    parts = text.split(":")
    if len(parts) != 2 or len(parts[0]) != 2 or len(parts[1]) != 2:
        return None
    if not (parts[0].isdigit() and parts[1].isdigit()):
        return None
    hours, minutes = int(parts[0]), int(parts[1])
    if hours > 23 or minutes > 59:
        return None
    return hours * 60 + minutes


def validate() -> list[str]:
    errors: list[str] = []

    def err(message: str) -> None:
        errors.append(message)

    characters = json.loads((DATA / "Characters.json").read_text(encoding="utf-8"))["Characters"]
    by_id = {c["Id"]: c for c in characters}
    if len(by_id) != len(characters):
        err("Duplicate character id.")
    for cid, name, age, job, theme, _chapter in PROTAGONISTS:
        got = by_id.get(cid)
        if not got:
            err(f"Missing protagonist {cid}")
            continue
        if got["DisplayName"] != name or got["Age"] != age or got["Profession"] != job or got["Theme"] != theme:
            err(f"Character drift: {cid}")
        if not got["NotebookIntro"] or not got["VoiceNote"]:
            err(f"Character {cid} missing notebook intro or voice.")
    stranger = by_id.get("CH_STRANGER")
    if not stranger or stranger["DisplayName"] != "Stranger":
        err("The stranger's public name must stay Stranger.")
    if stranger and "grim reaper" in stranger["DisplayName"].lower():
        err("Stranger display name reveals the identity.")

    late = json.loads((DATA / "Locked" / "late_game.json").read_text(encoding="utf-8"))
    if late.get("Playable"):
        err("Late game must not be playable in this slice.")
    if late.get("GrimReaperIsVillain"):
        err("The Grim Reaper is not the villain.")
    if late.get("GrimReaperIdentity") != "The Grim Reaper":
        err("Locked identity missing.")
    expected_lines = [
        "You thought you were solving their deaths.",
        "You were learning who they were.",
        "And I was learning who you are.",
        "I don't decide who deserves another life.",
        "You showed me.",
        "There is one soul left.",
    ]
    if late.get("GrimReaperLines") != expected_lines:
        err("Final Grim Reaper lines drifted.")
    if late.get("AwakeningLine") != "Now we begin.":
        err("Awakening line drifted.")
    if late.get("Soul11Title") != "SOUL 11" or late.get("Soul11Then") != "YOU" or late.get("Soul11Chairs") != 11:
        err("Soul 11 lock drifted.")
    if late.get("FinalMessage") != ["You cannot change what happened.", "But you can change what it means."]:
        err("Final message drifted.")
    if late.get("VoidName") != "The Void":
        err("Void name missing from the lock file.")
    ending_titles = [e["Title"] for e in late.get("Endings", [])]
    if ending_titles != ["THE RETURN", "THE RELEASE", "THE LOST", "THE SACRIFICE", "THE AWAKENING"]:
        err("Ending list drifted.")
    for step in (16, 17, 18, 19):
        if step not in late.get("ImplementSteps", []):
            err(f"Implement step {step} not locked.")

    music = json.loads((DATA / "Music.json").read_text(encoding="utf-8"))
    if music.get("StrangerMotifFull") != "LOCKED_STEP_16":
        err("Full stranger motif must stay locked.")
    for cid, *_rest in PROTAGONISTS:
        if cid not in music.get("Identities", {}):
            err(f"Music identity missing for {cid}")
    if "CH_STRANGER" not in music.get("Identities", {}):
        err("Stranger motif fragment missing.")

    save_header = (ROOT / "Source/HowDidIDie/Public/Save/HDIDSaveGame.h").read_text(encoding="utf-8")
    for prop in SAVE_PROPS:
        if prop not in save_header:
            err(f"SaveGame missing {prop}")
    for field in SOUL_FIELDS + DEDICATION_FIELDS:
        if field not in save_header and field not in (ROOT / "Source/HowDidIDie/Public/Data/HDIDTypes.h").read_text(encoding="utf-8"):
            err(f"Soul/dedication field missing {field}")
    types = (ROOT / "Source/HowDidIDie/Public/Data/HDIDTypes.h").read_text(encoding="utf-8")
    for field in SOUL_FIELDS:
        if field not in types:
            err(f"Soul field {field} missing from types.")
    for field in DEDICATION_FIELDS:
        if field not in types:
            err(f"Dedication field {field} missing from types.")

    chapters = []
    for path in sorted((DATA / "Chapters").glob("*.json")):
        chapters.append(json.loads(path.read_text(encoding="utf-8")))
    if [c["ChapterID"] for c in chapters] != [f"CH{i:02d}" for i in range(1, 11)]:
        err("Expected chapters CH01–CH10.")

    clue_ids = set()
    dialogue_ids = set()
    choice_ids = set()
    card_ids = set()
    event_ids = set()
    memory_ids = set()
    known_chapters = {c["ChapterID"] for c in chapters} | {"ALL", "LATE"}

    for chapter in chapters:
        cid = chapter["ChapterID"]
        if chapter["ProtagonistId"] != dict((p[5], p[0]) for p in PROTAGONISTS)[cid]:
            err(f"{cid} protagonist mismatch.")
        if chapter["Theme"] != dict((p[5], p[4]) for p in PROTAGONISTS)[cid]:
            err(f"{cid} theme mismatch.")
        if chapter["CoinBeat"] != COIN_BEATS[cid]:
            err(f"{cid} coin beat drifted.")
        if chapter["HourglassBeat"] != HOURGLASS_BEATS[cid]:
            err(f"{cid} hourglass beat drifted.")
        if "hourglass" not in chapter["HourglassBeat"].lower():
            err(f"{cid} hourglass beat does not mention the hourglass.")
        if chapter["StrangerStage"] != STRANGER_STAGES[cid]:
            err(f"{cid} stranger stage drifted.")
        if abs(float(chapter["ColourSaturation"]) - SATURATION[cid]) > 0.001:
            err(f"{cid} colour saturation drifted.")
        if cid != "CH01" and chapter["Playable"]:
            err(f"{cid} must not be playable in this slice.")
        if cid == "CH01" and not chapter["Playable"]:
            err("Chapter 01 must be playable.")
        if len(chapter["Cards"]) != 12:
            err(f"{cid} must have exactly 12 cards.")
        refs = chapter.get("CrossChapterRefs", [])
        if len(refs) < 3:
            err(f"{cid} needs at least three cross-chapter references.")
        kinds = {r.get("Kind") for r in refs}
        if "coin" not in kinds or "hourglass" not in kinds:
            err(f"{cid} cross refs must include coin and hourglass.")
        for ref in refs:
            if ref.get("Kind") not in CROSS_KINDS:
                err(f"{cid} bad cross kind {ref.get('Kind')}")
            if ref.get("TargetChapter") not in known_chapters:
                err(f"{cid} cross target {ref.get('TargetChapter')} missing.")
            if not ref.get("Note"):
                err(f"{cid} cross ref {ref.get('Id')} missing note.")

        cons_by_id = {}
        for cons in chapter["Consequences"]:
            if cons["Id"] in cons_by_id:
                err(f"Duplicate consequence {cons['Id']}")
            cons_by_id[cons["Id"]] = cons

        local_cards = {}
        previous_time = -1
        for index, card_obj in enumerate(chapter["Cards"]):
            for key in CARD_KEYS:
                if key not in card_obj:
                    err(f"{cid} card missing {key}")
            if card_obj["CardID"] in card_ids:
                err(f"Duplicate card {card_obj['CardID']}")
            card_ids.add(card_obj["CardID"])
            local_cards[card_obj["CardID"]] = card_obj
            if card_obj["ChapterID"] != cid:
                err(f"Card {card_obj['CardID']} chapter mismatch.")
            if parse_clock(card_obj["Time"]) is None:
                err(f"Card {card_obj['CardID']} bad time.")
            if index < len(chapter["Cards"]) - 1:
                if card_obj["NextCards"] != [chapter["Cards"][index + 1]["CardID"]]:
                    err(f"{card_obj['CardID']} must unlock the next card.")
            elif card_obj["NextCards"]:
                err(f"{card_obj['CardID']} is last and must not point onward.")

        times = []
        local_events = {}
        for event in chapter["Timeline"]:
            if event["EventID"] in event_ids:
                err(f"Duplicate event {event['EventID']}")
            event_ids.add(event["EventID"])
            local_events[event["EventID"]] = event
            minutes = parse_clock(event["Time"])
            if minutes is None:
                err(f"Event {event['EventID']} bad time.")
            else:
                times.append(minutes)
            if event.get("LocationId") not in {loc["LocationId"] for loc in chapter["Locations"]}:
                err(f"Event {event['EventID']} missing location.")
        if times != sorted(times) or len(times) != len(set(times)):
            err(f"{cid} timeline is not strictly chronological.")

        local_clues = {}
        for item in chapter["Clues"]:
            for key in CLUE_KEYS:
                if key not in item:
                    err(f"Clue missing {key}")
            if item["ClueID"] in clue_ids:
                err(f"Duplicate clue {item['ClueID']}")
            clue_ids.add(item["ClueID"])
            local_clues[item["ClueID"]] = item
            if item["ChapterID"] != cid:
                err(f"Clue {item['ClueID']} chapter mismatch.")
            if item["CardID"] not in local_cards:
                err(f"Clue {item['ClueID']} card missing.")
            if not item["Description"] or not item["Location"] or not item["DiscoveryMethod"]:
                err(f"Clue {item['ClueID']} empty required text.")
            if item["DiscoveryMethod"] not in DISCOVERY:
                err(f"Clue {item['ClueID']} bad discovery method.")
            if item["Importance"] not in IMPORTANCE:
                err(f"Clue {item['ClueID']} bad importance.")
            if item["MemoryReference"] and not any(m["MemoryID"] == item["MemoryReference"] for m in chapter["Memories"]):
                err(f"Clue {item['ClueID']} memory reference missing.")

        for item in chapter["Clues"]:
            for related in item["RelatedClues"]:
                if related not in local_clues and related not in clue_ids:
                    err(f"Clue {item['ClueID']} related clue {related} missing.")
            for who in item["RelatedCharacters"]:
                if who not in by_id:
                    err(f"Clue {item['ClueID']} unknown character {who}.")
            for event_id in item["RelatedEvents"]:
                if event_id not in local_events:
                    err(f"Clue {item['ClueID']} unknown event {event_id}.")

        local_dialogues = {}
        local_choices = {}
        for dlg in chapter["Dialogue"]:
            for key in DIALOGUE_KEYS:
                if key not in dlg:
                    err(f"Dialogue missing {key}")
            if dlg["DialogueID"] in dialogue_ids:
                err(f"Duplicate dialogue {dlg['DialogueID']}")
            dialogue_ids.add(dlg["DialogueID"])
            local_dialogues[dlg["DialogueID"]] = dlg
            if dlg["Speaker"] not in by_id:
                err(f"Dialogue {dlg['DialogueID']} speaker missing.")
            if dlg.get("Reveal"):
                err(f"Reveal dialogue {dlg['DialogueID']} is locked.")
            signatures = []
            for ch in dlg["Choices"]:
                if ch["ChoiceID"] in choice_ids:
                    err(f"Duplicate choice {ch['ChoiceID']}")
                choice_ids.add(ch["ChoiceID"])
                local_choices[ch["ChoiceID"]] = ch
                cons = cons_by_id.get(ch["ConsequenceID"])
                if not cons:
                    err(f"Choice {ch['ChoiceID']} consequence missing.")
                    continue
                signature = (
                    ch["ConsequenceID"],
                    ch.get("NextNode", ""),
                    tuple(sorted((cons.get("SetFlags") or {}).items())),
                    tuple(cons.get("RevealClues") or []),
                    tuple(sorted((cons.get("Soul") or {}).items())),
                )
                signatures.append(signature)
            if len(dlg["Choices"]) >= 2 and len(set(signatures)) < len(signatures):
                err(f"Dialogue {dlg['DialogueID']} has identical choice results.")
            if dlg["NextNode"] and dlg["NextNode"] not in {d["DialogueID"] for d in chapter["Dialogue"]}:
                err(f"Dialogue {dlg['DialogueID']} next node missing.")
            for ch in dlg["Choices"]:
                if ch["NextNode"] and ch["NextNode"] not in {d["DialogueID"] for d in chapter["Dialogue"]}:
                    err(f"Choice {ch['ChoiceID']} next node missing.")

        groups: dict[str, list] = {}
        for ch in local_choices.values():
            if ch["BranchGroup"]:
                groups.setdefault(ch["BranchGroup"], []).append(ch)
        for group, group_choices in groups.items():
            flag_sets = []
            for ch in group_choices:
                flag_sets.append(tuple(sorted((cons_by_id[ch["ConsequenceID"]].get("SetFlags") or {}).items())))
            if len(set(flag_sets)) < 2:
                err(f"{cid} branch {group} does not change flags.")
            flags = {key for pairs in flag_sets for key, _value in pairs}
            used = False
            for card_obj in chapter["Cards"]:
                for variant in card_obj.get("Variants", []):
                    if any(c["Flag"] in flags for c in variant.get("IfAll", [])):
                        used = True
            for item in chapter["Interactables"]:
                if any(c["Flag"] in flags for c in item.get("PresentIf", [])):
                    used = True
            for item in chapter["Clues"]:
                if any(c["Flag"] in flags for c in item.get("ActiveIf", [])):
                    used = True
            if not used:
                err(f"{cid} branch {group} does not change later narrative state.")

        for card_obj in chapter["Cards"]:
            for nxt in card_obj["NextCards"]:
                if nxt not in local_cards:
                    err(f"Next card {nxt} missing.")
            for clue_id in card_obj["Clues"]:
                if clue_id not in local_clues:
                    err(f"Card {card_obj['CardID']} unknown clue {clue_id}.")
            for did in card_obj["Dialogue"]:
                if did not in local_dialogues:
                    err(f"Card {card_obj['CardID']} unknown dialogue {did}.")
            for choice_id in card_obj["Choices"]:
                if choice_id not in local_choices:
                    err(f"Card {card_obj['CardID']} unknown choice {choice_id}.")
            for memory_id in card_obj["Flashbacks"]:
                if not any(m["MemoryID"] == memory_id for m in chapter["Memories"]):
                    err(f"Card {card_obj['CardID']} unknown flashback {memory_id}.")

        for event in chapter["Timeline"]:
            if event["UnlockOnClue"] and event["UnlockOnClue"] not in local_clues:
                err(f"Event {event['EventID']} unlock clue missing.")
            for branch in event["Branches"]:
                if branch["ChoiceID"] not in local_choices:
                    err(f"Event {event['EventID']} branch choice missing.")

        for memory in chapter["Memories"]:
            if memory["MemoryID"] in memory_ids:
                err(f"Duplicate memory {memory['MemoryID']}")
            memory_ids.add(memory["MemoryID"])
            if not memory["QuestionAnswered"] or not memory["QuestionOpened"]:
                err(f"Memory {memory['MemoryID']} does not reveal and open a question.")
            if memory["RequiredClue"] not in local_clues:
                err(f"Memory {memory['MemoryID']} required clue missing.")
            if memory["TimelineEventID"] not in local_events:
                err(f"Memory {memory['MemoryID']} timeline event missing.")

        if cid == "CH01":
            blob = json.dumps(chapter).lower()
            for phrase in FORBIDDEN_CH01:
                if phrase in blob:
                    err(f"Chapter 01 contains forbidden phrase: {phrase}")
            playable_memory = [m for m in chapter["Memories"] if m["Playable"]]
            if len(playable_memory) < 1:
                err("Chapter 01 needs a playable flashback.")
            blob_raw = json.dumps(chapter)
            if "You don't have much time." not in blob_raw or "You shouldn't open that door." not in blob_raw:
                err("Chapter 01 is missing the stranger's double-edged lines.")
            if not any(i["Kind"] == "GrimReaper" for i in chapter["Interactables"]):
                err("Chapter 01 stranger interactable missing.")
            if not any(i["ClueID"] == "CLUE_COIN" for i in chapter["Interactables"]):
                err("Chapter 01 coin interactable missing.")
            branched = [e for e in chapter["Timeline"] if len(e["Branches"]) >= 2]
            if not branched:
                err("Chapter 01 timeline branch missing.")

    return errors


def main() -> int:
    if len(sys.argv) > 1 and sys.argv[1] == "validate":
        errors = validate()
    else:
        build_all()
        errors = validate()
    if errors:
        print(f"NARRATIVE VALIDATION FAILED ({len(errors)})")
        for error in errors:
            print(f" - {error}")
        return 1
    print("Narrative data valid: 10 chapters, Chapter 01 playable, late game locked.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
