// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIDialogueTextBox.h"

#include "Components/RetainerBox.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Basic/SoAudioManager.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueTextBox::NativeConstruct()
{
	Super::NativeConstruct();

	TextBoxState = ESoDialogueTextBoxState::EDTBS_Closed;
	SetRetainerPercentSafe(0.0f);

	SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueTextBox::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// NOTE: in the first frames this will be most likely not valid, totally normal, one prepas on the renderer has to be done
	// before desired size returns anything approaching to reality
	// Simulate what GetDesiredSize does
	TSharedPtr<SWidget> TextFieldSlateWidget = RichTextField->GetCachedWidget();
	if (TextFieldSlateWidget.IsValid())
	{
		CurrentHeight = TextFieldSlateWidget->GetDesiredSize().Y;
	}
	// RichTextField->SetJustification(CurrentHeight < 80.0f ? ETextJustify::Center : ETextJustify::Left);
	UpdateTextJustificationBP(CurrentHeight < 90.0f);
	RichTextField->SynchronizeProperties();

	auto UpdateTextBoxSize = [this](float Percent)
	{
		// reference points:
		// y0 = 0.142   x0 = 100
		// y1 = 1.0		x1 = 605
		// m = (y1 - y0) / (x1 - x0)
		// b = y0 - x0 * m
		static const float m = 0.00169901;
		static const float b = -0.02790099;
		const float NewValue =  Percent * FMath::Max(0.0f, CurrentHeight * m + b);
		SetRetainerPercentSafe(NewValue);
	};


	const UCurveFloat* ChangeCurve = GetCurrentCurve();
	if (ChangeCurve != nullptr)
	{
		// Animate
		const float CurrentDuration = GetCurrentChangeDuration();
		Counter = FMath::Min(Counter + InDeltaTime * AnimationSpeedMultiplier, CurrentDuration);
		const float NewPercent = Counter / CurrentDuration;
		UpdateTextBoxSize(ChangeCurve->GetFloatValue(NewPercent));


		if (FMath::IsNearlyEqual(CurrentDuration, Counter, KINDA_SMALL_NUMBER))
		{
			switch (TextBoxState)
			{
				case ESoDialogueTextBoxState::EDTBS_Changing:
					if (!bTextUpdated)
					{
						bTextUpdated = true;
						RichTextField->SetText(NewText);
						Counter = 0.0f;
						USoAudioManager::PlaySound2D(this, SFXTextBoxOpen);
						break;
					}
				case ESoDialogueTextBoxState::EDTBS_Opening:
					TextBoxState = ESoDialogueTextBoxState::EDTBS_Opened;
					AnimationSpeedMultiplier = 1.0f;
					OnDialogueTextboxOpenned.Broadcast();
					break;

				case ESoDialogueTextBoxState::EDTBS_Closing:
					TextBoxState = ESoDialogueTextBoxState::EDTBS_Closed;
					AnimationSpeedMultiplier = 1.0f;
					SetVisibility(ESlateVisibility::Collapsed);
					break;

				case ESoDialogueTextBoxState::EDTBS_Closed:
				case ESoDialogueTextBoxState::EDTBS_Opened:
				default:
					break;
			}
		}
	}
	else if (TextBoxState == ESoDialogueTextBoxState::EDTBS_Opened)
	{
		// have to keep update it be cause of some rare low performance issue
		UpdateTextBoxSize(1.0f);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueTextBox::SetSpeakerData(UTexture2D* Background, UTexture2D* NameImage, const FText& Name)
{
	static const FName TextureName = FName("Bgr_Texture");
	TextBackground->GetDynamicMaterial()->SetTextureParameterValue(TextureName, Background);
	NameBackground->SetBrushFromTexture(NameImage);
	NameText->SetText(Name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueTextBox::DisplayText(const FText& TextToDisplay)
{
	switch(TextBoxState)
	{
		case ESoDialogueTextBoxState::EDTBS_Changing:
		case ESoDialogueTextBoxState::EDTBS_Opening:
		case ESoDialogueTextBoxState::EDTBS_Closing:
			return false;

		case ESoDialogueTextBoxState::EDTBS_Closed:
			RichTextField->SetText(TextToDisplay);
			SetVisibility(ESlateVisibility::Visible);
			SetRetainerPercentSafe(0.0f);
			TextBoxState = ESoDialogueTextBoxState::EDTBS_Opening;
			Counter = 0.0f;
			bTextUpdated = true;
			AnimationSpeedMultiplier = 1.0f;

			USoAudioManager::PlaySound2D(this, SFXTextBoxOpen);

			return true;

		case ESoDialogueTextBoxState::EDTBS_Opened:
			NewText = TextToDisplay;
			TextBoxState = ESoDialogueTextBoxState::EDTBS_Changing;
			bTextUpdated = false;
			Counter = 0.0f;
			AnimationSpeedMultiplier = 1.0f;

			// USoAudioManager::PlaySound2D(this, SFXTextBoxClose);

		default:
			return true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueTextBox::HideDialogueTextBox()
{
	switch(TextBoxState)
	{
		case ESoDialogueTextBoxState::EDTBS_Changing:
		case ESoDialogueTextBoxState::EDTBS_Opening:
		case ESoDialogueTextBoxState::EDTBS_Closing:
			return false;

		case ESoDialogueTextBoxState::EDTBS_Opened:
			TextBoxState = ESoDialogueTextBoxState::EDTBS_Closing;
			Counter = 0.0f;

			// USoAudioManager::PlaySound2D(this, SFXTextBoxClose);

		case ESoDialogueTextBoxState::EDTBS_Closed:
		default:
			return true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueTextBox::ForceHideDialogueTextBox()
{
	SetRetainerPercentSafe(0.0f);
	TextBoxState = ESoDialogueTextBoxState::EDTBS_Closed;
	AnimationSpeedMultiplier = 1.0f;
	SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueTextBox::ForceFinishChange()
{
	switch (TextBoxState)
	{
		case ESoDialogueTextBoxState::EDTBS_Changing:
			RichTextField->SetText(NewText);

		case ESoDialogueTextBoxState::EDTBS_Opening:
			SetRetainerPercentSafe(1.0f);
			TextBoxState = ESoDialogueTextBoxState::EDTBS_Opened;
			AnimationSpeedMultiplier = 1.0f;
			OnDialogueTextboxOpenned.Broadcast();
			SetVisibility(ESlateVisibility::Visible);
			break;

		case ESoDialogueTextBoxState::EDTBS_Closing:
			ForceHideDialogueTextBox();
			break;

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueTextBox::IsDialogueTextBoxAnimated() const
{
	switch(TextBoxState)
	{
		case ESoDialogueTextBoxState::EDTBS_Changing:
		case ESoDialogueTextBoxState::EDTBS_Opening:
		case ESoDialogueTextBoxState::EDTBS_Closing:
			return true;

		case ESoDialogueTextBoxState::EDTBS_Opened:
		case ESoDialogueTextBoxState::EDTBS_Closed:
		default:
			return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const UCurveFloat* USoUIDialogueTextBox::GetCurrentCurve() const
{
	switch (TextBoxState)
	{
		case ESoDialogueTextBoxState::EDTBS_Changing:
			return bTextUpdated ? TextChangeAnimCurveClosing : TextChangeAnimCurveOpening;

		case ESoDialogueTextBoxState::EDTBS_Opening:
			return TextDisplayAnimCurve;

		case ESoDialogueTextBoxState::EDTBS_Closing:
			return TextHideAnimCurve;

		case ESoDialogueTextBoxState::EDTBS_Closed:
		case ESoDialogueTextBoxState::EDTBS_Opened:
		default:
			return nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoUIDialogueTextBox::GetCurrentChangeDuration() const
{
	switch (TextBoxState)
	{
		case ESoDialogueTextBoxState::EDTBS_Changing:
			return TextChangeDuration * CurrentHeight / MaxHeight;

		case ESoDialogueTextBoxState::EDTBS_Opening:
			return TextDisplayDuration * CurrentHeight / MaxHeight;

		case ESoDialogueTextBoxState::EDTBS_Closing:
			return TextHideDuration * CurrentHeight / MaxHeight;

		case ESoDialogueTextBoxState::EDTBS_Closed:
		case ESoDialogueTextBoxState::EDTBS_Opened:
		default:
			return -1.0f;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueTextBox::SetRetainerPercentSafe(float Percent)
{
	static const FName EffectMaterialParamNamePercent = FName("EdgeSlide");

	if (TextRetainer != nullptr)
		if (auto* EffectMaterial = TextRetainer->GetEffectMaterial())
			EffectMaterial->SetScalarParameterValue(EffectMaterialParamNamePercent, Percent);

	if (TextBackground && TextBackground->GetDynamicMaterial())
		TextBackground->GetDynamicMaterial()->SetScalarParameterValue(EffectMaterialParamNamePercent, Percent);
}
