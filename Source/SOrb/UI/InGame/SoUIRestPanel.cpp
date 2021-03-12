// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIRestPanel.h"

#include "Blueprint/UserWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "FMODEvent.h"

#include "DlgDialogueParticipant.h"

#include "Character/SoCharacter.h"
#include "Basic/SoGameInstance.h"
#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Dialogues/SoUIDialogueChoiceBox.h"
#include "UI/InGame/Spells/SoUISpellSelection.h"
#include "UI/SoUIHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUIRestPanel, All, All);


const FName USoUIRestPanel::AnimNameFadeIn = FName("BackgroundFadeIn");
const FName USoUIRestPanel::AnimNameFadeOut = FName("BackgroundFadeOut");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIRestPanel::USoUIRestPanel(const FObjectInitializer& PCIP) :
	Super(PCIP)
{
	OptionTexts.SetNum(5);
	OptionPositions.SetNum(5);
}


#if WITH_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts or when spawned
void USoUIRestPanel::PreSave(const ITargetPlatform* TargetPlatform)
{
	for (FSoUIMusicEntry& Entry : MusicMap)
		if (Entry.Music != nullptr)
			Entry.DisplayName = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(*Entry.DisplayName.ToString(),
																												TEXT("MusicName"),
																												*(Entry.Music->GetName() + "_display_name"));
	Super::PreSave(TargetPlatform);
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIRestPanel::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	USoStaticHelper::BuildAnimationMap(this, AnimationMap);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIRestPanel::SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable)
{
	if (bEnable)
	{
		if (bOpened)
			return true;

		FadeIn();
	}
	else
	{
		if (bOpened)
		{
			bOpened = false;
			FadeOut();
		}
	}

	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIRestPanel::HandleCommand_Implementation(ESoUICommand Command)
{
	Command = USoUIHelper::TryTranslateMenuCommandDirectionToGame(Command);
	if (SoPanelState == ESoRestPanelState::ERPS_SpellSelection)
	{
		if (Command == ESoUICommand::EUC_MainMenuBack)
		{
			SpellSelection->SetInGameActivityEnabled(this, false);
			SoPanelState = ESoRestPanelState::ERPS_WaitForInput;
		}
		else
			SpellSelection->HandleCommand(Command);

		return true;
	}

	if (Command == ESoUICommand::EUC_MainMenuBack && SoPanelState != ESoRestPanelState::ERPS_FadeOut)
	{
		bOpened = false;
		FadeOut();
		return true;
	}

	if (SoPanelState != ESoRestPanelState::ERPS_WaitForInput)
		return true;

	int32 Delta = 0;
	switch (Command)
	{
		case ESoUICommand::EUC_MainMenuEnter:
			OnEnterOption();
			USoAudioManager::PlaySound2D(this, SFXOptionSelected);
			break;

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

	if (Delta != 0)
	{
		const int32 OldIndex = ActiveOptionIndex;
		const int32 NewIndex = USoMathHelper::WrapIndexAround(ActiveOptionIndex + Delta, PlayerChoiceTextBoxArray.Num());
		PlayerChoiceTextBoxArray[OldIndex]->SetHighlighted(false);
		PlayerChoiceTextBoxArray[NewIndex]->SetHighlighted(true);
		SoPanelState = ESoRestPanelState::ERPS_Change;
		ActiveOptionIndex = NewIndex;
		OnSelectedOptionChange(OldIndex, NewIndex);


		bFirstActive = !bFirstActive;
		BlendValue = 0.0f;
		if (BallImages.IsValidIndex(NewIndex))
			(bFirstActive ? Ball0 : Ball1)->SetBrushFromTexture(BallImages[NewIndex]);

		USoAudioManager::PlaySound2D(this, SFXOptionChange);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIRestPanel::Update_Implementation(float DeltaSeconds)
{
	if (BlendValue < 1.0f)
	{
		BlendValue += DeltaSeconds * BlendSpeedMultiplier;

		Ball0->SetRenderOpacity(bFirstActive ? BlendValue : 1.0f - BlendValue);
		Ball1->SetRenderOpacity(bFirstActive ? 1.0f - BlendValue : BlendValue);
	}

	switch (SoPanelState)
	{
		case ESoRestPanelState::ERPS_FadeIn:
			if (!IsPlayingAnimation())
				SoPanelState = ESoRestPanelState::ERPS_WaitForInput;
			break;

		case ESoRestPanelState::ERPS_Change:
			if (!IsAnyTextBoxAnimated())
				SoPanelState = ESoRestPanelState::ERPS_WaitForInput;
			break;

		case ESoRestPanelState::ERPS_FadeOut:
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
void USoUIRestPanel::FadeIn()
{
	bOpened = true;
	SetVisibility(ESlateVisibility::Visible);
	SoPanelState = ESoRestPanelState::ERPS_FadeIn;
	ActiveOptionIndex = 0;
	PlayedMusicIndex = 0;

	PlayAnimationSafe(AnimNameFadeIn);
	for (int32 i = 0; i < PlayerChoiceTextBoxArray.Num(); ++i)
		if (OptionTexts.IsValidIndex(i))
			PlayerChoiceTextBoxArray[i]->DisplayText(OptionTexts[i], OptionPositions[i], 0.5f, i == 0);

	PlayedMusicIndex = -1;
	// SwitchMusic();

	bFirstActive = true;
	BlendValue = 1.0f;
	if (BallImages.IsValidIndex(0))
		Ball0->SetBrushFromTexture(BallImages[0]);
	Ball0->SetRenderOpacity(1.0f);
	Ball1->SetRenderOpacity(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIRestPanel::FadeOut()
{
	SoPanelState = ESoRestPanelState::ERPS_FadeOut;
	PlayAnimationSafe(AnimNameFadeOut);

	for (USoUIDialogueChoiceBox* PlayerChoice : PlayerChoiceTextBoxArray)
		PlayerChoice->HideChoiceTextBox();

	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		Character->UpdateMusic(true);

	USoAudioManager::PlaySound2D(this, SFXClosePanel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIRestPanel::IsAnyTextBoxAnimated() const
{
	for (auto* ChoiceBox : PlayerChoiceTextBoxArray)
		if (ChoiceBox != nullptr && ChoiceBox->IsChoiceTextBoxAnimated())
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIRestPanel::PlayAnimationSafe(FName AnimName)
{
	UWidgetAnimation** AnimPtrPtr = AnimationMap.Find(AnimName);
	if (AnimPtrPtr == nullptr || (*AnimPtrPtr) == nullptr)
	{
		UE_LOG(LogSoUIRestPanel, Warning, TEXT("Failed to play animation: %s"), *AnimName.ToString());
		return;
	}

	UE_LOG(LogSoUIRestPanel, Display, TEXT("Played animation: %s"), *AnimName.ToString());
	PlayAnimation(*AnimPtrPtr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIRestPanel::OpenSpellSelection()
{
	SpellSelection->SetInGameActivityEnabled(this, true);
	SoPanelState = ESoRestPanelState::ERPS_SpellSelection;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUIRestPanel::SwitchMusic()
{
	AActor* Character = USoStaticHelper::GetPlayerCharacterAsActor(this);

	PlayedMusicIndex += 1;
	while (MusicMap.IsValidIndex(PlayedMusicIndex))
	{
		if (MusicMap[PlayedMusicIndex].Music != nullptr && IDlgDialogueParticipant::Execute_GetBoolValue(Character, MusicMap[PlayedMusicIndex].Music->GetFName()))
		{
			USoAudioManager::Get(this).SetMusic(MusicMap[PlayedMusicIndex].Music, true, 0.2f);
			return PlayedMusicIndex;
		}
		++PlayedMusicIndex;
	}

	PlayedMusicIndex = 0;
	if (MusicMap.Num() > 0 && MusicMap[0].Music != nullptr)
		USoAudioManager::Get(this).SetMusic(MusicMap[PlayedMusicIndex].Music, true, 0.2f);
	return PlayedMusicIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UUserWidget* USoUIRestPanel::GetNotUsedWidget(TSubclassOf<UUserWidget> Class)
{
	FSoUIWidgetArray* WidgetArray = WidgetStorage.Find(Class);

	if (WidgetArray == nullptr)
		WidgetArray = &WidgetStorage.Add(Class);

	for (UUserWidget* Widget : WidgetArray->Widgets)
		if (!Widget->IsVisible())
			return Widget;

	UUserWidget* Widget = CreateWidget(USoStaticHelper::GetPlayerController(this), Class);
	Widget->AddToViewport();
	WidgetArray->Widgets.Add(Widget);

	return Widget;
}
