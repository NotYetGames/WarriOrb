// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUICommandTooltip.h"

#include "Components/TextBlock.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/HorizontalBox.h"

#include "SoUICommandImage.h"
#include "Basic/SoGameSingleton.h"
#include "UI/SoUIHelper.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (!PreviewText.IsEmpty())
		SetText(PreviewText);

	if (CommandImage)
	{
		if (USoUICommandImage::IsValidOverrideWidthAndHeight(ImageOverrideWidthAndHeight))
			CommandImage->SetOverrideWidthAndHeight(ImageOverrideWidthAndHeight);

		if (PreviewImage)
			CommandImage->SetPreviewImage(PreviewImage);
	}

	// Set left sided or right sided
	if (Container && CommandText && CommandImage)
	{
		SetCacheSlots();

		Container->ClearChildren();
		UHorizontalBoxSlot* ImageSlot = nullptr;
		UHorizontalBoxSlot* TextSlot = nullptr;
		if (bLeftSided)
		{
			ImageSlot = AddWidgetToContainer(CommandImage);
			TextSlot = AddWidgetToContainer(CommandText);
		}
		else
		{
			TextSlot = AddWidgetToContainer(CommandText);
			ImageSlot = AddWidgetToContainer(CommandImage);
		}

		// Copy From Cache
		USoUIHelper::CopyHorizontalBoxSlotToAnother(CachedSlotText, TextSlot);
		USoUIHelper::CopyHorizontalBoxSlotToAnother(CachedSlotImage, ImageSlot);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::NativeConstruct()
{
	Super::NativeConstruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::NativeDestruct()
{
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SetPaddingBetween(float Size)
{
	if (Size < 1.0f)
		return;

	if (UHorizontalBoxSlot* FirstSlot = GetSlotFromContainer(0))
	{
		FirstSlot->Padding.Right = Size;
		FirstSlot->SynchronizeProperties();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SetText(FText InText)
{
	if (CommandText)
		CommandText->SetText(InText);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SetUICommand(ESoUICommand InCommand, bool bUpdateDeviceType)
{
	if (CommandImage)
		CommandImage->SetUICommand(InCommand, bUpdateDeviceType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SetUICommandPriorities(const FSoInputUICommandPriorities& Priorities, bool bUpdateImage)
{
	if (CommandImage)
		CommandImage->SetUICommandPriorities(Priorities, bUpdateImage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SetActionNameType(ESoInputActionNameType InType, bool bUpdateDeviceType)
{
	if (CommandImage)
		CommandImage->SetActionNameType(InType, bUpdateDeviceType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SetDeviceType(ESoInputDeviceType NewDevice)
{
	if (CommandImage)
		CommandImage->SetDeviceType(NewDevice);

	SetLocalDeviceType(NewDevice);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SetOverrideWidthAndHeight(const FVector2D& WidthAndHeight)
{
	if (CommandImage)
		CommandImage->SetOverrideWidthAndHeight(WidthAndHeight);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUICommandTooltip::GetOverrideWidthAndHeight(FVector2D& OutVector) const
{
	if (CommandImage)
		return CommandImage->GetOverrideWidthAndHeight(OutVector);

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SetOverrideGamepadTextures(const FSoInputGamepadKeyTextures& Textures)
{
	if (CommandImage)
		CommandImage->SetOverrideGamepadTextures(Textures);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UHorizontalBoxSlot* USoUICommandTooltip::AddWidgetToContainer(UWidget* Widget)
{
	if (!Container)
		return nullptr;

	if (UHorizontalBoxSlot* ContainerSlot = Container->AddChildToHorizontalBox(Widget))
	{
		//ContainerSlot->Size = FSlateChildSize(ESlateSizeRule::Type::Automatic);
		//ContainerSlot->HorizontalAlignment = EHorizontalAlignment::HAlign_Fill;
		//ContainerSlot->VerticalAlignment = EVerticalAlignment::VAlign_Fill;
		return ContainerSlot;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UHorizontalBoxSlot* USoUICommandTooltip::GetSlotFromContainer(int32 Index)
{
	if (!Container)
		return nullptr;

	const TArray<UPanelSlot*>& ContainerSlots = Container->GetSlots();
	if (ContainerSlots.IsValidIndex(Index))
		return Cast<UHorizontalBoxSlot>(ContainerSlots[Index]);

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandTooltip::SetCacheSlots()
{
	if (!Container)
		return;

	CachedSlotImage = nullptr;
	CachedSlotText = nullptr;

	if (CommandText)
		CachedSlotText = Cast<UHorizontalBoxSlot>(CommandText->Slot);

	if (CommandImage)
		CachedSlotImage = Cast<UHorizontalBoxSlot>(CommandImage->Slot);
}
