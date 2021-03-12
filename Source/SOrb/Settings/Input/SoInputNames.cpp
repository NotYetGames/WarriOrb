// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoInputNames.h"

#include "InputCoreTypes.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Localization/SoLocalization.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoInputKey
bool FSoInputKey::bIsInitialized = false;
TArray<FKey> FSoInputKey::AllKeyboardKeys;
TArray<FKey> FSoInputKey::AllGamepadKeys;
TSet<FName> FSoInputKey::GamepadButtonNames;
TSet<FName> FSoInputKey::GamepadFaceButtonNames;
TMap<FName, FName> FSoInputKey::GamepadStickDirectionToStickMap;
TMap<FKey, FText> FSoInputKey::KeyToFTextMap;


#define SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(VariableName) \
	FName FSoInputKey::_##VariableName;

#define SO_ASSSIGN_GAMEPAD_KEY_CONST(VariableName) \
	FSoInputKey::_##VariableName = EKeys::VariableName.GetFName()


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_LeftThumbstick);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_RightThumbstick);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_Special_Left);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_Special_Right);

SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_FaceButton_Bottom);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_FaceButton_Right);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_FaceButton_Left);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_FaceButton_Top);

SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_LeftShoulder);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_RightShoulder);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_LeftTrigger);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_RightTrigger);

SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_DPad_Up);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_DPad_Down);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_DPad_Right);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_DPad_Left);

SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_LeftStick_Up);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_LeftStick_Down);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_LeftStick_Right);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_LeftStick_Left);

SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_RightStick_Up);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_RightStick_Down);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_RightStick_Right);
SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT(Gamepad_RightStick_Left);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoInputKey::Init()
{
	if (bIsInitialized)
		return;
	bIsInitialized = true;

	// GamepadKeys
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_LeftThumbstick);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_RightThumbstick);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_Special_Left);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_Special_Right);

	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_FaceButton_Bottom);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_FaceButton_Right);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_FaceButton_Left);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_FaceButton_Top);

	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_LeftShoulder);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_RightShoulder);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_LeftTrigger);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_RightTrigger);

	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_DPad_Up);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_DPad_Down);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_DPad_Right);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_DPad_Left);

	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_LeftStick_Up);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_LeftStick_Down);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_LeftStick_Right);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_LeftStick_Left);

	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_RightStick_Up);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_RightStick_Down);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_RightStick_Right);
	SO_ASSSIGN_GAMEPAD_KEY_CONST(Gamepad_RightStick_Left);

	AllKeyboardKeys.Empty();
	AllKeyboardKeys = {
		EKeys::Backslash,
		EKeys::Tab,
		EKeys::Enter,

		EKeys::CapsLock,
		EKeys::Escape,
		EKeys::SpaceBar,
		EKeys::PageUp,
		EKeys::PageDown,
		EKeys::End,
		EKeys::Home,

		EKeys::Left,
		EKeys::Up,
		EKeys::Right,
		EKeys::Down,

		EKeys::Insert,
		EKeys::Delete,

		EKeys::Zero,
		EKeys::One,
		EKeys::Two,
		EKeys::Three,
		EKeys::Four,
		EKeys::Five,
		EKeys::Six,
		EKeys::Seven,
		EKeys::Eight,
		EKeys::Nine,

		EKeys::A,
		EKeys::B,
		EKeys::C,
		EKeys::D,
		EKeys::E,
		EKeys::F,
		EKeys::G,
		EKeys::H,
		EKeys::I,
		EKeys::J,
		EKeys::K,
		EKeys::L,
		EKeys::M,
		EKeys::N,
		EKeys::O,
		EKeys::P,
		EKeys::Q,
		EKeys::R,
		EKeys::S,
		EKeys::T,
		EKeys::U,
		EKeys::V,
		EKeys::W,
		EKeys::X,
		EKeys::Y,
		EKeys::Z,

		EKeys::NumPadZero,
		EKeys::NumPadOne,
		EKeys::NumPadTwo,
		EKeys::NumPadThree,
		EKeys::NumPadFour,
		EKeys::NumPadFive,
		EKeys::NumPadSix,
		EKeys::NumPadSeven,
		EKeys::NumPadEight,
		EKeys::NumPadNine,

		EKeys::Multiply,
		EKeys::Add,
		EKeys::Subtract,

		EKeys::F1,
		EKeys::F2,
		EKeys::F3,
		EKeys::F4,
		EKeys::F5,
		EKeys::F6,
		EKeys::F7,
		EKeys::F8,
		EKeys::F9,
		EKeys::F10,
		EKeys::F11,
		EKeys::F12,

		EKeys::NumLock,
		EKeys::LeftShift,
		EKeys::RightShift,
		EKeys::LeftControl,
		EKeys::RightControl,
		EKeys::LeftAlt,
		EKeys::RightAlt,
		EKeys::LeftCommand,
		EKeys::RightCommand,

		EKeys::Tilde,
		EKeys::LeftBracket,
		EKeys::RightBracket,

		EKeys::Semicolon,
		EKeys::Quote,

		// Obscure
		EKeys::Equals,
		EKeys::Comma,
		EKeys::Underscore,
		EKeys::Hyphen,
		EKeys::Period,
		EKeys::Slash,
		EKeys::Apostrophe,
		EKeys::Ampersand,
		EKeys::Asterix,
		EKeys::Caret,
		EKeys::Colon,
		EKeys::Dollar,
		EKeys::Exclamation,
		EKeys::LeftParantheses,
		EKeys::RightParantheses,
		EKeys::Pause,
		EKeys::Decimal,
		EKeys::Divide,
		EKeys::ScrollLock,
	};

	AllGamepadKeys.Empty();
	AllGamepadKeys = {
		EKeys::Gamepad_LeftThumbstick,
		EKeys::Gamepad_RightThumbstick,
		EKeys::Gamepad_Special_Left,
		EKeys::Gamepad_Special_Right,

		EKeys::Gamepad_FaceButton_Bottom,
		EKeys::Gamepad_FaceButton_Right,
		EKeys::Gamepad_FaceButton_Left,
		EKeys::Gamepad_FaceButton_Top,

		EKeys::Gamepad_LeftShoulder,
		EKeys::Gamepad_RightShoulder,
		EKeys::Gamepad_LeftTrigger,
		EKeys::Gamepad_RightTrigger,

		EKeys::Gamepad_DPad_Up,
		EKeys::Gamepad_DPad_Down,
		EKeys::Gamepad_DPad_Right,
		EKeys::Gamepad_DPad_Left,

		EKeys::Gamepad_LeftStick_Up,
		EKeys::Gamepad_LeftStick_Down,
		EKeys::Gamepad_LeftStick_Right,
		EKeys::Gamepad_LeftStick_Left,

		EKeys::Gamepad_RightStick_Up,
		EKeys::Gamepad_RightStick_Down,
		EKeys::Gamepad_RightStick_Right,
		EKeys::Gamepad_RightStick_Left,

		// Axis bellow

		// Gamepad Left Thumbstick X-Axis
		EKeys::Gamepad_LeftX,

		// Gamepad Left Thumbstick Y-Axis
		EKeys::Gamepad_LeftY,

		// Gamepad Right Thumbstick X-Axis
		EKeys::Gamepad_RightX,

		// Gamepad Right Thumbstick Y-Axis
		EKeys::Gamepad_RightY,

		// L2 axis
		EKeys::Gamepad_LeftTriggerAxis,

		// R2 axis
		EKeys::Gamepad_RightTriggerAxis,

		// PS4 touchpad buttons
		EKeys::Gamepad_Special_Left_X,
		EKeys::Gamepad_Special_Left_Y,
	};

	// No duplicates
	const TSet<FKey> KeyboardKeysUnique(AllKeyboardKeys);
	const TSet<FKey> GamepadKeysUnique(AllGamepadKeys);
	verify(KeyboardKeysUnique.Num() == AllKeyboardKeys.Num());
	verify(GamepadKeysUnique.Num() == AllGamepadKeys.Num());

	GamepadButtonNames = {
		Gamepad_LeftThumbstick(),
		Gamepad_RightThumbstick(),
		Gamepad_Special_Left(),
		Gamepad_Special_Right(),

		Gamepad_FaceButton_Bottom(),
		Gamepad_FaceButton_Right(),
		Gamepad_FaceButton_Left(),
		Gamepad_FaceButton_Top(),

		Gamepad_LeftShoulder(),
		Gamepad_RightShoulder(),
		Gamepad_LeftTrigger(),
		Gamepad_RightTrigger(),

		Gamepad_DPad_Up(),
		Gamepad_DPad_Down(),
		Gamepad_DPad_Right(),
		Gamepad_DPad_Left(),

		Gamepad_DPad_Up(),
		Gamepad_DPad_Down(),
		Gamepad_DPad_Right(),
		Gamepad_DPad_Left(),

		Gamepad_LeftStick_Up(),
		Gamepad_LeftStick_Down(),
		Gamepad_LeftStick_Right(),
		Gamepad_LeftStick_Left(),

		Gamepad_RightStick_Up(),
		Gamepad_RightStick_Down(),
		Gamepad_RightStick_Right(),
		Gamepad_RightStick_Left(),
	};
	verify(GamepadButtonNames.Num() > 20);

	GamepadFaceButtonNames = {
		Gamepad_FaceButton_Bottom(),
		Gamepad_FaceButton_Right(),
		Gamepad_FaceButton_Left(),
		Gamepad_FaceButton_Top(),
	};
	verify(GamepadFaceButtonNames.Num() == 4);

	GamepadStickDirectionToStickMap.Empty();
	GamepadStickDirectionToStickMap.Add(Gamepad_LeftThumbstick(), Gamepad_LeftThumbstick());
	GamepadStickDirectionToStickMap.Add(Gamepad_LeftStick_Up(), Gamepad_LeftThumbstick());
	GamepadStickDirectionToStickMap.Add(Gamepad_LeftStick_Down(), Gamepad_LeftThumbstick());
	GamepadStickDirectionToStickMap.Add(Gamepad_LeftStick_Right(), Gamepad_LeftThumbstick());
	GamepadStickDirectionToStickMap.Add(Gamepad_LeftStick_Left(), Gamepad_LeftThumbstick());
	GamepadStickDirectionToStickMap.Add(Gamepad_RightThumbstick(), Gamepad_RightThumbstick());
	GamepadStickDirectionToStickMap.Add(Gamepad_RightStick_Up(), Gamepad_RightThumbstick());
	GamepadStickDirectionToStickMap.Add(Gamepad_RightStick_Down(), Gamepad_RightThumbstick());
	GamepadStickDirectionToStickMap.Add(Gamepad_RightStick_Right(), Gamepad_RightThumbstick());
	GamepadStickDirectionToStickMap.Add(Gamepad_RightStick_Left(), Gamepad_RightThumbstick());

	// Only a subset of the keys in InputCoreTypes.h
#define LOCTEXT_NAMESPACE "InputKeys"
	KeyToFTextMap.Empty();

	KeyToFTextMap.Add(EKeys::AnyKey, LOCTEXT("AnyKey", "Any Key"));

	// KeyToFTextMap.Add(EKeys::LeftMouseButton, LOCTEXT("LeftMouseButton", "Left Mouse Button"));
	// KeyToFTextMap.Add(EKeys::RightMouseButton, LOCTEXT("RightMouseButton", "Right Mouse Button"));
	// KeyToFTextMap.Add(EKeys::MiddleMouseButton, LOCTEXT("MiddleMouseButton", "Middle Mouse Button"));
	// KeyToFTextMap.Add(EKeys::ThumbMouseButton, LOCTEXT("ThumbMouseButton", "Thumb Mouse Button"));
	// KeyToFTextMap.Add(EKeys::ThumbMouseButton2, LOCTEXT("ThumbMouseButton2", "Thumb Mouse Button 2"));

	// KeyToFTextMap.Add(EKeys::Escape, LOCTEXT("Escape", "Escape"));
	// KeyToFTextMap.Add(EKeys::Tab, LOCTEXT("Tab", "Tab"));
	// KeyToFTextMap.Add(EKeys::Enter, LOCTEXT("Enter", "Enter"));
	// KeyToFTextMap.Add(EKeys::CapsLock, LOCTEXT("CapsLock", "Caps Lock"));
	// KeyToFTextMap.Add(EKeys::SpaceBar, LOCTEXT("SpaceBarShort", "Space"));
	// KeyToFTextMap.Add(EKeys::BackSpace, LOCTEXT("BackSpace", "Backspace"));


	// KeyToFTextMap.Add(EKeys::PageUp, LOCTEXT("PageUp", "Page Up"));
	// KeyToFTextMap.Add(EKeys::PageDown, LOCTEXT("PageDown", "Page Down"));
	// KeyToFTextMap.Add(EKeys::End, LOCTEXT("End", "End"));
	// KeyToFTextMap.Add(EKeys::Home, LOCTEXT("Home", "Home"));
	// KeyToFTextMap.Add(EKeys::Insert, LOCTEXT("Insert", "Insert"));
	// KeyToFTextMap.Add(EKeys::Pause, LOCTEXT("Pause", "Pause"));
	// KeyToFTextMap.Add(EKeys::Delete, LOCTEXT("Delete", "Delete"));
	// KeyToFTextMap.Add(EKeys::NumLock, LOCTEXT("NumLock", "Num Lock"));
	// KeyToFTextMap.Add(EKeys::ScrollLock, LOCTEXT("ScrollLock", "Scroll Lock"));

	KeyToFTextMap.Add(EKeys::Left, LOCTEXT("Left", "Left"));
	KeyToFTextMap.Add(EKeys::Up, LOCTEXT("Up", "Up"));
	KeyToFTextMap.Add(EKeys::Right, LOCTEXT("Right", "Right"));
	KeyToFTextMap.Add(EKeys::Down, LOCTEXT("Down", "Down"));

#if PLATFORM_XBOXONE
	KeyToFTextMap.Add(EKeys::Gamepad_Special_Right, FROM_STRING_TABLE_UI("gamepad_special_right_xbox"));
	KeyToFTextMap.Add(EKeys::Gamepad_Special_Left, FROM_STRING_TABLE_UI("gamepad_special_left_xbox"));
#endif

#if PLATFORM_SWITCH
	KeyToFTextMap.Add(EKeys::Gamepad_FaceButton_Top, FROM_STRING_TABLE_UI("Switch_FaceButton_Top"));
	KeyToFTextMap.Add(EKeys::Gamepad_FaceButton_Bottom, FROM_STRING_TABLE_UI("Switch_FaceButton_Bottom"));
	KeyToFTextMap.Add(EKeys::Gamepad_FaceButton_Left, FROM_STRING_TABLE_UI("Switch_FaceButton_Left"));
	KeyToFTextMap.Add(EKeys::Gamepad_FaceButton_Right, FROM_STRING_TABLE_UI("Switch_FaceButton_Right"));

	KeyToFTextMap.Add(EKeys::Gamepad_LeftTrigger, FROM_STRING_TABLE_UI("Switch_Trigger_Left"));
	KeyToFTextMap.Add(EKeys::Gamepad_LeftShoulder, FROM_STRING_TABLE_UI("Switch_Shoulder_Left"));
	KeyToFTextMap.Add(EKeys::Gamepad_RightTrigger, FROM_STRING_TABLE_UI("Switch_Trigger_Right"));
	KeyToFTextMap.Add(EKeys::Gamepad_RightShoulder, FROM_STRING_TABLE_UI("Switch_Shoulder_Right"));

	KeyToFTextMap.Add(EKeys::Gamepad_Special_Right, FROM_STRING_TABLE_UI("Switch_special_right"));
	KeyToFTextMap.Add(EKeys::Gamepad_Special_Left, FROM_STRING_TABLE_UI("Switch_special_left"));

	KeyToFTextMap.Add(EKeys::Gamepad_DPad_Up, FROM_STRING_TABLE_UI("Switch_special_dpad_up"));
	KeyToFTextMap.Add(EKeys::Gamepad_DPad_Down, FROM_STRING_TABLE_UI("Switch_special_dpad_down"));
	KeyToFTextMap.Add(EKeys::Gamepad_DPad_Left, FROM_STRING_TABLE_UI("Switch_special_dpad_left"));
	KeyToFTextMap.Add(EKeys::Gamepad_DPad_Right, FROM_STRING_TABLE_UI("Switch_special_dpad_right"));
	KeyToFTextMap.Add(EKeys::Gamepad_LeftThumbstick, FROM_STRING_TABLE_UI("Switch_LeftThumbstick"));
	KeyToFTextMap.Add(EKeys::Gamepad_RightThumbstick, FROM_STRING_TABLE_UI("Switch_RightThumbstick"));
#endif

	KeyToFTextMap.Add(EKeys::Gamepad_LeftStick_Up, FROM_STRING_TABLE_UI("Switch_Gamepad_LeftStick_Up"));
	KeyToFTextMap.Add(EKeys::Gamepad_LeftStick_Down, FROM_STRING_TABLE_UI("Switch_Gamepad_LeftStick_Down"));
	KeyToFTextMap.Add(EKeys::Gamepad_LeftStick_Right, FROM_STRING_TABLE_UI("Switch_Gamepad_LeftStick_Right"));
	KeyToFTextMap.Add(EKeys::Gamepad_LeftStick_Left, FROM_STRING_TABLE_UI("Switch_Gamepad_LeftStick_Left"));
	KeyToFTextMap.Add(EKeys::Gamepad_RightStick_Up, FROM_STRING_TABLE_UI("Switch_Gamepad_RightStick_Up"));
	KeyToFTextMap.Add(EKeys::Gamepad_RightStick_Down, FROM_STRING_TABLE_UI("Switch_Gamepad_RightStick_Down"));
	KeyToFTextMap.Add(EKeys::Gamepad_RightStick_Right, FROM_STRING_TABLE_UI("Switch_Gamepad_RightStick_Right"));
	KeyToFTextMap.Add(EKeys::Gamepad_RightStick_Left, FROM_STRING_TABLE_UI("Switch_Gamepad_RightStick_Left"));

	// KeyToFTextMap.Add(EKeys::LeftShift, LOCTEXT("LeftShift", "Left Shift"));
	// KeyToFTextMap.Add(EKeys::RightShift, LOCTEXT("RightShift", "Right Shift"));
	// KeyToFTextMap.Add(EKeys::LeftControl, LOCTEXT("LeftControl", "Left Ctrl"));
	// KeyToFTextMap.Add(EKeys::RightControl, LOCTEXT("RightControl", "Right Ctrl"));
	// KeyToFTextMap.Add(EKeys::LeftAlt, LOCTEXT("LeftAlt", "Left Alt"));
	// KeyToFTextMap.Add(EKeys::RightAlt, LOCTEXT("RightAlt", "Right Alt"));
#undef LOCTEXT_NAMESPACE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText FSoInputKey::GetInputChordDisplayName(const FInputChord& InputChord)
{
	// Modified from FInputChord::GetInputText
#if PLATFORM_MAC
	const FText CommandText = Text::FromString(TEXT("Ctrl"));
	const FText ControlText = Text::FromString(TEXT("Cmd"));
#else
	const FText ControlText = FText::FromString(TEXT("Ctrl"));
	const FText CommandText = FText::FromString(TEXT("Cmd"));
#endif
	const FText AltText = FText::FromString(TEXT("Alt"));
	const FText ShiftText = FText::FromString(TEXT("Shift"));


	const FText AppenderText = InputChord.Key != EKeys::Invalid ? FText::FromString(TEXT("+")) : FText::GetEmpty();

	FFormatNamedArguments Args;
	int32 ModCount = 0;

	if (InputChord.bCtrl)
	{
		Args.Add(FString::Printf(TEXT("Mod%d"), ++ModCount), ControlText);
	}
	if (InputChord.bCmd)
	{
		Args.Add(FString::Printf(TEXT("Mod%d"), ++ModCount), CommandText);
	}
	if (InputChord.bAlt)
	{
		Args.Add(FString::Printf(TEXT("Mod%d"), ++ModCount), AltText);
	}
	if (InputChord.bShift)
	{
		Args.Add(FString::Printf(TEXT("Mod%d"), ++ModCount), ShiftText);
	}

	for (int32 i = 1; i <= 4; ++i)
	{
		if (i > ModCount)
		{
			Args.Add(FString::Printf(TEXT("Mod%d"), i), FText::GetEmpty());
			Args.Add(FString::Printf(TEXT("Appender%d"), i), FText::GetEmpty());
		}
		else
		{
			Args.Add(FString::Printf(TEXT("Appender%d"), i), AppenderText);
		}

	}

	FText KeyText;
	if (InputChord.Key.IsValid() && !InputChord.Key.IsModifierKey())
	{
		KeyText = GetKeyDisplayName(InputChord.Key);
	}
	Args.Add(TEXT("Key"), KeyText);

	return FText::Format(FText::FromString(TEXT("{Mod1}{Appender1}{Mod2}{Appender2}{Mod3}{Appender3}{Mod4}{Appender4}{Key}")), Args);
}


#undef SO_CREATE_GAMEPAD_KEY_CONST_STATIC_INIT


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoInputAxisName
bool FSoInputAxisName::bIsInitialized = false;
TSet<FName> FSoInputAxisName::AllAxisNames;

const FName FSoInputAxisName::GamepadLeftX(TEXT("GamepadLeftX"));
const FName FSoInputAxisName::GamepadLeftY(TEXT("GamepadLeftY"));
const FName FSoInputAxisName::GamepadRightX(TEXT("GamepadRightX"));
const FName FSoInputAxisName::GamepadRightY(TEXT("GamepadRightY"));
const FName FSoInputAxisName::Move(TEXT("Move"));
const FName FSoInputAxisName::MoveY(TEXT("MoveY"));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoInputAxisName::Init()
{
	if (bIsInitialized)
		return;
	bIsInitialized = true;

	AllAxisNames.Empty();
	AllAxisNames = {
		GamepadLeftX,
		GamepadLeftY,
		GamepadRightX,
		GamepadRightY,
		Move,
		MoveY
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoInputActionName
bool FSoInputActionName::bIsInitialized = false;
TSet<FName> FSoInputActionName::GameActionNames;
TSet<FName> FSoInputActionName::EditorActionNames;
TSet<FName> FSoInputActionName::UIActionNames;
TSet<FName> FSoInputActionName::IgnoreDuplicatesActionNames;
TMap<FName, FText> FSoInputActionName::ActionNameToFTextMap;
TMap<ESoInputActionNameType, FName> FSoInputActionName::ActionNameTypeToActionNameMap;
TMap<FName, ESoInputActionNameType> FSoInputActionName::ActionNameToActionNameTypeMap;
TMap<FName, ESoUICommand> FSoInputActionName::ActionNameToUICommandMap;
TMap<ESoUICommand, FName> FSoInputActionName::UICommandToActionNameMap;
TMap<ESoUICommand, TSet<ESoUICommand>> FSoInputActionName::UICommandConflictMap;
TMap<FName, FName> FSoInputActionName::ActionNamesSyncIfModifiedMap;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Game shortcuts
const FName FSoInputActionName::MoveLeft(TEXT("Left"));
const FName FSoInputActionName::MoveRight(TEXT("Right"));
const FName FSoInputActionName::Strike0(TEXT("Strike0"));
const FName FSoInputActionName::Strike1(TEXT("Strike1"));
const FName FSoInputActionName::Roll(TEXT("Roll"));
const FName FSoInputActionName::Umbrella(TEXT("Umbrella"));
const FName FSoInputActionName::TakeWeaponAway(TEXT("TakeWeaponAway"));
const FName FSoInputActionName::Jump(TEXT("Jump"));
const FName FSoInputActionName::Interact0(TEXT("Interact0"));
const FName FSoInputActionName::Interact1(TEXT("Interact1"));
const FName FSoInputActionName::UseItemFromSlot0(TEXT("UseItemFromSlot0"));
const FName FSoInputActionName::ToggleWeapon(TEXT("ToggleWeapon"));
const FName FSoInputActionName::ToggleItem(TEXT("ToggleItem"));
const FName FSoInputActionName::ToggleSpell(TEXT("ToggleSpell"));
const FName FSoInputActionName::LockFaceDirection(TEXT("LockFaceDirection"));
const FName FSoInputActionName::CharacterPanels(TEXT("CharacterPanels"));
const FName FSoInputActionName::QuickSaveLoad0(TEXT("QuickSaveLoad0"));
const FName FSoInputActionName::QuickSaveLoad1(TEXT("QuickSaveLoad1"));
const FName FSoInputActionName::StartVideoLoopPlayback(TEXT("StartVideoLoopPlayback"));
const FName FSoInputActionName::StartVideoLoopPlaybackController(TEXT("StartVideoLoopPlaybackController"));
const FName FSoInputActionName::StopVideoLoopPlayback(TEXT("StopVideoLoopPlayback"));
const FName FSoInputActionName::RestartDemo(TEXT("RestartDemo"));


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Editor shortcuts
const FName FSoInputActionName::EditorCameraEditMode(TEXT("CameraEditMode"));
const FName FSoInputActionName::EditorSkyEditMode(TEXT("SkyEditMode"));
const FName FSoInputActionName::EditorCharShadowEditMode(TEXT("CharShadowEditMode"));
const FName FSoInputActionName::EditorCreateKey(TEXT("CreateKey"));
const FName FSoInputActionName::EditorReloadKeys(TEXT("ReloadKeys"));
const FName FSoInputActionName::EditorSaveKeys(TEXT("SaveKeys"));
const FName FSoInputActionName::EditorDeleteKey(TEXT("DeleteKey"));
const FName FSoInputActionName::EditorCopyFromKey(TEXT("CopyFromKey"));
const FName FSoInputActionName::EditorPasteToKey(TEXT("PasteToKey"));
const FName FSoInputActionName::EditorMoveClosestKeyNodeHere(TEXT("MoveClosestKeyNodeHere"));
const FName FSoInputActionName::EditorJumpToNextKey(TEXT("JumpToNextKey"));
const FName FSoInputActionName::EditorJumpToPrevKey(TEXT("JumpToPrevKey"));
const FName FSoInputActionName::EditorSpecialEditButtonPressed0(TEXT("SpecialEditButtonPressed0"));
const FName FSoInputActionName::EditorSpecialEditButtonPressed1(TEXT("SpecialEditButtonPressed1"));
const FName FSoInputActionName::EditorCtrl(TEXT("Ctrl"));
const FName FSoInputActionName::EditorMiddleMouseButton(TEXT("MMB"));
const FName FSoInputActionName::EditorSuperEditMode(TEXT("SuperEditMode"));
const FName FSoInputActionName::EditorDebugFeature(TEXT("DebugFeature"));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoInputActionName::Init()
{
	if (bIsInitialized)
		return;
	bIsInitialized = true;

	// GameActionNames Set
	GameActionNames.Empty();
	GameActionNames = {
		MoveLeft,
		MoveRight,
		Strike0,
		Strike1,
		Roll,
		Umbrella,
		TakeWeaponAway,
		Jump,
		Interact0,
		Interact1,
		UseItemFromSlot0,
		ToggleWeapon,
		ToggleItem,
		ToggleSpell,
		LockFaceDirection,
		CharacterPanels,
		QuickSaveLoad0,
		QuickSaveLoad1,

		// Does this break the video demo?
		// NOTE: if you enable this the game controller settings will show an invalid texture if the button shares the same key
		StartVideoLoopPlayback,
		StartVideoLoopPlaybackController,
		StopVideoLoopPlayback
	};

	// EditorActionNames Set
	EditorActionNames.Empty();
	EditorActionNames = {
		EditorCameraEditMode,
		EditorSkyEditMode,
		EditorCharShadowEditMode,
		EditorCreateKey,
		EditorReloadKeys,
		EditorSaveKeys,
		EditorDeleteKey,
		EditorCopyFromKey,
		EditorPasteToKey,
		EditorMoveClosestKeyNodeHere,
		EditorJumpToNextKey,
		EditorJumpToPrevKey,
		EditorSpecialEditButtonPressed0,
		EditorSpecialEditButtonPressed1,
		EditorCtrl,
		EditorMiddleMouseButton,
		EditorSuperEditMode,
		EditorDebugFeature
	};

	// Fill UI
	{
		ActionNameToUICommandMap.Empty();
		const int32 CommandNum = static_cast<int32>(ESoUICommand::EUC_ReleasedMax);
		for (int32 Index = 0; Index < CommandNum; Index++)
		{
			const ESoUICommand Command = static_cast<ESoUICommand>(Index);
			const FName CommandName = GetNameFromUICommand(Command, false);
			if (CommandName != NAME_None)
				ActionNameToUICommandMap.Add(CommandName, Command);
		}
		verify(ActionNameToUICommandMap.Num() == CommandNum - 1);

		// Add to inverse and fill UI commands
		UICommandToActionNameMap.Empty();
		UIActionNames.Empty();
		for (const auto& Elem : ActionNameToUICommandMap)
		{
			verify(Elem.Key != NAME_None);
			verify(IsValidUICommand(Elem.Value));

			UIActionNames.Add(Elem.Key);
			UICommandToActionNameMap.Add(Elem.Value, Elem.Key);
		}

		verify(UICommandToActionNameMap.Num() == ActionNameToUICommandMap.Num());
		verify(UIActionNames.Num() == UICommandToActionNameMap.Num());

		// Conflict map
		UICommandConflictMap.Empty();
		TMap<ESoUICommand, TSet<ESoUICommand>> ConflictMap;
		ConflictMap.Add(ESoUICommand::EUC_Left, { ESoUICommand::EUC_MainMenuLeft });
		ConflictMap.Add(ESoUICommand::EUC_Right, { ESoUICommand::EUC_MainMenuRight });
		ConflictMap.Add(ESoUICommand::EUC_Up, { ESoUICommand::EUC_MainMenuUp });
		ConflictMap.Add(ESoUICommand::EUC_Down, { ESoUICommand::EUC_MainMenuDown });
		//ConflictMap.Add(ESoUICommand::EUC_ActionBack, { ESoUICommand::EUC_MainMenuBack });

		// Complete our map
		for (const auto& Elem : ConflictMap)
		{
			const ESoUICommand Command = Elem.Key;
			UICommandConflictMap.FindOrAdd(Command).Append(Elem.Value);

			// add value as keys aka the inverse
			for (const ESoUICommand SetElem : Elem.Value)
			{
				UICommandConflictMap.FindOrAdd(SetElem).Add(Command);
			}
		}

		verify(UICommandConflictMap.Num() > ConflictMap.Num());
	}


	// Fill action names to sync if modified
	ActionNamesSyncIfModifiedMap.Add(UICommandToActionName(ESoUICommand::EUC_Left), UICommandToActionName(ESoUICommand::EUC_MainMenuLeft));
	ActionNamesSyncIfModifiedMap.Add(UICommandToActionName(ESoUICommand::EUC_Right), UICommandToActionName(ESoUICommand::EUC_MainMenuRight));
	ActionNamesSyncIfModifiedMap.Add(UICommandToActionName(ESoUICommand::EUC_Up), UICommandToActionName(ESoUICommand::EUC_MainMenuUp));
	ActionNamesSyncIfModifiedMap.Add(UICommandToActionName(ESoUICommand::EUC_Down), UICommandToActionName(ESoUICommand::EUC_MainMenuDown));

	// Ignore duplicates
	IgnoreDuplicatesActionNames.Empty();
	IgnoreDuplicatesActionNames = {
		// UICommandToActionName(ESoUICommand::EUC_Up),
		// UICommandToActionName(ESoUICommand::EUC_Down),
		// TODO do we need to add left/right?

		StartVideoLoopPlayback,
		StartVideoLoopPlaybackController,
		StopVideoLoopPlayback,

		// NOTE: this is kinda a hack because we are ignoring some duplicates UI commands
		//UICommandToActionName(ESoUICommand::EUC_MainMenuLeft),
		//UICommandToActionName(ESoUICommand::EUC_MainMenuRight),
		//UICommandToActionName(ESoUICommand::EUC_MainMenuUp),
		//UICommandToActionName(ESoUICommand::EUC_MainMenuDown),
		//UICommandToActionName(ESoUICommand::EUC_MainMenuEnter),
		//UICommandToActionName(ESoUICommand::EUC_MainMenuBack),

		//// TODO configure spells
		//UICommandToActionName(ESoUICommand::EUC_SpellLeft),
		//UICommandToActionName(ESoUICommand::EUC_SpellRight),
		//UICommandToActionName(ESoUICommand::EUC_SpellUp),
		//UICommandToActionName(ESoUICommand::EUC_SpellDown),
		//UICommandToActionName(ESoUICommand::EUC_SpellSelect),
		//UICommandToActionName(ESoUICommand::EUC_SpellSelectAndCast)
	};

	// Verify that categories do not intersect with any editor action names
	verify(GameActionNames.Intersect(EditorActionNames).Num() == 0);
	verify(UIActionNames.Intersect(EditorActionNames).Num() == 0);

	// ESoInputActionNameType maps
	{
		ActionNameTypeToActionNameMap.Empty();
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_None, NAME_None);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_MoveLeft, MoveLeft);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_MoveRight, MoveRight);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_MoveUp, UICommandToActionName(ESoUICommand::EUC_Up));
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_MoveDown, UICommandToActionName(ESoUICommand::EUC_Down));

		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_Strike0, Strike0);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_Strike1, Strike1);

		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_Roll, Roll);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_Umbrella, Umbrella);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_Jump, Jump);

		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_ToggleItem, ToggleItem);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_ToggleSpell, ToggleSpell);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_ToggleWeapon, ToggleWeapon);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_TakeWeaponAway, TakeWeaponAway);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_CharacterPanels, CharacterPanels);

		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_UseItemFromSlot0, UseItemFromSlot0);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_Interact0, Interact0);
		ActionNameTypeToActionNameMap.Add(ESoInputActionNameType::IANT_Interact1, Interact1);

		// Add to inverse
		ActionNameToActionNameTypeMap.Empty();
		for (const auto& Elem : ActionNameTypeToActionNameMap)
		{
			const ESoInputActionNameType NameType = Elem.Key;
			const FName Name = Elem.Value;

			// sanity check
			if (NameType != ESoInputActionNameType::IANT_None)
				verify(IsValid(Name));

			// It is Inverse
			ActionNameToActionNameTypeMap.Add(Name, NameType);
		}

		verify(ActionNameTypeToActionNameMap.Num() == static_cast<int32>(ESoInputActionNameType::IANT_Num));
		verify(ActionNameTypeToActionNameMap.Num() == ActionNameToActionNameTypeMap.Num());
	}

	// ActionNameToFTextMap Map
	ActionNameToFTextMap.Empty();
	ActionNameToFTextMap.Add(MoveLeft, FROM_STRING_TABLE_UI("input_left"));
	ActionNameToFTextMap.Add(MoveRight, FROM_STRING_TABLE_UI("input_right"));
	ActionNameToFTextMap.Add(Strike0, FROM_STRING_TABLE_UI("input_strike_primary"));
	ActionNameToFTextMap.Add(Strike1, FROM_STRING_TABLE_UI("input_strike_special"));
	ActionNameToFTextMap.Add(Roll, FROM_STRING_TABLE_UI("input_roll"));
	ActionNameToFTextMap.Add(Umbrella, FROM_STRING_TABLE_UI("input_float"));
	ActionNameToFTextMap.Add(TakeWeaponAway, FROM_STRING_TABLE_UI("input_take_weapon"));
	ActionNameToFTextMap.Add(Jump, FROM_STRING_TABLE_UI("input_jump"));
	ActionNameToFTextMap.Add(Interact0, FROM_STRING_TABLE_UI("input_interact"));
	ActionNameToFTextMap.Add(Interact1, FROM_STRING_TABLE_UI("input_alt_interact"));
	ActionNameToFTextMap.Add(UseItemFromSlot0, FROM_STRING_TABLE_UI("input_use_item"));
	ActionNameToFTextMap.Add(ToggleItem, FROM_STRING_TABLE_UI("input_change_item"));
	ActionNameToFTextMap.Add(ToggleSpell, FROM_STRING_TABLE_UI("input_change_spell"));
	ActionNameToFTextMap.Add(ToggleWeapon, FROM_STRING_TABLE_UI("input_change_weapon"));
	ActionNameToFTextMap.Add(CharacterPanels, FROM_STRING_TABLE_UI("input_open_inventory"));

	// UI
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_MainMenuBack), FROM_STRING_TABLE_UI("input_menu_back"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_Up), FROM_STRING_TABLE_UI("input_up"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_Down), FROM_STRING_TABLE_UI("input_down"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_Action0), FROM_STRING_TABLE_UI("input_action_first"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_Action1), FROM_STRING_TABLE_UI("input_action_second"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_Action2), FROM_STRING_TABLE_UI("input_action_third"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_TopLeft), FROM_STRING_TABLE_UI("input_top_left"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_TopRight), FROM_STRING_TABLE_UI("input_top_right"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_ActionBack), FROM_STRING_TABLE_UI("input_action_back"));

	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_SpellLeft), FROM_STRING_TABLE_UI("input_spell_left"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_SpellRight), FROM_STRING_TABLE_UI("input_spell_right"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_SpellUp), FROM_STRING_TABLE_UI("input_spell_up"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_SpellDown), FROM_STRING_TABLE_UI("input_spell_down"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_SpellSelect), FROM_STRING_TABLE_UI("input_spell_select"));
	ActionNameToFTextMap.Add(UICommandToActionName(ESoUICommand::EUC_SpellSelectAndCast), FROM_STRING_TABLE_UI("input_spell_select_and_cast"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoInputActionName::IsValidActionNameForCategory(FName ActionName, ESoInputActionCategoryType ForCategoryType)
{
	switch (ForCategoryType)
	{
	case ESoInputActionCategoryType::Game:
		return Self::IsGameActionName(ActionName);
	case ESoInputActionCategoryType::Editor:
		return Self::IsEditorActionName(ActionName);
	case ESoInputActionCategoryType::UI:
		return Self::IsUIActionName(ActionName);
	case ESoInputActionCategoryType::GameOrUI:
		return Self::IsGameActionName(ActionName) || Self::IsUIActionName(ActionName);

	default:
		checkNoEntry();
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputActionCategoryType FSoInputActionName::GetCategoryForActionName(FName ActionName, bool bPriorityGame)
{
	const bool bIsGame = Self::IsGameActionName(ActionName);
	const bool bIsUI = Self::IsUIActionName(ActionName);

	if (bPriorityGame)
	{
		// Return Game first
		if (bIsGame)
			return ESoInputActionCategoryType::Game;
		if (bIsUI)
			return ESoInputActionCategoryType::UI;
	}
	else
	{
		// Return UI first
		if (bIsUI)
			return ESoInputActionCategoryType::UI;
		if (bIsGame)
			return ESoInputActionCategoryType::Game;
	}

	if (Self::IsEditorActionName(ActionName))
		return ESoInputActionCategoryType::Editor;

	return ESoInputActionCategoryType::None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputActionUICategoryType FSoInputActionName::GetUICategoryForUICommand(ESoUICommand Command)
{
	switch (Command)
	{
	case ESoUICommand::EUC_Left:
	case ESoUICommand::EUC_Right:
	case ESoUICommand::EUC_Up:
	case ESoUICommand::EUC_Down:
	case ESoUICommand::EUC_TopLeft:
	case ESoUICommand::EUC_TopRight:
	case ESoUICommand::EUC_Action0:
	case ESoUICommand::EUC_Action1:
	case ESoUICommand::EUC_Action2:
	case ESoUICommand::EUC_ActionBack:
	case ESoUICommand::EUC_RAction2:
		return ESoInputActionUICategoryType::Normal;

	case ESoUICommand::EUC_MainMenuLeft:
	case ESoUICommand::EUC_MainMenuRight:
	case ESoUICommand::EUC_MainMenuUp:
	case ESoUICommand::EUC_MainMenuDown:
	case ESoUICommand::EUC_MainMenuEnter:
	case ESoUICommand::EUC_MainMenuBack:
		return ESoInputActionUICategoryType::MainMenu;

	case ESoUICommand::EUC_SpellLeft:
	case ESoUICommand::EUC_SpellRight:
	case ESoUICommand::EUC_SpellUp:
	case ESoUICommand::EUC_SpellDown:
	case ESoUICommand::EUC_SpellSelect:
	case ESoUICommand::EUC_SpellSelectAndCast:
		return ESoInputActionUICategoryType::Spell;

	default:
		break;
	}

	return ESoInputActionUICategoryType::None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoInputActionNameAxisNameLink
bool FSoInputActionNameAxisNameLink::bIsInitialized = false;
TMap<FName, TSet<FName>> FSoInputActionNameAxisNameLink::AxisNameToActionNamesMap;
TMap<FName, TSet<FName>> FSoInputActionNameAxisNameLink::ActionNameToAxisNamesMap;
const TSet<FName> FSoInputActionNameAxisNameLink::EmptySet;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoInputActionNameAxisNameLink::Init()
{
	if (bIsInitialized)
		return;
	bIsInitialized = true;

	AxisNameToActionNamesMap.Empty();
	AxisNameToActionNamesMap.Add(FSoInputAxisName::Move, { FSoInputActionName::MoveLeft, FSoInputActionName::MoveRight });

	// Add to inverse
	for (const auto& Elem : AxisNameToActionNamesMap)
	{
		const FName AxisName = Elem.Key;
		verify(FSoInputAxisName::IsValid(AxisName)); // sanity check

		// It is inverse
		for (const FName ActionName : Elem.Value)
		{
			verify(FSoInputActionName::IsValid(ActionName)) // sanity check
			ActionNameToAxisNamesMap.FindOrAdd(ActionName).Add(AxisName);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoInputConfigurableActionName
bool FSoInputConfigurableActionName::bIsInitialized = false;
TArray<FSoInputConfigurableActionName> FSoInputConfigurableActionName::AllOptions;
TArray<FSoInputConfigurableActionName> FSoInputConfigurableActionName::GamepadOptions;
TSet<FName> FSoInputConfigurableActionName::GamepadOptionsActionNames;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoInputConfigurableActionName::Init()
{
	if (bIsInitialized)
		return;
	bIsInitialized = true;

	AllOptions.Empty();
	AddToAllOptions(FSoInputActionName::MoveLeft);
	AddToAllOptions(FSoInputActionName::MoveRight);
	AddToAllOptions(FSoInputActionName::Jump);
	AddToAllOptions(FSoInputActionName::Roll);
	AddToAllOptions(FSoInputActionName::UseItemFromSlot0);
	AddToAllOptions(FSoInputActionName::Strike0);
	AddToAllOptions(FSoInputActionName::Strike1);
	AddToAllOptions(FSoInputActionName::TakeWeaponAway);
	AddToAllOptions(FSoInputActionName::ToggleWeapon);
	AddToAllOptions(FSoInputActionName::ToggleItem);
	AddToAllOptions(FSoInputActionName::ToggleSpell);
	AddToAllOptions(FSoInputActionName::CharacterPanels);
	AddToAllOptions(FSoInputActionName::Interact0);
	AddToAllOptions(FSoInputActionName::Interact1);
	AddToAllOptions(FSoInputActionName::Umbrella);

	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_Down));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_Up));

	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_Action0));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_Action1));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_Action2));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_TopLeft));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_TopRight));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_ActionBack));

	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_SpellLeft));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_SpellRight));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_SpellUp));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_SpellDown));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_SpellSelect));
	AddToAllOptions(FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_SpellSelectAndCast));

	GamepadOptions.Empty();
	GamepadOptionsActionNames.Empty();
	AddToGamepadOptions(FSoInputActionName::Jump);
	AddToGamepadOptions(FSoInputActionName::Roll);
	AddToGamepadOptions(FSoInputActionName::UseItemFromSlot0);
	AddToGamepadOptions(FSoInputActionName::Strike0);
	AddToGamepadOptions(FSoInputActionName::Strike1);
	AddToGamepadOptions(FSoInputActionName::TakeWeaponAway);
	AddToGamepadOptions(FSoInputActionName::ToggleWeapon);
	AddToGamepadOptions(FSoInputActionName::ToggleItem);
	AddToGamepadOptions(FSoInputActionName::ToggleSpell);
	AddToGamepadOptions(FSoInputActionName::CharacterPanels);
	AddToGamepadOptions(FSoInputActionName::Interact0);
	AddToGamepadOptions(FSoInputActionName::Interact1);
	AddToGamepadOptions(FSoInputActionName::Umbrella);
}
