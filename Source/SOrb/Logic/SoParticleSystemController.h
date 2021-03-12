// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoParticleSystemController.generated.h"

class AEmitter;
class ASoSpline;

UCLASS()
class SORB_API ASoParticleSystemController : public AActor
{
	GENERATED_BODY()

public:

	ASoParticleSystemController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	UFUNCTION()
	void OnPlayerSplineChanged(const ASoSpline* OldSpline, const ASoSpline* NewSpline);


protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ControllerData)
	TArray<AEmitter*> EmitterList;

	/**
	 *  Particles are only enabled if the palyer is on one of these splines
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ControllerData)
	TArray<TAssetPtr<ASoSpline>> WhiteListedSplinePtrList;


	UPROPERTY(BlueprintReadOnly, Category = ControllerData)
	TArray<ASoSpline*> WhiteListedSplineList;

	bool bParticlesActive = true;
	bool bJustBeganPlay = true;
};
