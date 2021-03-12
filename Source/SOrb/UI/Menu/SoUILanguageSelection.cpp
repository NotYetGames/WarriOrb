// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUILanguageSelection.h"

#include "Components/TextBlock.h"
#include "Localization/SoLocalizationHelper.h"
#include "UI/General/Buttons/SoUIButtonArray.h"
#include "Settings/SoGameSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUILanguageSelection, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUILanguageSelection::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	CreateButtonTexts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUILanguageSelection::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(bOpened ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// Pressed language button
	LanguageButtons->OnNavigateOnPressedHandleChildEvent().BindLambda([this](int32 SelectedChild, USoUIUserWidget* SoUserWidget)
	{
		if (!CanHandleInput())
			return;
		
		const ESoSupportedCulture SelectedCulture = SupportedCultures[SelectedChild];
		USoLocalizationHelper::SetCurrentCultureEverywhere(SelectedCulture, true);
		FireOnLanguageSelected();
	});

	// Get Current selected language and select that button as default
	const ESoSupportedCulture Current = USoLocalizationHelper::GetCurrentLanguageNameType();
	const int32 CurrentIndex = SupportedCultures.Find(Current);
	check(SupportedCultures.IsValidIndex(CurrentIndex));
	LanguageButtons->SetSelectedIndex(CurrentIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUILanguageSelection::NativeDestruct()
{
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUILanguageSelection::Open_Implementation(bool bOpen)
{
	bOpened = bOpen;
	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUILanguageSelection::CreateButtonTexts()
{
	if (LanguageButtons == nullptr)
		return;

	TArray<FText> ButtonsTexts;
	SupportedCultures = USoLocalizationHelper::GetAllConfigurableCultures();
	for (const auto& CultureType : SupportedCultures)
	{
		ButtonsTexts.Add(USoLocalizationHelper::GetDisplayTextFromSupportedCulture(CultureType, true));
	}

	LanguageButtons->CreateButtons(ButtonsTexts);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUILanguageSelection::FireOnLanguageSelected()
{
	if (bLanguageSelected)
		return;
	
	bLanguageSelected = true;
	USoGameSettings::Get().OnLanguageSelected();
	LanguageSelectedEvent.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUILanguageSelection::CanHandleInput() const
{
	const bool bThisCanHandle = !IsAnyAnimationPlaying() && GetVisibility() == ESlateVisibility::Visible;
	if (ParentUserWidget)
		return bThisCanHandle && !ParentUserWidget->IsAnyAnimationPlaying();
	
	return bThisCanHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUILanguageSelection::OnUICommand_Implementation(ESoUICommand Command)
{
	if (bLanguageSelected)
		return false;

	if (!CanHandleInput())
		return false;

	// Navigate buttons
	if (Command != ESoUICommand::EUC_MainMenuEnter)
	{
		LanguageButtons->Navigate(Command, true);
		return true;
	}

	// Pressed language button
	const int32 SelectedIndex = LanguageButtons->GetSelectedIndex();
	check(SupportedCultures.IsValidIndex(SelectedIndex));
	const ESoSupportedCulture SelectedCulture = SupportedCultures[SelectedIndex];
	USoLocalizationHelper::SetCurrentCultureEverywhere(SelectedCulture, true);
	FireOnLanguageSelected();

	return true;
}
