// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIButtonArray.h"
#include "SoUIButton.h"

#include "Basic/Helpers/SoMathHelper.h"
#include "../SoUIUserWidget.h"
#include "Basic/SoAudioManager.h"
#include "Components/HorizontalBox.h"
#include "Character/SoCharacter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIButtonArray::USoUIButtonArray(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::NativeConstruct()
{
	Super::NativeConstruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::CreateButtons(const TArray<FText>& ElementTexts, bool bIgnoreEmptyTexts)
{
	EmptyContainerChildren(true);

	// Create the Children
	for (int32 Index = 0; Index < ElementTexts.Num(); Index++)
	{
		if (bIgnoreEmptyTexts && ElementTexts[Index].IsEmpty())
			continue;

		USoUIButton* Button = CreateWidget<USoUIButton>(GetWorld(), ChildWidgetClass);
		if (Button != nullptr)
		{
			Button->SetButtonText(ElementTexts[Index]);
			AddContainerButton(Button);
		}
	}

	UpdateAllOverrides();
	SetSelectedIndex(0); // Reset
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::SetButtons(const TArray<USoUIButton*>& InButtons)
{
	EmptyContainerChildren(true);
	for (USoUIButton* Button : InButtons)
		AddContainerButton(Button);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::SetButtonsTexts(const TArray<FText>& ButtonTexts)
{
	for (int32 Index = 0; Index < NumContainerChildren(); ++Index)
	{
		USoUIButton* Button = GetContainerButtonAt(Index);
		if (!Button)
			continue;

		if (ButtonTexts.IsValidIndex(Index))
		{
			Button->Show();
			Button->SetButtonText(ButtonTexts[Index]);
		}
		else
		{
			Button->Hide();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::AddContainerButton(USoUIButton* Button)
{
	Super::AddContainerChild(Button);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::AddContainerButtonWithText(USoUIButton* Button, FText Text)
{
	if (Button)
		Button->SetButtonText(Text);

	AddContainerButton(Button);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::RemoveContainerButton(USoUIButton* Button)
{
	Super::RemoveContainerChild(Button);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIButton* USoUIButtonArray::GetContainerButtonAt(int32 ButtonIndex) const
{
	return Cast<USoUIButton>(GetContainerChildAt(ButtonIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIButtonArray::SetButtonText(int32 ButtonIndex, const FText& Text)
{
	USoUIButton* Button = GetContainerButtonAt(ButtonIndex);
	if (!Button)
		return false;

	Button->SetButtonText(Text);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoUIButtonArray::GetButtonText(int32 ButtonIndex)
{
	USoUIButton* Button = GetContainerButtonAt(ButtonIndex);
	if (!Button)
		return FText::GetEmpty();

	return Button->GetButtonText();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonArray::UpdateWidgetOverrides(UUserWidget* Widget, int32 ChildIndex)
{
	Super::UpdateWidgetOverrides(Widget, ChildIndex);

	// Handle button bellow
	USoUIButton* Button = Cast<USoUIButton>(Widget);
	if (!Button)
		return;

	// Cycle through images
	if (OverrideButtonImages.Num() > 0)
		if (UTexture2D * OverrideImage = OverrideButtonImages[USoMathHelper::WrapIndexAround(ChildIndex, OverrideButtonImages.Num())])
			Button->SetOverrideBackgroundImage(OverrideImage, false);

	if (bOverrideButtonsVisibility)
		Button->SetShowText(bShowButtonsText)
			  ->SetShowBackgroundImage(bShowButtonsBackgroundImage);

	if (bOverrideTextAlignment)
		Button->SetTextAlignment(TextAlignmentOverride);

	Button->UpdateAll();
}
