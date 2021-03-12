// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Engine/EngineTypes.h"
#include "DlgDialogueParticipant.h"

#include "Basic/SoDialogueParticipant.h"

#include "SoWizard.generated.h"


class USoInGameUIActivity;
class ASoCharacter;
class UDlgDialogue;

/**
 *  Wizard dialogue participant, it is created and managed by the character class
 */
UCLASS(BlueprintType)
class SORB_API USoWizard : public UObject, public IDlgDialogueParticipant, public ISoDialogueParticipant
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	USoWizard();

	void Initialize(ASoCharacter* PlayerCharacter);

	// <SoDialogueParticipant>
	virtual const FSoDialogueParticipantData& GetParticipantData() const override { return DialogueData; }

	// <DlgDialogueParticipant interface>
	FName GetParticipantName_Implementation() const override { return DialogueData.ParticipantName; }
	FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const override { return DialogueData.ParticipantDisplayName; }
	ETextGender GetParticipantGender_Implementation() const { return ETextGender::Neuter; }
	UTexture2D* GetParticipantIcon_Implementation(FName ActiveSpeaker, FName ActiveSpeakerState) const override { return DialogueData.ParticipantIcon; }

	// should be implemented in blueprint:
	bool ModifyIntValue_Implementation(FName ValueName, bool bDelta, int32 Value) override { return false; }
	bool ModifyFloatValue_Implementation(FName ValueName, bool bDelta, float Value) override { return false; }
	bool ModifyNameValue_Implementation(FName ValueName, FName NameValue) override { return false; }
	bool ModifyBoolValue_Implementation(FName ValueName, bool bValue) override { return false; }
	bool OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName) override { return false; }
	bool CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const override { return false; }
	float GetFloatValue_Implementation(FName ValueName) const override { return 0.0f; }
	int32 GetIntValue_Implementation(FName ValueName) const override { return 0; }
	bool GetBoolValue_Implementation(FName ValueName) const override { return false; }
	FName GetNameValue_Implementation(FName ValueName) const override { return NAME_None; }
	// </DlgDialogueParticipant interface>


	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool StartDialogueWithPlayer(UDlgDialogue* Dialogue);

	void ResumeDialogueWithPlayer();

	const TSubclassOf<USoInGameUIActivity>& GetWizardDialogueUIActivityClass() const { return WizardDialogueUIActivityClass; }

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogues")
	FSoDialogueParticipantData DialogueData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogues")
	TSubclassOf<USoInGameUIActivity> WizardDialogueUIActivityClass;

	UPROPERTY(BlueprintReadWrite)
	ASoCharacter* Character;
};
