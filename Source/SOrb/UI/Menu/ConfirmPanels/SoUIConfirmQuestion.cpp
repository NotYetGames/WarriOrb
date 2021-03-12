// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIConfirmQuestion.h"

#include "Engine/Engine.h"
#include "Components/TextBlock.h"

#include "Localization/SoLocalization.h"
#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/SoUIUserWidget.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoAudioManager.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmQuestion::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	bBehaveAsCommandTooltips = false;

	if (ButtonsArray)
	{
		ButtonsArray
			->SetCyclicNavigation(true)
			->SetVerticalLayout(true)
			->SetSkipDeactivatedChildren(true)
			->SetUseCustomPadding(false, {})
			->SetTrackPressedInput(true)
			->SetSFXChildSelectionChanged(USoGameSingleton::GetSFX(ESoSFX::MenuButtonSwitch));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmQuestion::NativeConstruct()
{
	Super::NativeConstruct();

	ButtonsArray->OnNavigateOnPressedHandleChildEvent().BindLambda([this](int32 SelectedChild, USoUIUserWidget* SoUserWidget)
	{
		// SFX
		SoUserWidget->NavigateOnPressed(true);

		SetAnswer(AnswerButtonOption[SelectedChild]);
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmQuestion::Open_Implementation(bool bOpen)
{
	if (bOpened == bOpen)
		return;

	bOpened = bOpen;
	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bOpened)
	{
		// Opened again, reset the answer from last time
		ResetAnswer();

		// Set visibility on texts
		if (TextSubTitle)
		{
			TextSubTitle->SetVisibility(TextSubTitle->GetText().IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		}

		// Select the default index
		if (ButtonsArray)
			ButtonsArray->SetSelectedIndex(GetAnswerButtonOptionIndexFor(DefaultSelectedAnswerType));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIConfirmQuestion::OnUICommand_Implementation(ESoUICommand Command)
{
	if (!bOpened)
		return false;

	check(ButtonsArray);

	bool bHandled = false;
	switch (Command)
	{
	case ESoUICommand::EUC_Left:
	case ESoUICommand::EUC_Right:
	case ESoUICommand::EUC_Up:
	case ESoUICommand::EUC_Down:
	case ESoUICommand::EUC_MainMenuEnter:
		ButtonsArray->Navigate(Command);
		bHandled = true;
		break;

	case ESoUICommand::EUC_MainMenuBack:
		// Back, translate to NO
		SetAnswer(ESoUIConfirmQuestionAnswerType::No);
		bHandled = true;
		break;

	default:
		break;
	};

	if (bHandled)
		UpdateFromCurrentAnswer();

	return bHandled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmQuestion::SetTexts(FText Text, FText SubText)
{
	if (TextTitle)
		TextTitle->SetText(Text);
	if (TextSubTitle)
		TextSubTitle->SetText(SubText);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmQuestion::RefreshButtonsArray()
{
	if (!ButtonsArray)
		return;

	TArray<FText> ChildrenTexts;
	for (const ESoUIConfirmQuestionAnswerType Option : AnswerButtonOption)
	{
		switch (Option)
		{
		case ESoUIConfirmQuestionAnswerType::Yes:
			ChildrenTexts.Add(FROM_STRING_TABLE_UI("yes"));
			break;

		case ESoUIConfirmQuestionAnswerType::No:
			ChildrenTexts.Add(FROM_STRING_TABLE_UI("no"));
			break;

		default:
			checkNoEntry();
			break;
		}
	}

	ButtonsArray->CreateButtons(ChildrenTexts);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmQuestion::UpdateFromCurrentAnswer()
{
	if (IsAnswerNo())
	{
		if (bPlayBackToMenuRootSFXOnNo)
			USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::MenuReturnToRoot));

		ISoUIEventHandler::Execute_Open(this, false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmQuestion::SetAnswer(ESoUIConfirmQuestionAnswerType NewAnswer)
{
	ConfirmAnswerType = NewAnswer;
	UpdateFromCurrentAnswer();
	QuestionAnsweredEvent.Broadcast();
}
