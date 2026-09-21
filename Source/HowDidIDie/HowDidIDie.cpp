#include "HowDidIDie.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, HowDidIDie, "HowDidIDie");

DEFINE_LOG_CATEGORY(LogHDID);

// Blueprint wrappers named by the architecture spec are implemented in C++
// because this slice cannot author .uasset binaries:
//   BP_PlayerCharacter        AHDIDPlayerCharacter
//   BP_Interactable           AHDIDInteractable
//   BP_Clue                   AHDIDClue
//   BP_MemoryTrigger          AHDIDMemoryTrigger
//   BP_FlashbackManager       UHDIDFlashbackManager
//   BP_TimelineManager        UHDIDTimelineManager
//   BP_StoryCardManager       UHDIDStoryCardManager
//   BP_DialogueManager        UHDIDDialogueManager
//   BP_InvestigationManager   UHDIDInvestigationManager
//   BP_SaveManager            UHDIDSaveManager
//   BP_ChoiceManager          UHDIDChoiceManager
//   BP_ConsequenceManager     UHDIDConsequenceManager
//   BP_SoulEvaluationManager  UHDIDSoulEvaluationManager
//   BP_GrimReaper             AHDIDGrimReaper
//   BP_Void                   AHDIDVoidTrace
//   BP_CinematicManager       UHDIDCinematicManager
//   BP_AudioManager           UHDIDAudioManager
//   BP_UIManager              UHDIDUIManager
