// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoCommandImageBilboardComponent.h"

#include "Character/SoPlayerController.h"
#include "Basic/SoGameSingleton.h"
#include "Settings/Input/SoInputHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCommandImageBilboardComponent::BeginPlay()
{
	Super::BeginPlay();
	SubscribeToDeviceChanged();

	DeviceType = USoInputHelper::GetCurrentDeviceType(this);
	UpdateImage();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCommandImageBilboardComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnSubscribeFromDeviceChanged();
	Super::EndPlay(EndPlayReason);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCommandImageBilboardComponent::SetSprite(UTexture2D* NewSprite)
{
	// Ignore Super
	UpdateImage();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCommandImageBilboardComponent::SetActionNameType(const ESoInputActionNameType InType)
{
	ActionNameType = InType;
	if (!IsActionNameTypeValid())
		return;

	DeviceType = USoInputHelper::GetCurrentDeviceType(this);
	UpdateImage();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCommandImageBilboardComponent::HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType)
{
	DeviceType = InDeviceType;
	UpdateImage();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCommandImageBilboardComponent::SubscribeToDeviceChanged()
{
	if (bIsListeningToDevicesChanges)
		return;

	if (ASoPlayerController* Controller = ASoPlayerController::GetInstance(this))
	{
		Controller->OnDeviceTypeChanged().AddDynamic(this, &Self::HandleDeviceTypeChanged);
		bIsListeningToDevicesChanges = true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCommandImageBilboardComponent::UnSubscribeFromDeviceChanged()
{
	if (!bIsListeningToDevicesChanges)
		return;

	if (ASoPlayerController* Controller = ASoPlayerController::GetInstance(this))
	{
		Controller->OnDeviceTypeChanged().RemoveDynamic(this, &Self::HandleDeviceTypeChanged);
		bIsListeningToDevicesChanges = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCommandImageBilboardComponent::UpdateImage()
{
	// Maybe needed, not sure
	SubscribeToDeviceChanged();

	Sprite = nullptr;
	if (IsActionNameTypeValid())
		Sprite = USoGameSingleton::GetIconForInputActionNameType(ActionNameType, DeviceType);

	MarkRenderStateDirty();
}
