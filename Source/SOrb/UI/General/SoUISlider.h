// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SoUIUserWidget.h"

#include "SoUITypes.h"
#include "Components/Slider.h"

#include "SoUISlider.generated.h"


class UTextBlock;
class USlider;
class ASoCharacter;


UENUM(BlueprintType)
enum class ESoUISliderValueType : uint8
{
	Percentage = 0,

	// Do not display the value
	Hidden,

	// The float value, Same as Percentage only without the %
	FloatValue,

	// The normalized value
	FloatValueNormalized,
};

/**
 * Slider Element that also has a text box widget besides it
 * [slider][text percentage]
 */
UCLASS()
class SORB_API USoUISlider : public USoUIUserWidget
{
	GENERATED_BODY()

public:
	USoUISlider(const FObjectInitializer& ObjectInitializer);

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void NativeConstruct() override;
	void NativeDestruct() override;


	//
	// USoUIUserWidget interface
	//

	// NOTE: left empty on purpose as we are not using the active state
	void SetIsActive(bool bActive, bool bPlaySound = false) override {}
	bool IsValidWidget() const override { return GetIsEnabled() && IsVisible(); }
	void OnStateChanged() override;

	//
	// Own methods
	//

	/** Move the slider by using navigation. */
	bool Navigate(ESoUICommand Command, bool bPlaySound = true) override;

	/** Gets the current value of the slider between [0, 1] */
	UFUNCTION(BlueprintPure, Category = Data)
	float GetValueNormalized() const;

	/** Gets the current value as a percent between [0, 100] */
	UFUNCTION(BlueprintPure, Category = Data)
	float GetValue() const;

	/** Sets the amount to adjust the value by, when using a controller or keyboard */
	UFUNCTION(BlueprintCallable, Category = Data)
	void SetStepSizeNormalized(float InValueNormalized);

	/** Set step size in the normal display range. */
	UFUNCTION(BlueprintCallable, Category = Data)
	void SetStepSize(float InValue);

	/** Sets the current value (between [0, 1]) of the slider. Returns true if value changed. */
	UFUNCTION(BlueprintCallable, Category = Data)
	bool SetValueNormalized(float InValueNormalized, bool bPlaySound = false);

	/** Sets the current value (between [MinDisplayValue, MaxDisplayValue]) of the slider.*/
	UFUNCTION(BlueprintCallable, Category = Data)
	bool SetValue(float InValue, bool bPlaySound = false);

	/** Sets the Percentage text min/max displayed values ranges.  */
	void SetRangeTextDisplayValues(float Min, float Max)
	{
		MinDisplayValue = Min;
		MaxDisplayValue = Max;
	}

	ThisClass* SetTrackPressedInput(bool bEnable)
	{
		bTrackPressedInput = bEnable;
		return this;
	}

	FOnFloatValueChangedEvent& OnValueChangedEvent() { return ValueChangedEvent; }

protected:


	// Listen to USlider events
	UFUNCTION()
	void HandleOnMouseCaptureBegin();
	UFUNCTION()
	void HandleOnMouseCaptureEnd();
	UFUNCTION()
	void HandleOnControllerCaptureBegin();
	UFUNCTION()
	void HandleOnControllerCaptureEnd();
	UFUNCTION()
	void HandleUSliderOnValueChanged(float InValue);

	// Gets a valid value in range for the provided normalized value
	float GetValueInDisplayRangeFromNormalizedValue(float NormalizedValue) const
	{
		return FMath::Lerp(MinDisplayValue, MaxDisplayValue, FMath::Clamp(NormalizedValue, 0.f, 1.f));
	}

	// Gets the normalized value for this range value, opposite of GetValueInDisplayRange
	float GetNormalizedValueFromDisplayRange(float ValueInRange) const
	{
		return (ValueInRange - MinDisplayValue) / (MaxDisplayValue - MinDisplayValue);
	}

	void UpdateValueText();

	// Return true if value was changed
	bool OnNewValueSet(bool bPlaySound);

protected:
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FOnFloatValueChangedEvent ValueChangedEvent;

	/** The Value text element */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* ValueText = nullptr;

	/** The actual slider */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	USlider* Slider = nullptr;

	// How should we display the value
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Slider")
	ESoUISliderValueType ValueType = ESoUISliderValueType::Percentage;

	// Should be in range [0, MaxDisplayValue]
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Slider")
	float StepSize = 1.f;

	// At what value multiple of this should the slider snap at.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Slider")
	int32 SnapMultiplier = 1;

	// The minimum displayed value, used for the PercentageText
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Slider")
	float MinDisplayValue = 0.f;

	// The maximum displayed value, used for the PercentageText
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Slider")
	float MaxDisplayValue = 100.f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Slider")
	FSoUITrackedInputKeyDown TrackedInput;

	// Enable moving navigation if a certain button is pressed
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Input")
	bool bTrackPressedInput = true;

	// Keep track of the previous broadcasted value to know if we can broadcast now too
	float PreviousBroadcastedValue = -1.f;


	//
	// SFX
	//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	bool bSFXSetDefault = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXValueChanged = nullptr;

	//
	// Cached
	//

	UPROPERTY()
	ASoCharacter* SoCharacter = nullptr;
};
