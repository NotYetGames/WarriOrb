// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIScrollingButtonArray.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"

#include "SoUIButton.h"
#include "UI/General/SoUITypes.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoMathHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIScrollingButtonArray::USoUIScrollingButtonArray(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bVerticalLayout = false;
	bExtraAttachedVerticalLayout = false;
	bCyclicNavigation = true;
	bSkipDeactivatedChildren = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::SynchronizeProperties()
{
	bOverrideButtonsVisibility = false;

	// Call parent but don't set the parents default if we can
	const bool bPreviousValue = bSFXSetDefault;
	bSFXSetDefault = false;
	Super::SynchronizeProperties();
	bSFXSetDefault = bPreviousValue;

	if (bSFXSetDefault && SFXChildSelectionChanged == nullptr)
		SFXChildSelectionChanged = USoGameSingleton::GetSFX(ESoSFX::SettingsValueSwitch);

	// Should never happen?
	bSkipDeactivatedChildren = false;
	bVerticalLayout = false;
	bExtraAttachedVerticalLayout = false;
	bListenToChildHighlightRequest = false;
	bListenToChildPressedRequest = false;

	// Sync SFX to child buttons
	if (LeftArrowButton)
	{
		LeftArrowButton->SetHandleOwnMouseEvents(true);
		LeftArrowButton->SetSFXOnPressed(nullptr)
					   ->SetSFXHighlightChanged(SFXChildSelectionChanged);
	}
	if (RightArrowButton)
	{
		LeftArrowButton->SetHandleOwnMouseEvents(true);
		RightArrowButton->SetSFXOnPressed(nullptr)
						->SetSFXHighlightChanged(SFXChildSelectionChanged);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::NativeConstruct()
{
	Super::NativeConstruct();

	if (LeftArrowButton)
		LeftArrowButton->OnPressedEvent().AddUObject(this, &ThisClass::OnPressedArrowLeft);
	if (RightArrowButton)
		RightArrowButton->OnPressedEvent().AddUObject(this, &ThisClass::OnPressedArrowRight);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::NativeDestruct()
{
	if (LeftArrowButton)
		LeftArrowButton->OnPressedEvent().RemoveAll(this);
	if (RightArrowButton)
		RightArrowButton->OnPressedEvent().RemoveAll(this);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::CreateButtons(const TArray<FText>& ElementTexts, bool bIgnoreEmptyTexts)
{
	// NOTE: we need to have different children
	Super::CreateButtons(ElementTexts, bIgnoreEmptyTexts);

	// Empty the container but keep the ContainerChildren
	// NOTE: Container (children) != ContainerChildren
	if (Container)
	{
		Container->ClearChildren();
		Container->AddChild(LeftArrowButton);
		Container->AddChild(SelectedButton);
		Container->AddChild(RightArrowButton);

		bListenToChildHighlightRequest = true;
		SubscribeToChildEvents(SelectedButton, 0);
		bListenToChildHighlightRequest = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIScrollingButtonArray::Navigate(ESoUICommand Command, bool bPlaySound)
{
	Super::Navigate(Command, bPlaySound);
	//UpdateSelectedButton();
	//HighlightSelectedChild(false);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUIScrollingButtonArray::SelectFirstActiveChild(int32 StartFrom)
{
	const int32 Num = NumContainerChildren();
	if (Num == 0)
		return INDEX_NONE;

	SetSelectedIndex(FMath::Clamp(StartFrom, 0, Num - 1));
	return GetSelectedIndex();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::OnRequestChildHighlightChange(int32 ChildIndex, bool bHighlight)
{
	UE_LOG(
		LogSoUI,
		Verbose,
		TEXT("[%s] USoUIScrollingButtonArray::OnRequestChildHighlightChange(ChildIndex = %d, bHighlight = %d)"),
		*GetName(), ChildIndex,bHighlight
	);

	// Send back to a parent if any is listening
	if (bHandleOwnMouseEvents)
		SetIsHighlighted(bHighlight, bHighlight);
	else
		HighlightRequestChangeEvent.Broadcast(bHighlight);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIScrollingButtonArray::SetIsHighlighted(bool bInHighlighted, bool bPlaySound)
{
	// Ignore bPlaySound
	if (bIsHighlighted != bInHighlighted)
	{
		bIsHighlighted = bInHighlighted;
		SelectedButton->SetIsHighlighted(bInHighlighted);
		LeftArrowButton->SetIsHighlighted(bInHighlighted);
		RightArrowButton->SetIsHighlighted(bInHighlighted);
		if (bPlaySound)
			USoAudioManager::PlaySound2D(this, SFXChildSelectionChanged);

		OnHighlightChanged(bInHighlighted);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::SetSelectedIndex(int32 ElementIndex, bool bPlaySound)
{
	InternalSetSelectedIndex(ElementIndex);
	UpdateSelectedButton(bPlaySound);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::UpdateSelectedButton(bool bPlaySound)
{
	if (!SelectedButton)
		return;

	if (!IsValidIndex(SelectedIndex))
	{
		UnhighlightSelectedChild();
		SelectedButton->Deactivate();
		LeftArrowButton->Deactivate();
		RightArrowButton->Deactivate();
		return;
	}

	// Can't cycle just disable left/right buttons if it is the case
	if (!bCyclicNavigation)
	{
		// Enable/Disable arrows
		// at least one element left on the left ;)
		const bool bEnableLeftButton = SelectedIndex > 0;
		LeftArrowButton->SetIsActive(bEnableLeftButton);
		LeftArrowButton->SetIsEnabled(bEnableLeftButton);

		// at least one element left on the right
		const bool bEnableRightButton = SelectedIndex < NumContainerChildren() - 1;
		RightArrowButton->SetIsActive(bEnableRightButton);
		RightArrowButton->SetIsEnabled(bEnableRightButton);
	}

	SelectedButton->Activate();
	SelectedButton->SetButtonText(GetButtonText(SelectedIndex));

	if (bPlaySound)
		USoAudioManager::PlaySound2D(this, SFXChildSelectionChanged);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIScrollingButtonArray::IsChildActive(int32 ChildIndex) const
{
	return SelectedButton->IsActive();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIScrollingButtonArray::IsChildHighlighted(int32 ChildIndex) const
{
	return SelectedButton->IsHighlighted();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIScrollingButtonArray::IsChildEnabled(int32 ChildIndex) const
{
	return SelectedButton->GetIsEnabled();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::UpdateWidgetOverrides(UUserWidget* Widget, int32 ChildIndex)
{
	UpdateOverrideCustomPadding(Widget);
	UpdateOverridesSFX(Widget);

	// Handle only the Selected Button
	USoUIButton* Button = Cast<USoUIButton>(Widget);
	if (Button != SelectedButton)
		return;

	// Cycle through images
	if (OverrideButtonImages.Num() > 0)
		if (UTexture2D * OverrideImage = OverrideButtonImages[USoMathHelper::WrapIndexAround(ChildIndex, OverrideButtonImages.Num())])
			Button->SetOverrideBackgroundImage(OverrideImage, false);


	if (bOverrideButtonsVisibility)
		Button->SetShowText(bShowButtonsText)
			  ->SetShowBackgroundImage(bShowButtonsBackgroundImage);

	Button->UpdateAll();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::OnPressedArrowLeft()
{
	Navigate(ESoUICommand::EUC_Left, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIScrollingButtonArray::OnPressedArrowRight()
{
	Navigate(ESoUICommand::EUC_Right, true);
}
