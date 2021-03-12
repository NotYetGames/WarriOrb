// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "UI/General/SoUITypes.h"

#include "SoUIChallenges.generated.h"


class USoUIButtonArray;
class USoUIButton;
class USoUICommandTooltipArray;
class USoUIConfirmPanel;
class UImage;
class UTextBlock;
class UFMODEvent;

/**
 * Shows the save slots of the game
 */
UCLASS()
class SORB_API USoUIChallenges : public UUserWidget, public ISoUIEventHandler
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// ISoUIEventHandler Interface
	//
	bool OnUICommand_Implementation(ESoUICommand Command) override;
	void Open_Implementation(bool bOpen) override;
	bool IsOpened_Implementation() const override { return bOpened; }
	bool CanBeInterrupted_Implementation() const override { return true; }

	//
	// Own methods
	//

	// No animation
	void InstantOpen(bool bOpen);

	UFUNCTION(BlueprintNativeEvent)
	void OnEpisodeChanged(int32 EpisodeButtonIndex);

	// Refreshes the command tooltips depending on the currently selected index.
	UFUNCTION(BlueprintCallable)
	void RefreshCommandTooltips();

	// Start the new challenge
	void StartEpisode(FName EpisodeName);

protected:
	void CreateDefaultChallenges();
	void CreateChallengesFromWorldStateChallenges();

	FName GetSelectedEpisodeName() const;

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonArray* ChallengeButtons = nullptr;

	// Show the actions (shortcuts) possible on each challenge
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltipArray* CommandTooltips = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* ChallengeImage;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* DescriptionValue;

	// UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	// UPanelWidget* CheckpointContainer;

	// UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	// UTextBlock* CheckpointValue;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* StatusValue;

	// Number of challenge buttons
	UPROPERTY(BlueprintReadOnly, Category = ">Challenge")
	int32 NumEpisodes = 3;

	// Maps from
	// Key: The episode button index
	// Value: Episode Name
	TArray<FName> MapEpisodeButtonToEpisodeName;

	// Are the save slots still opened?
	bool bOpened = false;

	// Actions for the save  slots
	static constexpr ESoUICommand UICommand_StartEpisode = ESoUICommand::EUC_Action0;

	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXOpenSelected = nullptr;
};
