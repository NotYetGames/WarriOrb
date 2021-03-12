// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoInputSettingsTypes.h"
#include "SoInputHelper.h"
#include "Engine/Texture2D.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoInputActionKeyMapping::IsKeyboard() const
{
	return USoInputHelper::IsKeyboardKey(Default.Key);
}

bool FSoInputActionKeyMapping::IsGamepad() const
{
	return USoInputHelper::IsGamepadKey(Default.Key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoInputActionKeyMapping::operator==(const FSoInputActionKeyMapping& Other) const
{
	return Default == Other.Default && Modified == Other.Modified;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoInputActionKeyMapping::operator<(const FSoInputActionKeyMapping& Other) const
{
	if (Default == Other.Default)
	{
		return Modified < Other.Modified;
	}

	return Default < Other.Default;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputIntegrity FSoInputActionKeyMapping::ValidateIntegrity() const
{
	if (Default.ActionName != Modified.ActionName)
		return ESoInputIntegrity::NamesMismatch;

	// We can only determine category if both keys are valid
	if (Default.Key.IsValid() && Modified.Key.IsValid())
		if (USoInputHelper::IsKeyboardKey(Default.Key) != USoInputHelper::IsKeyboardKey(Modified.Key))
		{
			return ESoInputIntegrity::KeyCategoryMismatch;
		}

	return ESoInputIntegrity::Ok;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoInputActionKeyMapping::ToString() const
{
	return FString::Printf(TEXT("FSoInputActionKeyMapping(Default=%s, Modified=%s)"),
		*USoInputHelper::ActionMappingToString(Default), *USoInputHelper::ActionMappingToString(Modified));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoInputAxisKeyMapping::IsKeyboard() const
{
	return USoInputHelper::IsKeyboardKey(Default.Key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoInputAxisKeyMapping::IsGamepad() const
{
	return USoInputHelper::IsGamepadKey(Default.Key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoInputAxisKeyMapping::operator==(const FSoInputAxisKeyMapping& Other) const
{
	return USoInputHelper::AreAxisMappingsEqual(Default, Other.Default) &&
		USoInputHelper::AreAxisMappingsEqual(Modified, Other.Modified);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoInputAxisKeyMapping::operator<(const FSoInputAxisKeyMapping& Other) const
{
	if (USoInputHelper::AreAxisMappingsEqual(Default, Other.Default))
	{
		return Modified < Other.Modified;
	}

	return Default < Other.Default;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputIntegrity FSoInputAxisKeyMapping::ValidateIntegrity() const
{
	if (Default.AxisName != Modified.AxisName)
		return ESoInputIntegrity::NamesMismatch;

	// We can only determine category if both keys are valid
	if (Default.Key.IsValid() && Modified.Key.IsValid())
		if (USoInputHelper::IsKeyboardKey(Default.Key) != USoInputHelper::IsKeyboardKey(Modified.Key))
			return ESoInputIntegrity::KeyCategoryMismatch;


	return ESoInputIntegrity::Ok;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoInputAxisKeyMapping::ToString() const
{
	return FString::Printf(TEXT("FSoInputAxisKeyMapping(Default=%s, Modified=%s)"),
		*USoInputHelper::AxisMappingToString(Default), *USoInputHelper::AxisMappingToString(Modified));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* FSoInputUITextures::GetTextureForDeviceType(const ESoInputDeviceType DeviceType) const
{
	switch (DeviceType)
	{
	case ESoInputDeviceType::Keyboard:
		return Keyboard;
	case ESoInputDeviceType::Gamepad_PlayStation:
		return PlayStation;
	case ESoInputDeviceType::Gamepad_Switch:
		return Switch;
	case ESoInputDeviceType::Gamepad_Generic:
	case ESoInputDeviceType::Gamepad_Xbox:
		return Xbox;
	default:
		break;
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* FSoInputKeyboardKeyTextures::GetTexture(bool bPressed) const
{
	return bPressed ? Pressed : Unpressed;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* FSoInputGamepadKeyTextures::GetTextureForDeviceType(const ESoInputDeviceType DeviceType) const
{
	switch (DeviceType)
	{
	case ESoInputDeviceType::Gamepad_PlayStation:
		return PlayStation;
	case ESoInputDeviceType::Gamepad_Generic:
	case ESoInputDeviceType::Gamepad_Xbox:
		return Xbox;
	case ESoInputDeviceType::Gamepad_Switch:
		return Switch;
	default:
		break;
	}

	return nullptr;
}
