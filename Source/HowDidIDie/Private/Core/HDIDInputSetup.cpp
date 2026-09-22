#include "Core/HDIDInputSetup.h"

#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "PlayerMappableKeySettings.h"
#include "Save/HDIDSaveGame.h"

void UHDIDInputSetup::BuildDefaults()
{
	Actions.Reset();
	Slots.Reset();
	AddAction(TEXT("Move"), true);
	AddAction(TEXT("LookMouse"), true);
	AddAction(TEXT("LookGamepad"), true);
	AddAction(TEXT("Interact"), false);
	AddAction(TEXT("Inspect"), false);
	AddAction(TEXT("Sprint"), false);
	AddAction(TEXT("Advance"), false);
	AddAction(TEXT("Pause"), false);
	AddAction(TEXT("Notebook"), false);
	AddAction(TEXT("Timeline"), false);
	AddAction(TEXT("Memory"), false);
	AddAction(TEXT("Rewind"), false);
	AddAction(TEXT("Choice1"), false);
	AddAction(TEXT("Choice2"), false);
	AddAction(TEXT("Choice3"), false);
	AddAction(TEXT("Choice4"), false);
	AddAction(TEXT("Map"), false);
	AddAction(TEXT("UIUp"), false);
	AddAction(TEXT("UIDown"), false);
	AddAction(TEXT("UILeft"), false);
	AddAction(TEXT("UIRight"), false);
	AddAction(TEXT("Back"), false);

	AddSlot(TEXT("MoveForward"), TEXT("Move"), EKeys::W, TEXT("Move Forward"), TEXT("Movement"), false, true);
	AddSlot(TEXT("MoveBack"), TEXT("Move"), EKeys::S, TEXT("Move Back"), TEXT("Movement"), true, true);
	AddSlot(TEXT("MoveLeft"), TEXT("Move"), EKeys::A, TEXT("Move Left"), TEXT("Movement"), true, false);
	AddSlot(TEXT("MoveRight"), TEXT("Move"), EKeys::D, TEXT("Move Right"), TEXT("Movement"));
	AddSlot(TEXT("MoveGamepad"), TEXT("Move"), EKeys::Gamepad_Left2D, TEXT("Move (Gamepad)"), TEXT("Movement"));
	AddSlot(TEXT("LookMouse"), TEXT("LookMouse"), EKeys::Mouse2D, TEXT("Look (Mouse)"), TEXT("Camera"), false, false, true);
	AddSlot(TEXT("LookGamepad"), TEXT("LookGamepad"), EKeys::Gamepad_Right2D, TEXT("Look (Gamepad)"), TEXT("Camera"));
	AddSlot(TEXT("InteractKey"), TEXT("Interact"), EKeys::E, TEXT("Interact"), TEXT("Interaction"));
	AddSlot(TEXT("InteractMouse"), TEXT("Interact"), EKeys::LeftMouseButton, TEXT("Interact (Mouse)"), TEXT("Interaction"));
	AddSlot(TEXT("InteractPad"), TEXT("Interact"), EKeys::Gamepad_FaceButton_Bottom, TEXT("Interact (Gamepad)"), TEXT("Interaction"));
	AddSlot(TEXT("InspectMouse"), TEXT("Inspect"), EKeys::RightMouseButton, TEXT("Inspect"), TEXT("Interaction"));
	AddSlot(TEXT("InspectPad"), TEXT("Inspect"), EKeys::Gamepad_FaceButton_Left, TEXT("Inspect (Gamepad)"), TEXT("Interaction"));
	AddSlot(TEXT("SprintKey"), TEXT("Sprint"), EKeys::LeftShift, TEXT("Sprint"), TEXT("Movement"));
	AddSlot(TEXT("SprintPad"), TEXT("Sprint"), EKeys::Gamepad_LeftThumbstick, TEXT("Sprint (Gamepad)"), TEXT("Movement"));
	AddSlot(TEXT("Advance"), TEXT("Advance"), EKeys::SpaceBar, TEXT("Continue Dialogue"), TEXT("Narrative"));
	AddSlot(TEXT("PauseKey"), TEXT("Pause"), EKeys::Escape, TEXT("Pause"), TEXT("Interface"));
	AddSlot(TEXT("PausePad"), TEXT("Pause"), EKeys::Gamepad_Special_Right, TEXT("Pause (Gamepad)"), TEXT("Interface"));
	AddSlot(TEXT("NotebookKey"), TEXT("Notebook"), EKeys::Tab, TEXT("Investigation Notebook"), TEXT("Narrative"));
	AddSlot(TEXT("NotebookPad"), TEXT("Notebook"), EKeys::Gamepad_FaceButton_Top, TEXT("Notebook (Gamepad)"), TEXT("Narrative"));
	AddSlot(TEXT("TimelineKey"), TEXT("Timeline"), EKeys::Q, TEXT("Timeline"), TEXT("Narrative"));
	AddSlot(TEXT("TimelinePad"), TEXT("Timeline"), EKeys::Gamepad_RightTrigger, TEXT("Timeline (Gamepad)"), TEXT("Narrative"));
	AddSlot(TEXT("MemoryKey"), TEXT("Memory"), EKeys::F, TEXT("Memory"), TEXT("Narrative"));
	AddSlot(TEXT("MemoryPad"), TEXT("Memory"), EKeys::Gamepad_LeftTrigger, TEXT("Memory (Gamepad)"), TEXT("Narrative"));
	AddSlot(TEXT("Rewind"), TEXT("Rewind"), EKeys::R, TEXT("Contextual Rewind"), TEXT("Narrative"));
	AddSlot(TEXT("Choice1"), TEXT("Choice1"), EKeys::One, TEXT("Dialogue Choice 1"), TEXT("Narrative"));
	AddSlot(TEXT("Choice2"), TEXT("Choice2"), EKeys::Two, TEXT("Dialogue Choice 2"), TEXT("Narrative"));
	AddSlot(TEXT("Choice3"), TEXT("Choice3"), EKeys::Three, TEXT("Dialogue Choice 3"), TEXT("Narrative"));
	AddSlot(TEXT("Choice4"), TEXT("Choice4"), EKeys::Four, TEXT("Dialogue Choice 4"), TEXT("Narrative"));
	AddSlot(TEXT("Map"), TEXT("Map"), EKeys::M, TEXT("Location"), TEXT("Interface"));
	AddSlot(TEXT("UIUpKey"), TEXT("UIUp"), EKeys::Up, TEXT("Navigate Up"), TEXT("Interface"));
	AddSlot(TEXT("UIDownKey"), TEXT("UIDown"), EKeys::Down, TEXT("Navigate Down"), TEXT("Interface"));
	AddSlot(TEXT("UILeftKey"), TEXT("UILeft"), EKeys::Left, TEXT("Navigate Left"), TEXT("Interface"));
	AddSlot(TEXT("UIRightKey"), TEXT("UIRight"), EKeys::Right, TEXT("Navigate Right"), TEXT("Interface"));
	AddSlot(TEXT("UIUpPad"), TEXT("UIUp"), EKeys::Gamepad_DPad_Up, TEXT("Navigate Up (Gamepad)"), TEXT("Interface"));
	AddSlot(TEXT("UIDownPad"), TEXT("UIDown"), EKeys::Gamepad_DPad_Down, TEXT("Navigate Down (Gamepad)"), TEXT("Interface"));
	AddSlot(TEXT("UILeftPad"), TEXT("UILeft"), EKeys::Gamepad_DPad_Left, TEXT("Navigate Left (Gamepad)"), TEXT("Interface"));
	AddSlot(TEXT("UIRightPad"), TEXT("UIRight"), EKeys::Gamepad_DPad_Right, TEXT("Navigate Right (Gamepad)"), TEXT("Interface"));
	AddSlot(TEXT("BackPad"), TEXT("Back"), EKeys::Gamepad_FaceButton_Right, TEXT("Back"), TEXT("Interface"));
	AddSlot(TEXT("BackKey"), TEXT("Back"), EKeys::BackSpace, TEXT("Back (Keyboard)"), TEXT("Interface"));
	RebuildContext();
}

UInputAction* UHDIDInputSetup::AddAction(FName Id, bool bAxis2D)
{
	UInputAction* Action = NewObject<UInputAction>(this);
	Action->ValueType = bAxis2D ? EInputActionValueType::Axis2D : EInputActionValueType::Boolean;
	Actions.Add(Id, Action);
	return Action;
}

void UHDIDInputSetup::AddSlot(FName SlotId, FName ActionId, const FKey& Key, const FString& Display, const FString& Category, bool bNegate, bool bSwizzleY, bool bMouseScale)
{
	FSlot Slot;
	Slot.SlotId = SlotId;
	Slot.ActionId = ActionId;
	Slot.DefaultKey = Key;
	Slot.CurrentKey = Key;
	Slot.bNegate = bNegate;
	Slot.bSwizzleY = bSwizzleY;
	Slot.bMouseScale = bMouseScale;
	Slot.Display = Display;
	Slot.Category = Category;
	Slots.Add(Slot);
}

void UHDIDInputSetup::ApplyRebinds(const TArray<FHDIDKeyRebind>& Rebinds)
{
	for (FSlot& Slot : Slots)
	{
		Slot.CurrentKey = Slot.DefaultKey;
		for (const FHDIDKeyRebind& Rebind : Rebinds)
		{
			if (Rebind.SlotId == Slot.SlotId.ToString())
			{
				const FKey Key(*Rebind.KeyName);
				if (Key.IsValid())
				{
					Slot.CurrentKey = Key;
				}
			}
		}
	}
	RebuildContext();
}

void UHDIDInputSetup::SetSlotKey(FName SlotId, const FKey& Key)
{
	for (FSlot& Slot : Slots)
	{
		if (Slot.SlotId == SlotId)
		{
			Slot.CurrentKey = Key;
		}
	}
	RebuildContext();
}

void UHDIDInputSetup::Register(UEnhancedInputLocalPlayerSubsystem* InSubsystem)
{
	Subsystem = InSubsystem;
	if (Subsystem.IsValid() && Context)
	{
		Subsystem->RemoveMappingContext(Context);
		Subsystem->AddMappingContext(Context, 0);
	}
}

UInputAction* UHDIDInputSetup::GetAction(FName ActionId) const
{
	const TObjectPtr<UInputAction>* Found = Actions.Find(ActionId);
	return Found ? Found->Get() : nullptr;
}

TArray<FHDIDBindSlotView> UHDIDInputSetup::GetBindViews() const
{
	TArray<FHDIDBindSlotView> Views;
	for (const FSlot& Slot : Slots)
	{
		FHDIDBindSlotView View;
		View.SlotId = Slot.SlotId;
		View.ActionId = Slot.ActionId;
		View.Display = Slot.Display;
		View.Category = Slot.Category;
		View.KeyName = Slot.CurrentKey.GetDisplayName().ToString();
		Views.Add(View);
	}
	return Views;
}

void UHDIDInputSetup::RebuildContext()
{
	if (Subsystem.IsValid() && Context)
	{
		Subsystem->RemoveMappingContext(Context);
	}
	Context = NewObject<UInputMappingContext>(this);
	for (const FSlot& Slot : Slots)
	{
		UInputAction* Action = GetAction(Slot.ActionId);
		if (!Action || !Slot.CurrentKey.IsValid())
		{
			continue;
		}
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Slot.CurrentKey);
		if (Slot.bSwizzleY)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (Slot.bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
		if (Slot.bMouseScale)
		{
			UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(Context);
			Scalar->Scalar = FVector(0.07f, 0.07f, 0.07f);
			Mapping.Modifiers.Add(Scalar);
		}
		UPlayerMappableKeySettings* Mappable = NewObject<UPlayerMappableKeySettings>(Context);
		Mappable->Name = Slot.SlotId;
		Mappable->DisplayName = FText::FromString(Slot.Display);
		Mappable->DisplayCategory = FText::FromString(Slot.Category);
		Mapping.PlayerMappableKeySettings = Mappable;
	}
	if (Subsystem.IsValid())
	{
		Subsystem->AddMappingContext(Context, 0);
	}
}
