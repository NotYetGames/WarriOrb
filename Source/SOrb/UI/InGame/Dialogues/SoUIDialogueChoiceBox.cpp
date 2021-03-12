// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIDialogueChoiceBox.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"

#include "Materials/MaterialInstanceDynamic.h"

#include "Basic/Helpers/SoMathHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUIDialogueChoiceBox, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueChoiceBox::NativeConstruct()
{
	Super::NativeConstruct();

	ChoiceState = ESoDialogueChoiceBoxState::EDCBS_Inactive;
	SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueChoiceBox::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bUpdateCalledFromParent)
		Update(InDeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueChoiceBox::Update(float InDeltaTime)
{
	switch (ChoiceState)
	{
		case ESoDialogueChoiceBoxState::EDCBS_FadeIn:
			CounterSeconds += InDeltaTime;
			if (CounterSeconds > StartDelay)
			{
				const float Percent = FMath::Min((CounterSeconds - StartDelay) / FadeInTime, 1.0f);
				const FVector2D Position = USoMathHelper::InterpolateDeceleration(EndPosition + StartOffset, EndPosition, Percent);
				UCanvasPanelSlot* AsCanvasSlot = Cast<UCanvasPanelSlot>(Slot);
				if (AsCanvasSlot != nullptr)
					AsCanvasSlot->SetPosition(Position);

				SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, USoMathHelper::InterpolateDeceleration(0.0f, 1.0f, Percent)));

				if (HighlightedPercentTarget > 0.5f)
					UpdateHighlight(Percent);

				if (FMath::IsNearlyEqual(Percent, 1.f))
					ChoiceState = ESoDialogueChoiceBoxState::EDCBS_Active;
			}
			break;

		case ESoDialogueChoiceBoxState::EDCBS_FadeOut:
			{
				CounterSeconds += InDeltaTime;
				const float Percent = FMath::Min(CounterSeconds / FadeOutTime, 1.0f);
				SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f - Percent));

				const FVector2D MaxOffset = HighlightedPercentTarget > 0.5f ? FadeOutOffsetHighlighted : FadeOutOffset;
				const FVector2D Position = USoMathHelper::InterpolateDeceleration(EndPosition, EndPosition + MaxOffset, Percent);
				UCanvasPanelSlot* AsCanvasSlot = Cast<UCanvasPanelSlot>(Slot);
				if (AsCanvasSlot != nullptr)
					AsCanvasSlot->SetPosition(Position);

				if (FMath::IsNearlyEqual(Percent, 1.f))
				{
					ChoiceState = ESoDialogueChoiceBoxState::EDCBS_Inactive;
					SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			break;

		case ESoDialogueChoiceBoxState::EDCBS_ChangeHighlight:
			{
				CounterSeconds += InDeltaTime;
				const float Percent = FMath::Min(CounterSeconds / HighlightTime, 1.0f);
				UpdateHighlight(HighlightedPercentTarget > 0.5f ? Percent : 1.0f - Percent);

				if (FMath::IsNearlyEqual(Percent, 1.f))
					ChoiceState = ESoDialogueChoiceBoxState::EDCBS_Active;
			}
			break;

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueChoiceBox::DisplayText(const FText& TextToDisplay, const FVector2D& InEndPosition, float InDelay, bool bHighlight)
{
	// Only allow to display text once this widget is in the inactive state
	if (ChoiceState != ESoDialogueChoiceBoxState::EDCBS_Inactive)
	{
		UE_LOG(LogSoUIDialogueChoiceBox,
			Warning,
			TEXT("Trying to call DisplayText(TextToDisplay = `%s`) on choicebox but the ChoiceState (%d) != ESoDialogueChoiceBoxState::EDCBS_Inactive. Ignoring fading in!"),
			*TextToDisplay.ToString(), static_cast<int32>(ChoiceState));
		return false;
	}

	ChoiceState = ESoDialogueChoiceBoxState::EDCBS_FadeIn;
	EndPosition = InEndPosition;

	// extra check cause of weird issue
	if (bUpdateCalledFromParent)
	{
		if (fabs(InEndPosition.X) > 60 || fabs(InEndPosition.Y) > 1000)
		{
			UE_LOG(LogSoUIDialogueChoiceBox,
				Warning,
				TEXT("Trying to call DisplayText on choicebox with invalid end position %f %f"),
				InEndPosition.X, InEndPosition.Y);

			EndPosition.X = FMath::Clamp(InEndPosition.X, -52.0f, 20.0f);
			EndPosition.Y = FMath::Clamp(InEndPosition.Y, -748.0f, 0.0f);
		}
	}

	StartDelay = InDelay;
	HighlightedPercentTarget = bHighlight ? 1.0f : 0.0f;
	CounterSeconds = 0.0f;

	UpdateHighlight(0.0f);

	ChoiceText->SetText(TextToDisplay);
	SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	SetVisibility(ESlateVisibility::Visible);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueChoiceBox::HideChoiceTextBox()
{
	if (ChoiceState != ESoDialogueChoiceBoxState::EDCBS_Active)
	{
		// UE_LOG(LogSoUIDialogueChoiceBox,
		// 	Warning,
		// 	TEXT("Trying to call HideChoiceTextBox() on choicebox but the ChoiceState (%d)  != ESoDialogueChoiceBoxState::EDCBS_Active. Ignoring fading out!"),
		// 	static_cast<int32>(ChoiceState));
		return false;
	}

	ChoiceState = ESoDialogueChoiceBoxState::EDCBS_FadeOut;
	CounterSeconds = 0.0f;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueChoiceBox::ForceHideChoiceTextBox()
{
	SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));

	const FVector2D MaxOffset = HighlightedPercentTarget > 0.5f ? FadeOutOffsetHighlighted : FadeOutOffset;
	const FVector2D Position = USoMathHelper::InterpolateDeceleration(EndPosition, EndPosition + MaxOffset, 1.0f);
	UCanvasPanelSlot* AsCanvasSlot = Cast<UCanvasPanelSlot>(Slot);
	if (AsCanvasSlot != nullptr)
		AsCanvasSlot->SetPosition(Position);
	ChoiceState = ESoDialogueChoiceBoxState::EDCBS_Inactive;
	SetVisibility(ESlateVisibility::Collapsed);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueChoiceBox::ForceFinishChange()
{
	UE_LOG(LogSoUIDialogueChoiceBox, Warning, TEXT("USoUIDialogueChoiceBox::ForceFinishChange called - something went wrong with the dialogue UI again!"));

	switch (ChoiceState)
	{
		case ESoDialogueChoiceBoxState::EDCBS_FadeIn:
			CounterSeconds = StartDelay + FadeInTime;
			if (UCanvasPanelSlot* AsCanvasSlot = Cast<UCanvasPanelSlot>(Slot))
				AsCanvasSlot->SetPosition(EndPosition);

			SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
			if (HighlightedPercentTarget > 0.5f)
				UpdateHighlight(1.0f);
			ChoiceState = ESoDialogueChoiceBoxState::EDCBS_Active;
			SetVisibility(ESlateVisibility::Visible);
			break;

		case ESoDialogueChoiceBoxState::EDCBS_FadeOut:
			ForceHideChoiceTextBox();
			break;

		case ESoDialogueChoiceBoxState::EDCBS_ChangeHighlight:
			CounterSeconds = HighlightTime;
			UpdateHighlight(HighlightedPercentTarget > 0.5f ? 1.0f : 0.0f);
			ChoiceState = ESoDialogueChoiceBoxState::EDCBS_Active;
			break;

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueChoiceBox::SetHighlighted(bool bHighlight)
{
	verify(ChoiceState == ESoDialogueChoiceBoxState::EDCBS_Active ||
		   ChoiceState == ESoDialogueChoiceBoxState::EDCBS_ChangeHighlight);

	HighlightedPercentTarget = bHighlight ? 1.0f : 0.0f;
	if (!FMath::IsNearlyEqual(HighlightedPercent, HighlightedPercentTarget))
	{
		ChoiceState = ESoDialogueChoiceBoxState::EDCBS_ChangeHighlight;
		CounterSeconds = 0.0f;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueChoiceBox::UpdateHighlight(float NewPercent)
{
	HighlightedPercent = NewPercent;

	const FVector2D RenderScale = USoMathHelper::InterpolateSmoothStep(FVector2D(1.0f, 1.0f), HighlightScale, HighlightedPercent);
	// SetRenderScale(RenderScale);
	BackgroundFrame->SetRenderScale(RenderScale);
	Background->SetRenderScale(RenderScale);

	static const FName HighlightName{ TEXT("Highlight") };
	BackgroundFrame->GetDynamicMaterial()->SetScalarParameterValue(HighlightName, HighlightedPercent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIDialogueChoiceBox::IsChoiceTextBoxAnimated() const
{
	check(ChoiceState != ESoDialogueChoiceBoxState::EDCBS_MAX);
	return ChoiceState != ESoDialogueChoiceBoxState::EDCBS_Active && ChoiceState != ESoDialogueChoiceBoxState::EDCBS_Inactive;
}
