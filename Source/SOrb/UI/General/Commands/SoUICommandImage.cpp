// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUICommandImage.h"

#include "Components/Image.h"

#include "Basic/SoGameSingleton.h"
#include "Character/SoPlayerController.h"
#include "UI/SoUIHelper.h"
#include "Settings/Input/SoInputHelper.h"
#include "Engine/Texture2D.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UpdateImage(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::NativeConstruct()
{
	Super::NativeConstruct();

	SubscribeToDeviceChanged();
	SetDeviceType(USoInputHelper::GetCurrentDeviceType(this));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::NativeDestruct()
{
	UnSubscribeFromDeviceChanged();
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::SetPreviewImage(UTexture2D* Preview)
{
	if (Image && Preview)
	{
		TextureOriginalSize = FIntPoint(Preview->GetSizeX(), Preview->GetSizeY());
		Image->SetBrushFromTexture(Preview, false);
		UpdateImageSize();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::SetUICommand(ESoUICommand InCommand, bool bUpdateDeviceType)
{
	UICommand = InCommand;
	ResetActionNameType();
	if (!IsUICommandValid())
		return;

	if (bUpdateDeviceType)
		SetDeviceType(USoInputHelper::GetCurrentDeviceType(this));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::SetActionNameType(ESoInputActionNameType InType, bool bUpdateDeviceType)
{
	ActionNameType = InType;
	ResetUICommand();
	if (!IsActionNameTypeValid())
		return;

	if (bUpdateDeviceType)
		SetDeviceType(USoInputHelper::GetCurrentDeviceType(this));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType)
{
	if (!bChangeImageOnInputDeviceChange)
		return;

	SetDeviceType(InDeviceType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::SubscribeToDeviceChanged()
{
	if (!bChangeImageOnInputDeviceChange || bIsListeningToDevicesChanges)
		return;

	if (ASoPlayerController* Controller = USoUIHelper::GetSoPlayerControllerFromUWidget(this))
	{
		Controller->OnDeviceTypeChanged().AddDynamic(this, &ThisClass::HandleDeviceTypeChanged);
		bIsListeningToDevicesChanges = true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::UnSubscribeFromDeviceChanged()
{
	if (!bIsListeningToDevicesChanges)
		return;

	if (ASoPlayerController* Controller = USoUIHelper::GetSoPlayerControllerFromUWidget(this))
	{
		Controller->OnDeviceTypeChanged().RemoveDynamic(this, &ThisClass::HandleDeviceTypeChanged);
		bIsListeningToDevicesChanges = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::EnableStaticTextureMode(UTexture2D* Texture, bool bMatchSize)
{
	StaticTextureBrush.SetResourceObject(Texture);
	bStaticTextureMatchSize = bMatchSize;
	SetStaticTextureMode(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::UpdateImage(bool bFromSyncronize)
{
	UTexture2D* NewTexture = nullptr;

	if (bStaticTextureMode)
	{
		// Use static texture
		NewTexture = Cast<UTexture2D>(StaticTextureBrush.GetResourceObject());
	}
	else
	{
		// Use Dynamic
		NewTexture = GetDynamicImage(bFromSyncronize);
	}

	if (NewTexture)
		TextureOriginalSize = FIntPoint(NewTexture->GetSizeX(), NewTexture->GetSizeY());

	if (Image)
	{
		Image->SetBrushFromTexture(NewTexture, bStaticTextureMode ? bStaticTextureMatchSize : false);
		UpdateImageSize();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* USoUICommandImage::GetDynamicImage(bool bFromSyncronize)
{
	// Override with gamepad texture:
	// Is override gamepad textures set
	// AND either of
	// - is NOT bChangeImageOnInputDeviceChange
	// - bIsKeyboard AND bChangeImageOnInputDeviceChange
	const bool bIsGamepad = DeviceType != ESoInputDeviceType::Keyboard;
	const bool bIsKeyboard = !bIsGamepad;
	const bool bCanOverrideGamepad = OverrideGamepadTextures.IsValid() &&
		((bChangeImageOnInputDeviceChange && bIsGamepad) || !bChangeImageOnInputDeviceChange);

	if (bCanOverrideGamepad)
	{
		// Overridden by gamepad textures
		return OverrideGamepadTextures.GetTextureForDeviceType(DeviceType);
	}

	if (bFromSyncronize)
	{
		// Preview, Set the current image
		return Image ? Cast<UTexture2D>(Image->Brush.GetResourceObject()) : nullptr;
	}
	
	// Normal
	if (IsUICommandValid())
	{
		ResetActionNameType();
		return USoGameSingleton::GetIconForUICommand(UICommand, DeviceType, UICommandPriorities);
	}
	if (IsActionNameTypeValid())
	{
		ResetUICommand();
		return USoGameSingleton::GetIconForInputActionNameType(ActionNameType, DeviceType, bIsKeyboard);
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICommandImage::UpdateImageSize()
{
	if (!Image)
		return;

	if (bStaticTextureMode)
	{
		Image->Brush.ImageSize = StaticTextureBrush.ImageSize;
		Image->Brush.TintColor = StaticTextureBrush.TintColor;
		Image->Brush.DrawAs = StaticTextureBrush.DrawAs;
		Image->Brush.Tiling = StaticTextureBrush.Tiling;
	}
	else
	{
		if (IsValidOverrideWidthAndHeight())
		{
			Image->Brush.ImageSize = USoGameSingleton::GetVectorWidthHeightForUIIcons(DeviceType, OverrideWidthAndHeight);
		}
		else
		{
			// Should be safe for SynchronizeProperties()
			Image->Brush.ImageSize = USoGameSingleton::GetDefaultVectorWidthHeightForUIIcons(DeviceType);
		}
	}

	// Check if we need to scale by ratio
	if (TextureOriginalSize.X > 0 && TextureOriginalSize.Y > 0 && TextureOriginalSize.X != TextureOriginalSize.Y)
	{
		// Only Scale Up
		if (TextureOriginalSize.X > TextureOriginalSize.Y)
		{
			// Scale X
			const float Ratio = TextureOriginalSize.X / static_cast<float>(TextureOriginalSize.Y);
			Image->Brush.ImageSize.X *= Ratio;
		}
		else
		{
			// Scale Y
			const float Ratio = TextureOriginalSize.Y / static_cast<float>(TextureOriginalSize.X);
			Image->Brush.ImageSize.Y *= Ratio;
		}
	}

	// Force redraw
	InvalidateLayoutAndVolatility();
	Image->InvalidateLayoutAndVolatility();
}
