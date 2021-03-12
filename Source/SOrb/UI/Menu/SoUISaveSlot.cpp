// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.


#include "SoUISaveSlot.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"

#include "SaveFiles/SoWorldStateTable.h"
#include "SaveFiles/SoWorldState.h"
#include "SoBeforeGame/Public/SoLocalization.h"
#include "Levels/SoLevelHelper.h"
#include "Basic/SoGameSingleton.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlot::InitializeAsNew()
{
	HideAll();
	SetNotStartedSlotElementsVisibility(true);
	BGNew->SetBrushFromTexture(EmptyBackground);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlot::InitializeFromState(const FSoStateTable& State, bool bCurrentlyLoaded)
{
	HideAll();
	
	if (FSoWorldState::IsStateTableAGameCompleted(State))
	{
		SetFinishedSlotElementsVisibility(true);
	}
	else
	{
		SetRunningSlotElementsVisibility(true);

		// Chapter Text
		ChapterText->SetText(USoLevelHelper::ChapterNameToFriendlyText(State.Metadata.MapName));
		ChapterText->SetColorAndOpacity(bCurrentlyLoaded ? AreaTextColorActiveSlot : AreaTextColorDefault);
		if (!bCurrentlyLoaded)
			LoadedText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Progress text and image
	BG->SetBrushFromTexture(EmptyBackground);
	if (USoGameSingleton* Singleton = USoGameSingleton::GetInstance())
		if (FSoCharacterProgressInfo* ProgressInfoPtr = Singleton->CharacterProgressInfo.Find(State.Metadata.DisplayedProgressName))
		{
			ProgressText->SetText(ProgressInfoPtr->DisplayText);
			FinishedProgressText->SetText(ProgressInfoPtr->DisplayText);
			if (ProgressInfoPtr->Image != nullptr)
				BG->SetBrushFromTexture(ProgressInfoPtr->Image);
		}

	// update death count
	FNumberFormattingOptions Format;
	Format.SetUseGrouping(false);
	DeathCountText->SetText(FText::AsNumber(State.Metadata.ProgressStats.TotalDeathNum, &Format));
	FinishedDeathCountText->SetText(FText::AsNumber(State.Metadata.ProgressStats.TotalDeathNum, &Format));

	// difficulty
	switch (State.Metadata.Difficulty)
	{
	case ESoDifficulty::Sane:
		FinishedDifficulty->SetText(FROM_STRING_TABLE_UI("difficulty_sane"));
		DifficultyText->SetText(FROM_STRING_TABLE_UI("difficulty_sane"));
		DifficultyText->SetColorAndOpacity(DifficultyColorSane);
		break;

	case ESoDifficulty::Intended:
		FinishedDifficulty->SetText(FROM_STRING_TABLE_UI("difficulty_intended"));
		DifficultyText->SetText(FROM_STRING_TABLE_UI("difficulty_intended"));
		DifficultyText->SetColorAndOpacity(DifficultyColorIntended);
		break;

	case ESoDifficulty::Insane:
		FinishedDifficulty->SetText(FROM_STRING_TABLE_UI("difficulty_insane"));
		DifficultyText->SetText(FROM_STRING_TABLE_UI("difficulty_insane"));
		DifficultyText->SetColorAndOpacity(DifficultyColorInsane);
		break;

	default:
		ensure(false);
		break;
	}

	// update playtime text
	const int32 HoursPlayed = static_cast<int32>(State.Metadata.ProgressStats.TotalPlayTimeSeconds) / 3600;
	if (HoursPlayed > 0)
	{
		FFormatOrderedArguments SlotArguments;
		SlotArguments.Add(HoursPlayed);
		SlotArguments.Add(static_cast<int32>(State.Metadata.ProgressStats.TotalPlayTimeSeconds / 60.0f) - HoursPlayed * 60);
		PlayTimeText->SetText(FText::Format(FROM_STRING_TABLE_UI("save_slots_playtime"), SlotArguments));
		FinishedTime->SetText(FText::Format(FROM_STRING_TABLE_UI("save_slots_playtime"), SlotArguments));
	}
	else
	{
		FFormatOrderedArguments SlotArguments;
		SlotArguments.Add(static_cast<int32>(State.Metadata.ProgressStats.TotalPlayTimeSeconds / 60.0f));
		PlayTimeText->SetText(FText::Format(FROM_STRING_TABLE_UI("save_slots_playtime_short"), SlotArguments));
		FinishedTime->SetText(FText::Format(FROM_STRING_TABLE_UI("save_slots_playtime_short"), SlotArguments));
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlot::SetRunningSlotElementsVisibility(bool bVisible)
{
	ChapterText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ProgressText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	DifficultyText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	DeathCountText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	DeathCountIcon->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PlayTimeText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	LoadedText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	BG->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlot::SetNotStartedSlotElementsVisibility(bool bVisible)
{
	NewGameText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	BGNew->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlot::SetFinishedSlotElementsVisibility(bool bVisible)
{
	FinishedProgressText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	FinishedDifficulty->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	FinishedTime->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	FinishedDeathCountText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	FinishedDeathCountIcon->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	BG->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
