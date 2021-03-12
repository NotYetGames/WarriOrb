// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"

#include "Basic/SoEventHandler.h"
#include "SoTriggerable.h"
#include "SoTriggerChain.generated.h"

class UFMODEvent;

UENUM(BlueprintType)
enum class ESoTriggerChainBehavior : uint8
{
	ETCB_Randomized			UMETA(DisplayName = "Randomized"),
	ETCB_Pattern			UMETA(DisplayName = "Pattern"),
	ETCB_PatternNotHinted	UMETA(DisplayName = "PatternNotHinted"),
	ETCB_RandomizedPattern	UMETA(DisplayName = "RandomizedPattern"),
};


UCLASS()
class SORB_API ASoTriggerChain : public AActor, public ISoTriggerable, public ISoEventHandler
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoTriggerChain();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Trigger_Implementation(const FSoTriggerData& TriggerData) override;

	virtual void HandlePlayerRematerialize_Implementation() override;
	virtual void HandleSoPostLoad_Implementation() override;
	virtual void HandlePlayerRespawn_Implementation() override {};
	virtual void HandleWhitelistedSplinesEntered_Implementation() override {};
	virtual void HandleWhitelistedSplinesLeft_Implementation() override {};

protected:

	void UpdateExpectedInput();

	void ResetEverything();
	void InputFailed();

protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	TArray<AActor*> Targets;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	bool bResetOnDone = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	bool bResetOnPlayerRematerialize = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	bool bSerializeState = false;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	TArray<FSoTriggerableData> TriggerOnDone;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	UFMODEvent* SFXOnDone;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	TArray<UFMODEvent*> SFXOnCorrectInput;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain, Meta = (ClampMin = 0))
	int32 StepNum = 4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	ESoTriggerChainBehavior Behavior;

	/** Used if the array is not filled or with ETCB_Randomized */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	float DefaultWaitTime;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, EditFixedSize, Category = TriggerChain)
	TArray<int32> IndexTable;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, EditFixedSize, Category = TriggerChain)
	TArray<float> TimeTable;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	bool bRegisterAsMilestone;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TriggerChain)
	FName BoolToSetTrueOnCharacter;

	bool bSequenceStarted = false;
	bool bIgnoreTriggers = false;
	int32 StepCounter = 0;

	int32 ExpectedInput;

	FTimerHandle ResetTimer;
	FTimerHandle InputWaitTimer;
};
