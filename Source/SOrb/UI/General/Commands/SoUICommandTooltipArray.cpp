// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUICommandTooltipArray.h"

#include "Components/PanelWidget.h"
#include "Components/Spacer.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/HorizontalBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUICommandTooltipArray, All, All)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltipArray::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	// Set proper size form template
	if (*CommandTooltipClass && WidgetsContainer)
	{
		// if (const USoUICommandTooltip* Template = GetCommandTooltipWidget(false))
		// {
		// 	FVector2D OverrideWidthAndHeight;
		// 	if (Template->GetOverrideWidthAndHeight(OverrideWidthAndHeight))
		// 	{
		// 		for (int32 Index = WidgetsContainer->GetChildrenCount() - 1; Index >= 0; --Index)
		// 		{
		// 			UWidget* CurrentWidget = WidgetsContainer->GetChildAt(Index);
		// 			if (USoUICommandTooltip* CommandWidget = Cast<USoUICommandTooltip>(CurrentWidget))
		// 				CommandWidget->SetOverrideWidthAndHeight(OverrideWidthAndHeight);
		// 		}
		// 	}
		// }

		// Set Defaults
		for (int32 Index = WidgetsContainer->GetChildrenCount() - 1; Index >= 0; --Index)
			if (USoUICommandTooltip* CommandTooltip = Cast<USoUICommandTooltip>(WidgetsContainer->GetChildAt(Index)))
				SetHorizontalBoxSlotToDefaults(Cast<UHorizontalBoxSlot>(CommandTooltip->Slot));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltipArray::Clear()
{
	// Cache the current widgets
	const int32 NumWidgets = WidgetsContainer->GetChildrenCount();
	for (int32 Index = 0; Index < NumWidgets; Index++)
	{
		UWidget* CurrentWidget = WidgetsContainer->GetChildAt(Index);
		if (USoUICommandTooltip* CommandWidget = Cast<USoUICommandTooltip>(CurrentWidget))
		{
			if (*CommandTooltipClass != nullptr && CommandWidget->IsA(CommandTooltipClass))
			{
				CachedCommandTooltipWidgets.Add(CommandWidget);
			}
		}
	}

	WidgetsContainer->ClearChildren();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUICommandTooltipArray::IsEmpty() const
{
	return WidgetsContainer->GetChildrenCount() == 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUICommandTooltip* USoUICommandTooltipArray::GetOrCreateCommandTooltipWidget(bool bTryCache)
{
	// Pop from cache?
	if (bTryCache && CachedCommandTooltipWidgets.Num() > 0)
		return CachedCommandTooltipWidgets.Pop();

	if (*CommandTooltipClass == nullptr)
	{
		UE_LOG(LogSoUICommandTooltipArray, Error, TEXT("Invalid CommandTooltipClass for USoUICommandTooltipArray"));
		return nullptr;
	}
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogSoUICommandTooltipArray, Error, TEXT("Invalid World for USoUICommandTooltipArray"));
		return nullptr;
	}

	// Create the command tooltip
	return CreateWidget<USoUICommandTooltip>(World, CommandTooltipClass);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltipArray::AddTooltipFromUICommand(const FText InText, const ESoUICommand Command)
{
	// Add command tooltip
	if (USoUICommandTooltip* CommandTooltip = GetOrCreateCommandTooltipWidget())
	{
		WidgetsContainer->AddChild(CommandTooltip);
		// Can't get the slot before adding
		UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(CommandTooltip->Slot);
		verify(HorizontalSlot);
		SetHorizontalBoxSlotToDefaults(HorizontalSlot);
		OnPostAddTooltip(HorizontalSlot, CommandTooltip);

		CommandTooltip->InitializeFromUICommand(InText, Command);
		OnPostInitializeTooltip(HorizontalSlot, CommandTooltip);
	}

	if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(WidgetsContainer->Slot))
		SizeBoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);

	// Force redraw
	InvalidateLayoutAndVolatility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltipArray::SetupFromUICommands(const TArray<FSoCommandTooltipData>& Data)
{
	if (Data.Num() == 0)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::Visible);

	// Cache and remove all excess elements
	for (int32 Index = WidgetsContainer->GetChildrenCount() - 1; Index >= Data.Num(); --Index)
	{
		UWidget* CurrentWidget = WidgetsContainer->GetChildAt(Index);
		if (USoUICommandTooltip* CommandWidget = Cast<USoUICommandTooltip>(CurrentWidget))
		{
			if (*CommandTooltipClass != nullptr && CommandWidget->IsA(CommandTooltipClass))
			{
				CachedCommandTooltipWidgets.Add(CommandWidget);
			}
		}

		WidgetsContainer->RemoveChildAt(Index);
	}

	for (int32 i = 0; i < Data.Num(); ++i)
	{
		if (i >= WidgetsContainer->GetChildrenCount())
		{
			USoUICommandTooltip* TooltipAdded = GetOrCreateCommandTooltipWidget();
			WidgetsContainer->AddChild(TooltipAdded);
			if (TooltipAdded)
				OnPostAddTooltip(Cast<UHorizontalBoxSlot>(TooltipAdded->Slot), TooltipAdded);
		}

		if (USoUICommandTooltip* CommandTooltip = Cast<USoUICommandTooltip>(WidgetsContainer->GetChildAt(i)))
		{
			CommandTooltip->InitializeFromUICommandData(Data[i]);

			UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(CommandTooltip->Slot);
			verify(HorizontalSlot);
			SetHorizontalBoxSlotToDefaults(HorizontalSlot);
			OnPostInitializeTooltip(HorizontalSlot, CommandTooltip);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltipArray::SetHorizontalBoxSlotToDefaults(UHorizontalBoxSlot* HorizontalSlot)
{
	if (!HorizontalSlot)
		return;

	HorizontalSlot->Size = TooltipsSize;
	HorizontalSlot->Padding = TooltipsPadding;
	HorizontalSlot->HorizontalAlignment = TooltipsHorizontalAlignment;
	HorizontalSlot->VerticalAlignment = TooltipsVerticalAlignment;
	HorizontalSlot->SynchronizeProperties();
}
