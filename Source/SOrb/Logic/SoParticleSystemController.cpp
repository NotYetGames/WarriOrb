// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoParticleSystemController.h"

#include "Particles/Emitter.h"
#include "Particles/ParticleSystemComponent.h"

#include "SplineLogic/SoSpline.h"
#include "Character/SoCharacter.h"
#include "Basic/Helpers/SoStaticHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoParticleSystemController::ASoParticleSystemController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoParticleSystemController::BeginPlay()
{
	Super::BeginPlay();
	bJustBeganPlay = true;

	// build spline list from spline ptr-s
	for (TAssetPtr<ASoSpline>& SplinePtr : WhiteListedSplinePtrList)
	{
		if (ASoSpline* Spline = SplinePtr.Get())
			WhiteListedSplineList.Add(Spline);
	}

	if (WhiteListedSplineList.Num() > 0)
	{
		ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (Character != nullptr)
		{
			Character->OnPlayerSplineChanged.AddDynamic(this, &ASoParticleSystemController::OnPlayerSplineChanged);
			OnPlayerSplineChanged(nullptr, ISoSplineWalker::Execute_GetSplineLocationI(Character).GetSpline());
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoParticleSystemController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WhiteListedSplineList.Num() > 0)
	{
		ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (Character != nullptr)
			Character->OnPlayerSplineChanged.RemoveDynamic(this, &ASoParticleSystemController::OnPlayerSplineChanged);
		WhiteListedSplineList.Empty();
	}

	Super::EndPlay(EndPlayReason);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoParticleSystemController::OnPlayerSplineChanged(const ASoSpline* OldSpline, const ASoSpline* NewSpline)
{
	const bool bPlayerInWhitelisted = WhiteListedSplineList.Contains(NewSpline);
	if (bParticlesActive != bPlayerInWhitelisted || bJustBeganPlay)
	{
		bParticlesActive = bPlayerInWhitelisted;

		for (AEmitter* Emitter : EmitterList)
			if (Emitter != nullptr)
			{
				if (bParticlesActive)
					Emitter->GetParticleSystemComponent()->Activate(false);
				else
					Emitter->GetParticleSystemComponent()->Deactivate();
			}

		bJustBeganPlay = false;
	}
}
