// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.


#include "SoUIDifficultySelection.h"

#include "Components/RichTextBlock.h"
#include "Components/Image.h"

#include "UI/General/Buttons/SoUIButton.h"
#include "UI/Menu/ConfirmPanels/SoUIConfirmQuestion.h"
#include "SoLocalization.h"
#include "SaveFiles/SoWorldState.h"
#include "Character/SoCharacter.h"
#include "Character/SoCharStates/SoADefault.h"
#include "Levels/SoLevelHelper.h"
#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Online/Analytics/SoAnalytics.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDifficultySelection::NativeConstruct()
{
	bCyclicNavigation = true;
	CreateChildrenAndSetTexts();

	OnNavigateOnPressedHandleChildEvent().BindUObject(this, &USoUIDifficultySelection::OnHandleButtonPress);
	if (Confirmation != nullptr)
	{
		Confirmation->OnQuestionAnsweredEvent().AddDynamic(this, &ThisClass::OnConfirmQuestionAnswered);
		Confirmation->SetPlayBackToMenuRootSFXOnNo(false);
	}

	OnPostSetSelectedIndexEvent().AddLambda([this](int32 OldIndex, int32 NewIndex)
	{
		UpdateDescription();
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDifficultySelection::NativeDestruct()
{
	if (Confirmation != nullptr)
		Confirmation->OnQuestionAnsweredEvent().RemoveDynamic(this, &ThisClass::OnConfirmQuestionAnswered);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDifficultySelection::OnUICommand_Implementation(ESoUICommand Command)
{
	if (!ISoUIEventHandler::Execute_IsOpened(this))
		return false;

	// Confirmation Open
	if (IsValid(Confirmation) && ISoUIEventHandler::Execute_IsOpened(Confirmation))
	{
		return ISoUIEventHandler::Execute_OnUICommand(Confirmation, Command);
	}

	switch (Command)
	{
		case ESoUICommand::EUC_Left:
		case ESoUICommand::EUC_Right:
		case ESoUICommand::EUC_Up:
		case ESoUICommand::EUC_Down:
		case ESoUICommand::EUC_MainMenuEnter:
			// Navigate on the buttons
			Navigate(Command);
			UpdateDescription();
			return true;

		case ESoUICommand::EUC_MainMenuBack:
			ISoUIEventHandler::Execute_Open(this, false);
			return true;

		default:
			return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDifficultySelection::Open_Implementation(bool bOpen)
{
	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetSelectedIndex(1);
	UpdateDescription();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDifficultySelection::CreateChildrenAndSetTexts()
{
	// indices are expected to map to ESoDifficulty

	TArray<FText> ChildrenTexts;
	AddContainerButton(DifficultySane);
	ChildrenTexts.Add(FROM_STRING_TABLE_UI("difficulty_sane"));
	AddContainerButton(DifficultyIntended);
	ChildrenTexts.Add(FROM_STRING_TABLE_UI("difficulty_intended"));
	AddContainerButton(DifficultyInsane);
	ChildrenTexts.Add(FROM_STRING_TABLE_UI("difficulty_insane"));

	check(NumContainerChildren() == ChildrenTexts.Num());

	SetButtonsTexts(ChildrenTexts);

	DifficultySane->Activate();
	DifficultyIntended->Activate();
	DifficultyInsane->Activate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDifficultySelection::UpdateDescription()
{
	ESoDifficulty Difficulty = static_cast<ESoDifficulty>(SelectedIndex);
	switch (Difficulty)
	{
		case ESoDifficulty::Sane:
			DifficultyDescription->SetText(FROM_STRING_TABLE_UI("difficulty_desc_sane"));
			DifficultyIcon->SetBrushFromTexture(IconSane);
			break;

		case ESoDifficulty::Intended:
			DifficultyDescription->SetText(FROM_STRING_TABLE_UI("difficulty_desc_intended"));
			DifficultyIcon->SetBrushFromTexture(IconIntended);
			break;

		case ESoDifficulty::Insane:
			DifficultyDescription->SetText(FROM_STRING_TABLE_UI("difficulty_desc_insane"));
			DifficultyIcon->SetBrushFromTexture(IconInsane);
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDifficultySelection::SetButtonVisibility(bool bVisible)
{
	DifficultySane->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	DifficultyIntended->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	DifficultyInsane->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDifficultySelection::OnHandleButtonPress(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	if (SoUserWidget == nullptr || !IsValidIndex(SelectedChild))
		return;

	// SFX
	SoUserWidget->NavigateOnPressed(true);

	// External link handled
	if (IsIndexForExtraAttachedChildren(SelectedChild))
	{
		return;
	}

	if (SelectedIndex >= 0 && SelectedIndex < static_cast<int32>(ESoDifficulty::NumOf))
	{
		SelectedDifficulty = static_cast<ESoDifficulty>(SelectedIndex);

		Confirmation->SetTexts(
			FROM_STRING_TABLE_UI("difficulty_confirm_question"),
			FROM_STRING_TABLE_UI("difficulty_confirm_subtext")
		);
		SetButtonVisibility(false);
		ISoUIEventHandler::Execute_Open(Confirmation, true);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDifficultySelection::OnConfirmQuestionAnswered()
{
	if (Confirmation->IsAnswerYes())
	{
		DifficultySelected.Broadcast(SelectedDifficulty);
		ISoUIEventHandler::Execute_Open(this, false);
	}
	else
	{
		SetButtonVisibility(true);
	}
}
