// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/General/SoUIUserWidgetDeviceVisibility.h"

#include "SoUICommandTooltip.generated.h"

enum class ESoInputActionNameType : uint8;
class UHorizontalBox;
class UHorizontalBoxSlot;
class UTextBlock;
class USoUICommandImage;
struct FSoInputGamepadKeyTextures;


USTRUCT(BlueprintType, Blueprintable)
struct FSoCommandTooltipData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText InText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ESoUICommand Command;
};

/**
 * Display a shortcut and some text (command) besides it
 * NOTE: Assumes horizontal box
 * NOTE: You can set either a ESoUICommand OR a ESoInputActionNameType for the image to display
 * [Icon][Command name]
 * OR
 * [Command name] [Icon]
 */
UCLASS(HideCategories = ("Navigation"))
class SORB_API USoUICommandTooltip : public USoUIUserWidgetDeviceVisibility
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

	UFUNCTION(BlueprintCallable)
	void SetPaddingBetween(float Size);


	//
	// UICommand
	//

	UFUNCTION(BlueprintCallable, Category = Image)
	void SetUICommand(ESoUICommand InCommand, bool bUpdateDeviceType = true);

	UFUNCTION(BlueprintCallable, Category = Image)
	void SetUICommandPriorities(const FSoInputUICommandPriorities& Priorities, bool bUpdateImage = true);

	UFUNCTION(BlueprintCallable, Category = Initialize)
	void InitializeFromUICommandData(const FSoCommandTooltipData& Data)
	{
		InitializeFromUICommand(Data.InText, Data.Command);
	}

	UFUNCTION(BlueprintCallable, Category = Initialize)
	void InitializeFromUICommand(FText InText, ESoUICommand Command, bool bUpdateDeviceType = true, bool bAnimate = false)
	{
		SetText(InText);
		SetUICommand(Command, bUpdateDeviceType);
		UpdateAnimation(bAnimate);
	}

	//
	// ActionNameType
	//

	UFUNCTION(BlueprintCallable, Category = Initialize)
	void InitializeFromInputActionNameType(FText InText,
										   ESoInputActionNameType ActionNameType,
										   bool bUpdateDeviceType = true,
										   bool bAnimate = false)
	{
		SetText(InText);
		SetActionNameType(ActionNameType, bUpdateDeviceType);
		UpdateAnimation(bAnimate);
	}

	//
	// Override
	//

	UFUNCTION(BlueprintCallable, Category = Image)
	void SetOverrideWidthAndHeight(const FVector2D& WidthAndHeight);

	bool GetOverrideWidthAndHeight(FVector2D& OutVector) const;
	void SetOverrideGamepadTextures(const FSoInputGamepadKeyTextures& Textures);

	UFUNCTION(BlueprintCallable, Category = Image)
	void SetActionNameType(const ESoInputActionNameType InType, const bool bUpdateDeviceType = true);

	//
	// Other
	//

	UFUNCTION(BlueprintCallable)
	void SetText(FText InText);

	UFUNCTION(BlueprintCallable)
	void SetDeviceType(ESoInputDeviceType NewDevice) override;


protected:
	UHorizontalBoxSlot* AddWidgetToContainer(UWidget* Widget);
	UHorizontalBoxSlot* GetSlotFromContainer(int32 Index);
	void SetCacheSlots();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, BlueprintCosmetic, Category = ">Events")
	void UpdateAnimation(bool bShouldBeAnimated);

protected:
	/**
	 * bLeftSided = true [Icon][Command name]
	 * OR
	 * bLeftSided = false [Command name][Icon]
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">General")
	bool bLeftSided = true;

	// Only used for previews
	UPROPERTY(EditAnywhere, Category = ">Preview")
	FText PreviewText;

	// Only used for previews
	UPROPERTY(EditAnywhere, Category = ">Preview")
	UTexture2D* PreviewImage = nullptr;

	// If this is a positive number it uses this as the image size instead of the default ones from USoGameSingleton::GetDefaultWidthAndHeightForUIIcons
	// X - Width
	// Y - Height
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Image")
	FVector2D ImageOverrideWidthAndHeight = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* CommandText = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	USoUICommandImage* CommandImage = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UHorizontalBox* Container = nullptr;

	// Used for caching
	UPROPERTY()
	UHorizontalBoxSlot* CachedSlotText = nullptr;

	// Used for caching
	UPROPERTY()
	UHorizontalBoxSlot* CachedSlotImage = nullptr;
};
