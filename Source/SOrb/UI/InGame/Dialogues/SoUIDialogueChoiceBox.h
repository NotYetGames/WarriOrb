// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SoUIDialogueChoiceBox.generated.h"

class UImage;
class UTextBlock;

////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoDialogueChoiceBoxState : uint8
{
	EDCBS_Inactive			UMETA(DisplayName = "Inactive"),
	EDCBS_Active			UMETA(DisplayName = "Active"),
	EDCBS_FadeIn			UMETA(DisplayName = "Fade In"),
	EDCBS_FadeOut			UMETA(DisplayName = "Fade Out"),
	EDCBS_ChangeHighlight	UMETA(DisplayName = "ChangeHighlight"),

	EDCBS_MAX				UMETA(Hidden),
};

 ////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoUIDialogueChoiceBox : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void Update(float InDeltaTime);

	// Only has any effect if the option is inactive, otherwise simply returns false
	bool DisplayText(const FText& TextToDisplay, const FVector2D& InEndPosition, float InDelay, bool bHighlight);

	// Only has any effect if the option is active, otherwise simply returns false
	bool HideChoiceTextBox();

	// instantly hides box
	void ForceHideChoiceTextBox();

	void ForceFinishChange();

	// Only has any effect if the option is active, otherwise simply returns false
	bool SetHighlighted(bool bHighlight);


	bool IsChoiceTextBoxAnimated() const;

	bool CanHighlightChange() const { return ChoiceState == ESoDialogueChoiceBoxState::EDCBS_Active || ChoiceState == ESoDialogueChoiceBoxState::EDCBS_ChangeHighlight; }

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = UI)
	void PlayPressedEffect();

	void SetUpdateCalledFromParent(bool bCalled) { bUpdateCalledFromParent = bCalled; }

protected:
	UCurveFloat* GetCurrentCurve() const;
	float GetCurrentChangeDuration() const;

	void UpdateHighlight(float NewPercent);

protected:

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	UTextBlock* ChoiceText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	UImage* BackgroundFrame;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	UImage* Background;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	float FadeInTime = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	float FadeOutTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FVector2D FadeOutOffset = FVector2D(10.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FVector2D StartOffset = FVector2D(20.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FVector2D FadeOutOffsetHighlighted = FVector2D(-10.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	float HighlightTime = 0.3f;

	// Counter for Tick()
	float CounterSeconds = 0.f;

	/** Position of the widget after fully faded in */
	FVector2D EndPosition{ EForceInit::ForceInitToZero };

	/** Time to wait before fade starts */
	float StartDelay = 0.f;

	ESoDialogueChoiceBoxState ChoiceState = ESoDialogueChoiceBoxState::EDCBS_Inactive;

	float HighlightedPercent = 0.0f;
	float HighlightedPercentTarget = 0.0f;

	UPROPERTY(EditAnywhere, Category = Appearance)
	FVector2D HighlightScale = FVector2D(1.2f, 1.2f);

	bool bUpdateCalledFromParent = false;
};
