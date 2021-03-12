// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIDialoguePanel.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Animation/WidgetAnimation.h"

#include "FMODEvent.h"

#include "DlgContext.h"
#include "DlgDialogueParticipant.h"

#include "Basic/Helpers/SoMathHelper.h"
#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoDialogueParticipant.h"
#include "UI/General/Buttons/SoUIButtonArray.h"
#include "SoUIDialogueTextBox.h"
#include "SoUIDialogueChoiceBox.h"
#include "UI/InGame/SoUICharacterPreview.h"
#include "Character/SoCharacter.h"
#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "UI/SoUIHelper.h"
#include "Settings/SoGameSettings.h"


DEFINE_LOG_CATEGORY(LogSoDlgUI);

const FName USoUIDialoguePanel::AnimNameFadeIn = FName("BackgroundFadeIn");
const FName USoUIDialoguePanel::AnimNameFadeOut = FName("BackgroundFadeOut");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIDialoguePanel::USoUIDialoguePanel(const FObjectInitializer& PCIP) :
	Super(PCIP)
{
	// PlayerChoiceLocations2.SetNum(2);
	// PlayerChoiceLocations3.SetNum(3);
	// PlayerChoiceLocations4.SetNum(4);
	// PlayerChoiceLocations5.SetNum(5);
	// PlayerChoiceLocations6.SetNum(6);
	// PlayerChoiceLocations7.SetNum(7);
	// PlayerChoiceLocations8.SetNum(8);

	InactiveValues.SetNum(4);
	for (int32 i = 0; i < 4; ++i)
		InactiveValues[i] = 1.0f;

	bArmedStateAllowedOnLeave = false;
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	//TArray<TArray<FVector2D>*> ChoiceDataArrays = { &PlayerChoiceLocations2,
	//												&PlayerChoiceLocations3,
	//												&PlayerChoiceLocations4,
	//												&PlayerChoiceLocations5,
	//												&PlayerChoiceLocations6,
	//												&PlayerChoiceLocations7,
	//												&PlayerChoiceLocations8 };
	//for (int32 i = 0; i < ChoiceDataArrays.Num(); ++i)
	//	for (int32 j = 1; j < ChoiceDataArrays[i]->Num(); ++j)
	//		(*ChoiceDataArrays[i])[j].Y = (*ChoiceDataArrays[i])[0].Y + j * YDifferenceBetweenChoices;
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);

	DialogueTextBoxArray = { TextBox0, TextBox1, TextBox2, TextBox3 };
	PlayerChoiceTextBoxArray = { PlayerChoice0, PlayerChoice1, PlayerChoice2, PlayerChoice3, PlayerChoice4, PlayerChoice5, PlayerChoice6, PlayerChoice7 };

	for (USoUIDialogueChoiceBox* ChoiceBox : PlayerChoiceTextBoxArray)
		if (ChoiceBox != nullptr)
			ChoiceBox->SetUpdateCalledFromParent(true);

	USoStaticHelper::BuildAnimationMap(this, AnimationMap);

	for (int32 i = 0; i < DialogueTextBoxArray.Num(); ++i)
		DialogueTextBoxArray[i]->OnDialogueTextboxOpenned.AddDynamic(this, &USoUIDialoguePanel::OnDialogueTextboxOpenned);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::NativeDestruct()
{
	if (Instance != nullptr)
	{
		Instance->release();
		Instance = nullptr;
	}

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialoguePanel::SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable)
{
	if (bEnable)
	{
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
			ParticipantStateMap.Empty();
			FadeIn();
			PlayerEmotion = NAME_None;
			CharPreviewLeft->UpdateFromPlayerCharacter(PlayerEmotion);
			USoAudioManager::PlaySound2D(this, SFXOpen);
		}
	}
	else
	{
		if (bOpened)
		{
			bOpened = false;

			for (int32 i = 0; i < DialogueTextBoxArray.Num(); ++i)
				DialogueTextBoxArray[i]->ForceHideDialogueTextBox();

			for (USoUIDialogueChoiceBox* PlayerChoice : PlayerChoiceTextBoxArray)
				PlayerChoice->ForceHideChoiceTextBox();

			SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	USoGameInstance::Get(this).OnDialogueStateChange(bOpened);
	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialoguePanel::HandleCommand_Implementation(ESoUICommand Command)
{
	if (SoPanelState == ESoDialoguePanelState::EDPS_WaitForFadeOut)
		return true;

	Command = USoUIHelper::TryTranslateMenuCommandDirectionToGame(Command);

	// weird but whatever
	if (GetVisibility() != ESlateVisibility::Visible)
		return true;

	if (SoPanelState == ESoDialoguePanelState::EDPS_Change &&
		(Command == ESoUICommand::EUC_MainMenuEnter ||
		Command == ESoUICommand::EUC_MainMenuBack))
	{
		for (USoUIDialogueTextBox* TextBox : DialogueTextBoxArray)
			TextBox->SetAnimationSpeedMultiplier(10.0f);
	}

	if (ChangeActivePlayerChoice(Command))
		return true;

	if (SoPanelState != ESoDialoguePanelState::EDPS_WaitForInput)
		return true;

	if ((Command == ESoUICommand::EUC_MainMenuEnter ||
		 Command == ESoUICommand::EUC_MainMenuBack) && ActiveDlgContext != nullptr)
	{
		if (ActiveDlgContext->GetOptionsNum() > 1 && !bPlayerChoicesDisplayed)
		{
			bDisplayPlayerChoiceAfterFade = true;
			bWaitForTextBoxFadeOut = true;

			DialogueTextBoxArray[1]->HideDialogueTextBox();
			DialogueTextBoxArray[2]->HideDialogueTextBox();
			DialogueTextBoxArray[3]->HideDialogueTextBox();

			SoPanelState = ESoDialoguePanelState::EDPS_Change;
			LastChangeStartTime = GetWorld()->GetTimeSeconds();
		}
		else
		{
			if (bWaitForSpace)
			{
				bWaitForSpace = false;
				UpdateUI();
				return true;
			}

			bool bStillInDialogue = ActiveDlgContext->ChooseOption(ActivePlayerOption);

			if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
				while (bStillInDialogue && SoCharacter->bDebugSkipDialogues && ActiveDlgContext->GetOptionsNum() == 1)
					bStillInDialogue = ActiveDlgContext->ChooseOption(0);

			if (bStillInDialogue)
			{
				if (bPlayerChoicesDisplayed)
				{
					HidePlayerOptions();
					HideNotOptionTextBoxes();
					USoAudioManager::PlaySound2D(this, SFXOptionSelected);
				}
				else
				{
					if (bWaitForSpace || SoPanelState == ESoDialoguePanelState::EDPS_WaitForDuration)
					{
						for (int32 i = 0; i < DialogueTextBoxArray.Num(); ++i)
							if (DialogueTextBoxArray[i]->IsDialogueTextBoxOpenedAndInIdle())
							{
								bWaitForTextBoxFadeOut = true;
								DialogueTextBoxArray[i]->HideDialogueTextBox();
							}
					}
					else
						UpdateUI();
				}
			}
			else
			{
				if (FadeOutDelay > KINDA_SMALL_NUMBER)
				{
					SoPanelState = ESoDialoguePanelState::EDPS_WaitForFadeOut;
				}
				else
					FadeOut();
			}
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialoguePanel::Update_Implementation(float DeltaSeconds)
{
	// manual update cause of weird bug
	for (USoUIDialogueChoiceBox* ChoiceBox : PlayerChoiceTextBoxArray)
		if (ChoiceBox != nullptr)
			ChoiceBox->Update(DeltaSeconds);

	if (bOpened && SoPanelState != ESoDialoguePanelState::EDPS_FadeIn && SoPanelState != ESoDialoguePanelState::EDPS_FadeOut)
	{
		UImage* DlgImages[] = { DlgImage0, nullptr, DlgImage2, DlgImage3 };
		for (int32 i = 0; i < 4; ++i)
		{
			const float InactiveAlpha = 0.3f;
			InactiveValues[i] = FMath::FInterpTo(InactiveValues[i], (i != ActiveParticipantImageIndex ? InactiveAlpha : 1.0f), DeltaSeconds, 3.0f);

			if (DlgImages[i] != nullptr)
				DlgImages[i]->GetDynamicMaterial()->SetScalarParameterValue(ActivePercentName, InactiveValues[i]);
			else
				CharPreviewLeft->SetActivePercent(InactiveValues[i]);
		}
	}

	switch (SoPanelState)
	{
		case ESoDialoguePanelState::EDPS_WaitForDuration:
			WaitCounter -= DeltaSeconds;
			if (WaitCounter < 0.0f)
			{
				UpdateUI();
			}
			break;

		case ESoDialoguePanelState::EDPS_WaitForFadeOut:
			FadeOutDelay -= DeltaSeconds;
			if (FadeOutDelay < 0.0f)
				FadeOut();
			break;

		case ESoDialoguePanelState::EDPS_FadeIn:
			if (!IsPlayingAnimation())
				UpdateUI();
			break;

		case ESoDialoguePanelState::EDPS_Change:
			if (!IsAnyTextBoxAnimated())
			{
				if (bWaitForTextBoxFadeOut)
				{
					if (bDisplayPlayerChoiceAfterFade)
					{
						DisplayPlayerOptions();
						bDisplayPlayerChoiceAfterFade = false;
						bWaitForTextBoxFadeOut = false;
					}
					else
						UpdateUI();
				}
				else
					SoPanelState = ESoDialoguePanelState::EDPS_WaitForInput;
			}
			else
			{
				// Every 3 seconds
				const float ThresholdSeconds = LastChangeStartTime + USoDateTimeHelper::NormalizeTime(3.f);
				if (GetWorld()->GetTimeSeconds() > ThresholdSeconds)
				{
					for (auto* TextBox : DialogueTextBoxArray)
						if (TextBox != nullptr && TextBox->IsDialogueTextBoxAnimated())
							TextBox->ForceFinishChange();

					for (auto* ChoiceBox : PlayerChoiceTextBoxArray)
						if (ChoiceBox != nullptr && ChoiceBox->IsChoiceTextBoxAnimated())
							ChoiceBox->ForceFinishChange();
				}
			}
			break;

		case ESoDialoguePanelState::EDPS_FadeOut:
		{
			const bool bStillIn = IsPlayingAnimation();
			if (!bStillIn)
			{
				SetVisibility(ESlateVisibility::Collapsed);
				bOpened = false;
			}
			return bStillIn;
		}

		default:
			break;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialoguePanel::ModifyNameValue_Implementation(FName ValueName, FName NameValue)
{
 	static const FName PlayAnimation = FName("PlayAnimation");
	static const FName HideParticipant = FName("HideParticipant");
	static const FName ShowParticipant = FName("ShowParticipant");
	static const FName UpdateParticipantImage = FName("UpdateImage");

	const bool bHideParticipant = ValueName == HideParticipant;
	if (bHideParticipant || ValueName == ShowParticipant)
	{
		if (ActiveDlgContext != nullptr)
		{
			for (const auto& Pair : ActiveDlgContext->GetParticipantsMap())
				if (Pair.Key == NameValue)
				{
					const FSoDialogueParticipantData& ParticipantData = Cast<ISoDialogueParticipant>(Pair.Value)->GetParticipantData();

					PlayAnimationSafe(bHideParticipant ? ParticipantData.FadeOutAnimName : ParticipantData.FadeInAnimName);
					ParticipantStateMap[Pair.Key] = !bHideParticipant;

					USoAudioManager::PlaySound2D(this, bHideParticipant ? SFXParticipantDisappears : SFXParticipantAppears);
				}
		}
	}
	else if (ValueName == PlayAnimation)
		PlayAnimationSafe(NameValue);
	else if (ValueName == UpdateParticipantImage)
	{
		if (ActiveDlgContext != nullptr)
			for (const auto& Pair : ActiveDlgContext->GetParticipantsMap())
				if (Pair.Key == NameValue)
				{
					const FSoDialogueParticipantData& ParticipantData = Cast<ISoDialogueParticipant>(Pair.Value)->GetParticipantData();
					UImage* DlgImages[] = { DlgImage0, nullptr, DlgImage2, DlgImage3 };
					if (ParticipantData.PositionIndex >= 0 && ParticipantData.PositionIndex < 4 && DlgImages[ParticipantData.PositionIndex] != nullptr)
						// DlgImages[ParticipantData.PositionIndex]->SetBrushFromTexture(ParticipantData.ParticipantIcon);
						DlgImages[ParticipantData.PositionIndex]->GetDynamicMaterial()->SetTextureParameterValue(FName("Texture"), ParticipantData.ParticipantIcon);
				}
	}
	else
		UE_LOG(LogSoDlgUI, Warning, TEXT("Unhandled USoUIDialoguePanel::ModifyNameValue: %s"), *ValueName.ToString());

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialoguePanel::ModifyFloatValue_Implementation(FName ValueName, bool bDelta, float Value)
{
	if (ValueName == FName("FadeOutDelay"))
		FadeOutDelay = Value;
	else if (ValueName == FName("Wait"))
	{
		WaitCounter = Value;
		SoPanelState = ESoDialoguePanelState::EDPS_WaitForDuration;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialoguePanel::OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName)
{
	if (EventName == FName("SpaceRequest"))
		bWaitForSpace = true;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::OnDialogueTextboxOpenned()
{
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
void USoUIDialoguePanel::FadeIn()
{
	ensure(ActiveDlgContext != nullptr);

	// clear anims from invalid exits
	StopAllAnimations();

	bOpened = true;
	SetVisibility(ESlateVisibility::Visible);
	SoPanelState = ESoDialoguePanelState::EDPS_FadeIn;
	ActivePlayerOption = 0;
	bPlayerChoicesDisplayed = false;

	PlayAnimationSafe(AnimNameFadeIn);
	TMap<FName, UObject*> ParticipantMap = ActiveDlgContext->GetParticipantsMap();

	UImage* DlgImages[] = { DlgImage0, nullptr, DlgImage2, DlgImage3 };
	UImage* FloorImages[] = { Floor0, Floor1, nullptr, nullptr };

	CharPreviewLeft->SetActivePercent(1.0f);

	for (int32 i = 0; i < 4; ++i)
		InactiveValues[i] = 1.0f;

	// fade in the primary participants
	for (const auto& Pair : ParticipantMap)
		if (Pair.Value != nullptr && Pair.Value != this)
		{
			const FSoDialogueParticipantData& ParticipantData = Cast<ISoDialogueParticipant>(Pair.Value)->GetParticipantData();
			const int32 DlgPositionIndex = ParticipantData.PositionIndex;

			if (DlgPositionIndex == 0)
			{
				bKeepMusicAfterDialogue = true;
				if (ParticipantData.Music != nullptr)
				{
					USoAudioManager::Get(this).SetMusic(ParticipantData.Music, true, ParticipantData.PrevMusicFadeDuration);
					IDlgDialogueParticipant::Execute_ModifyBoolValue(USoStaticHelper::GetPlayerCharacterAsActor(this), ParticipantData.Music->GetFName(), true);
					bKeepMusicAfterDialogue = ParticipantData.bKeepMusicAfterDialogue;
				}
			}

			if (DlgPositionIndex < 2 && DlgPositionIndex >= 0)
			{
				PlayAnimationSafe(ParticipantData.FadeInAnimName);
				ParticipantStateMap.Add(Pair.Key, true);
			}
			else
				ParticipantStateMap.Add(Pair.Key, false);

			// not player stuff
			if (DlgPositionIndex >= 0 && DlgPositionIndex < 4)
			{
				if (DlgImages[DlgPositionIndex] != nullptr)
				{
					DlgImages[DlgPositionIndex]->GetDynamicMaterial()->SetTextureParameterValue(FName("Texture"), ParticipantData.ParticipantIcon);
					DlgImages[DlgPositionIndex]->GetDynamicMaterial()->SetScalarParameterValue(ActivePercentName, 1.0f);
				}

				// setup floor
				if (FloorImages[DlgPositionIndex] != nullptr)
				{
					FloorImages[DlgPositionIndex]->SetBrushFromTexture(ParticipantData.FloorImage, true);
					UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(FloorImages[DlgPositionIndex]->Slot);
					if (CanvasSlot != nullptr)
						CanvasSlot->SetPosition(ParticipantData.FloorPosition);
				}

				// setup name box
				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DialogueTextBoxArray[DlgPositionIndex]->Slot))
				{
					FVector2D Pos = CanvasSlot->GetPosition();
					Pos.Y = ParticipantData.TextBoxY;
					CanvasSlot->SetPosition(Pos);
				}

				DialogueTextBoxArray[DlgPositionIndex]->SetSpeakerData(ParticipantData.TextBoxBackground,
																	   ParticipantData.NameTextBackground,
																	   ParticipantData.ParticipantDisplayName);
			}

		}

	USoAudioManager::PlaySound2D(this, SFXDialogueStart);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::FadeOut()
{
	SoPanelState = ESoDialoguePanelState::EDPS_FadeOut;
	PlayAnimationSafe(AnimNameFadeOut);
	USoAudioManager::PlaySound2D(this, SFXClose);

	if (ActiveDlgContext != nullptr)
	{
		// fade out active participants
		TMap<FName, UObject*> ParticipantMap = ActiveDlgContext->GetParticipantsMap();
		for (const auto& Pair : ParticipantMap)
			if (Pair.Value != nullptr && ParticipantStateMap.Contains(Pair.Key) && ParticipantStateMap[Pair.Key])
			{
				const auto& ParticipantData = Cast<ISoDialogueParticipant>(Pair.Value)->GetParticipantData();
				PlayAnimationSafe(ParticipantData.FadeOutAnimName);

				// update this here because it can change during dialogue
				if (ParticipantData.Music != nullptr && ParticipantData.PositionIndex == 0)
					bKeepMusicAfterDialogue = ParticipantData.bKeepMusicAfterDialogue;
			}
	}

	ActiveDlgContext = nullptr;

	for (auto* TextBox : DialogueTextBoxArray)
		TextBox->HideDialogueTextBox();

	for (USoUIDialogueChoiceBox* PlayerChoice : PlayerChoiceTextBoxArray)
		PlayerChoice->HideChoiceTextBox();

	if (!bKeepMusicAfterDialogue)
		if (ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
			SoChar->UpdateMusic(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::UpdateUI()
{
	if (ActiveDlgContext == nullptr || ActiveDlgContext->GetOptionsNum() == 0)
	{
		FadeOut();
		return;
	}

	UObject* ActiveParticipant = ActiveDlgContext->GetActiveNodeParticipant();
	ensure(ActiveParticipant);

	const FSoDialogueParticipantData& ParticipantData = Cast<ISoDialogueParticipant>(ActiveParticipant)->GetParticipantData();

	ActiveParticipantImageIndex = ParticipantData.PositionIndex;

	ensure(DialogueTextBoxArray.IsValidIndex(ParticipantData.PositionIndex));
	if (!DialogueTextBoxArray.IsValidIndex(ParticipantData.PositionIndex))
		return;

	SoPanelState = ESoDialoguePanelState::EDPS_Change;
	LastChangeStartTime = GetWorld()->GetTimeSeconds();
	bWaitForTextBoxFadeOut = false;
	for (int32 i = 0; i < DialogueTextBoxArray.Num(); ++i)
		if (i != ParticipantData.PositionIndex && DialogueTextBoxArray[i]->IsDialogueTextBoxOpenedAndInIdle())
		{
			bWaitForTextBoxFadeOut = true;
			DialogueTextBoxArray[i]->HideDialogueTextBox();
		}

	if (!bWaitForTextBoxFadeOut)
	{
		if (ParticipantStateMap.Contains(ParticipantData.ParticipantName) && !ParticipantStateMap[ParticipantData.ParticipantName])
		{
			SoPanelState = ESoDialoguePanelState::EDPS_FadeIn;
			ParticipantStateMap[ParticipantData.ParticipantName] = true;
			PlayAnimationSafe(ParticipantData.FadeInAnimName);
			USoAudioManager::PlaySound2D(this, SFXParticipantAppears);
			return;
		}

		DialogueTextBoxArray[ParticipantData.PositionIndex]->DisplayText(ActiveDlgContext->GetActiveNodeText());

		if (ActiveParticipantImageIndex == 1)
		{
			PlayerEmotion = ActiveDlgContext->GetActiveNodeSpeakerState();
			CharPreviewLeft->UpdateFromPlayerCharacter(PlayerEmotion);
		}
	}

	UpdateNameFields();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::DisplayPlayerOptions()
{
	if (!ensure(ActiveDlgContext))
		return;

	ActiveParticipantImageIndex = 1;

	SoPanelState = ESoDialoguePanelState::EDPS_Change;
	LastChangeStartTime = GetWorld()->GetTimeSeconds();
	bPlayerChoicesDisplayed = true;

	const TArray<FVector2D> PCL2 = { {0.0f, -508.0f},  {0.0f, -428.0f} };
	const TArray<FVector2D> PCL3 = { {20.0f, -548.0f}, { -16.0f, -468.0f }, { 20.0f, -388.0f } };
	const TArray<FVector2D> PCL4 = { {20.f, -584.0f},  { -16.0f , -504.0f }, { -16.0f, -424.0f }, { 20.0f, -344.0f } };
	const TArray<FVector2D> PCL5 = { {20.f, -632.0f},  { -16.0f , -552.0f }, { -52.0f, -472.0f }, { -16.0f, -392.0f }, {20.f, -312.0f } };
	const TArray<FVector2D> PCL6 = { {20.f, -668.0f},  { -16.0f , -588.0f }, { -52.0f, -508.0f }, { -52.0f, -428.0f }, {-16.f, -348.0f}, { 20.0f, -268.0f } };
	const TArray<FVector2D> PCL7 = { {20.f, -712.0f},  { -16.0f , -632.0f }, { -34.0f, -552.0f }, { -52.0f, -472.0f }, {-34.f, -392.0f }, { -16.0f, -312.0f }, { 20.0f, -232.0f } };
	const TArray<FVector2D> PCL8 = { {20.f, -748.0f},  { -16.0f , -668.0f }, { -34.0f, -588.0f }, { -52.0f, -508.0f }, {-52.f, -428.0f }, { -34.0f, -348.0f }, { -16.0f, -268.0f }, { 20.0f, -188.0f } };

	const int32 PlayerOptionNum = FMath::Min(ActiveDlgContext->GetOptionsNum(), PlayerChoiceTextBoxArray.Num());
	const TArray<FVector2D>* ChoiceDataArrays[] = { nullptr,
													nullptr,
													&PCL2,
													&PCL3,
													&PCL4,
													&PCL5,
													&PCL6,
													&PCL7,
													&PCL8 };

	const TArray<FVector2D>* AppearanceDataPtr = ChoiceDataArrays[PlayerOptionNum];
	if (!ensure(AppearanceDataPtr))
		return;

	for (int32 i = 0; i < PlayerOptionNum; ++i)
	{
		ensure(i < AppearanceDataPtr->Num());
		PlayerChoiceTextBoxArray[i]->DisplayText(ActiveDlgContext->GetOptionText(i), (*AppearanceDataPtr)[i], (PlayerOptionNum <= 3 ? 0.3f : 0.2f) * i, i == 0);
	}

	ActivePlayerOption = 0;
	PlayerEmotion = ActiveDlgContext->GetOptionSpeakerState(ActivePlayerOption);
	CharPreviewLeft->UpdateFromPlayerCharacter(PlayerEmotion);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::HidePlayerOptions()
{
	if (!bPlayerChoicesDisplayed)
		return;

	ActivePlayerOption = 0;
	bWaitForTextBoxFadeOut = true;
	SoPanelState = ESoDialoguePanelState::EDPS_Change;
	LastChangeStartTime = GetWorld()->GetTimeSeconds();

	bPlayerChoicesDisplayed = false;
	for (USoUIDialogueChoiceBox* PlayerChoice : PlayerChoiceTextBoxArray)
		PlayerChoice->HideChoiceTextBox();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::HideNotOptionTextBoxes()
{
	for (int32 i = 0; i < DialogueTextBoxArray.Num(); ++i)
		DialogueTextBoxArray[i]->HideDialogueTextBox();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialoguePanel::ChangeActivePlayerChoice(ESoUICommand Command)
{
	int32 Delta = 0;
	switch (Command)
	{
		case ESoUICommand::EUC_Left:
		case ESoUICommand::EUC_Up:
			Delta = -1;
			break;
		case ESoUICommand::EUC_Right:
		case ESoUICommand::EUC_Down:
			Delta = 1;
			break;
		default:
			break;
	}

	if (Delta != 0 &&
		bPlayerChoicesDisplayed &&
		PlayerChoiceTextBoxArray.IsValidIndex(ActivePlayerOption) &&
		PlayerChoiceTextBoxArray[ActivePlayerOption]->CanHighlightChange())
	{
		const int32 NewActiveOption = USoMathHelper::WrapIndexAround(ActivePlayerOption + Delta, FMath::Min(ActiveDlgContext->GetOptionsNum(), PlayerChoiceTextBoxArray.Num()));
		if (PlayerChoiceTextBoxArray[NewActiveOption]->CanHighlightChange())
		{
			PlayerChoiceTextBoxArray[ActivePlayerOption]->SetHighlighted(false);
			ActivePlayerOption = NewActiveOption;
			PlayerChoiceTextBoxArray[ActivePlayerOption]->SetHighlighted(true);
			SoPanelState = ESoDialoguePanelState::EDPS_Change;
			LastChangeStartTime = GetWorld()->GetTimeSeconds();

			PlayerEmotion = ActiveDlgContext->GetOptionSpeakerState(ActivePlayerOption);
			CharPreviewLeft->UpdateFromPlayerCharacter(PlayerEmotion);

			USoAudioManager::PlaySound2D(this, SFXOptionSwitch);
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialoguePanel::IsAnyTextBoxAnimated() const
{
	for (auto* TextBox : DialogueTextBoxArray)
		if (TextBox != nullptr && TextBox->IsDialogueTextBoxAnimated())
			return true;

	for (auto* ChoiceBox : PlayerChoiceTextBoxArray)
		if (ChoiceBox != nullptr && ChoiceBox->IsChoiceTextBoxAnimated())
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::PlayAnimationSafe(FName AnimName)
{
	UWidgetAnimation** AnimPtrPtr = AnimationMap.Find(AnimName);
	if (AnimPtrPtr == nullptr || (*AnimPtrPtr) == nullptr)
	{
		UE_LOG(LogSoDlgUI, Warning, TEXT("Failed to play animation: %s"), *AnimName.ToString());
		return;
	}

	UE_LOG(LogSoDlgUI, Display, TEXT("Played animation: %s"), *AnimName.ToString());
	PlayAnimation(*AnimPtrPtr);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialoguePanel::UpdateNameFields()
{
	if (ActiveDlgContext == nullptr)
		return;

	for (const auto& Pair : ParticipantStateMap)
		if (Pair.Value)
		{
			const UObject* Participant = ActiveDlgContext->GetParticipant(Pair.Key);
			if (Participant != nullptr)
			{
				const FSoDialogueParticipantData& ParticipantData = Cast<ISoDialogueParticipant>(Participant)->GetParticipantData();

				if (!DialogueTextBoxArray.IsValidIndex(ParticipantData.PositionIndex))
					return;

				DialogueTextBoxArray[ParticipantData.PositionIndex]->SetSpeakerData(ParticipantData.TextBoxBackground,
																					ParticipantData.NameTextBackground,
																					ParticipantData.ParticipantDisplayName);
			}
		}
}
