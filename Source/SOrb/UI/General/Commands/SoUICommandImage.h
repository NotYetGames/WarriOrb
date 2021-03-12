// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "UI/General/SoUITypes.h"
#include "Settings/Input/SoInputNames.h"
#include "Settings/Input/SoInputSettingsTypes.h"

#include "SoUICommandImage.generated.h"

class UImage;

/**
 * Displays a shortcut image that changes if the input type also changes
 * NOTE: this is always visible. To have visibility only one some inputs see USoUICommandTooltip
 * You can set either a ESoUICommand OR a ESoInputActionNameType for the image to display,
 */
UCLASS(HideCategories = (Navigation))
class SORB_API USoUICommandImage : public UUserWidget
{
	GENERATED_BODY()
public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// Own Methods
	//

	// Only used for previews
	void SetPreviewImage(UTexture2D* Preview);

	void SetIsListeningToDeviceChanges(bool bIsListening)
	{
		if (bIsListeningToDevicesChanges == bIsListening)
			return;

		if (bIsListeningToDevicesChanges)
			UnSubscribeFromDeviceChanged();
		else
			SubscribeToDeviceChanged();
	}

	//
	// Sets the UICommand we want to display the image for
	// NOTE: this is mutual exclusive with the ActionNameType
	// UICommand
	//

	UFUNCTION(BlueprintCallable, Category = WhatToDisplay)
	void SetUICommand(ESoUICommand InCommand, bool bUpdateDeviceType = true);

	UFUNCTION(BlueprintCallable, Category = WhatToDisplay)
	void SetUICommandPriorities(const FSoInputUICommandPriorities& Priorities, bool bUpdateImage = true)
	{
		UICommandPriorities = Priorities;
		if (IsUICommandValid() && bUpdateImage)
			UpdateImage();
	}

	ESoUICommand GetUICommand() const { return UICommand; }
	bool IsUICommandValid() const { return FSoInputActionName::IsValidUICommand(UICommand); }
	void ResetUICommand()
	{
		UICommand = ESoUICommand::EUC_PressedMax;
		UICommandPriorities = {};
	}

	//
	// Sets the input action name type we want to set the image for
	// NOTE: this is mutual exclusive with the UICommand
	// ActionNameType
	//

	UFUNCTION(BlueprintCallable, Category = WhatToDisplay)
	void SetActionNameType(ESoInputActionNameType InType, bool bUpdateDeviceType = true);

	ESoInputActionNameType GetActionNameType() const { return ActionNameType; }
	bool IsActionNameTypeValid() const { return FSoInputActionName::IsValidActionNameType(ActionNameType); }
	void ResetActionNameType()
	{
		ActionNameType = ESoInputActionNameType::IANT_None;
	}

	//
	// Can be used to override the image size instead of relying on the texture size
	// OverrideWidthAndHeight
	//

	UFUNCTION(BlueprintCallable)
	void SetOverrideWidthAndHeight(const FVector2D& WidthAndHeight)
	{
		OverrideWidthAndHeight = WidthAndHeight;
		UpdateImageSize();
	}

	bool GetOverrideWidthAndHeight(FVector2D& OutVector) const
	{
		OutVector = OverrideWidthAndHeight;
		return IsValidOverrideWidthAndHeight();
	}

	static FORCEINLINE bool IsValidOverrideWidthAndHeight(const FVector2D& Override)
	{
		return !Override.IsNearlyZero() && Override.X > 1.f && Override.Y > 1.f;
	}

	bool IsValidOverrideWidthAndHeight() const { return IsValidOverrideWidthAndHeight(OverrideWidthAndHeight); }

	//
	// DeviceType
	//

	UFUNCTION(BlueprintCallable)
	void SetDeviceType(const ESoInputDeviceType NewDevice)
	{
		DeviceType = NewDevice;
		UpdateImage();
	}

	//
	// OverrideGamepadTextures
	// bChangeImageOnInputDeviceChange
	//

	// Used mainly in settings UI
	ThisClass* SetOverrideGamepadTextures(const FSoInputGamepadKeyTextures& Textures)
	{
		OverrideGamepadTextures = Textures;
		return this;
	}
	ThisClass* SetChangeImageOnInputDeviceChanged(bool bValue)
	{
		bChangeImageOnInputDeviceChange = bValue;
		return this;
	}

	void ForceUpdateImage() { UpdateImage(); }

	//
	// StaticTexture
	//

	UFUNCTION(BlueprintCallable, Category = StaticTexture)
	void EnableStaticTextureMode(UTexture2D* Texture, bool bMatchSize);

	UFUNCTION(BlueprintCallable, Category = StaticTexture)
	void EnableStaticTextureModeWithBrush(const FSlateBrush Brush, bool bMatchSize)
	{
		StaticTextureBrush = Brush;
		bStaticTextureMatchSize = bMatchSize;
		SetStaticTextureMode(true);
	}

	UFUNCTION(BlueprintCallable, Category = StaticTexture)
	void DisableStaticTextureMode()
	{
		StaticTextureBrush = {};
		bStaticTextureMatchSize = false;
		SetStaticTextureMode(true);
	}

protected:
	UFUNCTION()
	void HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType);

	void SetStaticTextureMode(bool bValue)
	{
		bStaticTextureMode = bValue;
		SetIsListeningToDeviceChanges(!bStaticTextureMode);
		UpdateImage();
	}

	void SubscribeToDeviceChanged();
	void UnSubscribeFromDeviceChanged();
	void UpdateImage(bool bFromSyncronize = false);
	UTexture2D* GetDynamicImage(bool bFromSyncronize = false);
	void UpdateImageSize();

protected:
	//
	// WhatToDisplay
	//

	// The current command if any
	// Mutual exclusive with ActionNameType
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">WhatToDisplay")
	ESoUICommand UICommand = ESoUICommand::EUC_PressedMax;

	// Priorities for the UI command
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">WhatToDisplay")
	FSoInputUICommandPriorities UICommandPriorities;

	// The current action name
	// Mutual exclusive with UICommand
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">WhatToDisplay")
	ESoInputActionNameType ActionNameType = ESoInputActionNameType::IANT_None;


	//
	// Override
	//

	// Sometimes used by the UI, if this is set it does not get the textures, it uses these
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Override")
	FSoInputGamepadKeyTextures OverrideGamepadTextures;

	// If this is a positive number it uses this as the image size instead of the default ones from USoGameSingleton::GetDefaultWidthAndHeightForUIIcons
	// X - Width
	// Y - Height
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Override")
	FVector2D OverrideWidthAndHeight = FVector2D::ZeroVector;


	//
	// Options
	//

	// If false it does not try to change the input device used
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Options")
	bool bChangeImageOnInputDeviceChange = true;


	//
	// Other
	//

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* Image = nullptr;

	// Cached Current device Type
	ESoInputDeviceType DeviceType = ESoInputDeviceType::Keyboard;

	// Keep track if we are listening or not
	UPROPERTY(BlueprintReadOnly)
	bool bIsListeningToDevicesChanges = false;

	// Keep track of the original texture size
	UPROPERTY(BlueprintReadOnly)
	FIntPoint TextureOriginalSize = FIntPoint::ZeroValue;


	//
	// StaticTexture
	//

	// If this is true this will act as a normal UImage
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">StaticTexture")
	bool bStaticTextureMode = false;

	// Only used if bStaticTextureMode = true
	// NOTE: this is not affected by OverrideWidthAndHeight
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">StaticTexture")
	FSlateBrush StaticTextureBrush;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">StaticTexture")
	bool bStaticTextureMatchSize = false;
};
