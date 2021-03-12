// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "UI/InGame/SoUIGameActivity.h"
#include "UI/General/SoUITypes.h"

#include "SoUIRestPanel.generated.h"

class USoUISpellSelection;
class UFMODEvent;
class USoUIDialogueChoiceBox;
class UImage;
class USoUICharacterPreview;

////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoUIMusicEntry
{
	GENERATED_USTRUCT_BODY()
public:

	// mesh to fade out
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UFMODEvent* Music;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText DisplayName;
};


////////////////////////////////////////////////////////////////////////////////////////
UENUM()
enum class ESoRestPanelState : uint8
{
	ERPS_FadeIn,
	ERPS_Change,
	ERPS_WaitForInput,
	ERPS_SpellSelection,
	ERPS_FadeOut
};

////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoRestButtons : uint8
{
	ERB_Sleep = 0						UMETA(DisplayName = "Sleep"),
	ERB_ChangeSpells					UMETA(DisplayName = "ChangeSpells"),
	ERB_ListenToMusic					UMETA(DisplayName = "ListenToMusic"),
	ERB_WakeUp							UMETA(DisplayName = "WakeUp")
};



UCLASS()
class SORB_API USoUIRestPanel : public USoInGameUIActivity
{
	GENERATED_BODY()

protected:

	USoUIRestPanel(const FObjectInitializer& PCIP);

#if WITH_EDITOR
	virtual void PreSave(const ITargetPlatform* TargetPlatform) override;
#endif

	// Begin UUserWidget Interface
	void NativeConstruct() override;
	// End UUserWidget Interface

	// Begin USoInGameUIActivity

	/** valid, started DlgContext is expected */
	bool SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable) override;

	bool HandleCommand_Implementation(ESoUICommand Command) override;

	bool Update_Implementation(float DeltaSeconds) override;

	bool ShouldHideUIElements_Implementation() const override { return true; }
	//End USoInGameUIActivity

protected:

	void FadeIn();
	void FadeOut();

	bool IsAnyTextBoxAnimated() const;

	void BuildAnimationMap();
	void PlayAnimationSafe(FName AnimName);

	UFUNCTION(BlueprintCallable)
	void OpenSpellSelection();

	UFUNCTION(BlueprintPure)
	ESoRestButtons GetActiveButton() { return static_cast<ESoRestButtons>(ActiveOptionIndex); }

	UFUNCTION(BlueprintImplementableEvent)
	void OnSelectedOptionChange(int32 Old, int32 New);

	UFUNCTION(BlueprintImplementableEvent)
	void OnEnterOption();

	UFUNCTION(BlueprintCallable, Category = UI)
	int32 SwitchMusic();

	UFUNCTION(BlueprintCallable, Category = UI)
	UUserWidget* GetNotUsedWidget(TSubclassOf<UUserWidget> Class);

protected:
	static const FName AnimNameFadeIn;
	static const FName AnimNameFadeOut;

	// Widgets

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISpellSelection* SpellSelection;


	//
	// DialogueChoiceBox
	//

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIDialogueChoiceBox* Sleep;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIDialogueChoiceBox* ChangeSpells;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIDialogueChoiceBox* ListenToMusic;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIDialogueChoiceBox* WakeUp;


	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* Ball0;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* Ball1;

	/** the choices */
	UPROPERTY(BlueprintReadWrite)
	TArray<USoUIDialogueChoiceBox*> PlayerChoiceTextBoxArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Choices")
	TArray<FText> OptionTexts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Choices")
	TArray<FVector2D> OptionPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Choices")
	TArray<UTexture2D*> BallImages;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Music")
	TArray<FSoUIMusicEntry> MusicMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Music")
	int32 PlayedMusicIndex = 0;


	// runtime stuff
	bool bOpened = false;

	UPROPERTY(BlueprintReadOnly)
	TMap<FName, UWidgetAnimation*> AnimationMap;

	ESoRestPanelState SoPanelState;

	UPROPERTY(BlueprintReadOnly)
	int32 ActiveOptionIndex = 0;

	UPROPERTY()
	TMap<TSubclassOf<UUserWidget>, FSoUIWidgetArray> WidgetStorage;


	UPROPERTY(EditAnywhere, Category = ">Choices")
	float BlendSpeedMultiplier = 2.0f;

	bool bFirstActive = true;
	float BlendValue = 0.0f;

	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXOptionChange;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXOptionSelected;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXClosePanel;
};
