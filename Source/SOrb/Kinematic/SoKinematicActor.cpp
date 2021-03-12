// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoKinematicActor.h"

#include "MeshMaterialShaderType.h"
#include "Engine/World.h"

#include "SoKProcess.h"
#include "SoKHelper.h"
#include "Basic/SoGameMode.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Character/SoCharacter.h"
#include "SplineLogic/SoMarker.h"
#include "Basic/SoEventHandler.h"
#include "Basic/Helpers/SoPlatformHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoKinematicActor::ASoKinematicActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	InitStateOfProcesses.Empty();
	for (int32 i = 0; i < Processes.Num(); ++i)
		InitStateOfProcesses.Add(Processes[i].CopyParamsButNoTasks());

	InitialTransform = GetActorTransform();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// init if not yet done
	if (InitStateOfProcesses.Num() != Processes.Num())
	{
		InitStateOfProcesses.Empty();
		for (int32 i = 0; i < Processes.Num(); ++i)
			InitStateOfProcesses.Add(Processes[i].CopyParamsButNoTasks());

		InitialTransform = GetActorTransform();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::BeginPlay()
{
	Super::BeginPlay();

	if (bResetInBeginPlay)
		ResetKinematicActor();
	else
		for (int32 i = 0; i < Processes.Num(); ++i)
			Processes[i].Initialize(TargetComponents, this, CrushDamageOnPushAndStuck);

	if (bResetOnReload)
		ASoGameMode::Get(this).OnPostLoad.AddDynamic(this, &ASoKinematicActor::ResetKinematicActor);

	if (bResetOnRematerialize)
		if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
			SoCharacter->OnPlayerRematerialized.AddDynamic(this, &ASoKinematicActor::ResetKinematicActor);

	if (bCanDisableTickWhileNoProcessIsActive)
		SetActorTickEnabled(true);

	PostBeginPlayBP();

	if (ShouldHandleWhitelistedSplines())
	{
		bool bInWhiteListed = false;
		USoEventHandlerHelper::SubscribeWhitelistedSplines(this, WhitelistedSplines);
		if (ASoSpline* PlayerSpline = USoStaticHelper::GetPlayerSplineLocation(this).GetSpline())
			for (TAssetPtr<ASoSpline>& AssetPtr : WhitelistedSplines)
				if (AssetPtr.Get() == PlayerSpline)
				{
					bInWhiteListed = true;
					break;
				}

		SetActorTickEnabled(bInWhiteListed);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bResetOnReload)
		ASoGameMode::Get(this).OnPostLoad.RemoveDynamic(this, &ASoKinematicActor::ResetKinematicActor);

	if (bResetOnRematerialize)
		if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
			SoCharacter->OnPlayerRematerialized.RemoveDynamic(this, &ASoKinematicActor::ResetKinematicActor);

	if (ShouldHandleWhitelistedSplines())
		USoEventHandlerHelper::UnsubscribeWhitelistedSplines(this);

	Super::EndPlay(EndPlayReason);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called every frame
void ASoKinematicActor::Tick(float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SoKinematicActor - Tick"), STAT_SoKinematicActorTick, STATGROUP_SoKinematic);

	Super::Tick(DeltaTime);

	if (IsPendingKill() || TargetComponents.Num() == 0 || TargetComponents[0] == nullptr || !TargetComponents[0]->IsRegistered())
		return;

	bool bWasAnyActive = false;

	// update velocity of target component 0
	const FVector OldLocation = TargetComponents[0]->GetComponentLocation();

	for (int32 i = 0; i < Processes.Num(); ++i)
	{
		if (Processes[i].IsActive())
		{
			bWasAnyActive = true;
			const float RestTime = Processes[i].Tick(TargetComponents, DeltaTime * ProcessTimeMultiplier);
			if (RestTime > -KINDA_SMALL_NUMBER)
				OnProcessFinished.Broadcast(i, RestTime);
		}
	}

	const FVector NewLocation = TargetComponents[0]->GetComponentLocation();
	// someone might needs this (e.g. the ball if it bounces on it)
	TargetComponents[0]->ComponentVelocity = (NewLocation - OldLocation) / DeltaTime;

	if (!bWasAnyActive && bCanDisableTickWhileNoProcessIsActive)
		SetActorTickEnabled(false);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint ASoKinematicActor::GetSplineLocationI_Implementation() const
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SoKinematicActor - GetSplineLocationI_Implementation"), STAT_SoKinematicActorGetSplineLocationI_Implementation, STATGROUP_SoKinematic);

	if (RematerializePoint != nullptr)
	{
		// need the interface call, because some rematerialize point implements that function differently, they don't override the base function of the marker
		if (RematerializePoint->GetClass()->ImplementsInterface(USoSplineWalker::StaticClass()))
			return ISoSplineWalker::Execute_GetSplineLocationI(RematerializePoint);

		return RematerializePoint->GetSplineLocation();
	}

	return FSoSplinePoint{};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::HandleWhitelistedSplinesEntered_Implementation()
{
	SetActorTickEnabled(true);
	USokHelper::StartStopLoopedSounds(Processes, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::HandleWhitelistedSplinesLeft_Implementation()
{
	SetActorTickEnabled(false);
	USokHelper::StartStopLoopedSounds(Processes, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::StartProcess(int32 ProcessID, bool bReverse)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SoKinematicActor - StartProcess"), STAT_SoKinematicActorStartProcess, STATGROUP_SoKinematic);

	if (bCanDisableTickWhileNoProcessIsActive)
		SetActorTickEnabled(true);

#if WITH_EDITOR
	if (ProcessID >= Processes.Num())
	{
		UE_LOG(LogSoKinematicSystem, Error, TEXT("%s: Startprocess called with invalid ProcessID(%d / %d)"), *GetActorLabel(), ProcessID, Processes.Num());
		return;
	}
#endif
 	Processes[ProcessID].Start(bReverse, TargetComponents);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::ReverseProcessDirection(int32 ProcessID)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SoKinematicActor - ReverseProcessDirection"), STAT_ReverseProcessDirection, STATGROUP_SoKinematic);

	if (bCanDisableTickWhileNoProcessIsActive)
		SetActorTickEnabled(true);

#if WITH_EDITOR
	if (ProcessID >= Processes.Num())
	{
		UE_LOG(LogSoKinematicSystem, Error, TEXT("%s: ReverseProcessDirection called with invalid ProcessID(%d / %d)"), *GetActorLabel(), ProcessID, Processes.Num());
		return;
	}
#endif
	Processes[ProcessID].ReverseDirection();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::SetProcessDirection(int32 ProcessID, bool bForward)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SoKinematicActor - SetProcessDirection"), STAT_SetProcessDirection, STATGROUP_SoKinematic);

	if (bCanDisableTickWhileNoProcessIsActive)
		SetActorTickEnabled(true);

#if WITH_EDITOR
	if (ProcessID >= Processes.Num())
	{
		UE_LOG(LogSoKinematicSystem, Error, TEXT("%s: ReverseProcessDirection called with invalid ProcessID(%d / %d)"), *GetActorLabel(), ProcessID, Processes.Num());
		return;
	}
#endif
	Processes[ProcessID].SetDirection(bForward);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::StartProcessAndTick(int32 ProcessID, bool bReverse, float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SoKinematicActor - StartProcessAndTick"), STAT_StartProcessAndTick, STATGROUP_SoKinematic);

	StartProcess(ProcessID, bReverse);
	if (Processes.IsValidIndex(ProcessID))
		Processes[ProcessID].Tick(TargetComponents, DeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::ForceFinishProcess(int32 ProcessID)
{
	if (Processes.IsValidIndex(ProcessID))
		Processes[ProcessID].ForceFinish(TargetComponents);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::TerminateProcess(int32 ProcessID)
{
	if (Processes.IsValidIndex(ProcessID))
		Processes[ProcessID].Terminate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoKinematicActor::IsProcessActive(int32 Index) const
{
	return Index >= 0 && Index < Processes.Num() && Processes[Index].IsActive();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoKinematicActor::HasAnyActiveProcess() const
{
	for (const FSoKProcess& Process : Processes)
		if (Process.IsActive())
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoKinematicActor::ResetKinematicActor()
{
	SetActorTransform(InitialTransform);

	ensure(Processes.Num() == InitStateOfProcesses.Num());
	for (int32 i = 0; i < Processes.Num(); ++i)
		Processes[i].CopyParamsButNoTasks(InitStateOfProcesses[i]);

	for (int32 i = 0; i < Processes.Num(); ++i)
		Processes[i].Initialize(TargetComponents, this, CrushDamageOnPushAndStuck);

	if (bCanDisableTickWhileNoProcessIsActive)
		SetActorTickEnabled(true);

	OnResetKinematicActorBP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoKinematicActor::ShouldHandleWhitelistedSplines() const
{
	if (USoPlatformHelper::HasWeakHardware())
		return WhitelistedSplines.Num() > 0;

	return WhitelistedSplines.Num() > 0 && (!bUseWhitelistedSplinesOnlyOnSwitch);
}