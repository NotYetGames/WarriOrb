// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "UI/General/SoUITypes.h"

#include "SoUICredits.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoCreditsFinished);

class UFMODEvent;
class URichTextBlock;
class UWidgetAnimation;
class USoUIPressAndHoldConfirmation;

UENUM(BlueprintType)
enum class ESoUICreditsState : uint8
{
	// only the buttons are visible
	Inactive				UMETA(DisplayName = "Inactive"),
	FadeIn					UMETA(DisplayName = "FadeIn"),
	Scrolling				UMETA(DisplayName = "Scrolling"),
	FadeOut					UMETA(DisplayName = "FadeOut"),

	NumOf				UMETA(meta = Hidden)
};


UCLASS()
class SORB_API USoUICredits : public UUserWidget
{
	GENERATED_BODY()

public:

	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** can be used from outside to directly start scroll instead of fading in (when the screen is black already anyway */
	UFUNCTION(BlueprintCallable)
	void StartScroll();

	UFUNCTION(BlueprintCallable)
	void FadeIn(bool bInUseFadeOut);

	UFUNCTION(BlueprintCallable)
	void FadeOut();

	UFUNCTION()
	void Skip();


	UFUNCTION(BlueprintCallable)
	void HandleCommand(ESoUICommand Command);


	FSoCreditsFinished& OnCreditsFinished() { return CreditsFinished; }
	FSoCreditsFinished& OnFadeOutFinished() { return FadeOutFinished; }

protected:
	void Hide();

	UFUNCTION(BlueprintImplementableEvent)
	void FadeInBP();

	UFUNCTION(BlueprintImplementableEvent)
	void FadeOutBP();

	UFUNCTION(BlueprintImplementableEvent)
	void StartScrollBP();

protected:

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoCreditsFinished CreditsFinished;

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoCreditsFinished FadeOutFinished;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	UFMODEvent* Music;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	UFMODEvent* MusicOnExit;


	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	URichTextBlock* CreditsText;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIPressAndHoldConfirmation* SkipButton;


	float FadeOutCounter = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Duration = 112.0f;

	float Counter;


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ESoUICreditsState ActiveState = ESoUICreditsState::Inactive;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UWidgetAnimation* FadeAnim = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UWidgetAnimation* ScrollAnim = nullptr;

	bool bUseFadeOut = false;
};
