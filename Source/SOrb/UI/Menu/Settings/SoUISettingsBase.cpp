// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISettingsBase.h"

#include "UI/General/Buttons/SoUIScrollingButtonArray.h"
#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "UI/General/Buttons/SoUIButton.h"
#include "UI/General/SoUISlider.h"
#include "UI/General/SoUICheckbox.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Character/SoCharacter.h"
#include "Settings/SoGameSettings.h"
#include "UI/SoUIHelper.h"
#include "UI/General/Buttons/SoUIButtonArray.h"

DEFINE_LOG_CATEGORY(LogSoUISettings);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUISettingsBase::USoUISettingsBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TrackedInput.KeyDownThresholdSeconds = 0.2f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (WidgetsArray)
	{
		WidgetsArray
			->SetVerticalLayout(true)
			->SetSkipDeactivatedChildren(true)
			->SetCyclicNavigation(true)
			->SetFakeVirtualContainer(bFakeWidgetsArray)
			->SetSFXChildSelectionChanged(USoGameSingleton::GetSFX(ESoSFX::SettingsLineSwitch));

		if (bFakeWidgetsArray)
			WidgetsArray->SetVisibility(ESlateVisibility::Collapsed);
	}

	CreateChildrenAndSetTexts(true);
	RefreshButtonImagesTooltips(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!SoCharacter || !IsValidWidget() || !SoCharacter->IsAnyUIInputPressed() || ShouldChildHandleCommands())
	{
		TrackedInput.Reset();
		return;
	}

	// Pressed left/right change value
	TrackedInput.Tick(InDeltaTime, [&]()
	{
		const bool bFromPressedInput = true;
		if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Left, ESoUICommand::EUC_MainMenuLeft }))
			NavigateLeftRightOnSelectedLine(ESoUICommand::EUC_Left, bFromPressedInput);
		else if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Right, ESoUICommand::EUC_MainMenuRight }))
			NavigateLeftRightOnSelectedLine(ESoUICommand::EUC_Right, bFromPressedInput);
		else if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Up, ESoUICommand::EUC_MainMenuUp }))
			NavigateSwitchLineUp();
		else if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Down, ESoUICommand::EUC_MainMenuDown }))
			NavigateSwitchLineDown();
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);

	SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	UserSettings = USoGameSettings::GetInstance();
	SoController = USoUIHelper::GetSoPlayerControllerFromUWidget(this);
	verify(SoCharacter);
	verify(UserSettings);
	verify(SoController);

	if (WidgetsArray && TitlesArray)
	{
		// Navigate on simple child
		WidgetsArray->OnNavigateOnPressedHandleChildEvent().BindUObject(this, &ThisClass::OnPressedChild);

		// New selected index
		WidgetsArray->OnPostSetSelectedIndexEvent().AddUObject(this, &ThisClass::OnPostSetSelectedIndex);

		// Sync widgets array if we the titles array modified but we are different
		TitlesArray->OnPostSetSelectedIndexEvent().AddLambda([this](int32 OldIndex, int32 NewIndex)
		{
			UE_LOG(
				LogSoUI,
				Verbose,
				TEXT("[%s] TitlesArray->OnPostSetSelectedIndexEvent(OldIndex = %d, NewIndex = %d)"),
				*GetName(), OldIndex, NewIndex
			);
			if (WidgetsArray->GetSelectedIndex() != NewIndex)
				WidgetsArray->SetSelectedIndex(NewIndex, true);
		});

		// Highlight sync
		TitlesArray->OnChildHighlightChangedEvent().AddLambda([this](int32 ChildIndex, bool bHighlight)
		{
			UE_LOG(
				LogSoUI,
				Verbose,
				TEXT("[%s] TitlesArray->OnChildHighlightChangedEvent(ChildIndex = %d, bHighlight = %d)"),
				*GetName(), ChildIndex, bHighlight
			);
			WidgetsArray->SetIsHighlightedOnChildAt(ChildIndex, bHighlight, bHighlight);
		});
		WidgetsArray->OnChildHighlightChangedEvent().AddLambda([this](int32 ChildIndex, bool bHighlight)
		{
			UE_LOG(
				LogSoUI,
				Verbose,
				TEXT("[%s] WidgetsArray->OnChildHighlightChangedEvent(ChildIndex = %d, bHighlight = %d)"),
				*GetName(), ChildIndex, bHighlight
			);
			TitlesArray->SetIsHighlightedOnChildAt(ChildIndex, bHighlight, bHighlight);
		});

		// Enabled sync
		WidgetsArray->OnChildEnabledChangedEvent().AddLambda([this](int32 ChildIndex, bool bEnabled)
		{
			UE_LOG(
				LogSoUI,
				Verbose,
				TEXT("[%s] WidgetsArray->OnChildEnabledChangedEvent(ChildIndex = %d, bEnabled = %d)"),
				*GetName(), ChildIndex, bEnabled
			);
			TitlesArray->SetIsEnabledOnChildAt(ChildIndex, bEnabled);
		});

		// Add our array as an extra attached child so it won't be affected
		// By manipulating the WidgetArray Container
		if (ButtonImagesTooltipsArray)
		{
			WidgetsArray->AddExtraAttachedChild(ButtonImagesTooltipsArray);
			WidgetsArray->SetExtraAttachedVerticalLayout(ButtonImagesTooltipsArray->IsVerticalLayout());

			// Handle NavigateOnPressed ButtonImagesTooltipsArray
			ButtonImagesTooltipsArray->OnNavigateOnPressedHandleChildEvent().BindUObject(this, &ThisClass::OnPressedButtonTooltipsChild);
		}
	}

	// First line
	SelectFirstValidLineIndex();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::NativeDestruct()
{
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	if (SoUserWidget)
		SoUserWidget->SetIsActive(!SoUserWidget->IsActive(), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::OnPressedButtonTooltipsChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	const ESoUICommand SelectedCommand = ButtonImagesTooltipsArray->GetSelectedButtonCommand();
	check(SelectedCommand == ButtonImagesTooltipsArray->GetButtonIndexCommand(SelectedChild));

	OnPressedButtonTooltipsCommand(ButtonImagesTooltipsArray->GetSelectedButtonCommand());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBase::HandleOnUICommand(ESoUICommand Command)
{
	switch (Command)
	{
		case ESoUICommand::EUC_Left:
		case ESoUICommand::EUC_Right:
			NavigateLeftRightOnSelectedLine(Command);
			return true;

		case ESoUICommand::EUC_Up:
			NavigateSwitchLineUp();
			return true;

		case ESoUICommand::EUC_Down:
			NavigateSwitchLineDown();
			return true;

		case ESoUICommand::EUC_MainMenuEnter:
			NavigateOnPressed();
			return true;

		case ESoUICommand::EUC_MainMenuBack:
			NavigateBackCommand();
			return true;

		default:
			return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::NavigateBackCommand()
{
	// Close this widget
	ISoUIEventHandler::Execute_Open(this, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::Open_Implementation(bool bOpen)
{
#if PLATFORM_SWITCH
	USoGameSettings::Get().SetCacheSaveRequestsMode(bOpen);
#endif

	bOpened = bOpen;
	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// Clear
	if (bOpen)
	{
		SelectFirstValidLineIndex();
		EmptyAndRefreshButtonImagesTooltips();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::CreateChildrenAndSetTexts(bool bFromSynchronizeProperties)
{
	if (TitlesArray)
		TitlesArray->CreateButtons(TitlesArrayTexts);

	if (TitlesArrayTexts.Num() != NumLines())
		UE_LOG(LogSoUISettings, Error, TEXT("CreateChildrenAndSetTexts: TitlesArrayTexts.Num(%d) != NumLines(%d)"), TitlesArrayTexts.Num(), NumLines());

	UpdateWidgetsVisibilities();

	if (BackgroundsContainer)
	{
		// Check if the number of lines matches the background
		const int32 BackgroundChildrenNum = BackgroundsContainer->GetChildrenCount();

		// Show all background up to the lines
		for (int32 Index = 0; Index < NumLines() && Index < BackgroundChildrenNum; Index++)
		{
			BackgroundsContainer->GetChildAt(Index)->SetVisibility(ESlateVisibility::Visible);
		}
		// Hide the rest
		for (int32 Index = NumLines(); Index < BackgroundChildrenNum; Index++)
		{
			BackgroundsContainer->GetChildAt(Index)->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBase::NavigateSwitchLine(bool bUp)
{
	if (!bAllowChangingLines)
		return false;

	if (!WidgetsArray)
		return false;

	const ESoUICommand Command = bUp ? ESoUICommand::EUC_Up : ESoUICommand::EUC_Down;

	// Navigate along the buttons
	WidgetsArray->Navigate(Command);
	return WidgetsArray->WasChildChangedInNavigate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBase::NavigateOnPressed()
{
	// OnPressedChild should be called from the array
	return WidgetsArray->NavigateOnPressed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBase::NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput)
{
	// Ignore navigation is on slider
	if (bAllowChangingLines)
	{
		if (USoUISlider* Slider = GetSliderAtLineIndex(GetSelectedLineIndex()))
		{
			if (bFromPressedInput)
				return false;
		}
	}

	return WidgetsArray->Navigate(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBase::IsValidLineIndex(int32 LineIndex) const
{
	return WidgetsArray ? WidgetsArray->IsValidIndex(LineIndex) : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBase::IsLineEnabled(int32 LineIndex) const
{
	return IsLineIndexForButtonImagesArray(LineIndex) ||
		(IsValidLineIndex(LineIndex) && WidgetsArray->IsChildEnabled(LineIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBase::SetIsLineEnabled(int32 LineIndex, bool bEnabled)
{
	if (!WidgetsArray)
		return false;

	return WidgetsArray->SetIsEnabledOnChildAt(LineIndex, bEnabled);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::AddLine(UUserWidget* Widget)
{
	if (!WidgetsArray)
		return;

	WidgetsArray->AddContainerChild(Widget);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::EmptyLines()
{
	if (!WidgetsArray)
		return;

	WidgetsArray->EmptyContainerChildren(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::EmptyTitles()
{
	TitlesArrayTexts.Empty();
	if (TitlesArray)
		TitlesArray->EmptyContainerChildren(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::EmptyAndRefreshButtonImagesTooltips(bool bResetSelectedLine)
{
	if (ButtonImagesTooltipsArray)
		ButtonImagesTooltipsArray->EmptyContainerChildren(true);

	RefreshButtonImagesTooltips();

	// Reset back to first index if selected the buttons image array but it is a invalid selection
	if (bResetSelectedLine && IsOnButtonTooltips() && !HasValidButtonImagesArray())
		SelectFirstValidLineIndex();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBase::HasValidButtonImagesArray() const
{
	return ButtonImagesTooltipsArray && ButtonImagesTooltipsArray->IsVisible() && ButtonImagesTooltipsArray->GetIsEnabled()
		&& ButtonImagesTooltipsArray->NumAllChildren() > 0 && ButtonImagesTooltipsArray->IsSelectedChildHighlighted();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBase::IsLineIndexForButtonImagesArray(int32 LineIndex) const
{
	return WidgetsArray && WidgetsArray->IsIndexForExtraAttachedChildren(LineIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::OnPostSetSelectedIndex(int32 OldIndex, int32 NewIndex)
{
	// UE_LOG(
	// 	LogSoUISettings,
	// 	Verbose,
	// 	TEXT("%s::OnPostSetSelectedIndex(OldIndex = %d, NewIndex = %d)"),
	// 	*GetName(), OldIndex, NewIndex
	// );

	OnLineChanged(OldIndex, NewIndex);
	if (TitlesArray)
		TitlesArray->SetSelectedIndex(NewIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::SetSelectedLineIndex(int32 NewIndex, bool bPlaySound)
{
	// ButtonImagesTooltipsArray->UnhighlightAllChildren();
	WidgetsArray->SetSelectedIndex(NewIndex, bPlaySound);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::SelectFirstValidLineIndex(bool bPlaySound)
{
	// ButtonImagesTooltipsArray->UnhighlightAllChildren();
	WidgetsArray->Highlight(bPlaySound);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBase::SelectButtonTooltips(bool bPlaySound)
{
	WidgetsArray->SetSelectedExtraAttachedIndex(0, bPlaySound);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISettingsBase::GetSelectedLineIndex() const
{
	return WidgetsArray->GetSelectedIndex();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISettingsBase::NumLines() const
{
	// NOTE: done on purpose because the children classes don't care about ButtonImagesTooltipsArray
	return WidgetsArray->NumContainerChildren();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UUserWidget* USoUISettingsBase::GetUserWidgetAtLineIndex(int32 LineIndex) const
{
	return WidgetsArray ? WidgetsArray->GetChildAt(LineIndex) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIButtonArray* USoUISettingsBase::GetButtonArrayAtLineIndex(int32 LineIndex) const
{
	return Cast<USoUIButtonArray>(GetUserWidgetAtLineIndex(LineIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIScrollingButtonArray* USoUISettingsBase::GetScrollingButtonArrayAtLineIndex(int32 LineIndex) const
{
	return Cast<USoUIScrollingButtonArray>(GetUserWidgetAtLineIndex(LineIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIButton* USoUISettingsBase::GetButtonAtLineIndex(int32 LineIndex) const
{
	return Cast<USoUIButton>(GetUserWidgetAtLineIndex(LineIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUICheckbox* USoUISettingsBase::GetCheckboxAtLineIndex(int32 LineIndex) const
{
	return Cast<USoUICheckbox>(GetUserWidgetAtLineIndex(LineIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUISlider* USoUISettingsBase::GetSliderAtLineIndex(int32 LineIndex) const
{
	return Cast<USoUISlider>(GetUserWidgetAtLineIndex(LineIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIButtonArray* USoUISettingsBase::GetWidgetsArrayAsButtons() const
{
	return Cast<USoUIButtonArray>(WidgetsArray);
}
