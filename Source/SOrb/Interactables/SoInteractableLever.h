// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "SoInteractable.h"
#include "Logic/SoTriggerable.h"
#include "SplineLogic/SoSplinePointPtr.h"
#include "SoInteractableLever.generated.h"

class UAnimSequenceBase;
class USoInteractableComponent;

UCLASS()
class SORB_API ASoInteractableLever : public AActor, public ISoInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoInteractableLever();

	// update spline location if necessary
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	virtual void Interact_Implementation(ASoCharacter* Character) override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual FSoInteractableMessage GetInteractionMessage_Implementation() const override { return Message; }
	virtual bool IsExclusive_Implementation() const override { return false; }
	virtual bool IsSecondKeyPrefered_Implementation() const override { return false; }
	virtual bool CanBeInteractedWithFromAir_Implementation() const override { return false; }

protected:

	void UpdateState(bool bFromTick);

	void SwitchToReset();

	void SetPulledStatePercent(float NewPercent);

	UFUNCTION(BlueprintCallable)
	void OnReload();

	UFUNCTION(BlueprintImplementableEvent)
	void OnStateUpdated(float StatePercent, bool bFromTick);

	UFUNCTION(BlueprintImplementableEvent)
	void OnInteractBP();

	UFUNCTION(BlueprintImplementableEvent)
	void OnInteractEndBP();

	UFUNCTION(BlueprintImplementableEvent)
	void OnTargetsTriggeredBP();


protected:

	enum ESoLeverActivity
	{
		ELS_Rest,
		ELS_Pulled,
		ELS_Reset
	};

	// lever is placed on the spline - the character interacting with it will teleport to this spline location
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineLocation")
	FSoSplinePointPtr SplineLocationPtr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoInteractableComponent* SoInteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayTextUsed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoInteractableMessage Message;

	// the lever goes from 0 (default state) to 1 (fully pulled)
	// if it reaches the max state (1.0f) it triggers the targets
	// after the first trigger it won't trigger again until the value goes below this threshold and back to 1.0f again
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.01", ClampMax = "1.1"))
	float LeverResetThreshold = 0.5f;

	// after the first trigger the lever won't be interactable anymore
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bTriggerOnlyOnce;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSavePulledStatePercent = false;

	static const FName PercentName;

	// value between 0.0f (default) to 1.0f (pulled)
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float PulledStatePercent = 0.0f;

	// used when the level is loaded if serialized data isn't specified otherwise
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DefaultPulledStatePercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PullSpeed = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ResetSpeed = 1.0f;

	UPROPERTY(EditAnywhere)
	bool bUseImprovedResetMethod;

	float ResetTime = 0.0;
	float ResetStartPercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAutomaticPull = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float WaitAfterLeverPull = -1.0f;

	// -1 or 1 depending on the spline
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 CharFaceDir = 1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Movement)
	UAnimSequenceBase* CharacterAnimation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FSoTriggerableDataArray> TriggerData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<AActor*> QuickTriggerList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 QuickTriggerListValue = 1;

	/** used only if trigger only once, ModifyBoolValue(NameToAddToPlayer, true) is called on player if it isn't None */
	UPROPERTY(EditAnywhere)
	FName NameToAddToPlayer = NAME_None;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* ControllableTarget = nullptr;


	ESoLeverActivity LeverActivity = ESoLeverActivity::ELS_Rest;

	// set to true on trigger if the Threshold is smaller than 0.999f
	// blocks trigger, cleared if percent became smaller than LeverResetThreshold
	bool bTriggerBlock = false;

	UPROPERTY()
	ASoCharacter* SoCharacter = nullptr;
};
