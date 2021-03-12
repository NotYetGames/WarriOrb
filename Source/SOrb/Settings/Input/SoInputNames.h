// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Framework/Commands/InputChord.h"

#include "UI/General/SoUITypes.h"

#include "SoInputNames.generated.h"

#define SO_CREATE_GAMEPAD_KEY_CONST_GETTER(VariableName)                         \
	public:                                                                      \
		static FORCEINLINE FName VariableName() { return _ ## VariableName; }    \
	private:                                                                     \
		static FName _ ## VariableName;

// Keeps track of all the input key names
struct SORB_API FSoInputKey
{
	typedef FSoInputKey Self;
public:
	// Statically initialize to set some values
	static void Init();

	static const TArray<FKey>& GetAllKeyboardKeys() { return AllKeyboardKeys; }
	static const TArray<FKey>& GetAllGamepadKeys() { return AllGamepadKeys; }

	static FORCEINLINE bool IsGamepadButton(FName ButtonName) { return GamepadButtonNames.Contains(ButtonName); }
	static FORCEINLINE bool IsGamepadFaceButton(FName ButtonName) { return GamepadFaceButtonNames.Contains(ButtonName);  }

	static FORCEINLINE FName GetThumbStickForStickDirection(FName StickDirection)
	{
		if (const FName* NamePtr = GamepadStickDirectionToStickMap.Find(StickDirection))
			return *NamePtr;

		return NAME_None;
	}

	static FORCEINLINE bool IsRightThumbStickDirection(FName StickDirection)
	{
		if (const FName* NamePtr = GamepadStickDirectionToStickMap.Find(StickDirection))
			return Gamepad_RightThumbstick() == *NamePtr;

		return false;
	}

	static FText GetKeyDisplayName(const FKey& Key)
	{
		// Search our overrides
		if (FText* TextPtr = KeyToFTextMap.Find(Key))
			return *TextPtr;

		// Use the default fallback, always in short form
		static constexpr bool bLongDisplayName = false;
		return Key.GetDisplayName(bLongDisplayName);
	}

	static FText GetInputChordDisplayName(const FInputChord& InputChord);

	// Gamepad Button keys, no Axis here
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_LeftThumbstick);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_RightThumbstick);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_Special_Left);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_Special_Right);

	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_FaceButton_Bottom);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_FaceButton_Right);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_FaceButton_Left);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_FaceButton_Top);

	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_LeftShoulder);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_RightShoulder);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_LeftTrigger);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_RightTrigger);

	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_DPad_Up);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_DPad_Down);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_DPad_Right);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_DPad_Left);

	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_LeftStick_Up);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_LeftStick_Down);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_LeftStick_Right);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_LeftStick_Left);

	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_RightStick_Up);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_RightStick_Down);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_RightStick_Right);
	SO_CREATE_GAMEPAD_KEY_CONST_GETTER(Gamepad_RightStick_Left);

protected:
	// Keep track of all keyboard key names from EKeys
	static TArray<FKey> AllKeyboardKeys;

	// Keep track of all gamepad key names from EKeys
	static TArray<FKey> AllGamepadKeys;

	// Contains all the names for the gamepad buttons, no axis names here
	static TSet<FName> GamepadButtonNames;

	// Converts from
	// Key: Stick direction
	// Value: Stick Name
	static TMap<FName, FName> GamepadStickDirectionToStickMap;

	// Converts from
	// Key: Key
	// Value: Localizable FText
	static TMap<FKey, FText> KeyToFTextMap;

	// Keeps track of all the gamepad face buttons names
	static TSet<FName> GamepadFaceButtonNames;

	// Set in Initialize
	static bool bIsInitialized;
};

#undef SO_CREATE_GAMEPAD_KEY_CONST_GETTER

// Keep track of all input axis names
struct SORB_API FSoInputAxisName
{
public:
	// Statically initialize to set some values
	static void Init();

	// Is this axis name editable? Does it exist?
	static FORCEINLINE bool IsValid(const FName AxisName)
	{
		return AllAxisNames.Contains(AxisName);
	}

public:
	static const FName GamepadLeftX;
	static const FName GamepadLeftY;
	static const FName GamepadRightX;
	static const FName GamepadRightY;
	static const FName MoveY;

	// Tied to MoveLeft/MoveRight
	static const FName Move;

private:
	static TSet<FName> AllAxisNames;

	// Set in Initialize
	static bool bIsInitialized;
};


// Enum equivalent of the action names, mainly used to get the UI icons
UENUM(BlueprintType)
enum class ESoInputActionNameType : uint8
{
	// Invalid default state
	IANT_None = 0			UMETA(DisplayName = "None"),

	IANT_MoveUp				UMETA(DisplayName = "Move Up (UI only)"),
	IANT_MoveDown			UMETA(DisplayName = "Move Down (UI only)"),
	IANT_MoveLeft			UMETA(DisplayName = "Move Left"),
	IANT_MoveRight			UMETA(DisplayName = "Move Right"),

	IANT_Strike0			UMETA(DisplayName = "Primary Strike (Strike0)"),
	IANT_Strike1			UMETA(DisplayName = "Special Strike (Strike1)"),

	IANT_Roll				UMETA(DisplayName = "Roll"),
	IANT_Umbrella			UMETA(DisplayName = "Floating (Umbrella)"),
	IANT_Jump				UMETA(DisplayName = "Jump"),

	IANT_ToggleItem			UMETA(DisplayName = "Change Item (ToggleItem)"),
	IANT_ToggleSpell		UMETA(DisplayName = "Change Spell (ToggleSpell)"),
	IANT_ToggleWeapon		UMETA(DisplayName = "Change Weapon (ToggleWeapon)"),
	IANT_TakeWeaponAway		UMETA(DisplayName = "Take Weapon Away (TakeWeaponAway)"),
	IANT_CharacterPanels	UMETA(DisplayName = "Open Inventory (CharacterPanels)"),

	IANT_UseItemFromSlot0	UMETA(DisplayName = "Use Item (UseItemFromSlot0)"),
	IANT_Interact0			UMETA(DisplayName = "Interact (Interact0)"),
	IANT_Interact1			UMETA(DisplayName = "Interact Alternative (Interact1)"),

	IANT_Num				UMETA(Hidden)
};


// The type of input category
// Used so that we can know when to warn users about key conflicts from different categories of input action names
UENUM(BlueprintType)
enum class ESoInputActionCategoryType : uint8
{
	None = 0,

	All,

	Game,
	Editor,
	UI,

	// Game or UI
	GameOrUI
};

// The subtype for the UI input category
// Used so that we know how to warn users if some UI Commands from the same category conflict
UENUM(BlueprintType)
enum class ESoInputActionUICategoryType : uint8
{
	// Not an Action UI command
	None = 0,

	Normal,
	MainMenu,
	Spell
};


// Keep track of all input action names
// Gamepad config helper: https://pjstrnad.com/mapping-of-xbox-controller-to-unreal-engine/
struct SORB_API FSoInputActionName
{
	typedef FSoInputActionName Self;
public:
	// Statically initialize to set some values
	static void Init();

	// Is this action name editable? Does it exist?
	static FORCEINLINE bool IsValid(FName ActionName)
	{
		return IsGameActionName(ActionName) || IsEditorActionName(ActionName) || IsUIActionName(ActionName);
	}

	// Is the ActionName an editor one?
	static FORCEINLINE bool IsEditorActionName(FName ActionName)
	{
		return EditorActionNames.Contains(ActionName);
	}

	// Is the ActionName part of the final game, in the game viewport
	static FORCEINLINE bool IsGameActionName(FName ActionName)
	{
		return GameActionNames.Contains(ActionName);
	}

	// Is the ActionName an UI command?
	static FORCEINLINE bool IsUIActionName(FName ActionName)
	{
		return UIActionNames.Contains(ActionName);
	}

	static FORCEINLINE bool CanIgnoreDuplicateBindingForActionName(FName ActionName)
	{
		return IgnoreDuplicatesActionNames.Contains(ActionName);
	}

	// Is this a valid UI command?
	static FORCEINLINE bool IsValidUICommand(ESoUICommand Command)
	{
		return Command != ESoUICommand::EUC_PressedMax && Command != ESoUICommand::EUC_ReleasedMax;
	}

	// Is this a valid ActionNameType
	static FORCEINLINE bool IsValidActionNameType(ESoInputActionNameType Type)
	{
		return Type != ESoInputActionNameType::IANT_None;
	}

	static FORCEINLINE const TSet<ESoUICommand>& GetUICommandsThatConflictWith(ESoUICommand Command)
	{
		static const TSet<ESoUICommand> EmptySet;
		if (UICommandConflictMap.Contains(Command))
			return UICommandConflictMap.FindChecked(Command);

		return EmptySet;
	}

	// Converts ESoInputActionNameType to FName
	static FORCEINLINE FName ActionNameTypeToActionName(ESoInputActionNameType Type)
	{
		if (const FName* NamePtr = ActionNameTypeToActionNameMap.Find(Type))
			return *NamePtr;

		return NAME_None;
	}

	// Converts ESoInputActionNameType to FName
	static FORCEINLINE ESoInputActionNameType ActionNameToActionNameType(FName ActionName)
	{
		if (const ESoInputActionNameType* TypePtr = ActionNameToActionNameTypeMap.Find(ActionName))
			return *TypePtr;

		return ESoInputActionNameType::IANT_None;
	}

	// Converts between and UI action name to the enum
	static FORCEINLINE ESoUICommand ActionNameToUICommand(FName ActionName)
	{
		if (const ESoUICommand* CommandPtr = ActionNameToUICommandMap.Find(ActionName))
			return *CommandPtr;

		return ESoUICommand::EUC_ReleasedMax;
	}

	// Converts between an ActionName
	// NOTE: This is just the cached version of GetNameFromUICommand
	static FORCEINLINE FName UICommandToActionName(ESoUICommand Command)
	{
		if (const FName* NamePtr = UICommandToActionNameMap.Find(Command))
			return *NamePtr;

		return NAME_None;
	}

	static FORCEINLINE FName GetSyncedActionNameIfModified(FName ActionName)
	{
		if (const FName* NamePtr = ActionNamesSyncIfModifiedMap.Find(ActionName))
			return *NamePtr;

		return NAME_None;
	}

	// Convert Enum to friendly string name
	static FName GetNameFromUICommand(ESoUICommand Command, bool bStripReleasePrefix = true)
	{
		if (Command == ESoUICommand::EUC_PressedMax || Command == ESoUICommand::EUC_ReleasedMax)
			return NAME_None;

		static const int32 UICmdPressedNum = static_cast<int32>(ESoUICommand::EUC_PressedMax);
		const UEnum* EnumPtr = GetUICommandEnum();
		if (!EnumPtr)
			return NAME_None;

		// Remove EUC_ or EUC_R
		const int32 CommandIndex = static_cast<int32>(Command);
		const bool bPressedEnum = CommandIndex < UICmdPressedNum;
		const FString EnumValue = EnumPtr->GetNameStringByIndex(CommandIndex);
		if (EnumValue.IsEmpty())
			return NAME_None;

		if (bPressedEnum || (!bPressedEnum && !bStripReleasePrefix))
			return FName(*EnumValue.Mid(4));

		// Released
		return FName(*EnumValue.Mid(5));
	}

	// Gets the enum
	static const UEnum* GetUICommandEnum()
	{
		static const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ESoUICommand"), true);
		return EnumPtr;
	}

	static FORCEINLINE FText GetTextForActionName(FName ActionName)
	{
		if (FText* TextPtr = ActionNameToFTextMap.Find(ActionName))
			return *TextPtr;

		return FText::GetEmpty();
	}

	static bool IsValidActionNameForCategory(FName ActionName, ESoInputActionCategoryType ForCategoryType);
	static ESoInputActionCategoryType GetCategoryForActionName(FName ActionName, bool bPriorityGame = true);
	static ESoInputActionUICategoryType GetUICategoryForUICommand(ESoUICommand Command);
	static ESoInputActionUICategoryType GetUICategoryForActionName(FName ActionName)
	{
		return GetUICategoryForUICommand(ActionNameToUICommand(ActionName));
	}


public:
	// Game shortcuts
	// NOTE: We only care if we modify these shortcuts, UI shortcuts can't be modified anyways
	static const FName MoveLeft;  // NOTE: also used in UI BUT also corresponds to in game left
	static const FName MoveRight; // NOTE: also used in UI BUT also corresponds to in game right
	static const FName Strike0;
	static const FName Strike1;
	static const FName Roll;
	static const FName Umbrella;
	static const FName TakeWeaponAway;
	static const FName Jump;
	static const FName Interact0;
	static const FName Interact1;
	static const FName UseItemFromSlot0;
	static const FName ToggleWeapon;
	static const FName ToggleItem;
	static const FName ToggleSpell;
	static const FName LockFaceDirection;
	static const FName CharacterPanels;
	static const FName QuickSaveLoad0;
	static const FName QuickSaveLoad1;

	// TODO why do we need a separate one for just he controller
	static const FName StartVideoLoopPlayback;
	static const FName StartVideoLoopPlaybackController;
	static const FName StopVideoLoopPlayback;

	static const FName RestartDemo;

	// UI commands
	// Equivalent of command from ESoUICommand
	// Useful variables:
	// - UIActionNames - Set to see if an FName is an UI action name
	// - ActionNameToUICommandMap - Maps FName => Enum
	// - UICommandToActionNameMap - Maps Enum => Fname
	// NOTE: no need to define here, just use GetNameFromUICommand(ESoUICommand)


	// Editor shortcuts
	// We do not really care if we override this shortcuts
	static const FName EditorCameraEditMode;
	static const FName EditorSkyEditMode;
	static const FName EditorCharShadowEditMode;
	static const FName EditorCreateKey;
	static const FName EditorReloadKeys;
	static const FName EditorSaveKeys;
	static const FName EditorDeleteKey;
	static const FName EditorCopyFromKey;
	static const FName EditorPasteToKey;
	static const FName EditorMoveClosestKeyNodeHere;
	static const FName EditorJumpToNextKey;
	static const FName EditorJumpToPrevKey;
	static const FName EditorSpecialEditButtonPressed0;
	static const FName EditorSpecialEditButtonPressed1;
	static const FName EditorCtrl;
	static const FName EditorMiddleMouseButton;
	static const FName EditorSuperEditMode;
	static const FName EditorDebugFeature;

protected:
	static TSet<FName> GameActionNames;
	static TSet<FName> EditorActionNames;
	static TSet<FName> UIActionNames;

	// For these action names we don't care if there are duplicates.
	static TSet<FName> IgnoreDuplicatesActionNames;

	// Converts from
	// Key: ActionName
	// Value: Localizable FText
	static TMap<FName, FText> ActionNameToFTextMap;

	// Converts from
	// Key: ESoInputActionNameType
	// Value: FName ActionName
	static TMap<ESoInputActionNameType, FName> ActionNameTypeToActionNameMap;

	// Opposite of ActionNameTypeToActionNameMap
	// Key: FName ActionName
	// Value: ESoInputActionNameType
	static TMap<FName, ESoInputActionNameType> ActionNameToActionNameTypeMap;

	// Cache that Converts from
	// Key: ActionName
	// Value: UI command
	static TMap<FName, ESoUICommand> ActionNameToUICommandMap;

	// Opposite of ActionNameToUICommandMap
	// Key: ActionName
	// Value: UI command
	static TMap<ESoUICommand, FName> UICommandToActionNameMap;

	// Key: Command
	// Value: Other Commands it conflicts with
	static TMap<ESoUICommand, TSet<ESoUICommand>> UICommandConflictMap;

	// If the key action name is modified also modify the value action name
	// Key: Action name
	// Value: The value action name to modify if the key is modified
	static TMap<FName, FName> ActionNamesSyncIfModifiedMap;

	// Set in Initialize
	static bool bIsInitialized;
};


// Contains the info about the dependencies between the AxisNames and ActionNames
struct SORB_API FSoInputActionNameAxisNameLink
{
public:
	// Statically initialize to set some values
	static void Init();

	// Gets the Axis names that are linked to an action name. This usually means the axis must also be modified alongside the action name
	static FORCEINLINE const TSet<FName>& GetAxisNamesLinkedToActionName(const FName ActionName)
	{
		if (const auto* NameSet = ActionNameToAxisNamesMap.Find(ActionName))
			return *NameSet;

		return EmptySet;
	}

	// Gets the action names that are linked to an axis name.
	static FORCEINLINE const TSet<FName>& GetActionNamesLinkedToAxisName(const FName AxisName)
	{
		if (const auto* NameSet = AxisNameToActionNamesMap.Find(AxisName))
			return *NameSet;

		return EmptySet;
	}

protected:
	// Key: Axis Name
	// Value: Action names linked to the axis name
	static TMap<FName, TSet<FName>> AxisNameToActionNamesMap;

	// Opposite of AxisNameToActionNameMap
	// Key: Action Name
	// Value: Axis names linked to the action name
	static TMap<FName, TSet<FName>> ActionNameToAxisNamesMap;

	// Used to return empty set by reference
	static const TSet<FName> EmptySet;

	// Set in Initialize
	static bool bIsInitialized;
};


// Action name that can be configured from the UI settings
USTRUCT(BlueprintType)
struct FSoInputConfigurableActionName
{
	GENERATED_USTRUCT_BODY()
	typedef FSoInputConfigurableActionName Self;
public:
	FSoInputConfigurableActionName() {}
	FSoInputConfigurableActionName(FName InActionName, const FText& InDisplayText) :
		ActionName(InActionName), DisplayText(InDisplayText) {}

	// Statically initialize to set some values
	static void Init();

	static const TArray<Self>& GetAllOptions() { return AllOptions; }
	static const TArray<Self>& GetGamepadOptions() { return GamepadOptions; }

	static bool IsGamepadActionName(FName ActionName) { return GamepadOptionsActionNames.Contains(ActionName); }

protected:
	static void AddToAllOptions(FName ActionName)
	{
		AllOptions.Add(Self(ActionName, FSoInputActionName::GetTextForActionName(ActionName)));
	}
	static void AddToGamepadOptions(FName ActionName)
	{
		GamepadOptions.Add(Self(ActionName, FSoInputActionName::GetTextForActionName(ActionName)));
		GamepadOptionsActionNames.Add(ActionName);
	}

public:
	// See Config\DefaultInput.ini and ASoCharacter::SetupPlayerInputComponent
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FName ActionName;

	// The displayed text for this action name
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FText DisplayText;

protected:
	// Set in Initialize
	static bool bIsInitialized;

	// Array of all configurable action names
	static TArray<Self> AllOptions;

	// All the configurable action names by the gamepad
	static TArray<Self> GamepadOptions;

	// Just the Action names from GamepadOptions
	static TSet<FName> GamepadOptionsActionNames;
};
