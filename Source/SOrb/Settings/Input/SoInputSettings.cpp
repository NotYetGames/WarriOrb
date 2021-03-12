// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoInputSettings.h"
#include "SoInputNames.h"
#include "SoInputHelper.h"
#include "SOrb.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FInputActionKeyMapping FSoInputPresetKeyRebind::ToUnrealActionMapping() const
{
	FInputActionKeyMapping UnrealMapping = USoInputHelper::InputChordToActionMapping(Key);
	UnrealMapping.ActionName = ActionName;
	return UnrealMapping;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoInputPreset::FSoInputPreset()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoInputPreset::CheckAndFixIntegrityKeyRebinds(TArray<FSoInputPresetKeyRebind>& KeyRebinds)
{
	if (KeyRebinds.Num() == 0)
	{
		// Reset to something sensible
		ResetToDefaultsKeyRebinds(KeyRebinds);
		return;
	}

	// 1. Remove action names that do not exist anymore
	// 2. Adds action names that were added
	// 3. Keep proper order from the AllOption

	// Collect current
	TMap<FName, FInputChord> CurrentKeyRebinds;
	for (const FSoInputPresetKeyRebind& Rebind : KeyRebinds)
		CurrentKeyRebinds.Add(Rebind.ActionName, Rebind.Key);

	// Reset and restore key bindings for preset
	ResetToDefaultsKeyRebinds(KeyRebinds);
	for (FSoInputPresetKeyRebind& Rebind : KeyRebinds)
		Rebind.Key = CurrentKeyRebinds.FindRef(Rebind.ActionName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoInputPreset::ResetToDefaultsKeyRebinds(TArray<FSoInputPresetKeyRebind>& KeyRebinds)
{
	// HACK!
	//FWarriorbGameModule::Init();

	// Defaults at the time of creation
	KeyRebinds.Empty();
	for (const FSoInputConfigurableActionName& Option : FSoInputConfigurableActionName::GetAllOptions())
		KeyRebinds.Add({ Option.ActionName });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoInputPreset::CheckAndFixIntegrity()
{
	CheckAndFixIntegrityKeyRebinds(KeyRebinds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoInputPreset::ResetToDefaults()
{
	ResetToDefaultsKeyRebinds(KeyRebinds);
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoInputSettings::CanEditChange(const UProperty* InProperty) const
{
	const bool bIsEditable = Super::CanEditChange(InProperty);
	return bIsEditable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInputSettings::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	FSoInputPreset::CheckAndFixIntegrityKeyRebinds(KeyboardDefaultArrowsPresetKeyRebinds);
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInputSettings::PostInitProperties()
{
	Super::PostInitProperties();

	ReloadRuntimeValues();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInputSettings::ReloadRuntimeValues()
{
	if (HasAnyFlags(RF_NeedLoad))
		return;
	
	KeyboardDefaultArrowsPreset.DeviceType = KeyboardDefaultArrowsPresetDeviceType;
	KeyboardDefaultArrowsPreset.KeyRebinds = KeyboardDefaultArrowsPresetKeyRebinds;
}