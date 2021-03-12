// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PanelWidget.h"

#include "UI/InGame/SoUIGameActivity.h"
#include "Basic/Helpers/SoMathHelper.h"

#include "SoUIChallengeCompleted.generated.h"

class UFMODEvent;
class UImage;
class UCanvasPanel;
class UTextBlock;
class USoUIButtonImage;
class UHorizontalBox;
class USoUIExternalLink;

UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUIChallengeCompleted : public USoInGameUIActivity
{
	GENERATED_BODY()

protected:
	// Begin UUserWidget Interface
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	// End UUserWidget Interface

	bool SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable) override;
	bool HandleCommand_Implementation(ESoUICommand Command) override;
	bool Update_Implementation(float DeltaSeconds) override;

	UFUNCTION()
	void OnExitPressed();

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* Subtitle;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* Description;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* TextDemoThankYou;

	// Shows the back button
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonImage* ButtonImageToolTipExit;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	USoUIUserWidgetArray* ButtonsArray;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UHorizontalBox* ButtonsContainer;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets",  meta = (BindWidget))
	USoUIExternalLink* ButtonDiscord;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets",  meta = (BindWidget))
	USoUIExternalLink* ButtonTwitter;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets",  meta = (BindWidget))
	USoUIExternalLink* ButtonSteam;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets",  meta = (BindWidget))
	USoUIExternalLink* ButtonExit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">State")
	bool bOpened = false;

	// Special case for demo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">State")
	bool bDemoEnding = false;

	// Command/shortcut used for leaving
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">State")
	ESoUICommand CommandLeave = ESoUICommand::EUC_Action1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">SFX")
	UFMODEvent* MusicToPlay;
};
