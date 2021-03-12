// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SoUIDialogueTextBox.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoDialogueTextboxNotify);

class UTextBlock;
class UTexture2D;
class UImage;
class URetainerBox;
class URichTextBlock;
class URichTextBlock;
class UFMODEvent;


////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoDialogueTextBoxState : uint8
{
	EDTBS_Closed			UMETA(DisplayName = "Closed"),
	EDTBS_Opened			UMETA(DisplayName = "Opened"),

	EDTBS_Opening			UMETA(DisplayName = "Opening"),
	EDTBS_Closing			UMETA(DisplayName = "Closing"),
	EDTBS_Changing			UMETA(DisplayName = "Changing"),

	EDTBS_MAX				UMETA(Hidden),
};


/**
 * 
 */
UCLASS()
class SORB_API USoUIDialogueTextBox : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void SetSpeakerData(UTexture2D* Background, UTexture2D* NameImage, const FText& Name);

	/** 
	 *  New text will only be displayed if the textbox was in an idle state: opened or closed
	 *  If it was opened it will switch to EDTBS_Changing, close, change text, and open again visually
	 *  If it was closed the widget becomes visible and it opens
	 *  @Return: false if the call was ignored
	 */
	bool DisplayText(const FText& TextToDisplay);

	/** Only happens if there wasn't any change in progress, switches to EDTBS_Closing, and the widget only becomes invisible once it is closed */
	bool HideDialogueTextBox();

	void ForceHideDialogueTextBox();

	void ForceFinishChange();

	bool IsDialogueTextBoxHidden() const { return TextBoxState == ESoDialogueTextBoxState::EDTBS_Closed; }
	bool IsDialogueTextBoxOpenedAndInIdle() const { return TextBoxState == ESoDialogueTextBoxState::EDTBS_Opened; }
	bool IsDialogueTextBoxAnimated() const;

	ESoDialogueTextBoxState GetDialogueTextBoxState() const { return TextBoxState; }

	void SetAnimationSpeedMultiplier(float InAnimationSpeedMultiplier) { AnimationSpeedMultiplier = InAnimationSpeedMultiplier; }

protected:
	const UCurveFloat* GetCurrentCurve() const;
	float GetCurrentChangeDuration() const;

	// Rescale the backgound and retainer
	void SetRetainerPercentSafe(float Percent);

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateTextJustificationBP(bool bCenter);

public:

	FSoDialogueTextboxNotify OnDialogueTextboxOpenned;

protected:

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	UTextBlock* NameText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	UImage* NameBackground;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	URetainerBox* TextRetainer;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	URichTextBlock* RichTextField;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	UImage* TextBackground;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	float TextChangeDuration;

	// Max Height of the Text box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widgets)
	float MaxHeight = 700.0f;

	/** first half of text change */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UCurveFloat* TextChangeAnimCurveClosing;

	/** second half of text change */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UCurveFloat* TextChangeAnimCurveOpening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	float TextDisplayDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UCurveFloat* TextDisplayAnimCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	float TextHideDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UCurveFloat* TextHideAnimCurve;

	
	// Height of the Text box
	float CurrentHeight = 200.0f;

	float Counter;
	bool bTextUpdated = false;
	FText NewText;

	ESoDialogueTextBoxState TextBoxState;

	float AnimationSpeedMultiplier = 1.0f;


	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXTextBoxOpen = nullptr;

	// UPROPERTY(EditAnywhere, Category = SFX)
	// UFMODEvent* SFXTextBoxClose = nullptr;
};
