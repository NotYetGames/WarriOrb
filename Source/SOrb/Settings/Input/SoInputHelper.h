// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/PlayerInput.h"

#include "SoInputNames.h"
#include "SoInputSettingsTypes.h"
#include "Basic/Helpers/SoPlatformHelper.h"

#include "SoInputHelper.generated.h"

/** Helper methods for the input */
UCLASS()
class SORB_API USoInputHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Input", meta = (WorldContext = "WorldContextObject"))
	static void SetGameInputBlockedByUI(UObject* WorldContextObject, bool bValue);

	UFUNCTION(BlueprintPure, Category = "Input", meta = (WorldContext = "WorldContextObject"))
	static bool IsLastInputFromGamepad(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Input", meta = (WorldContext = "WorldContextObject"))
	static bool IsLastInputFromKeyboard(const UObject* WorldContextObject) { return !IsLastInputFromGamepad(WorldContextObject); }

	// Gets the current player input device
	UFUNCTION(BlueprintPure, Category = "Input", meta = (WorldContext = "WorldContextObject"))
	static ESoInputDeviceType GetCurrentDeviceType(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Input", meta = (WorldContext = "WorldContextObject"))
	static ESoInputDeviceType GetCurrentGamepadDeviceTypeFromSettings(UObject* WorldContextObject);

	// Gets the Axis names that are linked to an action name. This usually means the axis must also be modified alongside the action name
	static FORCEINLINE const TSet<FName>& GetAxisNamesLinkedToActionName(const FName ActionName)
	{
		return FSoInputActionNameAxisNameLink::GetAxisNamesLinkedToActionName(ActionName);
	}

	// Gets the action names that are linked to an axis name.
	static FORCEINLINE const TSet<FName>& GetActionNamesLinkedToAxisName(const FName AxisName)
	{
		return FSoInputActionNameAxisNameLink::GetActionNamesLinkedToAxisName(AxisName);
	}

	// Gets the first FInputActionKeyMapping from the array, if any
	UFUNCTION(BlueprintPure, Category = "Input", meta = (WorldContext = "WorldContextObject"))
	static FORCEINLINE bool GetFirstInputActionKeyMapping(const TArray<FInputActionKeyMapping>& ActionMappings, FInputActionKeyMapping& OutFirst)
	{
		OutFirst = {};
		if (ActionMappings.Num() > 0)
		{
			OutFirst = ActionMappings[0];
			return true;
		}
		return false;
	}

	// Is the Key an allowed input keyboard key?
	static FORCEINLINE bool IsAllowedInputKeyboardKey(const FInputChord& InputChord)
	{
		// All keyboard keys are valid but without any modifier keys
		// OR it can be a modified key only
		return (IsKeyboardKey(InputChord.Key) && !InputChord.HasAnyModifierKeys()) || IsASingleModifierKey(InputChord);
	}

	// Checks InputChord has only one modifier key set
	static FORCEINLINE bool IsASingleModifierKey(const FInputChord& InputChord)
	{
		const uint8 SumValue = static_cast<uint8>(InputChord.bAlt)
			+ static_cast<uint8>(InputChord.bCtrl)
			+ static_cast<uint8>(InputChord.bCmd)
			+ static_cast<uint8>(InputChord.bShift);

		// SumValue == 1 means that only one modifier key is set, similar to HasAnyModifierKeys()
		return !InputChord.Key.IsValid() && SumValue == 1;
	}

	// Is the key an allowed gamepad key
	static FORCEINLINE bool IsAllowedInputGamepadKey(const FInputChord& InputChord)
	{
		return IsGamepadKey(InputChord.Key) && !InputChord.Key.IsFloatAxis() && !InputChord.Key.IsVectorAxis();
	}

	// Is the key from the keyboard? NOTE: This does not accept invalid keys
	static FORCEINLINE bool IsKeyboardKey(const FKey& Key)
	{
		return Key.IsValid() && !Key.IsGamepadKey() && !Key.IsMouseButton() && !Key.IsVectorAxis() && !Key.IsFloatAxis();
	}

	// Is the key from the gamepad? NOTE: This does not accept invalid keys
	// NOTE: that this can also return true on keys that are also axis
	static FORCEINLINE bool IsGamepadKey(const FKey& Key)
	{
		return Key.IsValid() && Key.IsGamepadKey();
	}

	static FORCEINLINE FString ActionMappingToString(const FInputActionKeyMapping& ActionMapping)
	{
		return FString::Printf(TEXT("FInputActionKeyMapping(ActionName=%s, Key=%s, bShift=%d, bCtrl=%d, bAlt=%d, bCmd=%d)"),
			*ActionMapping.ActionName.ToString(), *ActionMapping.Key.ToString(), ActionMapping.bShift, ActionMapping.bCtrl, ActionMapping.bAlt, ActionMapping.bCmd);
	}

	static FORCEINLINE FString AxisMappingToString(const FInputAxisKeyMapping& AxisMapping)
	{
		return FString::Printf(TEXT("FInputAxisKeyMapping(AxisName=%s, Key=%s, Scale=%f)"),
			*AxisMapping.AxisName.ToString(), *AxisMapping.Key.ToString(), AxisMapping.Scale);
	}

	static FORCEINLINE FString ChordToString(const FInputChord& InputChord)
	{
		return FString::Printf(TEXT("FInputChord(%s)"), *InputChord.GetInputText().ToString());
	}

	static FORCEINLINE bool AreAxisMappingsEqual(const FInputAxisKeyMapping& A, const FInputAxisKeyMapping& B, const bool bIgnoreScale = false)
	{
		return A.AxisName == B.AxisName && A.Key == B.Key &&
			(bIgnoreScale || FMath::IsNearlyEqual(A.Scale, B.Scale, KINDA_SMALL_NUMBER));
	}

	// Checks if the ActionMapping matches the AxisMapping
	static bool DoesActionMappingMatchAxisMapping(const FInputActionKeyMapping& ActionMapping, const FInputAxisKeyMapping& AxisMapping)
	{
		// The key matches we are on the good start
		if (ActionMapping.Key == AxisMapping.Key)
		{
			// Does the AxisName also match?
			const TSet<FName>& ActionNamesAxis = GetAxisNamesLinkedToActionName(ActionMapping.ActionName);
			return ActionNamesAxis.Contains(AxisMapping.AxisName);
		}

		return false;
	}

	// Checks if the ActionMapping matches the InputChord
	static FORCEINLINE bool DoesActionMappingMatchInputChord(const FInputActionKeyMapping& ActionMapping, const FInputChord& InputChord)
	{
		return ActionMapping.Key == InputChord.Key && ActionMapping.bShift == InputChord.bShift && ActionMapping.bCtrl == InputChord.bCtrl
			&& ActionMapping.bAlt == InputChord.bAlt && ActionMapping.bCmd == InputChord.bCmd;
	}

	// Converts FInputActionKeyMapping to an FInputChord
	static FInputChord ActionMappingToInputChord(const FInputActionKeyMapping& ActionMapping)
	{
		FInputChord InputChord;
		InputChord.Key = ActionMapping.Key;
		InputChord.bAlt = ActionMapping.bAlt;
		InputChord.bCmd = ActionMapping.bCmd;
		InputChord.bCtrl = ActionMapping.bCtrl;
		InputChord.bShift = ActionMapping.bShift;
		return InputChord;
	}

	// Converts FInputChord to an FInputActionKeyMapping
	// NOTE: the returned ActionMapping does not have the ActionName, for obvious reasons.
	static FInputActionKeyMapping InputChordToActionMapping(const FInputChord& InputChord)
	{
		FInputActionKeyMapping ActionMapping;
		ActionMapping.Key = InputChord.Key;
		ActionMapping.bAlt = InputChord.bAlt;
		ActionMapping.bCmd = InputChord.bCmd;
		ActionMapping.bCtrl = InputChord.bCtrl;
		ActionMapping.bShift = InputChord.bShift;
		return ActionMapping;
	}

	// Variant to be used with blueprints
	// Convert from ActionNameType to ActionName
	UFUNCTION(BlueprintPure)
	static FORCEINLINE FName ConvertActionNameTypeToFName(const ESoInputActionNameType Type)
	{
		return FSoInputActionName::ActionNameTypeToActionName(Type);
	}

	static FORCEINLINE bool IsInputDeviceTypeGamepad(const ESoInputDeviceType DeviceType)
	{
		return DeviceType != ESoInputDeviceType::Keyboard;
	}

	static FORCEINLINE ESoGamepadLayoutType ConvertInputDeviceTypeToInputGamepadUIType(const ESoInputDeviceType DeviceType)
	{
		switch (DeviceType)
		{
		case ESoInputDeviceType::Gamepad_PlayStation:
			return ESoGamepadLayoutType::PlayStation;
		case ESoInputDeviceType::Gamepad_Switch:
			return ESoGamepadLayoutType::Switch;
		default:
			return ESoGamepadLayoutType::Xbox;
		}
	}

	static FORCEINLINE ESoInputDeviceType ConvertInputGamepadUITypeToInputDeviceType(const ESoGamepadLayoutType UIType)
	{
		switch (UIType)
		{
		case ESoGamepadLayoutType::PlayStation:
			return ESoInputDeviceType::Gamepad_PlayStation;

		case ESoGamepadLayoutType::Switch:
			return ESoInputDeviceType::Gamepad_Switch;

		case ESoGamepadLayoutType::Xbox:
		default:
			return ESoInputDeviceType::Gamepad_Xbox;
		}
		// TODO add nintendo switch
	}

	static FString DeviceTypeToFriendlyString(const ESoInputDeviceType DeviceType);
};
