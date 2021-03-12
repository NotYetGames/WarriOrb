// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Components/BoxComponent.h"
#include "Logic/SoTriggerable.h"
#include "Basic/SoEventHandler.h"
#include "SoJellyfishPlatform.generated.h"

class UFMODAudioComponent;
class UParticleSystemComponent;

UENUM(BlueprintType)
enum class ESoJellyfishState : uint8
{
	// invisible, no collision
	EJS_Inactive		 UMETA(DisplayName = "Inactive"),
	// visible, collision on
	EJS_Active			 UMETA(DisplayName = "Active"),
	// turning on to active move
	EJS_FadeIn			 UMETA(DisplayName = "FadeIn"),
	// turning off to inactive move
	EJS_FadeOut			 UMETA(DisplayName = "FadeOut"),
};

UCLASS()
class SORB_API ASoJellyfishPlatform : public AActor, public ISoTriggerable, public ISoEventHandler
{
	GENERATED_BODY()

public:

	ASoJellyfishPlatform();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	void Trigger_Implementation(const FSoTriggerData& TriggerData) override;

	virtual void HandleSoPostLoad_Implementation() override;
	virtual void HandlePlayerRematerialize_Implementation() override;
	virtual void HandlePlayerRespawn_Implementation() override {};
	virtual void HandleWhitelistedSplinesEntered_Implementation() override {};
	virtual void HandleWhitelistedSplinesLeft_Implementation() override {};

	UFUNCTION()
	void OnHitCallback(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void StartFade();

protected:

	void ResetToDefaultState(bool bResetToDefaultState = true);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UBoxComponent* Collision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UParticleSystemComponent* ParticleComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UFMODAudioComponent* HitSFX;


	/** default idle state is active or inactive */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = JellyfishSetup)
	bool bActiveByDefault = false;

	ESoJellyfishState ActiveState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = JellyfishSetup)
	float ActivationTime = 0.2f;

	/** time between hit and fading out */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = JellyfishSetup)
	float DeactivationDelay = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = JellyfishSetup)
	float FadeDuration = 0.6f;

	float FadeCounter = 0.0f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = JellyfishSetup)
	bool bFadeOutOnHit = true;

	/** if > 0 the object fades back again every time is faded out after FadeBackTime */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = JellyfishSetup)
	float FadeBackTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = JellyfishSetup)
	bool bResetOnPlayerRematerialize = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = JellyfishSetup)
	bool bResetOnReload = true;

	static const FName BaseOPName;
	static const FName OPAddName;
	static const float BaseOpMin;
	static const float BaseOpMax;

	FTimerHandle Timer;
};
