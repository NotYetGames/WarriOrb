// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

// interface
#include "CoreMinimal.h"

#include "DlgDialogueParticipant.h"

#include "CharacterBase/SoVoiceUser.h"
#include "Basic/SoDialogueParticipant.h"
#include "Basic/SoEventHandler.h"
#include "Interactables/SoInteractableActor.h"

#include "Items/SoItem.h"

#include "SoNPC.generated.h"

class USceneComponent;

/** Can be talked to */
UCLASS()
class SORB_API ASoNPC : public ASoInteractableActor, public IDlgDialogueParticipant, public ISoDialogueParticipant, public ISoEventHandler, public ISoVoiceUser
{
	GENERATED_BODY()

public:

	ASoNPC(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Interfaces:

	// SoEventHandler interface
	virtual void HandleSoPostLoad_Implementation() override;
	virtual void HandlePlayerRematerialize_Implementation() override {};
	virtual void HandlePlayerRespawn_Implementation() override {};
	virtual void HandleWhitelistedSplinesEntered_Implementation() override {};
	virtual void HandleWhitelistedSplinesLeft_Implementation() override {};

	// interactable:
	virtual bool CanBeInteractedWithFromAir_Implementation() const override { return false; }
	virtual void Interact_Implementation(ASoCharacter* Character) override;

	// SoDialogueParticipant interface
	virtual const FSoDialogueParticipantData& GetParticipantData() const override
	{
		return DialogueData;
	}

	// SoVoiceUser interface
	virtual USceneComponent* GetComponent_Implementation() const { return Collision; }
	virtual FName GetSocketName_Implementation() const { return NAME_None; }
	virtual bool ShouldSpawnVO_Implementation(ESoVoiceType VoiceType) const;

	const TArray<FSoItem>& GetStartingItemList() const { return StartingItemList; }


	// DlgDialogueParticipant interface
	FName GetParticipantName_Implementation() const override { return DialogueData.ParticipantName; }
	FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const override { return DialogueData.ParticipantDisplayName; }
	ETextGender GetParticipantGender_Implementation() const { return ETextGender::Neuter; }
	UTexture2D* GetParticipantIcon_Implementation(FName ActiveSpeaker, FName ActiveSpeakerState) const override { return DialogueData.ParticipantIcon; }

	bool ModifyBoolValue_Implementation(FName ValueName, bool bValue) override;
	bool ModifyIntValue_Implementation(FName ValueName, bool bDelta, int32 Value) override { return false; }
	bool ModifyFloatValue_Implementation(FName ValueName, bool bDelta, float Value) override { return false; }
	bool ModifyNameValue_Implementation(FName ValueName, FName NameValue) override { return false; }

	bool GetBoolValue_Implementation(FName ValueName) const override;
	bool OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName) override { return false; }
	bool CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const override { return false; }

	// ones with some logic:
	int32 GetIntValue_Implementation(FName ValueName) const override { return 0; }
	FName GetNameValue_Implementation(FName ValueName) const override { return NAME_None; }
	float GetFloatValue_Implementation(FName ValueName) const override { return -1.0f; }

protected:
	UFUNCTION(BlueprintPure, Category = "State")
	bool IsAlreadyTriggered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Reload")
	void OnGameReloadBP();

protected:

	// Dialogue:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogues")
	bool bTriggerOnlyOnce = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogues")
	bool bHandlePostLoad;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogues")
	bool bSaveIfNameKnown;

	/** Used only if bSaveIfNameKnown is set */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogues")
	FText DlgParticipantDisplayName;

	/** Used only if bSaveIfNameKnown is set */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogues")
	FText DlgDisplayNameIfUnkown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogues")
	FSoDialogueParticipantData DialogueData;

	/** used for traders to remember the initial state, isn't in component cause instanced stuff resets if it is in component cause why not */
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FSoItem> StartingItemList;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float VOChance = 0.5f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float MaxVODistance = 3000;
};
