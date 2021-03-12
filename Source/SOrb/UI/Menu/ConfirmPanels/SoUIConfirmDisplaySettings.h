// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once


#include "CoreMinimal.h"
#include "UI/General/SoUIConfirmPanel.h"

#include "SoUIConfirmDisplaySettings.generated.h"

class UTextBlock;


UENUM(BlueprintType)
enum class ESoUIConfirmDisplaySettingsType : uint8
{
	// Invalid
	None = 0,

	Confirm,
	Cancel,
};

UCLASS()
class SORB_API USoUIConfirmDisplaySettings : public USoUIConfirmPanel
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	//
	// USoUIEventHandler Interface
	//
	void Open_Implementation(bool bOpen) override;
	bool OnUICommand_Implementation(ESoUICommand Command) override;

	//
	// USoUIConfirmPanel interface
	//

	void RefreshButtonsArray() override;
	bool OnPressedButtonTooltipsCommand(ESoUICommand Command) override;

	//
	// Own methods
	//

	// Can confirm video mode;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events")
	void ReceiveOnConfirmedVideoMode();

	UFUNCTION(BlueprintCallable, Category = ">Events")
	void OnConfirmedVideoMode();

	// Can revert video mode
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events")
	void ReceiveOnRevertVideoMode();

	UFUNCTION(BlueprintCallable, Category = ">Events")
	void OnRevertVideoMode();

	UFUNCTION(BlueprintCallable)
	void ResetSecondsToDefault();

protected:
	static constexpr ESoUICommand UICommand_ConfirmSettings = ESoUICommand::EUC_Action0;
	static constexpr ESoUICommand UICommand_CancelSettings = ESoUICommand::EUC_Action1;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* OverlayTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Defaults")
	float DefaultSeconds = 6.f;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	float CurrentSeconds = 10.f;


	// Map selected button index => ESoUIConfirmQuitAnswerType (button type)
	const TArray<ESoUIConfirmDisplaySettingsType> AnswerButtonOption = {
		ESoUIConfirmDisplaySettingsType::Confirm,
		ESoUIConfirmDisplaySettingsType::Cancel
	};
};
