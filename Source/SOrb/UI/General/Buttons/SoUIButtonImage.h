// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoUIButton.h"
#include "Settings/Input/SoInputSettingsTypes.h"

#include "SoUIButtonImage.generated.h"

enum class ESoInputActionNameType : uint8;
class UTextBlock;
class UTexture2D;
class USoUICommandImage;


// USoUICommandImage + USoUIButton
// Can be an Image that acts as a button
// A Command tooltip that acts as a button, etc
UCLASS()
class SORB_API USoUIButtonImage : public USoUIButton
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;

	//
	// USoUIButton interface
	//
	void SetOverrideBackgroundImage(UTexture2D* NewImage, bool bInMatchSize = false) override;

	//
	// Own methods
	//


	UFUNCTION(BlueprintCallable, Category = Image)
	void SetUICommand(ESoUICommand InCommand, bool bUpdateDeviceType = true);

	UFUNCTION(BlueprintCallable, Category = Image)
	void SetUICommandPriorities(const FSoInputUICommandPriorities& Priorities, bool bUpdateImage = true);

	UFUNCTION(BlueprintCallable, Category = Image)
	void SetActionNameType(ESoInputActionNameType InType, bool bUpdateDeviceType = true);

	// This also enables image mode and static texture mode on the image
	UFUNCTION(BlueprintCallable, Category = Image)
	void SetImageFromTexture(UTexture2D* NewTexture, bool bInMatchSize = false);

	UFUNCTION(BlueprintCallable, Category = Image)
	void EnableImageMode()
	{
		bImageMode = true;
		UpdateImageMode();
	}

	UFUNCTION(BlueprintCallable, Category = Image)
	void DisableImageMode()
	{
		bImageMode = false;
		UpdateImageMode();
	}

protected:
	void UpdateImageMode();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, BlueprintCosmetic, Category = ">Events")
	void UpdateAnimation(bool bShouldBeAnimated);

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandImage* Image = nullptr;


	// If this is a positive number it uses this as the image size instead of the default ones from USoGameSingleton::GetDefaultWidthAndHeightForUIIcons
	// X - Width
	// Y - Height
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Image")
	FVector2D ImageOverrideWidthAndHeight = FVector2D::ZeroVector;

	// Only used for previews
	UPROPERTY(EditAnywhere, Category = ">Override")
	UTexture2D* PreviewImage = nullptr;

	// Enable/Disable Image
	UPROPERTY(BlueprintReadOnly, Category = ">Image")
	bool bImageMode = true;
};
