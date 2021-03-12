// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SoUITypes.h"
#include "Settings/Input/SoInputSettingsTypes.h"

#include "SoUIPressAndHoldConfirmation.generated.h"


class UImage;
class UTextBlock;
class UFMODEvent;

namespace FMOD
{
namespace Studio
{
	class EventInstance;
}
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoConfirmation);


/**
 *  A main menu enter tip with a circular(console) or quadratic(keyboard) loading bar.
 *
 *  Fires an event if it has been pressed for a duration
 */
UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUIPressAndHoldConfirmation : public UUserWidget
{
	GENERATED_BODY()

public:

	//
	// UUserWidget Interface
	//
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	//
	// Own Methods
	//

	// Confirmed
	FSoConfirmation& OnConfirmed() { return Confirmed; }

	UFUNCTION(BlueprintCallable)
	void Reinitialize();

	bool IsConfirmed() const { return bConfirmed; }

	UFUNCTION(BlueprintCallable)
	void SetConfirmationEnabled(bool bEnabled) { bConfirmationEnabled = bEnabled; }


protected:

	void StopSFX();

	// Only update local so that we do not update the image, that is done already
	UFUNCTION(BlueprintCallable)
	void UpdateImageTexture(ESoInputDeviceType DeviceType);

	UFUNCTION(BlueprintImplementableEvent)
	void PlayFadeAnimation(bool bShow, bool bInstantChange);

	UFUNCTION()
	void HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType)
	{
		UpdateImageTexture(InDeviceType);
	}

	void SetPercent(float Percent);

	UFUNCTION(BlueprintCallable)
	void UpdateVisibility(bool bShow, bool bInstantChange);

protected:

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoConfirmation Confirmed;


	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* CommandImage;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* CommandImageProgressBar;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* SkipText;


	UPROPERTY(EditAnywhere)
	UTexture2D* ProgressImgKeyboard;

	UPROPERTY(EditAnywhere)
	UTexture2D* ProgressImgGamepad;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHideIfNotPressed = false;

	UPROPERTY(EditAnywhere)
	float ConfirmDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bAlsoAcceptMainMenuBack = false;

	float ConfirmCounter = 0.0f;

	bool bElementsVisible = true;

	bool bConfirmed = false;

	bool bConfirmationEnabled = true;


	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXProgress = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXDone = nullptr;

	FMOD::Studio::EventInstance* Instance = nullptr;
};
