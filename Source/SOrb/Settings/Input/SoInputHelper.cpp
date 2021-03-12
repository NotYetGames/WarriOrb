// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoInputHelper.h"

#include "Character/SoCharacter.h"
#include "Character/SoPlayerController.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoStringHelper.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInputHelper::SetGameInputBlockedByUI(UObject* WorldContextObject, bool bValue)
{
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (!Character)
		return;

	Character->SetGameInputBlockedByUI(bValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoInputHelper::IsLastInputFromGamepad(const UObject* WorldContextObject)
{
	const ASoPlayerController* Controller = ASoPlayerController::GetInstance(WorldContextObject);
	if (!Controller)
		return false;

	return Controller->IsLastInputFromGamepad();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputDeviceType USoInputHelper::GetCurrentDeviceType(UObject* WorldContextObject)
{
#if PLATFORM_SWITCH
	return ESoInputDeviceType::Gamepad_Switch;
#else
	if (ASoPlayerController* Controller = ASoPlayerController::GetInstance(WorldContextObject))
		return Controller->GetCurrentDeviceType();

#if PLATFORM_XBOXONE
	return ESoInputDeviceType::Gamepad_Xbox;
#else
	return ESoInputDeviceType::Keyboard;
#endif

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputDeviceType USoInputHelper::GetCurrentGamepadDeviceTypeFromSettings(UObject* WorldContextObject)
{
	if (ASoPlayerController* Controller = ASoPlayerController::GetInstance(WorldContextObject))
		return Controller->GetGamepadDeviceTypeFromSettings();

	return ESoInputDeviceType::Gamepad_Generic;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FString USoInputHelper::DeviceTypeToFriendlyString(const ESoInputDeviceType DeviceType)
{
	FString EnumValue;
	if (USoStringHelper::ConvertEnumToString<ESoInputDeviceType>(TEXT("ESoInputDeviceType"), DeviceType, false, EnumValue))
		return EnumValue.Mid(4);

	return EnumValue;
}
