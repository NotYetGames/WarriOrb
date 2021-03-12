// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISlider.h"

#include "Components/TextBlock.h"
#include "Components/Slider.h"

#include "Localization/SoLocalization.h"
#include "Character/SoCharacter.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "UI/SoUIHelper.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoAudioManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUISlider::USoUISlider(const FObjectInitializer& ObjectInitializer)
{
	bIsActive = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (bSFXSetDefault && SFXValueChanged == nullptr)
		SFXValueChanged = USoGameSingleton::GetSFX(ESoSFX::SettingsValueSwitch);

	bIsActive = true;
	SetStepSize(StepSize);
	UpdateValueText();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bTrackPressedInput)
		return;

	if (!SoCharacter || !IsValidWidget() || !IsHighlighted() || !SoCharacter->IsAnyUIInputPressed())
	{
		TrackedInput.Reset();
		return;
	}

	// Pressed left/right change value
	TrackedInput.Tick(InDeltaTime, [&]()
	{
		if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Left, ESoUICommand::EUC_MainMenuLeft }))
			Navigate(ESoUICommand::EUC_Left, false);
		else if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Right, ESoUICommand::EUC_MainMenuRight }))
			Navigate(ESoUICommand::EUC_Right, false);
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::NativeConstruct()
{
	Super::NativeConstruct();

	SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (Slider)
	{
		Slider->OnMouseCaptureBegin.AddDynamic(this, &ThisClass::HandleOnMouseCaptureBegin);
		Slider->OnMouseCaptureEnd.AddDynamic(this, &ThisClass::HandleOnMouseCaptureEnd);
		Slider->OnControllerCaptureBegin.AddDynamic(this, &ThisClass::HandleOnControllerCaptureBegin);
		Slider->OnControllerCaptureEnd.AddDynamic(this, &ThisClass::HandleOnControllerCaptureEnd);
		Slider->OnValueChanged.AddDynamic(this, &ThisClass::HandleUSliderOnValueChanged);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::NativeDestruct()
{
	if (Slider)
	{
		Slider->OnMouseCaptureBegin.RemoveDynamic(this, &ThisClass::HandleOnMouseCaptureBegin);
		Slider->OnMouseCaptureEnd.RemoveDynamic(this, &ThisClass::HandleOnMouseCaptureEnd);
		Slider->OnControllerCaptureBegin.RemoveDynamic(this, &ThisClass::HandleOnControllerCaptureBegin);
		Slider->OnControllerCaptureEnd.RemoveDynamic(this, &ThisClass::HandleOnControllerCaptureEnd);
		Slider->OnValueChanged.RemoveDynamic(this, &ThisClass::HandleUSliderOnValueChanged);
	}
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISlider::Navigate(ESoUICommand Command, bool bPlaySound)
{
	// Ignore navigation, widget is disabled
	if (!IsValidWidget())
		return false;

	bool bChanged = false;
	Command = USoUIHelper::TryTranslateMenuCommandDirectionToGame(Command);
	if (Command == ESoUICommand::EUC_Left)
		bChanged = SetValue(GetValue() - StepSize, bPlaySound);
	else if (Command == ESoUICommand::EUC_Right)
		bChanged = SetValue(GetValue() + StepSize, bPlaySound);

	return bChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoUISlider::GetValueNormalized() const
{
	return Slider ? Slider->GetValue() : 0.f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoUISlider::GetValue() const
{
	return GetValueInDisplayRangeFromNormalizedValue(GetValueNormalized());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::SetStepSizeNormalized(float InValueNormalized)
{
	// Normalized to MaxDisplayValue
	StepSize = FMath::Lerp(0.f, MaxDisplayValue, InValueNormalized);

	// SSlider is normalized to 1.f
	InValueNormalized = FMath::Clamp(InValueNormalized, 0.f, 1.f);
	if (Slider)
		Slider->SetStepSize(InValueNormalized);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::SetStepSize(float InValue)
{
	SetStepSizeNormalized(InValue / MaxDisplayValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISlider::SetValueNormalized(float InValueNormalized, bool bPlaySound)
{
	// SSlider is normalized to 1.f
	InValueNormalized = FMath::Clamp(InValueNormalized, 0.f, 1.f);

	// Snap to snap multiplier
	const int32 CurrentPercentValue = FMath::RoundToInt(GetValueInDisplayRangeFromNormalizedValue(InValueNormalized));
	const int32 Remainder = CurrentPercentValue % SnapMultiplier;
	if (Remainder != 0)
	{
		// Must snap, to the next value in range
		const int32 DifferenceUntilSnap = SnapMultiplier - Remainder;
		InValueNormalized = GetNormalizedValueFromDisplayRange(CurrentPercentValue + DifferenceUntilSnap);
	}

	// If successful it will call HandleUSliderOnValueChanged
	if (Slider)
		Slider->SetValue(InValueNormalized);

	return OnNewValueSet(bPlaySound);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISlider::SetValue(float InValue, bool bPlaySound)
{
	return SetValueNormalized(GetNormalizedValueFromDisplayRange(InValue), bPlaySound);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::OnStateChanged()
{
	Super::OnStateChanged();
	UpdateValueText();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::UpdateValueText()
{
	if (!ValueText)
		return;

	ValueText->SetVisibility(ESlateVisibility::Visible);
	switch (ValueType)
	{
	case ESoUISliderValueType::Percentage:
	{
		// ValueText->SetText(
		// 	FText::AsPercent(GetValueNormalized(),  &FNumberFormattingOptions::DefaultNoGrouping())
		// );
		// TODO Maybe use as percent?
		const int32 Number = FMath::RoundToInt(GetValue());
		FFormatOrderedArguments Arguments;
		Arguments.Add(FText::AsNumber(Number));
		ValueText->SetText(FText::Format(FText::FromString(TEXT("{0}%")), Arguments));
		break;
	}
	case ESoUISliderValueType::Hidden:
		ValueText->SetVisibility(ESlateVisibility::Collapsed);
		break;

	case ESoUISliderValueType::FloatValue:
	{
		ValueText->SetText(FText::AsNumber(GetValue()));
		break;
	}
	case ESoUISliderValueType::FloatValueNormalized:
	{
		ValueText->SetText(FText::AsNumber(GetValueNormalized()));
		break;
	}
	default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISlider::OnNewValueSet(bool bPlaySound)
{
	// Value changed?
	const float NewValue = GetValueNormalized();
	const bool bChanged = !FMath::IsNearlyEqual(PreviousBroadcastedValue, NewValue, KINDA_SMALL_NUMBER);

	// Play sound
	if (bPlaySound && bChanged)
		USoAudioManager::PlaySound2D(this, SFXValueChanged);

	// Update values
	if (bChanged)
	{
		UpdateValueText();

		// Broadcast to listeners the new value
		ValueChangedEvent.Broadcast(NewValue);
		PreviousBroadcastedValue = NewValue;
	}

	return bChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::HandleOnMouseCaptureBegin()
{
	//UE_LOG(LogTemp, Warning, TEXT("OnMouseCaptureBegin"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::HandleOnMouseCaptureEnd()
{
	//UE_LOG(LogTemp, Warning, TEXT("HandleOnMouseCaptureEnd"));

	// Snap to correct value
	SetValueNormalized(GetValueNormalized(), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::HandleUSliderOnValueChanged(float InValue)
{
	//UE_LOG(LogTemp, Warning, TEXT("HandleUSliderOnValueChanged: %f"), InValue);
	OnNewValueSet(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::HandleOnControllerCaptureBegin()
{
	// NOTE: not used as we handle it in Navigate
	// UE_LOG(LogTemp, Warning, TEXT("HandleOnControllerCaptureBegin"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISlider::HandleOnControllerCaptureEnd()
{
	// NOTE: not used as we handle it in Navigate
	// UE_LOG(LogTemp, Warning, TEXT("HandleOnControllerCaptureEnd"));
	SetValueNormalized(GetValueNormalized(), true);
}
