// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "SoInteractable.h"
#include "SplineLogic/SoSplinePointPtr.h"
#include "SoInteractableLeverSwitch.generated.h"

class UAnimSequenceBase;
class USoInteractableComponent;

UCLASS()
class SORB_API ASoInteractableLeverSwitch : public AActor, public ISoInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoInteractableLeverSwitch();

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
	UFUNCTION(BlueprintCallable, Category = LeverState)
	void OnReload();

	UFUNCTION(BlueprintImplementableEvent, Category = LeverState)
	void OnStateChangeBP();

	UFUNCTION(BlueprintImplementableEvent, Category = LeverState)
	void OnCharacterInteractBP();

	UFUNCTION(BlueprintImplementableEvent, Category = LeverState)
	void OnInteractionEndBP();

protected:

	// lever is placed on the spline - the character interacting with it will teleport to this spline location
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineLocation")
	FSoSplinePointPtr SplineLocationPtr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoInteractableComponent* SoInteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayTextInSetState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayTextInResetState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoInteractableMessage Message;

	// value between 0.0f (unset) to 1.0f (set)
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float PulledStatePercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSetByDefault = false;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool bSet = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool bSerializeState = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PullSpeed = 1.0f;

	// -1 or 1 depending on the spline and placement
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 CharFaceDir = 1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Movement)
	UAnimSequenceBase* CharacterAnimation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Trigger)
	TArray<AActor*> TriggerOnSet;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Trigger)
	TArray<AActor*> TriggerOnReset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Trigger)
	int32 SetValue = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Trigger)
	int32 ResetValue = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Trigger)
	float WaitAfterLeverPull = -1.0f;

	UPROPERTY()
	ASoCharacter* SoCharacter = nullptr;
};
