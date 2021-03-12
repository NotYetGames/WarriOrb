// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SoUISettingsBase.h"
#include "Localization/SoLocalizationHelper.h"

#include "SoUISettingsGame.generated.h"

class USoUIButton;
class UTextBlock;
class USoUIScrollingButtonArray;

UENUM(BlueprintType)
enum class ESoUIGameSettingsLine : uint8
{
	None = 0,

	Language,
	CharacterSkin,
	GameSpeed,
	MercifulBounceMode,
	ShowFPS,
	ShowTime,
	ShowDamageTexts,
	ShowEnemyHealthBar,
	ShowFloatingVOTexts,
	EnableAnalytics, // TODO
	PauseGameWhenUnfocused,
	PauseGameGameOnIdle,

	NumOf UMETA(Hidden)
};

/**
 *
 */
UCLASS()
class SORB_API USoUISettingsGame : public USoUISettingsBase
{
	GENERATED_BODY()

protected:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// ISoUIEventHandler Interface
	//
	void Open_Implementation(bool bOpen) override;

	//
	// USoUISettingsBase methods
	//
	void CreateChildrenAndSetTexts(bool bFromSynchronizeProperties = false) override;
	void OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget) override;
	bool IsValidLineIndex(int32 LineIndex) const override
	{
		return Super::IsValidLineIndex(LineIndex) && LineOptions.IsValidIndex(LineIndex);
	}
	void OnPostSetSelectedIndex(int32 OldIndex, int32 NewIndex) override;
	void RefreshButtonImagesTooltips(bool bForceShowAll = false) override {}

	void UpdateWidgetsVisibilities() override;

	//
	// Own methods
	//

	void RefreshLineOptions();

	// Reset settings to the system settings and updates selected options to match the user settings one.
	void ResetSelectedOptionsToUserSettings();

	UFUNCTION()
	void OnLanguageChildChanged(int32 PreviousChild, int32 NewChild);

	UFUNCTION()
	void OnCharacterSkinChanged(int32 PreviousChild, int32 NewChild);

	UFUNCTION()
	void OnGameSpeedChildChanged(int32 PreviousChild, int32 NewChild);

	UFUNCTION()
	void OnMercifulBounceModeChildChanged(int32 PreviousChild, int32 NewChild);

	//
	// GameSpeed methods
	//

	// Normalizes the GameSpeed
	static int32 ToNormalizedGameSpeed(float Value)
	{
		return FMath::RoundToInt(Value * 100.f);
	}
	// Converts back from a normalized speed
	static float FromNormalizedGameSpeed(int32 Normalized)
	{
		return static_cast<float>(Normalized) / 100.f;
	}

	// Returns a valid index inside GameSpeedOptions
	int32 GetIndexOptionForGameSpeed(float Value) const;

protected:
	// Lines:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* Language = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* CharacterSkin = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* GameSpeed = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* MercifulBounceMode = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* DisplayFPS = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* DisplayTime = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* DisplayDamageTexts = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* DisplayEnemyHealth = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* DisplayFloatingVOTexts = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* PauseGameWhenUnfocused = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* PauseGameWhenIdle = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* CurrentTooltipText = nullptr;

	// Set in UMG
	UPROPERTY(EditAnywhere, Category = ">Lines")
	TMap<ESoUIGameSettingsLine, FText> LinesTooltips;

	// Map Selected Line Index => ESoUISettingsControllerLine (line type)
	// NOTE for this you also need to change the order in the UMG Blueprint Widget of the USoUIButtonArray.
	TArray<ESoUIGameSettingsLine> LineOptions = {
		ESoUIGameSettingsLine::Language,
		ESoUIGameSettingsLine::CharacterSkin,
		ESoUIGameSettingsLine::GameSpeed,
		ESoUIGameSettingsLine::MercifulBounceMode,
		ESoUIGameSettingsLine::ShowFPS,
		ESoUIGameSettingsLine::ShowTime,
		ESoUIGameSettingsLine::ShowDamageTexts,
		ESoUIGameSettingsLine::ShowEnemyHealthBar,
		ESoUIGameSettingsLine::ShowFloatingVOTexts,
		ESoUIGameSettingsLine::PauseGameWhenUnfocused,
		ESoUIGameSettingsLine::PauseGameGameOnIdle,
	};

	// All the Language options
	TArray<ESoSupportedCulture> LanguageOptions;

	// All the game speed options
	// Normalized to int which is just float * 100
	const TArray<int32> GameSpeedOptions = {
		100, // 0 - 1.00f
		105, // 1 - 1.05f
		110, // 2 - 1.10f
		115, // 3 - 1.15f
		120, // 4 - 1.20f
		125, // 5 - 1.25f
		130, // 6 - 1.30f
		135, // 7 - 1.35f
		140, // 8 - 1.40f
	};
};
