// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIPressAndHoldConfirmation.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "UI/SoUIHelper.h"
#include "Basic/SoAudioManager.h"

#include "Settings/Input/SoInputHelper.h"
#include "Character/SoPlayerController.h"
#include "Character/SoCharacter.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameSingleton.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIPressAndHoldConfirmation::NativeConstruct()
{
	Super::NativeConstruct();

	if (ASoPlayerController* Controller = USoUIHelper::GetSoPlayerControllerFromUWidget(this))
		Controller->OnDeviceTypeChanged().AddDynamic(this, &ThisClass::HandleDeviceTypeChanged);

	UpdateImageTexture(USoInputHelper::GetCurrentDeviceType(this));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIPressAndHoldConfirmation::NativeDestruct()
{
	if (ASoPlayerController* Controller = USoUIHelper::GetSoPlayerControllerFromUWidget(this))
		Controller->OnDeviceTypeChanged().RemoveDynamic(this, &ThisClass::HandleDeviceTypeChanged);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIPressAndHoldConfirmation::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		const bool bConfirmPressed = bConfirmationEnabled &&
			(SoChar->IsUIInputPressed(ESoUICommand::EUC_MainMenuEnter) ||
			 (bAlsoAcceptMainMenuBack && SoChar->IsUIInputPressed(ESoUICommand::EUC_MainMenuBack)));
		if (bConfirmPressed)
			ConfirmCounter = FMath::Max(ConfirmCounter, 0.0f);

		const bool bUpdateMaterial = ((bConfirmPressed && ConfirmCounter < 1.0f) ||
									  (!bConfirmPressed && ConfirmCounter > 0.0f));


		if (!bConfirmed)
		{
			ConfirmCounter += InDeltaTime * (bConfirmPressed ? 1 : -1);
			if (bConfirmPressed)
			{
				if (Instance == nullptr && SFXProgress != nullptr)
				{
					Instance = USoAudioManager::PlaySound2D(this, SFXProgress, false).Instance;
					if (Instance != nullptr)
						Instance->start();
				}
			}
			else
				StopSFX();
		}

		if (bUpdateMaterial)
		{
			SetPercent(FMath::Clamp(ConfirmCounter, 0.0f, 1.0f));
			if (!bElementsVisible)
				UpdateVisibility(true, false);
		}

		if (!bConfirmed && ConfirmCounter >= 1.0f)
		{
			bConfirmed = true;
			Confirmed.Broadcast();
			StopSFX();
			if (SFXDone != nullptr)
				USoAudioManager::PlaySound2D(this, SFXDone, true);

			if (bHideIfNotPressed && bElementsVisible)
				UpdateVisibility(false, false);
		}
		else if (ConfirmCounter < -0.5f && bHideIfNotPressed && bElementsVisible)
		{
			UpdateVisibility(false, false);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIPressAndHoldConfirmation::Reinitialize()
{
	ConfirmCounter = 0.0f;
	SetPercent(0.0f);
	bConfirmed = false;

	if (ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		UpdateVisibility(!bHideIfNotPressed || SoChar->IsUIInputPressed(ESoUICommand::EUC_MainMenuEnter), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIPressAndHoldConfirmation::StopSFX()
{
	if (Instance != nullptr)
	{
		FMOD_STUDIO_PLAYBACK_STATE State;
		Instance->getPlaybackState(&State);
		if (State == FMOD_STUDIO_PLAYBACK_STATE::FMOD_STUDIO_PLAYBACK_PLAYING)
			Instance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);

		Instance->release();
		Instance = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIPressAndHoldConfirmation::SetPercent(float Percent)
{
	if (UMaterialInstanceDynamic* DynamicMaterial = CommandImageProgressBar->GetDynamicMaterial())
	{
		static const FName PercentParamName = FName("Percent");
		DynamicMaterial->SetScalarParameterValue(PercentParamName, Percent);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIPressAndHoldConfirmation::UpdateImageTexture(ESoInputDeviceType DeviceType)
{
	const bool bKeyboard = DeviceType == ESoInputDeviceType::Keyboard;
	if (bKeyboard)
	{
		CommandImage->SetBrushSize(FVector2D(64.0f, 32.0f));
		CommandImageProgressBar->SetBrushSize(FVector2D(72.0f, 42.0f));
	}
	else
	{
		CommandImage->SetBrushSize(FVector2D(48.0f, 48.0f));
		CommandImageProgressBar->SetBrushSize(FVector2D(56.0f, 56.0f));
	}

	CommandImage->SetBrushFromTexture(USoGameSingleton::GetIconForUICommand(ESoUICommand::EUC_MainMenuEnter, DeviceType, {}));

	if (UMaterialInstanceDynamic* DynamicMaterial = CommandImageProgressBar->GetDynamicMaterial())
	{
		static const FName TextureParamName = FName("Texture");
		DynamicMaterial->SetTextureParameterValue(TextureParamName, bKeyboard ? ProgressImgKeyboard : ProgressImgGamepad);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIPressAndHoldConfirmation::UpdateVisibility(bool bShow, bool bInstantChange)
{
	PlayFadeAnimation(bShow, bInstantChange);
	bElementsVisible = bShow;

	if (!bShow)
		StopSFX();
}
