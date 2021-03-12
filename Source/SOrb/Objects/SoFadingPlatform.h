// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoFadingPlatform.generated.h"

class UFMODEvent;
class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

enum ESoFadingPlatformState
{
	EFPS_FadedIn,
	EFPS_WaitBeforeFadeIn,
	EFPS_FadeIn,

	EFPS_FadedOut,
	EFPS_WaitBeforeFadeOut,
	EFPS_FadeOut
};

UCLASS()
class SORB_API ASoFadingPlatform : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoFadingPlatform();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void Reset();

	UFUNCTION(BlueprintCallable)
	void OnFadeInSignal(UPrimitiveComponent* OverlappedComponent,
						AActor* OtherActor,
						UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex,
						bool bFromSweep,
						const FHitResult & SweepResult);

	UFUNCTION(BlueprintCallable)
	void OnFadeOutSignal(UPrimitiveComponent* OverlappedComponent,
						AActor* OtherActor,
						UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex,
						bool bFromSweep,
						const FHitResult & SweepResult);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Update(float DeltaSeconds);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* Root;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBoxComponent* FadeInArea;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBoxComponent* FadeOutArea;

	UPROPERTY(EditAnywhere)
	bool bFadedInByDefault;

	UPROPERTY(EditAnywhere)
	float FadeInDelay = 1.0f;

	UPROPERTY(EditAnywhere)
	float FadeOutDelay = 1.0f;

	UPROPERTY(EditAnywhere)
	float FadeInTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float FadeOutTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float CollisionOffPercent = 0.5f;

	float Counter = 0.0f;

	ESoFadingPlatformState State;
	bool bFadeInRequestWasLast;

	static const FName OPAddName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	UFMODEvent* SFXFadeIn = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	UFMODEvent* SFXFadeOut = nullptr;
};
