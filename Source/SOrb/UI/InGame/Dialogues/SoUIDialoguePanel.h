// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DlgDialogueParticipant.h"
#include "UI/InGame/SoUIGameActivity.h"
#include "Basic/SoDialogueParticipant.h"

#include "SoUIDialoguePanel.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSoDlgUI, Log, All);


class UFMODEvent;
class UDlgContext;
class USoUIDialogueTextBox;
class UImage;
class USoUIDialogueChoiceBox;
class USoUICharacterPreview;

namespace FMOD
{
namespace Studio
{
	class EventInstance;
}
}

UENUM()
enum class ESoDialoguePanelState : uint8
{
	EDPS_FadeIn,
	EDPS_Change,
	EDPS_WaitForInput,
	EDPS_WaitForDuration,
	EDPS_FadeOut,
	EDPS_WaitForFadeOut,
};

/**  UI and control class for ingame dialogues */
UCLASS()
class SORB_API USoUIDialoguePanel : public USoInGameUIActivity, public IDlgDialogueParticipant, public ISoDialogueParticipant
{
	GENERATED_BODY()

protected:
	USoUIDialoguePanel(const FObjectInitializer& PCIP);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	// Begin UUserWidget Interface
	void NativeConstruct() override;
	void NativeDestruct() override;
	// End UUserWidget Interface

	// Begin USoInGameUIActivity

	/** valid, started DlgContext is expected */
	bool SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable) override;

	bool HandleCommand_Implementation(ESoUICommand Command) override;

	bool Update_Implementation(float DeltaSeconds) override;

	bool ShouldHideUIElements_Implementation() const override { return true; }

	//End USoInGameUIActivity


	// Begin IDlgDialogueParticipant
	FName GetParticipantName_Implementation() const override { return DlgName; }
	FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const override { return FText{}; }
	ETextGender GetParticipantGender_Implementation() const { return ETextGender::Neuter; }
	UTexture2D* GetParticipantIcon_Implementation(FName ActiveSpeaker, FName ActiveSpeakerState) const override { return nullptr; }

	bool ModifyNameValue_Implementation(FName ValueName, FName NameValue) override;
	bool ModifyFloatValue_Implementation(FName ValueName, bool bDelta, float Value) override;
	bool OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName) override;

	bool ModifyIntValue_Implementation(FName ValueName, bool bDelta, int32 Value) override { return false; }
	bool ModifyBoolValue_Implementation(FName ValueName, bool bValue) override { return false; }
	bool CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const override { return false; }
	float GetFloatValue_Implementation(FName ValueName) const override { return 0.0f; }
	int32 GetIntValue_Implementation(FName ValueName) const override { return -1; }
	bool GetBoolValue_Implementation(FName ValueName) const override { return false; }
	FName GetNameValue_Implementation(FName ValueName) const override { return NAME_None; }
	// End IDlgDialogueParticipant

protected:
	UFUNCTION()
	void OnDialogueTextboxOpenned();

	void FadeIn();
	void FadeOut();

	void UpdateUI();

	void DisplayPlayerOptions();
	void HidePlayerOptions();

	void HideNotOptionTextBoxes();

	bool ChangeActivePlayerChoice(ESoUICommand Command);


	bool IsAnyTextBoxAnimated() const;

	void BuildAnimationMap();
	void PlayAnimationSafe(FName AnimName);

	void UpdateNameFields();

protected:
	static const FName AnimNameFadeIn;
	static const FName AnimNameFadeOut;

	// Widgets

	// Default NPC Dialogue Box
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueTextBox* TextBox0;

	// Player Dialogue box
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueTextBox* TextBox1;

	// Wizard Dialogue box
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueTextBox* TextBox2;

	// Additional NPC Dialogue Box
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueTextBox* TextBox3;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Floor0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Floor1;


	// Default NPC Dialogue Image
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* DlgImage0;

	// Wizard Dialogue Image
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* DlgImage2;

	// Additional NPC Dialogue Image
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* DlgImage3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUICharacterPreview* CharPreviewLeft;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueChoiceBox* PlayerChoice0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueChoiceBox* PlayerChoice1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueChoiceBox* PlayerChoice2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueChoiceBox* PlayerChoice3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueChoiceBox* PlayerChoice4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueChoiceBox* PlayerChoice5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueChoiceBox* PlayerChoice6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USoUIDialogueChoiceBox* PlayerChoice7;

	/** the 4 normal textbox */
	UPROPERTY()
	TArray<USoUIDialogueTextBox*> DialogueTextBoxArray;

	/** the player choices */
	UPROPERTY()
	TArray<USoUIDialogueChoiceBox*> PlayerChoiceTextBoxArray;


	UPROPERTY(EditAnywhere)
	float YDifferenceBetweenChoices = 80.0f;


	UPROPERTY(BlueprintReadOnly)
	TMap<FName, UImage*> NameBackgroundImageMap;


	// runtime stuff

	UPROPERTY(BlueprintReadOnly)
	UDlgContext* ActiveDlgContext = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FName PlayerEmotion;

	bool bOpened = false;

	bool bKeepMusicAfterDialogue;

	UPROPERTY(BlueprintReadOnly)
	TMap<FName, UWidgetAnimation*> AnimationMap;

	ESoDialoguePanelState SoPanelState;

	float LastChangeStartTime;

	/** Stored for participants if they are faded in or out */
	TMap<FName, bool> ParticipantStateMap;

	/** if a textbox has to be fade out before the change */
	bool bWaitForTextBoxFadeOut = true;

	bool bPlayerChoicesDisplayed = false;

	bool bWaitForSpace = false;

	bool bDisplayPlayerChoiceAfterFade = false;

	int32 ActivePlayerOption = 0;

	const FName DlgName = FName("UIPanel");
	const FName ActivePercentName = FName("ActivePercent");

	int32 ActiveParticipantImageIndex = -1;

	TArray<float> InactiveValues;

	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXDialogueStart = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOptionSwitch = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOptionSelected = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXParticipantAppears = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXParticipantDisappears = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOpen = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXClose = nullptr;

	FMOD::Studio::EventInstance* Instance = nullptr;


	float FadeOutDelay = -1.0f;

	float WaitCounter = -1.0f;
};
