// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoInteractableActor.h"
#include "Logic/SoTriggerable.h"
#include "Basic/SoEventHandler.h"
#include "CharacterBase/SoIMortalTypes.h"
#include "SoCarryable.generated.h"

class UStaticMeshComponent;
class UFMODAudioComponent;
class UParticleSystemComponent;
class UParticleSystem;
class USoKTTranslateOnSpline;
class UFMODEvent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoCarryableNotify);

UENUM(BlueprintType)
enum class ESoCarryableState : uint8
{
	ECS_Idle			UMETA(DisplayName = "Idle"),
	ECS_PickedUp		UMETA(DisplayName = "PickedUp"),
	ECS_DropInProgress	UMETA(DisplayName = "DropInProgress"),
	ECS_WaitForSignal	UMETA(DisplayName = "WaitForSignal"),

	ECS_MAX				UMETA(DisplayName = "Max"),
};

UENUM(BlueprintType)
enum class ESoCarryableBehavior : uint8
{
	ECB_Default								UMETA(DisplayName = "Default"),
	ECB_IgnoreForceField					UMETA(DisplayName = "IgnoreForceField"),
	ECB_ReturnAfterCharacterHit				UMETA(DisplayName = "ReturnAfterCharacterHit"),
	ECB_ReturnAfterTime						UMETA(DisplayName = "ReturnAfterTime"),

	ECB_MAX									UMETA(DisplayName = "Max"),
};



UCLASS()
class SORB_API ASoCarryable : public ASoInteractableActor, public ISoTriggerable, public ISoEventHandler
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoCarryable();

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	virtual FVector GetVelocity() const override { return Velocity; }


	virtual void Interact_Implementation(ASoCharacter* Character) override;
	virtual bool IsExclusive_Implementation() const override { return ActiveState == ESoCarryableState::ECS_PickedUp; }

	virtual void Trigger_Implementation(const FSoTriggerData& TriggerData) override;

	virtual void HandleSoPostLoad_Implementation() override;
	virtual void HandlePlayerRespawn_Implementation() override {};
	virtual void HandlePlayerRematerialize_Implementation() override {};
	virtual void HandleWhitelistedSplinesEntered_Implementation() override {};
	virtual void HandleWhitelistedSplinesLeft_Implementation() override {};

	void Dropped(bool bActivatePlayerBlock);
	void CancelDrop();

	UFUNCTION(BlueprintCallable)
	void ForceReturn()
	{
		Dropped(false);
		OnGoHomeStart();
	}

	void ForceDrop()
	{
		OnPlayerDiedWithBox();
		Dropped(false);
	}

	UFUNCTION(BlueprintPure, Category = ">Time")
	float GetReturnTime() const;

protected:

	void InitTask();


	UFUNCTION()
	void OnBoxOverlapBegin(UPrimitiveComponent* OverlappedComponent,
						   AActor* OtherActor,
						   UPrimitiveComponent* OtherComp,
						   int32 OtherBodyIndex,
						   bool bFromSweep,
						   const FHitResult & SweepResult);

	UFUNCTION()
	void OnHitCallback(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayerDiedWithBox();

	void SwitchToSolid();
	void SwitchToOverlap();

	UFUNCTION(BlueprintCallable, Category = Movement)
	void StopMovement();

	void OnGoHomeStart();

	void OnGoHomeEnd();

	void CarryStarted();


	void OnCharHitProtectionTimeOver();

	void OnCharBlockTimeOver();

	float GetReturnProtectionTime() const;


protected:

	UPROPERTY(BlueprintAssignable, Category = Notify)
	FSoCarryableNotify OnTaken;

	UPROPERTY(BlueprintAssignable, Category = Notify)
	FSoCarryableNotify OnPlaced;

	UPROPERTY(BlueprintAssignable, Category = Notify)
	FSoCarryableNotify OnReturnStart;

	UPROPERTY(BlueprintAssignable, Category = Notify)
	FSoCarryableNotify OnReturned;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBoxComponent* SoBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBoxComponent* SoPlayerBlockArea;


	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UFMODAudioComponent* MagicBoxAnimationSFX;


	/** used for red box on character hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystemComponent* OnHitParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UFMODAudioComponent* OnHitParticleSFX;

	/** used for non-red box for return vfx */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* ReturnVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* SoRematerializePoint;

	ESoCarryableState ActiveState = ESoCarryableState::ECS_Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Behavior)
	ESoCarryableBehavior Behavior;

	/** time the box spends after it is gone before it respawns in starting location */
	UPROPERTY(EditAnywhere, Category = Behavior)
	float ReturnTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Behavior)
	float Acceleration = 1000;

	/** time the box returns to default state if used with ECB_ReturnAfterTime behavior */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Behavior)
	float PlacedTime = 5.0f;

	/** time the box is protected from character hit after placed with ECB_ReturnAfterCharacterHit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Behavior)
	float ReturnProtectionTime = 0.7f;

	UPROPERTY(BlueprintReadWrite)
	bool bInForceField = false;

	UPROPERTY(BlueprintReadWrite)
	FVector2D ForceFieldVelocity = FVector2D(1000.0f, 0.0f);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Instanced)
	USoKTTranslateOnSpline* SoTask;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoDmg Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoHitReactDesc HitReactDesc;

	FTimerHandle GoHomeStartTimer;
	FTimerHandle GoHomeEndTimer;

	FTimerHandle PlayerBlockTimer;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnGoHomeStart;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnGoHomeEnd;

	FVector Velocity = FVector(0.0f, 0.0f, 0.0f);

	FTransform SavedActorTransform;
	bool bTransformInitialized = false;

	float DroppedTime;
	FTimerHandle DroppedAndHitProtectedTimer;


	static const FName StaticMeshSpeedName;
	static const float StaticMeshAnimSpeedValue;
};
