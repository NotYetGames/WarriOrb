// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Logic/SoTriggerable.h"
#include "Basic/SoEventHandler.h"

#include "CharacterBase/SoIMortalTypes.h"

#include "SoFadingPlatformTimeBased.generated.h"

class UFMODEvent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESoFadingPlatformDisabledBehavior : uint8
{
	EFPT_Hide,
	EFPT_Kill
};

UCLASS()
class SORB_API ASoFadingPlatformTimeBased : public AActor, public ISoTriggerable, public ISoEventHandler
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoFadingPlatformTimeBased();

	virtual void Tick(float DeltaTime) override;

	void Trigger_Implementation(const FSoTriggerData& TriggerData) override;

	virtual void HandleSoPostLoad_Implementation() override;
	virtual void HandlePlayerRematerialize_Implementation() override {};
	virtual void HandlePlayerRespawn_Implementation() override {};
	virtual void HandleWhitelistedSplinesEntered_Implementation() override {};
	virtual void HandleWhitelistedSplinesLeft_Implementation() override {};


	UFUNCTION()
	void Reset();

	UFUNCTION()
	void OnEnabledTimeOver();

	UFUNCTION()
	void OnDisabledTimeOver();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnUpdate(float Percent);

	UFUNCTION()
	void OnHitCallback(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere)
	ESoFadingPlatformDisabledBehavior ActionOnDisabled;

	UPROPERTY(EditAnywhere)
	bool bFadedInByDefault;

	UPROPERTY(EditAnywhere)
	bool bHideOnTrigger;


	UPROPERTY(EditAnywhere)
	float EnabledTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float DisabledTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float FadeInTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float FadeOutTime = 1.0f;

	/** added to default state at the first time */
	UPROPERTY(EditAnywhere)
	float FirstTimeOffset = 0.0f;


	UPROPERTY(EditAnywhere)
	FName MaterialParamName = "OP_Add";

	UPROPERTY(EditAnywhere)
	float FadeInValue = 1.0f;

	UPROPERTY(EditAnywhere)
	float FadeOutValue = 0.0f;


	UPROPERTY(EditAnywhere)
	float ActAsDisabledPercent = 0.5f;

	UPROPERTY(EditAnywhere)
	FSoDmg Dmg;

	UPROPERTY(EditAnywhere)
	FSoHitReactDesc HitReactDesc;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnHit;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnHit2D;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnHitDeath;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnEnemyHit;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnHide;


	UPROPERTY(EditAnywhere)
	float Counter = 0.0f;

	bool bFadeIn;

	FTimerHandle TimeHandle;
};
