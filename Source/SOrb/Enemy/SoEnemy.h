// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEnemyDataTypes.h"
#include "CharacterBase/SoCharacterBase.h"
#include "CharacterBase/SoVoiceUser.h"
#include "SplineLogic/SoSplinePointPtr.h"
#include "Basic/SoEventHandler.h"

#include "SoEnemy.generated.h"

class ASoEnemySpawner;
class UAnimSequenceBase;
class USoEAction;
class UFMODEvent;
class ASoMarker;
class USceneComponent;
class USoCharacterMovementComponent;
class UWidgetComponent;
class USoEAWait;
class USoEAStunned;
class USoCombatComponent;

DECLARE_STATS_GROUP(TEXT("SoEnemy"), STATGROUP_SoEnemy, STATCAT_Advanced);

DECLARE_LOG_CATEGORY_EXTERN(LogSoEnemyAI, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoEnemyNotify, ASoEnemy*, Enemy);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Class for spline based AI characters
 */
UCLASS()
class SORB_API ASoEnemy : public ASoCharacterBase, public ISoEventHandler, public ISoVoiceUser
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoEnemy(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// AActor
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// SoCharacterBase
	virtual void OnBlocked() override;
	virtual bool OnPreLanded(const FHitResult& Hit);

	// SoMortal interface
	virtual void CauseDmg_Implementation(const FSoDmg& Dmg, const FSoHitReactDesc& HitReactDesc) override;
	virtual bool MeleeHit_Implementation(const FSoMeleeHitParam& HitParam) override;
	virtual void Kill_Implementation(bool bPhysical) override;
	virtual bool IsAlive_Implementation() const override;
	virtual bool IsBounceAble_Implementation() const override { return bIsBounceAble; }


	// SoEventHandler interface
	virtual void HandlePlayerRespawn_Implementation() override;
	virtual void HandleSoPostLoad_Implementation() override;
	virtual void HandlePlayerRematerialize_Implementation() override {};
	virtual void HandleWhitelistedSplinesEntered_Implementation() override {};
	virtual void HandleWhitelistedSplinesLeft_Implementation() override {};

	// SoVoiceUser interface
	virtual USceneComponent* GetComponent_Implementation() const;
	virtual FName GetSocketName_Implementation() const;
	virtual bool ShouldSpawnVO_Implementation(ESoVoiceType VoiceType) const;


	bool IsInAnotherDimension() const { return bIsInAnotherDimension; }

	UFUNCTION(BlueprintCallable, Category = Init)
	void ResetSoEnemy();

	/** Called before reset */
	UFUNCTION(BlueprintImplementableEvent, Category = Init)
	void OnResetSoEnemyBP();

	/** Called in endplay if the enemy isn't placed in level */
	UFUNCTION(BlueprintImplementableEvent, Category = Init)
	void OnRemoveSoEnemyBP();


	UFUNCTION(BlueprintImplementableEvent, Category = Init)
	void OnSpawnAnimFinishedBP();


	UFUNCTION(BlueprintCallable)
	void PauseEnemy();

	UFUNCTION(BlueprintCallable)
	void UnpauseEnemy();

	UFUNCTION(BlueprintImplementableEvent)
	void OnPauseEnemyBP();

	UFUNCTION(BlueprintImplementableEvent)
	void OnUnpauseEnemyBP();

	UFUNCTION(BlueprintCallable)
	void FadeOutAndDestroy();


	UFUNCTION(BlueprintImplementableEvent, Category = Init)
	void OnPostActionListChangeBP(ESoActionList NewList);

	UFUNCTION(BlueprintImplementableEvent, Category = Init)
	void OnRangeAttackBlockedBP();

	void ActivateSoEnemy();
	void DeactivateSoEnemy();

	UFUNCTION(BlueprintCallable, Category = Init)
	void SetActivity(ESoEnemyActivity NewActivity) { Activity = NewActivity; }

	/** called from animation BP to setup the spawn animations properly, it is called really early, even before PreInitializeComponents */
	UFUNCTION(BlueprintCallable, Category = Animation)
	const FSoEnemyAnimData& InitializeAnimationState();

	UFUNCTION(BlueprintCallable, Category = Animation)
	static void SetCachedSpawnAnim(UAnimSequenceBase* Sequence) { CachedSpawnAnim = Sequence; }

	// action functions

	/**
	* Changes one of the active actionlists
	* The active action isn't modified nor dropped
	* @PARAM NewList: the name of the list (key in NamedActionLists)
	* @PARAM bPrimary: whether the primary or the secondary list should be changed
	*/
	void ChangeActionList(ESoActionList NewList);



	/**
	 * Called from Strike Action to begin a strike - changes state, initializes combat component
	 * @PARAM StrikeName: name of the strike (defined in config, key in StrikeMap), has to match with the strike animation
	 * @Param SecondaryAnimationName: optional, used if the strike uses more than one animations
	 * @Return whether the strike with given name was found and started or not
	 */
	bool Strike(FName StrikeName);

	void OnStrikePhaseSwitch(const FSoStrikePhase& NewPhase, int32 PhaseIndex);

	/**
	 * Called from Strike Action when an action is finished
	 * Weapon collision is disabled here
	 * Rootmotion turned off
	 */
	void StrikeEnd();

	/** Called from action when the AI begin to use the range attack */
	void OnRangeAttackStarted(FName RangeAttackName, float Duration, ESoRangeAttackAnimType AnimType, bool bInterruptible);

	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnRangeAttackStartedBP();

	FSoERangeAttackProfile* InitializeAndGetRangeAttackProfile(int32 Index, int32 Variant);

	// dynamic values like location and orientation won't be valid
	const FSoERangeAttackProfile* GetRangeAttackProfile(int32 Index) const;

	const struct FSoProjectileInitData* GetRangeAttackInitData(int32 RangeAttackProfileIndex, bool& bSplineBased) const;
	bool ShouldRangeAttackPreferLargeArc(int32 Index) const;

	UFUNCTION(BlueprintNativeEvent, Category = Combat)
	void PrepareRangeAttackProfile(int32 RangeAttackProfileIndex, int32 Variant);

	UFUNCTION(BlueprintNativeEvent, Category = Combat)
	FVector GetRangeAttackLocation(int32 RangeAttackProfileIndex) const;

	UFUNCTION(BlueprintNativeEvent, Category = Combat)
	FVector GetDefaultDmgNumberLocation() const;

	/** Range attack finished, AI switches back to default mode */
	void OnRangeAttackEnded();

	UFUNCTION(BlueprintCallable, Category = Animations)
	void SetAnimation(FName SourceAnimName, FName TargetAnimName);

	UPROPERTY(BlueprintAssignable)
	FSoEnemyNotify OnAnimationMapChange;

	/**
	 * Called when the enemy blocks an attack
	 * Block name is saved so the animation can react accordingly
	 */
	void Block(FName BlockName, float Duration);
	void HitReact(FName HitReactName, float Duration);

	UFUNCTION(BlueprintCallable, Category = Animation)
	void OverrideAnimation(bool bInterruptible, FName AnimName, float Duration, bool bLoop = false);

	/**
	 *  Called when a teleport effect is performed
	 *  bInitPass decides if the call is executed when the "wait before teleport" time is started, or the actual teleport is performed
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnTeleportBP(bool bInitPass);


	/**
	 *  @Return pointer to the selected strike or nullptr
	 *  WARNING: the pointer should not be kept, it may becomes invalid (e.g. ReloadScripts)
	 */
	const FSoStrike* GetStrike(FName Name) const;

	UFUNCTION(BlueprintPure, Category = Strike)
	const FSoStrikePhase& GetActiveStrikePhase() const;

	/**
	*  @PARAM bActivate: whether to enable or disable collision
	*  @PARAM SlotID: index of the collision primitive, there is two atm
	*/
	UFUNCTION(BlueprintNativeEvent, Category = Combat)
	void ActivateWeaponCollision(bool bActivate = true, int32 SlotID = 0);


	/**
	*  Returns with the first satisfied action (Evalueate() > 0.0f) from the input array,
	*  or nullptr if there is not any
	*/
	USoEAction* GetFirstSatisfiedAction(TArray<USoEAction*>& Actions) const;

	/**
	 *  Evaluates action from a list, returns with the chosen one
	 *  It is supposed to bring balance to the force, not leave it in darkness
	 *  If the chosen one is not found nullptr takes his place
	 */
	USoEAction* ChooseActionFromList(TArray<USoEAction*>& Actions) const;

	FSoEActions* GetActionList(ESoActionList List) { return ActionMap.Find(NamedActionLists[List]); }

	UFUNCTION(BlueprintPure, Category = Actions)
	bool HasNamedActionList(ESoActionList List);

	UFUNCTION(BlueprintNativeEvent, Category = Combat)
	void OnDeath();

	UPROPERTY(BlueprintAssignable)
	FSoEnemyNotify OnDeathNotify;

	UFUNCTION(BlueprintCallable, Category = Enemy)
	ESoEnemyActivity GetActivity() const { return Activity; }

	UFUNCTION(BlueprintCallable, Category = Enemy)
	bool IsAlive() const { return Activity != ESoEnemyActivity::EEA_Dead; }

	UFUNCTION(BlueprintCallable, Category = Enemy)
	void SetCanStartNewAction(bool bCanStart) { bCanStartNewAction = bCanStart; }

	float GetLastBlockTime() const { return LastBlockTime; }

	/** temp junk until ragdol - look kinda cool tho */
	UFUNCTION(BlueprintCallable, Category = Movement)
	void DiedAndLanded();

	/** Name of actual strike for animation */
	UFUNCTION(BlueprintPure, Category = Strike)
	FName GetStrikeName() const { return LastStrikeName; }

	UFUNCTION(BlueprintPure, Category = Strike)
	FName GetRangeAttackName() const { return LastRangeAttackName; }

	/** Name of actual block for animation */
	UFUNCTION(BlueprintPure, Category = Strike)
	FName GetBlockName() const { return LastBlockName; }

	/** Name of actual hit react for animation */
	UFUNCTION(BlueprintPure, Category = Strike)
	FName GetHitReactName() const { return LastHitReactName; }

	USoCharacterMovementComponent* GetSoMovement() { return SoMovement; }
	USoCharacterMovementComponent* GetSoMovement() const { return SoMovement; }

	float GetAlertDistance() const { return AlertDistance; }

	AActor* GetTarget() const;

	/**
	 *  @PARAM ValueToChange: the value the function will modify - not all ESoEnemyFloat is modifiable, invalid calls are ignored
	 *  @PARAM Value: the value used to modify ValueToChange
	 *  @PARAM bDelta: whether to apply Value as delta or as absolute value (+= vs =)
	 */
	void ModifyFloatValue(ESoEnemyFloat ValueToChange, float Value, bool bDelta);

	float GetFloatValue(ESoEnemyFloat Value) const;

	void ReloadScripts();

	// can be handled in BP for special effects
	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnCapsuleComponentHitWhileStriking(AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnStrikeEnd();


	/** Used by the action system to communicate events to BP */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Combat)
	void ExecuteBlueprintEventBP(FName EventID);

	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void ExecuteBlueprintEventIndexedBP(FName EventID, int32 Index);

	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	bool CheckBlueprintConditionBP(FName ConditionID) const;

	UFUNCTION(BlueprintPure)
	ASoMarker* GetTargetMarker(int32 Index) const { return TargetMarkers.IsValidIndex(Index) ? TargetMarkers[Index] : nullptr; }

	ASoEnemySpawner* GetTargetSpawner(int32 Index) const { return TargetSpawners.IsValidIndex(Index) ? TargetSpawners[Index] : nullptr; }

	UFUNCTION(BlueprintPure)
	bool IsPlayerBetweenMarkers(int32 Index0, int32 Index1, bool bResultOnInvalidMarkers = false) const;

	bool IsBetweenMarkers(int32 Index0, int32 Index1, bool bResultOnInvalidMarkers = false) const;
	bool IsFacingMarker(int32 Index0, bool bResultOnInvalidMarkers = false) const;
	bool GetSplineLocationFromMarker(int32 Index, FSoSplinePoint& OutSplineLocation) const;

	FVector GetInitialLocation() const { return InitialTransform.GetLocation(); }
	const FSoSplinePoint& GetInitialSplineLocation() const { return InitialSplinePoint; }

	FName GetSoGroupName() const { return SoGroupName; }
	void SetSoGroupName(FName GroupName);

	ASoEnemySpawner* GetSoSpawner() const { return SoSpawner; }
	void SetSoSpawner(ASoEnemySpawner* Spawner) { SoSpawner = Spawner; }

	FName GetBoneToTarget() const { return BoneToTarget; }
	bool IsAutoAimSupported() const { return bSupportAutoAim; }

	int32 GetSpawnMaterialAnimationIndex() const { return SpawnMaterialAnimationIndex; }

	/** Has to be called with false if actor is spawned runtime */
	void SetPlacedInLevel(bool bPlaced);

	bool IsPlacedInLevel() const { return bPlacedInLevel; }

	UFUNCTION(BlueprintCallable, Category = Init)
	void SetTargetMarkers(const TArray<ASoMarker*>& Markers) { TargetMarkers = Markers; }

	UFUNCTION(BlueprintCallable, Category = Script)
	void SetActiveActionList(FName Name);

	UFUNCTION(BlueprintCallable, Category = Script)
	void Stun(float DeltaSeconds, ESoStatusEffect StatusEffect = ESoStatusEffect::ESE_Stun, bool bIgnoreRes = false);

	void LookAtPlayer();
	void LookAtPlayerIgnoreSpline();

	void SetForcedLookAtPlayer(bool bActive) { bForceLookAtPlayer = bActive; }

	bool CanBeMovedByPlayer() const { return bCanBeMovedByPlayer; }
	bool CanBeMovedByLogic() const { return bCanBeMovedByLogic; }

	bool ShouldKeepZLocationOnSwap() const { return bKeepZLocationOnSwap; }

	UFUNCTION(BlueprintNativeEvent, Category = Combat)
	FVector GetFloorLocation() const;

	const TMap<FName, UAnimSequenceBase*>& GetAnimationMap() { return AnimationMap; }

	void SetDynamicName(FName NewValue) { DynamicName = NewValue; }
	FName GetDynamicName() const { return DynamicName; }

	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnStunned();

	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnStunOver();

	void SetIdleVOOn(bool bEnable) { bIdleVOOn = bEnable; }

	UFUNCTION(BlueprintCallable, Category = HitReact)
	void ChangeHitReactColor()
	{
		bChangedHitReactColor = true;
		bUpdateHitReactColor = true;
	}

	bool IsSoPaused() const { return bPaused; }

	float GetMinIdleVODelay() const;
	float GetMaxIdleVODelay() const;
	float GetRandRangeIdleVODelay() const;
	float GetRagdollVisibleTime() const;
protected:

	void UpdateOverlapSetup();

	void UpdateNamedActionListChange(float DeltaSeconds);

	void SwitchActionListImmediately(ESoActionList NewList, float WaitTime = -1.0f);

	void ChooseNewAction();
	void ChooseNewAction(FName ListName);

	UFUNCTION(BlueprintCallable, Category = Script)
	void Wait(float DeltaSeconds, bool bCanBeInterrupted = true);

	UFUNCTION(BlueprintCallable, Category = Script)
	void SwitchToAction(USoEAction* Action);

	UFUNCTION(BlueprintCallable, Category = Script)
	void SelectActionFromActions(FName ActionsName, bool bForced = true);

	UFUNCTION(BlueprintCallable, Category = State)
	void ForceLeaveHitReactState() { Activity = ESoEnemyActivity::EEA_Default; }

	UFUNCTION(BlueprintPure, Category = State)
	bool IsStunned() const;

	UFUNCTION(BlueprintCallable, Category = Script)
	void InterruptAction();

	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnActionInterruptedBP();

	bool IsCriticalHit(const TSet<FName> AssociatedBones);

	UFUNCTION(BlueprintCallable, Category = Combat)
	FVector GetClosestHitLocationToPoint(const FSoHitReactDesc& HitParam, const FVector& SourceLocation, bool bCritical);

	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnMeleeHitTaken(const FSoMeleeHitParam& HitParam, FVector EstimatedHitLocation, bool bCritical);

	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnDamageTakenBP(bool bStillAlive);

	UFUNCTION(BlueprintImplementableEvent, Category = SoEnemyState)
	void OnActivateSoEnemyBP();

	UFUNCTION(BlueprintImplementableEvent, Category = SoEnemyState)
	void OnDeactivateSoEnemyBP();

	UFUNCTION(BlueprintCallable)
	void FadeOutRagdoll();

	UFUNCTION()
	void RagdollFadeFinished();

	UFUNCTION()
	void PauseFadeFinished();

	UFUNCTION()
	void DestroyFadeFinished();

	UFUNCTION()
	void OnMeshHitCallback(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);


	UFUNCTION(BlueprintNativeEvent, Category = VO)
	void OnIdleVO();

	void OnSpawnFinished();

	UFUNCTION(BlueprintCallable)
	void UpdateHealthBar3D(bool bFordeHide = false);

	void SetHealthBar3DPercent(float Percent);

	UFUNCTION()
	void OnHealthBarSettingsChanged();

	bool CanHaveHealthWidget() const;

public:
	static bool bDisplayMeleeHitTraceLines;

	static const FName FadeStrName;
	static const FName FadeOutColorParamName;

	static const float PauseFadeSpeed;

	static const FName SwappedTagName;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthWidget3D")
	UWidgetComponent* HealthWidget3D;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthWidget3D")
	float WidgetPercentUpdateTime = 0.6f;

	float WidgetLastTargetPercent = 1.0f;
	float WidgetCurrentPercent = 1.0f;
	float WidgetPercentUpdateCounter = -1.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthWidget3D")
	USceneComponent* HealthWidget3DParent;

	UPROPERTY(EditAnywhere, Category = "HealthWidget3D")
	bool bCanHaveHealthWidget = true;

	UPROPERTY(EditAnywhere, Category = "HealthWidget3D")
	bool bAttachHealthWidgetToMesh = false;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// spawn

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineLocation", Meta = (ExposeOnSpawn = true))
	FSoSplinePointPtr SplinePointPtr;

	/** optional animation, played once on spawn if provided */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineLocation", Meta = (ExposeOnSpawn = true))
	UAnimSequenceBase* SpawnAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Meta = (ExposeOnSpawn = true))
	int32 SpawnMaterialAnimationIndex = -1;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// State variables
	UPROPERTY(BlueprintReadWrite)
	ESoEnemyActivity Activity = ESoEnemyActivity::EEA_Default;

	UPROPERTY(EditAnywhere)
	bool bCanBePaused = true;

	UPROPERTY(BlueprintReadOnly)
	bool bPaused = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShouldBePausedIfPlacedInLevel = true;

	UPROPERTY(VisibleAnywhere)
	USoEAction* ActiveAction = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bRagdollFadeOutInProgress = false;

	bool bTempIgnoreAllDmg = false;

	/** can be set from BP to prevent the enemy from taking damage */
	UPROPERTY(BlueprintReadWrite)
	bool bIgnoreDamage = false;

protected:

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <actions>

	/** a valid key value in ActionMap, check ChooseNewAction() function - called secondary for legacy reasons */
	UPROPERTY(EditAnywhere, Category = "AI|Actions")
	FName ActiveActionListName;

	ESoActionList ActiveActionList = ESoActionList::EAL_Idle;

	// only used if > 0.0f
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Actions")
	float SwitchBetweenNamedActionListsInterval = -1.0f;

	float SwitchCounter = 0.0f;


	/** each value has to match a file name (without extension) in SO/AiConfig/ActionLists, used to initialize the ActionMap */
	UPROPERTY(EditAnywhere, Category = "AI|Actions")
	TSet<FName> ActionLists;

	/** Action sequences the enemy can use */
	UPROPERTY(VisibleAnywhere, Category = "AI|Actions")
	TMap<FName, FSoEActions> ActionMap;

	/** these action lists don't have to be in the ActionLists set, check ESoActionList for more info */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize, Category = "AI|Actions")
	TMap<ESoActionList, FName> NamedActionLists;

	UPROPERTY()
	USoEAWait* DefaultWaitAction;

	UPROPERTY()
	USoEAStunned* DefaultStunnedAction;

	/** some react action can be created in BP and set as active action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "AI|Actions")
	TArray<USoEAction*> SpecialActions;

	UPROPERTY(BlueprintReadWrite)
	float WaitAfterNextAction = -1.0f;
	// </actions>
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/** time in "spawn" phase if a spawn anim is provided, matches the animation length */
	float SpawnCounter = 0.0f;

	float MaxSpawnCounterValue = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bOverlapDamageBlocked = false;


	UPROPERTY(BlueprintReadOnly)
	bool bIgnoreBreakHitReact = false;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// combat related stuff

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat)
	USoCombatComponent* SoCombat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	TSet<FName> CriticalBones;

	/** bones used to check the simulated location when spawning water is considered */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TArray<FName> BonesToSplash;

	/** this bone and its children are ignored on melee hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	FName BoneToIgnore;

	/** if set auto target projectiles are supposed to aim at this one instead of the capsule
	 *  VO is attached to this bone as well
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	FName BoneToTarget;

	/** effects aim at this mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	FName BoneCenter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	bool bCheckAllBonesWithBodyIfNoBoneNameIsRecieved = false;

	TSet<FName> BonesWithBody;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	TArray<UClass*> AttackSourcesToIgnore;


	/** The distance the enemy will attack the player from, can be used in actions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	float AlertDistance = 600;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	float AlertDistanceZ = 1200;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	float AlertOffDistance = 3600;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	float AlertOffDistanceZ = 3600;

	UPROPERTY(EditAnywhere, Category = "AI|Strike")
	FName StrikeMapConfigName;

	/** Data used to define strike behaviour, used by USoEAStrike, defined in config */
	UPROPERTY(VisibleAnywhere, Category = "AI|Strike")
	TMap<FName, FSoStrike> StrikeMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Strike")
	TArray<FSoERangeAttackProfile> RangeAttackProfileList;


	// used for preconditons
	// TODO: use dialogue variables instead of this
	UPROPERTY(BlueprintReadOnly)
	FName LastStrikeName;

	UPROPERTY(BlueprintReadOnly)
	FName DynamicName;

	FName LastRangeAttackName;
	FName LastBlockName;
	FName LastHitReactName;

	UPROPERTY(BlueprintReadOnly)
	int32 LastStrikePhaseIndex = 0;

	UPROPERTY(BlueprintReadOnly)
	FSoEnemyAnimData CurrentAnimData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, UAnimSequenceBase*> AnimationMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	TArray<ASoMarker*> TargetMarkers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	TArray<ASoEnemySpawner*> TargetSpawners;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	FName SoGroupName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	bool bPrintWarningIfGroupIsGoneOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	bool bUseMeshForOverlaps = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	bool bCanCodeModifyCapsuleCollisionProfile = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	bool bEnableMeshCollisionInBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	ASoEnemySpawner* SoSpawner = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay, Meta = (ExposeOnSpawn = true))
	bool bPlacedInLevel = true;

	UPROPERTY(EditAnywhere, Category = Anim)
	EVisibilityBasedAnimTickOption DefaultVisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bCanStartActionInAir = false;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAllowCoverTraceCheck = true;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bCoverTraceCheckWorldDyanmicToo = false;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bMeshUsedForCoverTraceCheck = true;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bActivateRagdollOnDeath = true;

	/** must be used if enemy parts are suppsoed to spawn vfx and sfx e.g. on water surfaces */
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bRagdollShouldGenerateOverlapEvents = false;

	UPROPERTY(EditAnywhere, Category = Combat)
	float RagdollVisibleTime = 10.0f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bHideRagdollAfterVisibleTime = false;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bIgnoreRagdollSelfHit = false;

	UPROPERTY(EditAnywhere, Category = Combat)
	int32 RagdollFadeOutMaterialAnimationIndex = 1;

	UPROPERTY()
	FTimerHandle RagdollFadeOutTimer;

	UPROPERTY()
	FTimerHandle PauseFadeOutTimer;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bForceLookAtPlayer = false;

	/** time the enemy has to wait after landing (generates a wait action, but only if the enemy wasn't doing anything already) */
	UPROPERTY(EditAnywhere, Category = Combat)
	float WaitSecAfterLand = 0.0f;

	UPROPERTY(EditAnywhere, Category = Combat)
	int32 RessistanceToInterrupts = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	bool bHitreactBlock = false;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bKeepZLocationOnSwap = false;

	UPROPERTY(EditAnywhere, Category = Combat)
	float StunResistance = 0.0f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bCanBeTurnedIntoBoots = true;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bIsBounceAble = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	bool bSupportAutoAim = true;

	/** used to decide if it can be detected via certain spells (Swap) */
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bCanBeMovedByPlayer = true;

	/** used to decide if enemy is currently movable by SoEnemyHelper */
	UPROPERTY(BlueprintReadWrite, Category = Combat)
	bool bCanBeMovedByLogic = false;

	/** Value between 0 and 1, 0: no block at all, 1: it tries to select a block from the block action list (that can still fail) */
	UPROPERTY(EditAnywhere, Category = Combat)
	float BlockChance = 0.0f;
	/** the amount BlockChance is changed per seconds */
	UPROPERTY(EditAnywhere, Category = Combat)
	float BlockChanceDelta = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	bool bBlockRangedAttacks = false;

	/** can be disabled from BP when the enemy becomes invisible between teleports */
	UPROPERTY(BlueprintReadWrite, Category = Combat)
	bool bIsInAnotherDimension = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	FSoHitReactDesc OverlapHitReact;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	UFMODEvent* SFXHitReact = nullptr;



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	float VOChanceHitReact = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	float VOChanceAttack = 0.5f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	TArray<UFMODEvent*> SFXHitReactVOs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	TArray<UFMODEvent*> SFXOnDeathVOs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	TArray<UFMODEvent*> SFXIdleVOs;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	float MinIdleVODelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	float MaxIdleVODelay = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	float MaxIdleDistanceFromPlayer = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SFX)
	bool bIdleVOOn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SFX)
	bool bControlIdleVOBasedOnActionList = false;

	UPROPERTY(EditAnywhere, Category = SFX)
	bool bUseIdleVO = false;

	UPROPERTY()
	FTimerHandle IdleVOTimer;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	UFMODEvent* SFXOnBodyHit = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SFX)
	float MinImpulseToSpawnSFX = 10000.0f;

	UPROPERTY(BlueprintReadOnly, Category = SFX)
	float LastSFXSpawnTime;

	UPROPERTY(EditAnywhere, Category = SFXSpamProtection)
	float SFXSPPeriod = 0.5f;

	UPROPERTY(EditAnywhere, Category = SFXSpamProtection)
	int32 SFXSPThreshold = 50;

	UPROPERTY(EditAnywhere, Category = SFXSpamProtection)
	int32 SFXSPMaxSpamCountBeforeBan = 2;


	UPROPERTY(BlueprintReadOnly, Category = SFX)
	float SFXTCounter = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = SFX)
	int32 SFXTCount;

	UPROPERTY(BlueprintReadOnly, Category = SFX)
	int32 SFXSpamCounter;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SFX)
	FVector RagdollFadeOutColor = FVector(30.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SFX)
	FVector PauseFadeOutColor = FVector(0.0f, 0.0f, 30.0f);


	UPROPERTY(BlueprintReadOnly, Category = SFX)
	bool bChangedHitReactColor = false;

	UPROPERTY(BlueprintReadWrite, Category = SFX)
	bool bUpdateHitReactColor = true;

	bool bSubscribedToMeshHit = false;


	bool bCanStartNewAction = true;

	/** time in seconds when the enemy last blocked an attack */
	float LastBlockTime = 0;
	/** time in seconds when the enemy last suffered a melee attack */
	float LastHitTime = 0;

	/** last monologue end time */
	float LastMonologueEndTime = 0;

	bool bMeleeHitTakeInProgress = false;

	bool bWasHitThisFrame = false;

	UPROPERTY(BlueprintReadOnly)
	bool bSoMarkedForDestroy = false;


	FTransform InitialTransform;
	FSoSplinePoint InitialSplinePoint;

	/** Relative, stored in BeginPlay(), used to reset mesh (cause of ragdoll) */
	FVector InitialMeshLocation;
	/** Relative, stored in BeginPlay(), used to reset mesh (cause of ragdoll) */
	FRotator InitialMeshRotation;



	/**
	  *  this is the only way to have it passed from spawner to InitializeAnimationState(), which is called from AnimBP::InitAnim script
	  *  it has to be setup before the Spawn node is called and the object is created, because the spawn process starts the animation immediately,
	  *  even before the exposed properties are initialized (-> would be first frame glitch otherwise)
	  */
	static UAnimSequenceBase* CachedSpawnAnim;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
};
