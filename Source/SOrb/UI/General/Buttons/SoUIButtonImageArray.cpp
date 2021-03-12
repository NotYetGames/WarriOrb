// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIButtonImageArray.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"

#include "SoUIButton.h"
#include "UI/General/SoUITypes.h"
#include "SoUIButtonImage.h"
#include "Basic/SoAudioManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUIButtonImageArray, All, All)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIButtonImageArray::USoUIButtonImageArray(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bVerticalLayout = false;
	bExtraAttachedVerticalLayout = false;
	bCyclicNavigation = true;
	bSkipDeactivatedChildren = true;
	ChildWidgetClass = USoUIButtonImage::StaticClass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImageArray::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImageArray::NativeConstruct()
{
	Super::NativeConstruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIButtonImage* USoUIButtonImageArray::GetOrCreateButtonImage(bool bTryCache)
{
	// TOOD use cache?

	if (*ChildWidgetClass == nullptr)
	{
		UE_LOG(LogSoUIButtonImageArray, Error, TEXT("Invalid ButtonClass for USoUIButtonImageArray"));
		return nullptr;
	}
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogSoUIButtonImageArray, Error, TEXT("Invalid World for USoUIButtonImageArray"));
		return nullptr;
	}
	if (!ChildWidgetClass->IsChildOf(USoUIButtonImage::StaticClass()))
	{
		UE_LOG(LogSoUIButtonImageArray, Error, TEXT("ButtonClass does not derive from ButtonClass for USoUIButtonImageArray"));
		return nullptr;
	}

	return CreateWidget<USoUIButtonImage>(World, ChildWidgetClass);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImageArray::UpdateEnsureOnlyTheseCommandsExist(const TArray<FSoUITextCommandPair> CommandsToBeDisplayed,  bool bAllowEmpty)
{
	ClearEditorPreviewButtons();

	// Should have used EmptyAllUIButtons
	if (!bAllowEmpty && CommandsToBeDisplayed.Num() == 0)
		return;

	// TODO handle case where the order is not right
	// because we removed and added to many

	// deduplicate Commands
	TSet<ESoUICommand> CommandsToAdd;
	for (auto& Elem : CommandsToBeDisplayed)
		CommandsToAdd.Add(Elem.Command);

	// Check if we have commands that are not in the set of approved values
	TSet<ESoUICommand> CommandsToRemove;
	for (auto& Elem : CreatedCommandsMap)
		if (!CommandsToAdd.Contains(Elem.Key))
			CommandsToRemove.Add(Elem.Key);

	// Remove old commands first
	for (ESoUICommand Command : CommandsToRemove)
		RemoveButtonForUICommand(Command);

	// Add new commands or update texts
	for (const auto& Elem : CommandsToBeDisplayed)
	{
		const ESoUICommand Command = Elem.Command;
		const FText Text = Elem.Text;

		if (CreatedCommandsMap.Contains(Command))
		{
			// Modify only text
			SetButtonTextForUICommand(Text, Command);
		}
		else
		{
			// Add new UI Command
			AddButtonForUICommand(Text, Command);
		}
	}

	if (IsEmpty())
		SetVisibility(ESlateVisibility::Collapsed);
	else
		SetVisibility(ESlateVisibility::Visible);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIButtonImageArray::SetButtonTextForUICommand(FText InText, ESoUICommand Command)
{
	// Can't remove as it does not exist
	if (!CreatedCommandsMap.Contains(Command))
		return false;

	const int32 ButtonIndex = CreatedCommandsMap.FindChecked(Command);
	SetButtonText(ButtonIndex, InText);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImageArray::AddButtonForUICommand(FText InText, ESoUICommand Command)
{
	ClearEditorPreviewButtons();

	// Already contains the command
	if (CreatedCommandsMap.Contains(Command))
		return;

	USoUIButtonImage* ButtonImage = GetOrCreateButtonImage();
	if (!ButtonImage)
		return;

	ButtonImage->SetUICommand(Command);
	ButtonImage->SetButtonText(InText);
	AddContainerButton(ButtonImage);
	CreatedCommandsMap.Add(Command, NumContainerChildren() - 1);

	// Force redraw
	InvalidateLayoutAndVolatility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImageArray::RemoveButtonForUICommand(ESoUICommand Command)
{
	// Can't remove as it does not exist
	if (!CreatedCommandsMap.Contains(Command))
		return;

	const int32 RemovedButtonIndex = CreatedCommandsMap.FindChecked(Command);
	RemoveContainerChildAt(RemovedButtonIndex);

	CreatedCommandsMap.Remove(Command);
	
	// Fix the indices by reducing the index
	// of the buttons after the removed button
	for (auto& Elem : CreatedCommandsMap)
		if (Elem.Value > RemovedButtonIndex)
			Elem.Value -= 1;
	
	// Force redraw
	InvalidateLayoutAndVolatility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImageArray::EmptyAllUIButtons()
{
	CreatedCommandsMap.Empty();
	if (Container && Container->GetChildrenCount() > 0)
	{
		Container->ClearChildren();
		InvalidateLayoutAndVolatility();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoUICommand USoUIButtonImageArray::GetButtonIndexCommand(int32 ButtonIndex) const
{
	for (const auto& Pair : CreatedCommandsMap)
		if (Pair.Value == ButtonIndex)
			return Pair.Key;

	return ESoUICommand::EUC_ReleasedMax;
}