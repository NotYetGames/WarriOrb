// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEnemy.h"
#include "TimerManager.h"
#include "Components/ShapeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "EngineMinimal.h" // ANY_PACKAGE enum search
#include "DrawDebugHelpers.h"

#include "DlgManager.h"

#include "Settings/SoGameSettings.h"
#include "SoEnemyVoiceManager.h"
#include "EActions/SoEAGeneral.h"
#include "EActions/SoEAStunned.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "CharacterBase/SoCombatComponent.h"
#include "CharacterBase/SoCharacterSheet.h"
#include "Character/SoCharacter.h"
#include "Basic/SoGameMode.h"
#include "SplineLogic/SoSplineHelper.h"
#include "SplineLogic/SoMarker.h"
#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoAudioManager.h"
#include "Projectiles/SoProjectileSpawnerComponent.h"
#include "SoEnemyHelper.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "SplineLogic/SoEditorGameInterface.h"
#include "Online/Achievements/SoAchievementManager.h"
#include "Effects/SoEffectHandlerComponent.h"
#include "Basic/Helpers/SoDateTimeHelper.h"

DEFINE_LOG_CATEGORY(LogSoEnemyAI);

bool ASoEnemy::bDisplayMeleeHitTraceLines = false;
const FName ASoEnemy::FadeStrName = FName("FadeStr");
const FName ASoEnemy::FadeOutColorParamName = FName("FadeEdgeColor");
const float ASoEnemy::PauseFadeSpeed = 2.0f;
const FName ASoEnemy::SwappedTagName = FName("Swapped");

UAnimSequenceBase* ASoEnemy::CachedSpawnAnim = nullptr;

FSoStrikePhase FSoStrikePhase::Invalid;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoEnemy::ASoEnemy(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<USoCharacterMovementComponent>(CharacterMovementComponentName))
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NamedActionLists.Empty();
	for (int32 i = 0; i < static_cast<int32>(ESoActionList::EAL_Max); ++i)
		NamedActionLists.Add(static_cast<ESoActionList>(i));

	SoCombat = ObjectInitializer.CreateDefaultSubobject<USoCombatComponent>(this, TEXT("SoCombatComponent"));

	HealthWidget3D = ObjectInitializer.CreateDefaultSubobject<UWidgetComponent>(this, TEXT("HealthWidget3D"));
	HealthWidget3D->SetupAttachment(GetRootComponent());
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == FName("DistanceAlongSpline"))
	{
		const FSoSplinePoint TempPoint = SplinePointPtr.Extract();
		if (TempPoint.IsValid(false))
			SetActorLocation(TempPoint.ToVector(GetActorLocation().Z));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASoEnemy, SoGroupName))
	{
		FSoEditorGameInterface::FixEnemyGroupName(SoGroupName);
		FSoEditorGameInterface::BuildEnemyGroupDataConfigFile(GetWorld());
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	USoSplineHelper::UpdateSplineLocationRef(this, SplinePointPtr, true, true);

	// if (HealthWidget3DParent != nullptr)
	// 	HealthWidget3D->AttachToComponent(HealthWidget3DParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ReloadScripts();

	OverlapHitReact.AssociatedActor = this;

	if (HealthWidget3DParent != nullptr)
		HealthWidget3D->AttachToComponent(HealthWidget3DParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts or when spawned
void ASoEnemy::BeginPlay()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("BeginPlay"), STAT_BeginPlay, STATGROUP_SoEnemy);

	Super::BeginPlay();

	SoMovement->SetSplineLocation(SplinePointPtr.Extract());

	InitialTransform = GetActorTransform();

	InitialMeshLocation = GetMesh()->RelativeLocation;
	InitialMeshRotation = GetMesh()->RelativeRotation;

	InitialSplinePoint = SoMovement->GetSplineLocation();
	HealthWidget3D->SetVisibility(false);

	const FName* NamePtr = AnimationMap.FindKey(SpawnAnimation);
	if (SpawnAnimation != nullptr && NamePtr != nullptr)
	{
		SpawnCounter = (SpawnAnimation->SequenceLength / SpawnAnimation->RateScale);
		MaxSpawnCounterValue = SpawnCounter;
		Activity = ESoEnemyActivity::EEA_Spawn;

		// before?
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SoMovement->Velocity = FVector(0, 0, 0);
		SoMovement->StopActiveMovement();
		SoMovement->SetMovementMode(MOVE_None);
		// during the Possess phase movement would be changed otherwise:
		SoMovement->SetOppressSetDefaultMovementMode(true);

		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		// UE_LOG(LogTemp, Warning, TEXT("Spawn Counter: %f, Anim: %s"), SpawnCounter, *NamePtr->ToString());
	}
	else
	{
		Activity = ESoEnemyActivity::EEA_Default;
		CurrentAnimData.Type = ESoAnimationType::EAT_Default;
		GetMesh()->VisibilityBasedAnimTickOption = DefaultVisibilityBasedAnimTickOption;
		UpdateOverlapSetup();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetMesh()->SetCollisionEnabled(bEnableMeshCollisionInBeginPlay ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}

	// callback subscribes to player rematerialize if it is needed
	if (bPlacedInLevel)
		USoEventHandlerHelper::SubscribeToSoPostLoad(this);


	ASoGameMode::Get(this).RegisterEnemy(this, SoGroupName);

	if (!bPlacedInLevel && bUseIdleVO)
		GetWorld()->GetTimerManager().SetTimer(
			IdleVOTimer,
			this,
			&ThisClass::OnIdleVO,
			GetRandRangeIdleVODelay()
		);

	if (bPlacedInLevel && bShouldBePausedIfPlacedInLevel && bCanBePaused)
		PauseEnemy();

#if PLATFORM_SWITCH
	RagdollVisibleTime = 1.0f;
#endif

	USoGameSettings::Get().OnDisplayEnemyHealthBarChanged.AddDynamic(this, &ASoEnemy::OnHealthBarSettingsChanged);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USoGameSettings* GameSettings = USoGameSettings::GetInstance())
		GameSettings->OnDisplayEnemyHealthBarChanged.RemoveDynamic(this, &ASoEnemy::OnHealthBarSettingsChanged);

	if (bPlacedInLevel)
		ResetSoEnemy();
	else
		OnRemoveSoEnemyBP();

	// After ResetSoEnemy call because that function registers the enemy!!!
	ASoGameMode::Get(this).UnregisterEnemy(this, SoGroupName);

	Super::EndPlay(EndPlayReason);

	if (bPlacedInLevel)
	{
		USoEventHandlerHelper::UnsubscribeFromPlayerRespawn(this);
		USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);
	}

	if (RagdollFadeOutTimer.IsValid())
	{
		bRagdollFadeOutInProgress = false;
		GetWorld()->GetTimerManager().ClearTimer(RagdollFadeOutTimer);
	}

	if (bSubscribedToMeshHit)
	{
		GetMesh()->OnComponentHit.RemoveDynamic(this, &ASoEnemy::OnMeshHitCallback);
		bSubscribedToMeshHit = false;
	}

	GetWorld()->GetTimerManager().ClearTimer(IdleVOTimer);
	GetWorld()->GetTimerManager().ClearTimer(PauseFadeOutTimer);
	bShouldBePausedIfPlacedInLevel = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::SetPlacedInLevel(bool bPlaced)
{
	if (bPlacedInLevel && !bPlaced)
	{
		USoEventHandlerHelper::UnsubscribeFromPlayerRespawn(this);
		USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);
	}

	bPlacedInLevel = bPlaced;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::SetActiveActionList(FName Name)
{
	if (!ActionLists.Contains(Name))
	{
		UE_LOG(LogSoEnemyAI, Warning, TEXT("ASoEnemy::SetActiveActionList failed: list %s isn't a valid script for %s"), *Name.ToString(), *GetName());
		return;
	}

	ActiveActionListName = Name;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::Tick(float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick"), STAT_Tick, STATGROUP_SoEnemy);

	Super::Tick(DeltaTime);

	if (bPaused || bSoMarkedForDestroy)
		return;

	bWasHitThisFrame = false;

	SFXTCounter -= DeltaTime;
	if (SFXTCounter < 0.0f && !bRagdollFadeOutInProgress)
	{
		SFXTCounter = SFXSPPeriod;
		if (SFXTCount > SFXSPThreshold)
		{
			SFXTCount = 0;
			SFXSpamCounter += 1;
			if (SFXSpamCounter > SFXSPMaxSpamCountBeforeBan)
			{
				FadeOutRagdoll();
				SFXSpamCounter = 0;
			}
		}
		else
			SFXSpamCounter = 0;
	}

	if (bForceLookAtPlayer)
		LookAtPlayer();

#if WITH_EDITOR
	if (!SoMovement->GetSplineLocation().IsValid())
		return;
#endif

	switch (Activity)
	{
		case ESoEnemyActivity::EEA_Spawn:
			SpawnCounter -= DeltaTime;
			if (SpawnCounter < KINDA_SMALL_NUMBER)
				OnSpawnFinished();
			else if (MaxSpawnCounterValue - SpawnCounter > 0.1f)
				GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			break;

		case ESoEnemyActivity::EEA_Dead:
			break;

		case ESoEnemyActivity::EEA_Default:
			CurrentAnimData.Type = ESoAnimationType::EAT_Default;
		default:
		{
			BlockChance = FMath::Clamp(BlockChance + BlockChanceDelta * DeltaTime, 0.0f, 1.0f);

			if (SwitchBetweenNamedActionListsInterval > 0.0f)
				UpdateNamedActionListChange(DeltaTime);

			if (ActiveAction == nullptr)
			{
				if (bCanStartNewAction && (bCanStartActionInAir || SoMovement->IsMovingOnGround()))
					ChooseNewAction();
			}
			else
			{
				USoEAction* OldActiveAction = ActiveAction;
				const bool bStillActive = ActiveAction->Tick(DeltaTime, this);
				if (!bStillActive && OldActiveAction == ActiveAction)
					ChooseNewAction();
			}
		}
	}

	// update health widget if necessary
	if (WidgetPercentUpdateCounter > 0.0f)
	{
		WidgetPercentUpdateCounter -= DeltaTime;
		const float Alpha = FMath::Clamp(1.0f - WidgetPercentUpdateCounter / WidgetPercentUpdateTime, 0.0f, 1.0f);
		WidgetCurrentPercent = USoMathHelper::InterpolateDeceleration(WidgetLastTargetPercent, FMath::Max(SoCharacterSheet->GetHealthPercent(), 0.1f), Alpha);
		SetHealthBar3DPercent(WidgetCurrentPercent);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoEnemyAnimData& ASoEnemy::InitializeAnimationState()
{
	if (SpawnAnimation == nullptr)
	{
		// maybe the cache is used to set this - but we have to reset it, because we don't want other enemies to use _our_ spawn animation
		SpawnAnimation = CachedSpawnAnim;
		CachedSpawnAnim = nullptr;
	}

	const FName* NamePtr = AnimationMap.FindKey(SpawnAnimation);
	if (SpawnAnimation != nullptr && NamePtr != nullptr)
	{
		CurrentAnimData.Type = ESoAnimationType::EAT_Spawn;
		CurrentAnimData.SpawnAnim = *NamePtr;
	}

	return CurrentAnimData;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::UpdateOverlapSetup()
{
	GetMesh()->SetGenerateOverlapEvents(bUseMeshForOverlaps);
	// GetMesh()->SetCollisionEnabled(bUseMeshForOverlaps ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	static FName SoEnemy = FName("SoEnemyCapsule");
	static FName SoEnemyNoMeshOverlap = FName("SoEnemyNoMeshOverlap");
	if (bCanCodeModifyCapsuleCollisionProfile)
		GetCapsuleComponent()->SetCollisionProfileName(bUseMeshForOverlaps ? SoEnemy: SoEnemyNoMeshOverlap);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::UpdateNamedActionListChange(float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UpdateNamedActionListChange"), STAT_UpdateNamedActionListChange, STATGROUP_SoEnemy);

	SwitchCounter += DeltaSeconds;
	if (SwitchCounter > SwitchBetweenNamedActionListsInterval)
	{
		SwitchCounter = 0.0f;

		AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(this);
		const bool bPlayerAlive = ISoMortal::Execute_IsAlive(Player);
		const FSoSplinePoint OwnerSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(this);
		const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(this);
		const float DistanceZ = fabs(GetActorLocation().Z - Player->GetActorLocation().Z);

		switch (ActiveActionList)
		{
			case ESoActionList::EAL_Idle:
				if (bPlayerAlive)
				{
					const float Distance = fabs(OwnerSplineLocation.GetDistanceFromSplinePoint(PlayerSplineLocation, AlertDistance + 10.0f));
					if (Distance < AlertDistance && DistanceZ < AlertDistanceZ)
						SwitchActionListImmediately(ESoActionList::EAL_Melee);
				}
				break;

			case ESoActionList::EAL_Melee:
				{
					const float Distance = fabs(OwnerSplineLocation.GetDistanceFromSplinePoint(PlayerSplineLocation, AlertOffDistance + 10.0f));
					if (!bPlayerAlive || Distance > AlertOffDistance || DistanceZ > AlertOffDistanceZ)
						SwitchActionListImmediately(ESoActionList::EAL_Idle, 0.5f);
				}
				break;

			default:
				break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::SwitchActionListImmediately(ESoActionList NewList, float WaitTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SwitchActionListImmediately"), STAT_SwitchActionListImmediately, STATGROUP_SoEnemy);

	InterruptAction();

	ActiveActionListName = NamedActionLists[NewList];
	ActiveActionList = NewList;

	if (WaitTime > KINDA_SMALL_NUMBER)
		Wait(0.5f);

	if (bControlIdleVOBasedOnActionList)
		bIdleVOOn = (NewList == ESoActionList::EAL_Idle);

	OnPostActionListChangeBP(NewList);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::ChooseNewAction()
{
	ActiveAction = nullptr;

	if (WaitAfterNextAction > KINDA_SMALL_NUMBER)
	{
		Wait(WaitAfterNextAction, true);
		WaitAfterNextAction = -1.0f;
		return;
	}

	FSoEActions* List = ActionMap.Find(ActiveActionListName);
	if (List != nullptr)
	{
		ActiveAction = ChooseActionFromList(List->Array);
		if (ActiveAction != nullptr)
			ActiveAction->Start(this);
		// UE_LOG(LogSoEnemyAI, Display, TEXT("ASoEnemy::ChooseNewAction()"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::ChooseNewAction(FName ListName)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ChooseNewAction"), STAT_ChooseNewAction, STATGROUP_SoEnemy);

	ActiveAction = nullptr;
	if (FSoEActions* List = ActionMap.Find(ListName))
	{
		ActiveAction = ChooseActionFromList(List->Array);
		if (ActiveAction != nullptr)
			ActiveAction->Start(this);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEAction* ASoEnemy::GetFirstSatisfiedAction(TArray<USoEAction*>& Actions) const
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("GetFirstSatisfiedAction"), STAT_GetFirstSatisfiedAction, STATGROUP_SoEnemy);

	for (USoEAction* Action : Actions)
		if (Action->Evaluate(this) > KINDA_SMALL_NUMBER)
			return Action;

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEAction* ASoEnemy::ChooseActionFromList(TArray<USoEAction*>& Actions) const
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ChooseActionFromList"), STAT_ChooseActionFromList, STATGROUP_SoEnemy);

	struct FSoActionToChoose
	{
		USoEAction* Action;
		float Value;
	};

	TArray<FSoActionToChoose> ActionList;
	float Sum = 0.0f;
	for (int i = 0; i < Actions.Num(); ++i)
	{
		FSoActionToChoose A;
		A.Action = Actions[i];
		if (A.Action != nullptr)
		{
			A.Value = A.Action->Evaluate(this);
			if (A.Value > KINDA_SMALL_NUMBER)
			{
				Sum += A.Value;
				ActionList.Add(A);
			}
		}
	}

	// TODO: check if random generation works as intended, blame leyyin if it is not
	const float RandValue = FMath::RandRange(0.0f, Sum);
	Sum = 0.0f;
	for (int32 i = 0; i < ActionList.Num(); ++i)
	{
		Sum += ActionList[i].Value;
		if (RandValue < Sum)
			return ActionList[i].Action;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::HasNamedActionList(ESoActionList List)
{
	if (FName* NamePtr = NamedActionLists.Find(List))
	{
		return (*NamePtr) != NAME_None;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::Wait(float DeltaSeconds, bool bCanBeInterrupted)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Wait"), STAT_Wait, STATGROUP_SoEnemy);

	if (ActiveAction != nullptr)
	{
		UE_LOG(LogSoEnemyAI, Display, TEXT("ASoEnemy::Wait failed: an action is already active!"));
		return;
	}

	if (DefaultWaitAction == nullptr)
		DefaultWaitAction = NewObject<USoEAWait>(this, USoEAWait::StaticClass());

	DefaultWaitAction->SetDuration(DeltaSeconds);
	DefaultWaitAction->Start(this);
	DefaultWaitAction->SetInterruptible(bCanBeInterrupted);
	ActiveAction = DefaultWaitAction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::SwitchToAction(USoEAction* Action)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SwitchToAction"), STAT_SwitchToAction, STATGROUP_SoEnemy);

	if (ActiveAction != nullptr)
	{
		ActiveAction->Interrupt(this);
		ActiveAction = nullptr;
	}

	ActiveAction = Action;
	if (ActiveAction != nullptr)
		ActiveAction->Start(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::SelectActionFromActions(FName ActionsName, bool bForced)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SelectActionFromActions"), STAT_SelectActionFromActions, STATGROUP_SoEnemy);

	if (bForced && ActiveAction != nullptr)
	{
		ActiveAction->Interrupt(this);
		ActiveAction = nullptr;
	}

	if (ActiveAction != nullptr)
		return;

	ChooseNewAction(ActionsName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::Stun(float DeltaSeconds, ESoStatusEffect StatusEffect, bool bIgnoreRes)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Stun"), STAT_Stun, STATGROUP_SoEnemy);

	if (IsStunned())
		return;

	if (((StunResistance + KINDA_SMALL_NUMBER > 1.0f) && StatusEffect != ESoStatusEffect::ESE_BootTransmutation) ||
		(!bCanBeTurnedIntoBoots && StatusEffect == ESoStatusEffect::ESE_BootTransmutation))
		ASoGameMode::Get(this).DisplayText(GetActorLocation(), ESoDisplayText::EDT_Immune);
	else
	{
		if (bIgnoreRes ||
			StunResistance - KINDA_SMALL_NUMBER <= FMath::FRandRange(0.0f, 1.0f) ||
			StatusEffect == ESoStatusEffect::ESE_BootTransmutation)
		{
			InterruptAction();

			if (DefaultStunnedAction == nullptr)
				DefaultStunnedAction = NewObject<USoEAStunned>(this, USoEAStunned::StaticClass());

			DefaultStunnedAction->SetStatusEffect(StatusEffect);
			DefaultStunnedAction->SetDuration(DeltaSeconds);
			DefaultStunnedAction->Start(this);
			ActiveAction = DefaultStunnedAction;
		}
		else
			ASoGameMode::Get(this).DisplayText(GetActorLocation(), ESoDisplayText::EDT_Resisted);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::IsStunned() const
{
	return (ActiveAction != nullptr && ActiveAction->GetClass() == USoEAStunned::StaticClass());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::LookAtPlayer()
{
	const FSoSplinePoint SplineLocation = SoMovement->GetSplineLocation();
	const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(this);
	const float TargetDir = FMath::Sign((PlayerSplineLocation.GetDistanceFromSplinePoint(SplineLocation, 10000)));
	SetActorRotation((SplineLocation.GetDirection() * TargetDir).Rotation());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::LookAtPlayerIgnoreSpline()
{
	if (AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(this))
	{
		FVector Direction = Player->GetActorLocation() - GetActorLocation();
		Direction.Z = 0.0;
		SetActorRotation(Direction.Rotation());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoEnemy::GetFloorLocation_Implementation() const
{
	FVector Result = GetActorLocation();
	Result.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return Result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoEnemy::GetMinIdleVODelay() const
{
	return USoDateTimeHelper::NormalizeTime(MinIdleVODelay);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoEnemy::GetMaxIdleVODelay() const
{
	return USoDateTimeHelper::NormalizeTime(MaxIdleVODelay);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoEnemy::GetRandRangeIdleVODelay() const
{
	return FMath::RandRange(GetMinIdleVODelay(), GetMaxIdleVODelay());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoEnemy::GetRagdollVisibleTime() const
{
	return USoDateTimeHelper::NormalizeTime(RagdollVisibleTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::InterruptAction()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("InterruptAction"), STAT_InterruptAction, STATGROUP_SoEnemy);

	if (ActiveAction != nullptr)
	{
		ActiveAction->Interrupt(this);
		ActiveAction = nullptr;
	}

	OnActionInterruptedBP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::IsCriticalHit(const TSet<FName> AssociatedBones)
{
	for (auto& Bone : AssociatedBones)
		if (CriticalBones.Contains(Bone))
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoEnemy::GetClosestHitLocationToPoint(const FSoHitReactDesc& HitParam, const FVector& SourceLocation, bool bCritical)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("GetClosestHitLocationToPoint"), STAT_GetClosestHitLocationToPoint, STATGROUP_SoEnemy);

	if (!bMeshUsedForCoverTraceCheck)
	{
		FVector Value;
		GetCapsuleComponent()->GetClosestPointOnCollision(SourceLocation, Value);
		return Value;
	}

	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		FVector MinLocation = FVector(0.0f, 0.0f, 0.0f);
		float MinDist = BIG_NUMBER;

		const TSet<FName>* BonesToCheck = &HitParam.AssociatedBones;

		if (HitParam.AssociatedBones.Num() == 1 && HitParam.AssociatedBones.Contains(NAME_None))
		{
			if (bCheckAllBonesWithBodyIfNoBoneNameIsRecieved)
			{
				if (BonesWithBody.Num() == 0)
				{
					TArray<FName> BoneNames;
					SkeletalMesh->GetBoneNames(BoneNames);

					for (FName BoneName : BoneNames)
						if (SkeletalMesh->GetBodyInstance(BoneName) != nullptr)
							BonesWithBody.Add(BoneName);
				}

				BonesToCheck = &BonesWithBody;
			}
		}

		for (const FName BoneName : *BonesToCheck)
			if (!bCritical || CriticalBones.Contains(BoneName))
			{
				FVector Value;
				const float Dist = SkeletalMesh->GetClosestPointOnCollision(SourceLocation, Value, BoneName);
				if (Dist > 0.0f && Dist < MinDist)
				{
					MinDist = Dist;
					MinLocation = Value;
				}
			}

		return MinLocation;
	}
	return FVector(0.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::Strike(FName StrikeName)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Strike"), STAT_Strike, STATGROUP_SoEnemy);

	FSoStrike* StrikeData = StrikeMap.Find(StrikeName);
	if (StrikeData == nullptr || StrikeData->Phases.Num() == 0)
		return false;

	LastStrikeName = StrikeName;
	Activity = ESoEnemyActivity::EEA_Strike;
	SoCombat->SetupNewStrike(StrikeData->Phases[0]);

	CurrentAnimData.Type = ESoAnimationType::EAT_InterruptibleSingle;
	OnStrikePhaseSwitch(StrikeData->Phases[0], 0);

	if (StrikeData->bLookAtPlayer)
		LookAtPlayerIgnoreSpline();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnStrikePhaseSwitch(const FSoStrikePhase& NewPhase, int32 PhaseIndex)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnStrikePhaseSwitch"), STAT_OnStrikePhaseSwitch, STATGROUP_SoEnemy);

	const float AnimDuration = (NewPhase.bLooped || NewPhase.AnimationDuration < 0.0f) ? NewPhase.Duration : NewPhase.AnimationDuration;
	CurrentAnimData.InterruptibleSingle.SetAnimation(NewPhase.Animation, AnimDuration, NewPhase.bLooped);
	SoCombat->SetupNewStrike(NewPhase);
	LastStrikePhaseIndex = PhaseIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoStrike* ASoEnemy::GetStrike(FName StrikeName) const
{
	return StrikeMap.Find(StrikeName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoStrikePhase& ASoEnemy::GetActiveStrikePhase() const
{
	const FSoStrike* Strike = StrikeMap.Find(LastStrikeName);
	if (Strike != nullptr && Strike->Phases.IsValidIndex(LastStrikePhaseIndex))
		return Strike->Phases[LastStrikePhaseIndex];

	return FSoStrikePhase::Invalid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::StrikeEnd()
{
	Activity = ESoEnemyActivity::EEA_Default;

	SoCombat->DisableAllWeaponCollision();
	SoMovement->SetRootMotionDesc({});

	CurrentAnimData.Type = ESoAnimationType::EAT_Default;

	OnStrikeEnd();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnRangeAttackStarted(FName RangeAttackName, float Duration, ESoRangeAttackAnimType AnimType, bool bInterruptible)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnRangeAttackStarted"), STAT_OnRangeAttackStarted, STATGROUP_SoEnemy);

	LastRangeAttackName = RangeAttackName;
	if (AnimType != ESoRangeAttackAnimType::ERAT_NoAnim)
		Activity = ESoEnemyActivity::EEA_RangeAttack;
	OnRangeAttackStartedBP();

	bool bLooped = false;
	switch (AnimType)
	{
		case ESoRangeAttackAnimType::ERAT_PrepareAndLoop:
			bLooped = true;
		case ESoRangeAttackAnimType::ERAT_PrepareAndFinish:
		{
			CurrentAnimData.Type = ESoAnimationType::EAT_InterruptibleWithPreparation;
			const FName InitAnimName = FName(*(RangeAttackName.ToString() + "_start"));
			CurrentAnimData.InterruptibleWithPreparation.InitAsInterruptibleWithPreparation(InitAnimName, RangeAttackName, bLooped, 1.0f);
		}
		break;

		case ESoRangeAttackAnimType::ERAT_SingleLooped:
			bLooped = true;

		case ESoRangeAttackAnimType::ERAT_Single:
		case ESoRangeAttackAnimType::ERAT_SingleAnimBasedTiming:
			if (UAnimSequenceBase** AnimPtr = AnimationMap.Find(RangeAttackName))
				Duration = USoStaticHelper::GetScaledAnimLength(*AnimPtr);

		case ESoRangeAttackAnimType::ERAT_SingleTimeScaled:
		{
			CurrentAnimData.Type = bInterruptible ? ESoAnimationType::EAT_InterruptibleSingle : ESoAnimationType::EAT_UninterruptibleSingle;
			(bInterruptible ? CurrentAnimData.InterruptibleSingle : CurrentAnimData.UninterruptibleSingle).SetAnimation(RangeAttackName, Duration, bLooped);
		}
		break;

		case ESoRangeAttackAnimType::ERAT_NoAnim:
		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoERangeAttackProfile* ASoEnemy::InitializeAndGetRangeAttackProfile(int32 Index, int32 Variant)
{
	if (RangeAttackProfileList.IsValidIndex(Index))
	{
		PrepareRangeAttackProfile(Index, Variant);
		return &RangeAttackProfileList[Index];
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::PrepareRangeAttackProfile_Implementation(int32 RangeAttackProfileIndex, int32 Variant)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("PrepareRangeAttackProfile_Implementation"), STAT_PrepareRangeAttackProfile_Implementation, STATGROUP_SoEnemy);

	const AActor* Target = GetTarget();
	if (Target != nullptr)
	{
		const FVector StartPos = GetRangeAttackLocation(RangeAttackProfileIndex);
		const FVector TargetPos = Target->GetActorLocation();

		RangeAttackProfileList[RangeAttackProfileIndex].Location = StartPos;

		bool bSplineProjectile = false;
		const FSoProjectileInitData* InitData = GetRangeAttackInitData(RangeAttackProfileIndex, bSplineProjectile);
		if (InitData == nullptr)
			return;

		if (fabs(InitData->GravityScale) < KINDA_SMALL_NUMBER)
		{
			RangeAttackProfileList[RangeAttackProfileIndex].Orientation = (TargetPos - StartPos).Rotation();
			return;
		}

		if (bSplineProjectile)
		{
			const FSoSplinePoint SplineLocation = SoMovement->GetSplineLocation();
			const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(this);
			const FVector Delta = StartPos - FVector(SplineLocation);
			const FSoSplinePoint StartSplineLocation = SplineLocation + SplineLocation.GetDirectionModifierFromVector(Delta) * Delta.Size2D();
			const FVector Velocity = USoMathHelper::CalcRangeAttackVelocity(StartPos,
																	  StartSplineLocation,
																	  TargetPos,
																	  PlayerSplineLocation,
																	  InitData->Velocity.Size(),
																	  InitData->GravityScale,
																	  RangeAttackProfileList[RangeAttackProfileIndex].bPreferLargeArc ? 1 : -1);
			if (Velocity.Size() > KINDA_SMALL_NUMBER)
				RangeAttackProfileList[RangeAttackProfileIndex].Orientation = Velocity.Rotation();
			else
				RangeAttackProfileList[RangeAttackProfileIndex].Orientation = (TargetPos - StartPos).Rotation();
		}
		else
		{
			const FVector Velocity = USoMathHelper::CalcRangeAttackVelocity(StartPos,
																	  TargetPos,
																	  InitData->Velocity.Size(),
																	  InitData->GravityScale,
																	  RangeAttackProfileList[RangeAttackProfileIndex].bPreferLargeArc ? 1 : -1);
			if (Velocity.Size() > KINDA_SMALL_NUMBER)
				RangeAttackProfileList[RangeAttackProfileIndex].Orientation = Velocity.Rotation();
			else
				RangeAttackProfileList[RangeAttackProfileIndex].Orientation = (TargetPos - StartPos).Rotation();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoEnemy::GetRangeAttackLocation_Implementation(int32 RangeAttackProfileIndex) const
{
	if (ProjectileSpawner != nullptr)
		return ProjectileSpawner->GetComponentLocation();

	return FVector(0.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoEnemy::GetDefaultDmgNumberLocation_Implementation() const
{
	return GetActorLocation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoERangeAttackProfile* ASoEnemy::GetRangeAttackProfile(int32 Index) const
{
	if (RangeAttackProfileList.IsValidIndex(Index))
		return &RangeAttackProfileList[Index];

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoProjectileInitData* ASoEnemy::GetRangeAttackInitData(int32 RangeAttackProfileIndex, bool& bSplineBased) const
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("GetRangeAttackInitData"), STAT_GetRangeAttackInitData, STATGROUP_SoEnemy);

	if (!RangeAttackProfileList.IsValidIndex(RangeAttackProfileIndex))
		return nullptr;

	ASoProjectile* Projectile =  USoGameInstance::Get(this).ClaimProjectile(RangeAttackProfileList[RangeAttackProfileIndex].ProjectileClass);
	if (Projectile == nullptr)
		return nullptr;

	bool bUseOverride = RangeAttackProfileList[RangeAttackProfileIndex].bUseProjectileInitDataOverride;
	const FSoProjectileInitData* InitData = &RangeAttackProfileList[RangeAttackProfileIndex].InitDataOverride;
	if (!RangeAttackProfileList[RangeAttackProfileIndex].bUseThisInsteadOfComponentDefaults)
	{
		InitData = &ProjectileSpawner->GetProjectileInitData();
		bUseOverride = ProjectileSpawner->GetOverrideProjectileInitData();
	}
	if (!bUseOverride)
		InitData = &Projectile->GetDefaultInitData();

	bSplineBased = Projectile->IsSplineProjectile();

	USoGameInstance::Get(this).ReturnProjectile(Projectile);

	return InitData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::ShouldRangeAttackPreferLargeArc(int32 Index) const
{
	if (!RangeAttackProfileList.IsValidIndex(Index))
		return false;

	return RangeAttackProfileList[Index].bPreferLargeArc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnRangeAttackEnded()
{
	if (Activity == ESoEnemyActivity::EEA_RangeAttack)
	{
		Activity = ESoEnemyActivity::EEA_Default;
		CurrentAnimData.Type = ESoAnimationType::EAT_Default;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::SetAnimation(FName SourceAnimName, FName TargetAnimName)
{
	UAnimSequenceBase** SourcePtr = AnimationMap.Find(SourceAnimName);
	UAnimSequenceBase** TargetPtr = AnimationMap.Find(TargetAnimName);

	if (SourcePtr != nullptr && TargetPtr != nullptr)
	{
		*TargetPtr = *SourcePtr;
		OnAnimationMapChange.Broadcast(this);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::Block(FName BlockName, float Duration)
{
	Activity = ESoEnemyActivity::EEA_Block;
	LastBlockName = BlockName;

	CurrentAnimData.Type = ESoAnimationType::EAT_UninterruptibleSingle;
	CurrentAnimData.UninterruptibleSingle.SetAnimation(BlockName, Duration);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::HitReact(FName HitReactName, float Duration)
{
	if (Activity == ESoEnemyActivity::EEA_Spawn)
	{
		// collision change in OnSpawnFinished can lead to overlap updates which can lead to unintended damage and death
		// bTempIgnoreAllDmg is used to prevent that
		bTempIgnoreAllDmg = true;
		OnSpawnFinished();
		bTempIgnoreAllDmg = false;
	}

	if (Activity == ESoEnemyActivity::EEA_Dead)
		return;
	Activity = ESoEnemyActivity::EEA_HitReact;

	LastHitReactName = HitReactName;
	CurrentAnimData.Type = ESoAnimationType::EAT_UninterruptibleSingle;
	CurrentAnimData.UninterruptibleSingle.SetAnimation(HitReactName, Duration);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OverrideAnimation(bool bInterruptible, FName AnimName, float Duration, bool bLoop)
{
	CurrentAnimData.Type = bInterruptible ? ESoAnimationType::EAT_InterruptibleSingle : ESoAnimationType::EAT_UninterruptibleSingle;

	FSoPingPongAnimName& Anim = bInterruptible ? CurrentAnimData.InterruptibleSingle : CurrentAnimData.UninterruptibleSingle;
	Anim.SetAnimation(AnimName, Duration, bLoop);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::ActivateWeaponCollision_Implementation(bool bActivate, int32 SlotID)
{
	SoCombat->SetWeaponCollisionEnabled(SlotID, bActivate);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::Kill_Implementation(bool bPhysical)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Kill_Implementation"), STAT_Kill_Implementation, STATGROUP_SoEnemy);

	if (Activity == ESoEnemyActivity::EEA_Dead || bSoMarkedForDestroy)
		return;

	if (ActiveAction != nullptr)
	{
		ActiveAction->Interrupt(this);
		ActiveAction = nullptr;
	}
	// ActiveAction->Interrupt can change activity, so that must be called first
	Activity = ESoEnemyActivity::EEA_Dead;
	// TODO: weapon collision num variable?!
	ActivateWeaponCollision(false, 0);
	ActivateWeaponCollision(false, 1);

	OnDeath();
	OnDeathNotify.Broadcast(this);

	if (!ASoGameMode::Get(this).IsEnemyGroupStillActive(SoGroupName))
	{
		if (bPrintWarningIfGroupIsGoneOnDeath)
			UE_LOG(LogSoEnemyAI, Warning, TEXT("%s just died but his group is already dead?!"), *GetName());

		return;
	}

	ASoGameMode::Get(this).OnEnemyDestroyed(this, SoGroupName, GetActorLocation());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnDeath_Implementation()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnDeath_Implementation"), STAT_OnDeath_Implementation, STATGROUP_SoEnemy);

	if (bSoMarkedForDestroy)
		return;

	OnMortalDeath.Broadcast();
	SoMovement->Deactivate();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (SFXOnDeathVOs.Num() > 0)
		USoEnemyVoiceManager::Get(this).PlayEnemyVoice(this, SFXOnDeathVOs[FMath::RandHelper(SFXOnDeathVOs.Num())], ESoVoiceType::SoVoiceTypeDeath);

	if (bActivateRagdollOnDeath)
	{
		GetMesh()->SetCollisionProfileName(FName("Ragdoll"));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->bPauseAnims = true;

		if (SFXOnBodyHit != nullptr && !bSubscribedToMeshHit)
		{
			GetMesh()->OnComponentHit.AddDynamic(this, &ASoEnemy::OnMeshHitCallback);
			bSubscribedToMeshHit = true;
		}

		if (bHideRagdollAfterVisibleTime)
		{
			GetWorld()->GetTimerManager().SetTimer(
				RagdollFadeOutTimer,
				this,
				&ThisClass::FadeOutRagdoll,
				GetRagdollVisibleTime()
			);
		}

#if !PLATFORM_SWITCH
		if (bRagdollShouldGenerateOverlapEvents)
		{
			GetMesh()->SetGenerateOverlapEvents(true);
		}
#endif
	}
	else
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// GetMesh()->SetGenerateOverlapEvents(false);

	GetWorld()->GetTimerManager().ClearTimer(IdleVOTimer);

	if (bCanHaveHealthWidget)
		UpdateHealthBar3D(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::IsAlive_Implementation() const
{
	return Activity != ESoEnemyActivity::EEA_Dead;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::CauseDmg_Implementation(const FSoDmg& Dmg, const FSoHitReactDesc& HitReactDesc)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("CauseDmg_Implementation"), STAT_CauseDmg_Implementation, STATGROUP_SoEnemy);
	UE_LOG(LogSoEnemyAI, Display, TEXT("ASoEnemy::CauseDmg_Implementation"));

	if (Activity == ESoEnemyActivity::EEA_Dead || bTempIgnoreAllDmg || bIgnoreDamage || bSoMarkedForDestroy)
		return;

	if (bBlockRangedAttacks &&
		!bMeleeHitTakeInProgress &&
		HitReactDesc.HitReact != ESoHitReactType::EHR_FallToDeath &&
		HitReactDesc.HitReact != ESoHitReactType::EHR_BreakIntoPieces &&
		fabs(HitReactDesc.Irresistibility) < 10)
	{
		OnRangeAttackBlockedBP();

		const FVector TargetLocation = BoneToTarget == NAME_None ? GetActorLocation() : GetMesh()->GetSocketLocation(BoneToTarget);
		ASoGameMode::Get(this).DisplayText(TargetLocation, Dmg.Physical > Dmg.Magical ? ESoDisplayText::EDT_Blocked : ESoDisplayText::EDT_Immune);

		return;
	}

	if (HitReactDesc.AssociatedActor != nullptr && AttackSourcesToIgnore.Contains(HitReactDesc.AssociatedActor->GetClass()))
	{
		ASoGameMode::Get(this).DisplayText(HitReactDesc.AssociatedActor->GetActorLocation(), ESoDisplayText::EDT_Immune);
		return;
	}

	USoAudioManager::PlaySoundAttached(
		SFXHitReact,
		GetMesh(),
		NAME_None,
		FVector(0.0f, 0.0f, 0.0f),
		EAttachLocation::KeepRelativeOffset
	);

	if ((HitReactDesc.HitReact == ESoHitReactType::EHR_BreakIntoPieces && !bIgnoreBreakHitReact) ||
		HitReactDesc.HitReact == ESoHitReactType::EHR_FallToDeath)
	{
		if (Tags.Contains(SwappedTagName))
		{
			static const FName SwapKillName = FName("A_Swapkill");
			USoAchievementManager::Get(this).UnlockAchievement(this, SwapKillName);
		}

		ISoMortal::Execute_Kill(this, true);
		return;
	}

	if (!bMeleeHitTakeInProgress)
	{
		static const FName PoisonName = FName("Poison");
		if (bChangedHitReactColor != HitReactDesc.AssociatedBones.Contains(PoisonName))
		{
			bUpdateHitReactColor = true;
			bChangedHitReactColor = HitReactDesc.AssociatedBones.Contains(PoisonName);
		}

		if (!bIsInAnotherDimension)
			USoEnemyHelper::DisplayDamageText(this,
											  SoCharacterSheet->GetReducedDmg(Dmg),
											  (HitReactDesc.AssociatedActor == nullptr || HitReactDesc.AssociatedActor == this)
													? GetDefaultDmgNumberLocation()
													: HitReactDesc.AssociatedActor->GetActorLocation(),
											  GetActorForwardVector(),
											  bWasHitThisFrame,
											  false);
		bWasHitThisFrame = true;
	}

	const bool bStillAlive = SoCharacterSheet->ApplyDmg(Dmg);
	OnDamageTakenBP(bStillAlive);
	if (!bStillAlive)
	{
		// damage type is irrelevant here
		ISoMortal::Execute_Kill(this, true);
	}
	else
	{
		UpdateHealthBar3D();

		if (SFXHitReactVOs.Num() > 0)
			USoEnemyVoiceManager::Get(this).PlayEnemyVoice(this, SFXHitReactVOs[FMath::RandHelper(SFXHitReactVOs.Num())], ESoVoiceType::SoVoiceTypeHitReact);

		if (ActiveAction)
		{
			if (!bHitreactBlock && RessistanceToInterrupts < HitReactDesc.Irresistibility && ActiveAction->IsInterruptible())
			{
				ActiveAction->Interrupt(this);
				ActiveAction = nullptr;
			}
			else
				return;
		}

		FSoEActions* HitReacts = ActionMap.Find(NamedActionLists[ESoActionList::EAL_HitReact]);
		if (HitReacts != nullptr && !bHitreactBlock)
		{
			ActiveAction = GetFirstSatisfiedAction(HitReacts->Array);
			if (ActiveAction != nullptr)
				ActiveAction->Start(this);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::MeleeHit_Implementation(const FSoMeleeHitParam& HitParam)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("MeleeHit_Implementation"), STAT_MeleeHit_Implementation, STATGROUP_SoEnemy);
	UE_LOG(LogSoEnemyAI, Display, TEXT("ASoEnemy::MeleeHit_Implementation"));

	if (Activity == ESoEnemyActivity::EEA_Dead || bIgnoreDamage || bSoMarkedForDestroy)
		return true;

	const bool bCritical = IsCriticalHit(HitParam.HitReactDesc.AssociatedBones);
	const FVector EstimatedSourceLocation = HitParam.HitReactDesc.AssociatedActor->GetActorLocation();
	const FVector HitLocation = HitParam.Hit.ImpactPoint.IsNearlyZero() ? GetClosestHitLocationToPoint(HitParam.HitReactDesc, EstimatedSourceLocation, bCritical)
																		: FVector(HitParam.Hit.ImpactPoint);
	if (bAllowCoverTraceCheck)
	{
		FCollisionQueryParams QuaryParams;
		QuaryParams.bIgnoreTouches = true;
		FCollisionResponseParams ResponseParams;
		ResponseParams.CollisionResponse.SetAllChannels(ECollisionResponse::ECR_Ignore);
		ResponseParams.CollisionResponse.SetResponse(ECollisionChannel::ECC_WorldStatic, ECR_Block);
		if (bCoverTraceCheckWorldDyanmicToo)
			ResponseParams.CollisionResponse.SetResponse(ECollisionChannel::ECC_WorldDynamic, ECR_Block);
		const bool bBlocked = GetWorld()->LineTraceTestByChannel(EstimatedSourceLocation, HitLocation, ECollisionChannel::ECC_WorldStatic, QuaryParams, ResponseParams);

		if (bDisplayMeleeHitTraceLines)
			DrawDebugLine(GetWorld(), EstimatedSourceLocation, HitLocation, (bBlocked ? FColor::Red : FColor::Green), false, 3, 0, 3.0f);

		if (bBlocked)
			return false;
	}

	bool bHit = false;
	for (const FName BoneName : HitParam.HitReactDesc.AssociatedBones)
	{
		if (BoneToIgnore == NAME_None ||
			(BoneName != BoneToIgnore && !GetMesh()->BoneIsChildOf(BoneName, BoneToIgnore)))
		{
			bHit = true;
			break;
		}
	}

	if (!bHit)
		return true;


	// check blockchance
	if (BlockChance >= FMath::RandRange(0.0f, 1.0f))
	{
		// block is only possible atm if the current action is interruptible
		if (ActiveAction == nullptr || ActiveAction->IsInterruptible())
		{
			if (ActiveAction != nullptr)
			{
				ActiveAction->Interrupt(this);
				ActiveAction = nullptr;
			}

			FSoEActions* BlockActions = ActionMap.Find(NamedActionLists[ESoActionList::EAL_Block]);
			if (BlockActions != nullptr)
			{
				// maybe all failed - then the enemy could not block the attack
				ActiveAction = GetFirstSatisfiedAction(BlockActions->Array);
				if (ActiveAction != nullptr)
				{
					// TODO: some event or something if blocked with blocked damage amount
					ActiveAction->Start(this);
					LastBlockTime = GetWorld()->GetTimeSeconds();
					return false;
				}
			}
		} // can block
	}

	if (bChangedHitReactColor != (HitParam.ImpactFX != nullptr))
	{
		bUpdateHitReactColor = true;
		bChangedHitReactColor = (HitParam.ImpactFX != nullptr);
	}

	OnMeleeHitSuffered.Broadcast(this, HitParam);

	OnMeleeHitTaken(HitParam, HitLocation, bCritical);

	if (HitParam.ImpactFX != nullptr)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParam.ImpactFX, HitLocation);

	// Reduce friendly fire effect
	const FSoDmg Damage = (HitParam.HitReactDesc.AssociatedActor == USoStaticHelper::GetPlayerCharacterAsActor(this) ? HitParam.Dmg : HitParam.Dmg * 0.1f);
	if (!bIsInAnotherDimension)
		USoEnemyHelper::DisplayDamageText(this, SoCharacterSheet->GetReducedDmg(Damage), HitLocation, GetActorForwardVector(), bWasHitThisFrame, HitParam.bCritical);
	bWasHitThisFrame = true;

	// before CauseDamage, so certain effects can modify ActiveAction to prevent HitReact
	for (auto& Effect : HitParam.Effects)
		ApplyEffect_Implementation(Effect, true);

	bMeleeHitTakeInProgress = true;
	CauseDmg_Implementation(Damage, HitParam.HitReactDesc);
	bMeleeHitTakeInProgress = false;

	LastHitTime = GetWorld()->GetTimeSeconds();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::HandlePlayerRespawn_Implementation()
{
	if (ASoGameMode::Get(this).IsEnemyGroupStillActive(SoGroupName))
	{
		ResetSoEnemy();
	}
	else
		USoEventHandlerHelper::UnsubscribeFromPlayerRespawn(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::HandleSoPostLoad_Implementation()
{
	ensure(bPlacedInLevel);

	if (ASoGameMode::Get(this).IsEnemyGroupStillActive(SoGroupName))
	{
		Activity = ESoEnemyActivity::EEA_Default;
		ResetSoEnemy();
		USoEventHandlerHelper::SubscribeToPlayerRespawn(this);

		FSoSplinePoint& SplineLocation = SoMovement->GetSplineLocation();
		if (ASoPlayerSpline* Spline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline()))
			Spline->RegisterEnemySpawnLocation(SplineLocation.GetDistance(), GetActorLocation().Z, SoGroupName);
	}
	else
		DeactivateSoEnemy();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USceneComponent* ASoEnemy::GetComponent_Implementation() const
{
	return GetMesh();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName ASoEnemy::GetSocketName_Implementation() const
{
	return BoneToTarget;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::ShouldSpawnVO_Implementation(ESoVoiceType VoiceType) const
{
	switch (VoiceType)
	{
		case ESoVoiceType::SoVoiceTypeAttack:
			return FMath::RandRange(0.0f, 1.0f) <= VOChanceAttack;

		case ESoVoiceType::SoVoiceTypeHitReact:
			return FMath::RandRange(0.0f, 1.0f) <= VOChanceHitReact;

		case ESoVoiceType::SoVoiceTypeIdle:
		case ESoVoiceType::SoVoiceTypeDeath:
		case ESoVoiceType::SoVoiceTypeNoise:
			return true;

		default:
			return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::ResetSoEnemy()
{
	if (NamedActionLists[ESoActionList::EAL_Idle] != NAME_None)
	{
		ActiveActionListName = NamedActionLists[ESoActionList::EAL_Idle];
		ActiveActionList = ESoActionList::EAL_Idle;
	}

	OnResetSoEnemyBP();

	ProjectileSpawner->ClearSpawnedProjectiles();

	SetActorTransform(InitialTransform);
	SoMovement->SetSplineLocation(InitialSplinePoint);
	SoCharacterSheet->RestoreHealth();

	SoEffectHandler->OnOwnerSpellsReset();

	// 3d health
	WidgetPercentUpdateTime = 0.6f;
	WidgetLastTargetPercent = 1.0f;
	WidgetCurrentPercent = 1.0f;
	WidgetPercentUpdateCounter = -1.0f;
	SetHealthBar3DPercent(1.0f);

	if (ActiveAction != nullptr)
	{
		ActiveAction->Interrupt(this);
		ActiveAction = nullptr;
	}
	Activity = ESoEnemyActivity::EEA_Default;

	ActivateSoEnemy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::PauseEnemy()
{
	if (!bCanBePaused)
		return;

	bShouldBePausedIfPlacedInLevel = true;
	if (!bPaused && IsAlive() && ASoGameMode::Get(this).IsEnemyGroupStillActive(SoGroupName))
	{
		bPaused = true;
		SoMovement->SetComponentTickEnabled(false);
		UpdateHealthBar3D(true);

		if (MaterialAnimations.IsValidIndex(RagdollFadeOutMaterialAnimationIndex))
		{
			GetMesh()->SetVectorParameterValueOnMaterials(FadeOutColorParamName, PauseFadeOutColor);
			PlayMaterialAnimation(RagdollFadeOutMaterialAnimationIndex, true, PauseFadeSpeed);
			GetWorld()->GetTimerManager().SetTimer(PauseFadeOutTimer, this, &ASoEnemy::PauseFadeFinished, MaterialAnimations[RagdollFadeOutMaterialAnimationIndex].Duration, false);
		}
		else
		{
			CustomTimeDilation = 0.0f;
			SetActorHiddenInGame(true);
			SetActorTickEnabled(false);
		}

		GetMesh()->bNoSkeletonUpdate = true;
		GetMesh()->SetComponentTickEnabled(false);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		OnPauseEnemyBP();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::UnpauseEnemy()
{
	if (!ASoGameMode::Get(this).IsEnemyGroupStillActive(SoGroupName))
		return;

	GetWorld()->GetTimerManager().ClearTimer(PauseFadeOutTimer);

	bShouldBePausedIfPlacedInLevel = false;
	bPaused = false;
	if (!IsAlive())
	{
		DeactivateSoEnemy();
	}
	else
	{
		SoMovement->SetComponentTickEnabled(true);
		CustomTimeDilation = USoGameSettings::Get().GetGameSpeed();
		SetActorTickEnabled(true);
		SetActorHiddenInGame(false);

		GetMesh()->SetVectorParameterValueOnMaterials(FadeOutColorParamName, PauseFadeOutColor);

		if (MaterialAnimations.IsValidIndex(RagdollFadeOutMaterialAnimationIndex))
		{
			auto& MatAnim = MaterialAnimations[RagdollFadeOutMaterialAnimationIndex];
			if (MatAnim.Counter < MatAnim.Duration)
			{
				MatAnim.bPlayForward = false;
				MatAnim.Counter = MatAnim.Duration - MatAnim.Counter;
			}
			else
				PlayMaterialAnimation(RagdollFadeOutMaterialAnimationIndex, false, PauseFadeSpeed);
		}

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		GetMesh()->bNoSkeletonUpdate = false;
		GetMesh()->SetComponentTickEnabled(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		UpdateHealthBar3D();
		OnUnpauseEnemyBP();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::FadeOutAndDestroy()
{
	if (!MaterialAnimations.IsValidIndex(RagdollFadeOutMaterialAnimationIndex))
	{
		Destroy();
		return;
	}


	bPaused = true;
	SoMovement->SetComponentTickEnabled(false);
	UpdateHealthBar3D(true);

	GetMesh()->SetVectorParameterValueOnMaterials(FadeOutColorParamName, PauseFadeOutColor);
	PlayMaterialAnimation(RagdollFadeOutMaterialAnimationIndex, true, PauseFadeSpeed);
	GetWorld()->GetTimerManager().SetTimer(PauseFadeOutTimer, this, &ASoEnemy::DestroyFadeFinished, MaterialAnimations[RagdollFadeOutMaterialAnimationIndex].Duration, false);

	GetMesh()->bNoSkeletonUpdate = true;
	GetMesh()->SetComponentTickEnabled(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OnPauseEnemyBP();
	bSoMarkedForDestroy = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::ActivateSoEnemy()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ActivateSoEnemy"), STAT_ActivateSoEnemy, STATGROUP_SoEnemy);

	GetMesh()->SetVisibility(true, true);
	GetMesh()->SetCollisionProfileName(FName("SoCharMesh"));
	// TODO: enable this if it isn't just the sphere golem who needs it
	// GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	GetMesh()->bPauseAnims = false;

	StopAllMaterialAnimations();

	if (bActivateRagdollOnDeath)
	{
		GetMesh()->PutAllRigidBodiesToSleep();
		GetMesh()->SetSimulatePhysics(false);
		GetMesh()->SetAllBodiesPhysicsBlendWeight(0.0f);
		if (bSubscribedToMeshHit)
		{
			GetMesh()->OnComponentHit.RemoveDynamic(this, &ASoEnemy::OnMeshHitCallback);
			bSubscribedToMeshHit = false;
		}

		GetMesh()->ResetAllBodiesSimulatePhysics();
		GetMesh()->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		/** called here because somehow the ragdoll setup ruins something and the capsule does not update the mesh location without this anymore */
		GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepWorldTransform);

		/** After attachment is "restored" !!! */
		GetMesh()->SetRelativeLocation(InitialMeshLocation);
		GetMesh()->SetRelativeRotation(InitialMeshRotation);

		if (RagdollFadeOutTimer.IsValid())
		{
			bRagdollFadeOutInProgress = false;
			GetWorld()->GetTimerManager().ClearTimer(RagdollFadeOutTimer);
		}

		GetMesh()->SetScalarParameterValueOnMaterials(FadeStrName, 1.0f);
	}
	else
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	UpdateOverlapSetup();

	SoMovement->Activate(true);
	SoMovement->SetMovementMode(SoMovement->DefaultLandMovementMode);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	OnActivateSoEnemyBP();
	ASoGameMode::Get(this).RegisterEnemy(this, SoGroupName);

	if (bUseIdleVO)
		GetWorld()->GetTimerManager().SetTimer(
			IdleVOTimer,
			this,
			&ThisClass::OnIdleVO,
			GetRandRangeIdleVODelay()
		);

	HealthWidget3D->SetVisibility(false);

	if (bPlacedInLevel && bShouldBePausedIfPlacedInLevel && bCanBePaused)
		PauseEnemy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::DeactivateSoEnemy()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("DeactivateSoEnemy"), STAT_DeactivateSoEnemy, STATGROUP_SoEnemy);

	GetMesh()->SetVisibility(false, true);
	GetMesh()->bPauseAnims = true;
	SoMovement->Deactivate();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetGenerateOverlapEvents(false);
	Activity = ESoEnemyActivity::EEA_Dead;
	SoEffectHandler->OnOwnerSpellsReset();

	OnDeactivateSoEnemyBP();
	ASoGameMode::Get(this).UnregisterEnemy(this, SoGroupName);

	if (RagdollFadeOutTimer.IsValid())
	{
		bRagdollFadeOutInProgress = false;
		GetWorld()->GetTimerManager().ClearTimer(RagdollFadeOutTimer);
	}

	GetWorld()->GetTimerManager().ClearTimer(IdleVOTimer);
	if (bCanHaveHealthWidget)
		UpdateHealthBar3D(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::DiedAndLanded()
{
	SoMovement->StopActiveMovement();
	SoMovement->SetMovementMode(MOVE_Flying);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ActivateWeaponCollision(false, 0);
	ActivateWeaponCollision(false, 1);
	SoMovement->ClearRootMotionDesc();
	SoMovement->Velocity = FVector(0, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnBlocked()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnBlocked"), STAT_OnBlocked, STATGROUP_SoEnemy);
	UE_LOG(LogSoEnemyAI, Display, TEXT("ASoEnemy::OnBlocked"));

	if (ActiveAction != nullptr && ActiveAction->IsInterruptible())
	{
		ActiveAction->Interrupt(this);
		ActiveAction = nullptr;

		FSoEActions* BlockedActions = ActionMap.Find(NamedActionLists[ESoActionList::EAL_Blocked]);
		if (BlockedActions != nullptr)
		{
			ActiveAction = GetFirstSatisfiedAction(BlockedActions->Array);
			if (ActiveAction != nullptr)
				ActiveAction->Start(this);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::OnPreLanded(const FHitResult& Hit)
{
	if (ActiveAction == nullptr && WaitSecAfterLand > KINDA_SMALL_NUMBER)
		Wait(WaitSecAfterLand);

	Tags.Remove(SwappedTagName);
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AActor* ASoEnemy::GetTarget() const
{
	return USoStaticHelper::GetPlayerCharacterAsActor(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::ChangeActionList(ESoActionList NewList)
{
	FName* ListName = NamedActionLists.Find(NewList);
	if (ListName == nullptr || (*ListName) == NAME_None)
	{
		const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ESoActionList"), true);
		const FString EnumName = EnumPtr ? EnumPtr->GetNameStringByIndex(static_cast<int32>(NewList)) : "Invalid ESoActionList Value";
		UE_LOG(LogSoEnemyAI, Warning, TEXT("ASoEnemy::ChangeActionList called for named list %s but it does not have a valid value!"), *EnumName);
		return;
	}
	ActiveActionListName = *ListName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::ModifyFloatValue(ESoEnemyFloat ValueToChange, float Value, bool bDelta)
{
	switch (ValueToChange)
	{
		case ESoEnemyFloat::EEF_BlockChance:
			BlockChance = FMath::Clamp((bDelta ? (BlockChance + Value) : Value), 0.0f, 1.0f);
			return;

		case ESoEnemyFloat::EEF_BlockChanceDelta:
			BlockChanceDelta = (bDelta ? (BlockChanceDelta + Value) : Value);
			return;

		default:
			break;
	}

	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ESoEnemyFloat"), true);
	const FString EnumName = EnumPtr ? EnumPtr->GetNameStringByIndex(static_cast<int32>(ValueToChange)) : "Invalid ESoEnemyFloat Value";
	UE_LOG(LogSoEnemyAI, Warning, TEXT("ASoEnemy::ModifyFloatValue called on %s, but it is not a valid modifiable value"), *EnumName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoEnemy::GetFloatValue(ESoEnemyFloat Value) const
{
	switch (Value)
	{
		case ESoEnemyFloat::EEF_BlockChance:
			return BlockChance;

		case ESoEnemyFloat::EEF_BlockChanceDelta:
			return BlockChanceDelta;

		case ESoEnemyFloat::EEF_TimeSinceLastBlock:
			return GetWorld()->GetTimeSeconds() - LastBlockTime;

		case ESoEnemyFloat::EEF_TimeSinceLastHitReact:
			return GetWorld()->GetTimeSeconds() - LastHitTime;

		case ESoEnemyFloat::EEF_TimeSinceLastMonologueEnd:
			return GetWorld()->GetTimeSeconds() - LastMonologueEndTime;
	}

	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::ReloadScripts()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ReloadScripts"), STAT_ReloadScripts, STATGROUP_SoEnemy);

	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	auto* GameInstance = USoGameInstance::GetInstance(this);
	if (GameInstance == nullptr)
		return;

	// strike data - each melee attack for the enemy should be defined in the config file behind StrikeMapConfigName
	GameInstance->ReinitializeStrikeMap(StrikeMapConfigName, StrikeMap);

	if (ActiveAction != nullptr)
	{
		ActiveAction->Interrupt(this);
		ActiveAction = nullptr;
	}

	// action sets
	ActionMap.Empty();
	for (FName ListName : ActionLists)
	{
		FSoEActions& ActionList = ActionMap.Add(ListName);
		GameInstance->ReinitializeActionList(ListName, this, ActionList.Array);
	}

	// named action sets
	for (auto& Iter : NamedActionLists)
		if (Iter.Value != NAME_None)
		{
			FSoEActions& ActionList = ActionMap.Add(Iter.Value);
			GameInstance->ReinitializeActionList(Iter.Value, this, ActionList.Array);
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::IsPlayerBetweenMarkers(int32 Index0, int32 Index1, bool bResultOnInvalidMarkers) const
{
	const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(this);
	ASoMarker* TargetMarker0 = GetTargetMarker(Index0);
	ASoMarker* TargetMarker1 = GetTargetMarker(Index1);
	if (TargetMarker0 == nullptr || TargetMarker1 == nullptr)
		return bResultOnInvalidMarkers;

	return USoSplineHelper::IsSplinepointBetweenPoints(PlayerSplineLocation, TargetMarker0->GetSplineLocation(), TargetMarker1->GetSplineLocation());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::IsBetweenMarkers(int32 Index0, int32 Index1, bool bResultOnInvalidMarkers) const
{
	ASoMarker* TargetMarker0 = GetTargetMarker(Index0);
	ASoMarker* TargetMarker1 = GetTargetMarker(Index1);
	if (TargetMarker0 == nullptr || TargetMarker1 == nullptr)
		return bResultOnInvalidMarkers;

	return USoSplineHelper::IsSplinepointBetweenPoints(SoMovement->GetSplineLocation(), TargetMarker0->GetSplineLocation(), TargetMarker1->GetSplineLocation());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::IsFacingMarker(int32 Index0, bool bResultOnInvalidMarkers) const
{
	ASoMarker* TargetMarker = GetTargetMarker(Index0);
	if (TargetMarker == nullptr)
		return bResultOnInvalidMarkers;

	const int32 Direction0 = SoMovement->GetSplineLocation().GetDirectionModifierFromVector(GetActorForwardVector());
	const int32 Direction1 = FMath::Sign(TargetMarker->GetSplineLocation() - SoMovement->GetSplineLocation());
	return Direction0 == Direction1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::GetSplineLocationFromMarker(int32 Index, FSoSplinePoint& OutSplineLocation) const
{
	if (Index == -1)
	{
		OutSplineLocation = InitialSplinePoint;
		OutSplineLocation.SetReferenceZ(InitialTransform.GetLocation().Z);
	}
	else
	{
		ASoMarker* TargetMarker = GetTargetMarker(Index);
		if (!TargetMarker)
			return false;

		OutSplineLocation = TargetMarker->GetSplineLocation();
		OutSplineLocation.SetReferenceZ(TargetMarker->GetActorLocation().Z);
	}
	return (OutSplineLocation.IsValid(false));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::SetSoGroupName(FName GroupName)
{
	SoGroupName = GroupName;
	ASoGameMode::Get(this).RegisterEnemy(this, GroupName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::FadeOutRagdoll()
{
	GetMesh()->SetVectorParameterValueOnMaterials(FadeOutColorParamName, RagdollFadeOutColor);
	PlayMaterialAnimation(RagdollFadeOutMaterialAnimationIndex);
	if (MaterialAnimations.IsValidIndex(RagdollFadeOutMaterialAnimationIndex))
	{
		bRagdollFadeOutInProgress = true;
		GetWorld()->GetTimerManager().SetTimer(RagdollFadeOutTimer, this, &ASoEnemy::RagdollFadeFinished, MaterialAnimations[RagdollFadeOutMaterialAnimationIndex].Duration);
	}
#if !PLATFORM_SWITCH
	if (bRagdollShouldGenerateOverlapEvents)
	{
		GetMesh()->SetGenerateOverlapEvents(false);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::RagdollFadeFinished()
{
	bRagdollFadeOutInProgress = false;
	GetMesh()->SetVisibility(false);
	GetMesh()->PutAllRigidBodiesToSleep();
	GetMesh()->SetSimulatePhysics(false);
	if (bSubscribedToMeshHit)
	{
		GetMesh()->OnComponentHit.RemoveDynamic(this, &ASoEnemy::OnMeshHitCallback);
		bSubscribedToMeshHit = false;
	}

	// point of no return?
	if (!bPlacedInLevel)
	{
		Destroy();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::PauseFadeFinished()
{
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	CustomTimeDilation = 0.0f;
	// UE_LOG(LogTemp, Warning, TEXT("SetActorHiddenInGame(true)"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::DestroyFadeFinished()
{
	Destroy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnMeshHitCallback(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnMeshHitCallback"), STAT_OnMeshHitCallback, STATGROUP_SoEnemy);

	if (bIgnoreRagdollSelfHit && OtherActor == this && Comp == OtherComp)
		return;

	if (NormalImpulse.SizeSquared() > MinImpulseToSpawnSFX * MinImpulseToSpawnSFX)
	{
		SFXTCount += 1;

		const float TimeSeconds = GetWorld()->GetTimeSeconds();
		if (SFXSpamCounter == 0 && !bRagdollFadeOutInProgress && LastSFXSpawnTime + 0.1f <= TimeSeconds)
		{
			LastSFXSpawnTime = TimeSeconds;
			USoAudioManager::PlaySoundAtLocation(this, SFXOnBodyHit, FTransform(FVector(Hit.Location)));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnIdleVO_Implementation()
{
	if (SFXIdleVOs.Num() > 0 && bUseIdleVO)
	{
		if (AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(this))
		{
			if (!bPaused && bIdleVOOn && (GetActorLocation() - Player->GetActorLocation()).SizeSquared() < MaxIdleDistanceFromPlayer * MaxIdleDistanceFromPlayer)
				USoEnemyVoiceManager::Get(this).PlayEnemyVoice(this, SFXIdleVOs[FMath::RandHelper(SFXIdleVOs.Num())], ESoVoiceType::SoVoiceTypeIdle);
		}

		GetWorld()->GetTimerManager().SetTimer(
			IdleVOTimer,
			this,
			&ThisClass::OnIdleVO,
			GetRandRangeIdleVODelay()
		);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnSpawnFinished()
{
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Activity = ESoEnemyActivity::EEA_Default;
	CurrentAnimData.Type = ESoAnimationType::EAT_Default;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SoMovement->SetMovementMode(SoMovement->DefaultLandMovementMode);
	SoMovement->SetOppressSetDefaultMovementMode(false);

	GetMesh()->VisibilityBasedAnimTickOption = DefaultVisibilityBasedAnimTickOption;

	OnSpawnAnimFinishedBP();

	UpdateOverlapSetup();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::UpdateHealthBar3D(bool bForceHide)
{
	const float HealthPercent = SoCharacterSheet->GetHealthPercent();
	if (!bForceHide && CanHaveHealthWidget() && IsAlive() && HealthPercent < (1.0f - KINDA_SMALL_NUMBER))
	{
		HealthWidget3D->SetVisibility(true);

		WidgetLastTargetPercent = WidgetCurrentPercent;
		WidgetPercentUpdateCounter = WidgetPercentUpdateTime;
		SetHealthBar3DPercent(FMath::Max(SoCharacterSheet->GetHealthPercent(), 0.1f));
		if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
			SoCharacter->AddToEnemyHPWidgetList(HealthWidget3D);
	}
	else
	{
		HealthWidget3D->SetVisibility(false);
		if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
			SoCharacter->RemoveFromEnemyHPWidgetList(HealthWidget3D);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::SetHealthBar3DPercent(float Percent)
{
	if (HealthWidget3D == nullptr || HealthWidget3D->GetUserWidgetObject() == nullptr)
		return;

	if (USizeBox* SizeBox = Cast<USizeBox>(HealthWidget3D->GetUserWidgetObject()->GetRootWidget()))
		if (UProgressBar* ProgressBar = Cast<UProgressBar>(SizeBox->GetContent()))
		{
			ProgressBar->SetPercent(Percent);
			ProgressBar->SetRenderScale(FVector2D(1.0f, 1.0f + (FMath::Max(0.0f, (0.5f - Percent) * 20))));
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemy::OnHealthBarSettingsChanged()
{
	UpdateHealthBar3D(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemy::CanHaveHealthWidget() const
{
	if (!bCanHaveHealthWidget)
		return false;

	return USoGameSettings::Get().IsDisplayEnemyHealthBarEnabled();
}
