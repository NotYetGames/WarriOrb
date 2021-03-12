// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIDialogueWithWizard.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "DlgContext.h"
#include "DlgDialogueParticipant.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameMode.h"
#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/Commands/SoUICommandTooltip.h"
#include "Character/SoCharacter.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "FMODEvent.h"
#include "Settings/SoGameSettings.h"
#include "Basic/SoAudioManager.h"

const FName SoMatParamWarriorbWeight = FName("WarriorbWeight");
const FName SoMatParamWarriorbTexture = FName("WarriorbImage");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueWithWizard::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueWithWizard::NativeDestruct()
{
	if (Instance != nullptr)
	{
		Instance->release();
		Instance = nullptr;
	}

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueWithWizard::SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable)
{
	if (bEnable)
	{
		ASoGameMode::Get(this).ClearWizardDialogueAfterGroupDefeated();

		if (bOpened)
		{
			UE_LOG(LogSoUIActivity, Warning, TEXT("Dialogue request rejected: a dialogue is already in progress"));
			return true;
		}

		ActiveDlgContext = Cast<UDlgContext>(Source);
		if (ActiveDlgContext == nullptr)
			UE_LOG(LogSoUIActivity, Warning, TEXT("Dialogue request rejected: Source is not a dialogue context"))
		else
		{
			bOpened = true;
			SetVisibility(ESlateVisibility::Visible);

			ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
			const bool bWizardLine = ActiveDlgContext->GetActiveNodeParticipant() != Character;
			PlayAnimation(WidgetAnimFadeIn);
			PlayAnimation(bWizardLine ? WidgetAnimFadeInWizardText : WidgetAnimFadeInWarriorbText);
			bLastLineWasWizard = bWizardLine;
			Background->GetDynamicMaterial()->SetScalarParameterValue(SoMatParamWarriorbWeight, bWizardLine ? 0.0f : 1.0f);
			(bWizardLine ? WizardText : WarriorbText)->SetText(ActiveDlgContext->GetActiveNodeText());
			FixTextSizes();
			UpdatePlayerImage();
			UpdateTooltipsVisbility();
			bWaitUntilAnimEndWithVO = true;
			USoAudioManager::PlaySound2D(this, SFXOnOpen);
		}
	}
	else
	{
		bOpened = false;
		SetVisibility(ESlateVisibility::Collapsed);
		ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		Character->SetInterruptedWizardDialogue(ActiveDlgContext);
		ActiveDlgContext = nullptr;
	}

	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueWithWizard::HandleCommand_Implementation(ESoUICommand Command)
{
	switch (Command)
	{
		case ESoUICommand::EUC_MainMenuBack:
		case ESoUICommand::EUC_MainMenuEnter:
			CommandTooltip->SetVisibility(ESlateVisibility::Collapsed);
			CommandTooltip->SetIsListeningToDeviceChanges(false);
			if (ActiveDlgContext != nullptr && !IsAnyAnimationPlaying())
			{
				if (ActiveDlgContext->ChooseOption(0))
				{
					const bool bWizardLine = ActiveDlgContext->GetActiveNodeParticipant() != USoStaticHelper::GetPlayerCharacterAsActor(this);
					if (bWizardLine != bLastLineWasWizard)
					{
						PlayAnimation(WidgetAnimSwitchBackground, 0.0f, 1.0f, bWizardLine ? EUMGSequencePlayMode::Reverse : EUMGSequencePlayMode::Forward);
						USoAudioManager::PlaySound2D(this, SFXOnSpeakerChange);
						bWaitUntilAnimEndWithVO = true;
					}
					else
					{
						USoAudioManager::PlaySound2D(this, SFXOnTextSwitch);
						CheckAndPlayVoice();
					}

					(bWizardLine ? WizardText : WarriorbText)->SetText(ActiveDlgContext->GetActiveNodeText());
					FixTextSizes();
					bLastLineWasWizard = bWizardLine;
					UpdatePlayerImage();
				}
				else
				{
					ActiveDlgContext = nullptr;
					PlayAnimation(WidgetAnimFadeOut);
					PlayAnimation(bLastLineWasWizard ? WidgetAnimFadeOutWizardText : WidgetAnimFadeOutWarriorbText);
					USoAudioManager::PlaySound2D(this, SFXOnClose);
				}
			}
			break;

		default:
			break;
	}

	// let the menu open on escape
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueWithWizard::Update_Implementation(float DeltaSeconds)
{
	if (bWaitUntilAnimEndWithVO && !IsAnyAnimationPlaying())
	{
		bWaitUntilAnimEndWithVO = false;
		CheckAndPlayVoice();
	}

	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	const FVector2D Velocity = FVector2D(Character->GetSoMovement()->Velocity);
	if (Velocity.SizeSquared() > 0.1f)
	{
		FVector2D NewVelocity = Velocity - Velocity.GetSafeNormal() * DeltaSeconds * SlowDownSpeed;
		if ((NewVelocity.GetSafeNormal() | Velocity.GetSafeNormal()) < 0.0f)
			NewVelocity = FVector2D(0.0f, 0.0f);

		Character->GetSoMovement()->Velocity.X = NewVelocity.X;
		Character->GetSoMovement()->Velocity.Y = NewVelocity.Y;
	}

	return IsAnyAnimationPlaying() || (ActiveDlgContext != nullptr && ActiveDlgContext->GetOptionsNum() > 0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueWithWizard::UpdatePlayerImage()
{
	if (ActiveDlgContext != nullptr && ActiveDlgContext->GetActiveNodeParticipant() == USoStaticHelper::GetPlayerCharacterAsActor(this))
	{
		Background->GetDynamicMaterial()->SetTextureParameterValue(SoMatParamWarriorbTexture, GetWarriOrbImage());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueWithWizard::UpdateTooltipsVisbility()
{
	AActor* PlayerActor = USoStaticHelper::GetPlayerCharacterAsActor(this);
	static const FName DlgDisplayedBoolName = TEXT("DlgWithWizardDisplayed");
	const bool bWasHintDisplayed = IDlgDialogueParticipant::Execute_GetBoolValue(PlayerActor, DlgDisplayedBoolName);
	if (!bWasHintDisplayed)
	{
		IDlgDialogueParticipant::Execute_ModifyBoolValue(PlayerActor, DlgDisplayedBoolName, true);
	}

	CommandTooltip->SetVisibility(!bWasHintDisplayed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueWithWizard::CheckAndPlayVoice()
{
	if (ActiveDlgContext == nullptr)
		return;

	// Voice gibberish disabled by settings
	if (USoGameSettings::Get().IsDialogueVoiceGibberishMuted())
		return;

	if (UFMODEvent* VO = Cast<UFMODEvent>(ActiveDlgContext->GetActiveNodeGenericData()))
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

		Instance = USoAudioManager::PlaySound2D(this, VO, false).Instance;
		if (Instance != nullptr)
			Instance->start();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture* USoUIDialogueWithWizard::GetWarriOrbImage() const
{
	switch (USoGameSettings::Get().GetCharacterSkinType())
	{
		case ESoCharacterSkinType::Mummy:
			return WarriOrbImageMummy;

		case ESoCharacterSkinType::Pumpkin:
			return WarriOrbImagePumpkin;

		default:
		case ESoCharacterSkinType::Classic:
			return WarriOrbImageClassic;
	}
}