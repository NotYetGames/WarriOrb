// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/GameModeBase.h"

#include "Delegates/DelegateCombinations.h"
#include "Basic/SoConsoleCommands.h"
#include "Effects/SoEffectBase.h"

#include "SoGameMode.generated.h"

class ASoEnemy;
class USoGameInstance;
class ASoCharacter;
class ASoSpline;
class FSoTimerManager;
class UDlgDialogue;
class USoInGameUIActivity;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDialogueLineAdded, const UTexture2D*, Icon, bool, bLeftSide, const FText&, Text, float, Duration);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnDisplayDamageText, const FVector&, WorldPosition, int32, Amount, bool, bPhysical, bool, bOnPlayer, bool, bCritical);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDied, ASoEnemy*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGroupDefeated, FName, GroupName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoGameStateChange);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDisplayText, const FVector&, WorldPosition, ESoDisplayText, DisplayText);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoEnemyGroups
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY()
	TMap<FName, int32> EnemyGroupMap;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoEnemyArray
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY()
	TArray<ASoEnemy*> Enemies;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoEnemyCount
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<ASoEnemy> Class;

	UPROPERTY(BlueprintReadWrite)
	int32 Count;

	UPROPERTY(BlueprintReadWrite)
	int32 PlacedInLevelCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoEnemyCounts
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadWrite)
	TArray<FSoEnemyCount> Counts;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoSplineArray
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY()
	TArray<ASoSpline*> Splines;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoFlyingEnemyPositionRules
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere)
	TSubclassOf<ASoEnemy> FlyingEnemyClass;

	UPROPERTY(EditAnywhere)
	float MinDistance;

	UPROPERTY(EditAnywhere)
	float MaxCorrectionSpeed;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * 1.) Loads camera data, ticks LevelManger
 * 2.) Has data for dialogue UI (monologues and wizard-player conversations), controls it via implementable events
 * 3.) handles Save/Load, fires PreSave, PostLoad events
 */
UCLASS()
class SORB_API ASoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASoGameMode();
	virtual ~ASoGameMode();

	UFUNCTION(BlueprintPure, DisplayName = "Get So Game Mode", meta = (WorldContext = "WorldContextObject"))
	static ASoGameMode* GetInstance(const UObject* WorldContextObject);
	static ASoGameMode& Get(const UObject* WorldContextObject)
	{
		check(IsValid(WorldContextObject));
		auto* Instance = GetInstance(WorldContextObject);
		check(IsValid(Instance));
		return *Instance;
	}

	//
	// AGameModeBase interface
	//

	void StartPlay() override;
	void Tick(float DeltaSeconds) override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	bool AllowCheats(APlayerController* Controller) override;


	//
	// Own methods
	//

	void ReloadConsoleCommands() { ConsoleCommands.ReloadCommands(); }

	FSoTimerManager& GetTimerManager() const;

	UFUNCTION(BlueprintCallable, Category = EnemyGroup)
	void OnEnemyDestroyed(ASoEnemy* Enemy, FName Group, FVector WorldLocation);

	void IncreaseEnemyCountInGroup(FName Group, int32 Num);

	void RegisterEnemy(ASoEnemy* Enemy, FName Group);
	void UnregisterEnemy(ASoEnemy* Enemy, FName Group);

	const TArray<ASoEnemy*>& GetEnemiesFromGroup(FName Group);
	FORCEINLINE const TArray<ASoEnemy*>& GetEnemies() { return Enemies; }

	UFUNCTION(BlueprintPure, Category = EnemyGroup)
	bool IsEnemyGroupStillActive(FName Group) const;

	/** Registers USoEventHandler implementers and notifies them on reload. One call is immediately fired to match the initial state*/
	void SubscribeToSoPostLoad(UObject* Object);
	void UnsubscribeFromSoPostLoad(UObject* Object);

	/** Registers USoEventHandler implementers and notifies them on player rematerialize. Check the interface for more information */
	void SubscribeToPlayerRematerialize(UObject* Object);
	void UnsubscribeFromPlayerRematerialize(UObject* Object);

	/** Registers USoEventHandler implementers and notifies them on player respawn. Check the interface for more information */
	void SubscribeToPlayerRespawn(UObject* Object);
	void UnsubscribeFromPlayerRespawn(UObject* Object);

	/**
	 *  Registers USoEventHandler implementers and notifies them and handles area checks on player spline change.
	 *  Registers USoEventHandler implementers and notifies them based on area checks on player spline change
	 */
	void SubscribeWhitelistedSplines(UObject* Object, const TArray<TAssetPtr<ASoSpline>>& WhitelistedSplines);
	void UnsubscribeWhitelistedSplines(UObject* Object);


	void ResetGroupData() { EnemyGroupActualValues = EnemyGroupInitData; }

	void DisplayDamageText(const FVector& HitLocation, int32 DmgAmount, bool bPhysical, bool bOnPlayer, bool bCritical = false);

	UFUNCTION(BlueprintCallable)
	void DisplayText(const FVector& Location, ESoDisplayText DisplayText);

	UFUNCTION(BlueprintPure)
	TArray<FSoEnemyCount>& GetDestroyedEnemyCount(FName Group);

	UFUNCTION(BlueprintPure)
	int32 CalculateDestroyedEnemyCount(FName Group);

	UFUNCTION()
	void OnPlayerRematerialize();

	UFUNCTION()
	void OnPlayerRespawn();

	UFUNCTION()
	void OnPlayerSplineChanged(const ASoSpline* OldSpline, const ASoSpline* NewSpline);

	void OverrideGroupDefeatedDelay(FName GroupName, float OverrideTime);

	UFUNCTION()
	void OnGroupDestroyed();

	FORCEINLINE void TriggerPreSaveGame() { PreSaveGame(); }
	FORCEINLINE void TriggerPostLoadGame() { PostLoadGame(); }

	void ClearWizardDialogueAfterGroupDefeated() { WizardDialogueAfterGroupDefeated = nullptr; }
	ASoCharacter* GetSoCharacter();

protected:
	void PreSaveGame() const;
	void PostLoadGame();

public:
	UPROPERTY(BlueprintAssignable)
	FOnDialogueLineAdded OnDialogueLineAdded;

	UPROPERTY(BlueprintAssignable)
	FOnDisplayDamageText OnDisplayDamageText;

	UPROPERTY(BlueprintAssignable)
	FOnDisplayText OnDisplayText;

	UPROPERTY(BlueprintAssignable)
	FOnEnemyDied OnEnemyDied;

	UPROPERTY(BlueprintAssignable)
	FOnGroupDefeated OnEnemyGroupDefeated;

	/** called before the world state is saved to disc */
	UPROPERTY(BlueprintAssignable)
	FSoGameStateChange OnPreSave;

	/** called after the world state is reloaded from disc */
	UPROPERTY(BlueprintAssignable)
	FSoGameStateChange OnPostLoad;

	/** Called after all BeginPlay() calls */
	UPROPERTY(BlueprintAssignable)
	FSoGameStateChange OnPostBeginPlay;

	/** List of actors implementing SoPostLoadHandler interface, alternative (BP friendly) way to handle reload */
	UPROPERTY()
	TArray<UObject*> PostLoadHandlers;

	UPROPERTY()
	TArray<UObject*> PlayerRematerializedHandlers;

	UPROPERTY()
	TArray<UObject*> PlayerRematerializedUnsubscribeRequests;

	UPROPERTY()
	TArray<UObject*> PlayerRespawnHandlers;

	UPROPERTY()
	TArray<UObject*> PlayerRespawnUnsubscribeRequests;

	UPROPERTY()
	TMap<UObject*, FSoSplineArray> SplineGroupChangedHandlers;

protected:
	FSoConsoleCommands ConsoleCommands;

	UPROPERTY(EditAnywhere)
	UTexture2D* WarriorbIcon = nullptr;

	UPROPERTY(EditAnywhere)
	UTexture2D* WizardIcon = nullptr;

	// Cached game instance
	UPROPERTY()
	USoGameInstance* GameInstance = nullptr;

	// Cached Game Character
	UPROPERTY()
	ASoCharacter* CachedSoCharacter = nullptr;

	/** used to stop the array from being modified while it is iterated */
	bool bPlayerRematerializedHandlersArrayBlock = false;

	bool bPlayerRespawnHandlersArrayBlock = false;

	FSoTimerManager* SoTimerManager = nullptr;


	/** Read from config, max member count for each enemy group */
	FSoEnemyGroups EnemyGroupInitData;

	/**
	 *  Actual amount of not yet killed enemies. Once a group reaches 0 it won't respawn anymore.
	 *  Reinitialized from EnemyGroupInitData each time the character rematerializes
	 */
	FSoEnemyGroups EnemyGroupActualValues;


	/** Enemies are registered here per group */
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FSoEnemyArray> EnemiesAlive;

	UPROPERTY(BlueprintReadOnly)
	TArray<ASoEnemy*> Enemies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, float> EnemyGroupDefeatedDelayOverride;

	UPROPERTY(BlueprintReadOnly)
	FName LastDestroyedGroupName;

	/** Stores the destroyed enemy counts for the group defeated window */
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FSoEnemyCounts> DestroyedEnemies;

	FTimerHandle EnemyGroupDestroyedNotificationTimer;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USoInGameUIActivity> GroupDestroyedUIClass;

	bool bSplineChangedNotificationsInProgress = false;

	UPROPERTY(EditAnywhere, Category = AIHelper)
	TArray<FSoFlyingEnemyPositionRules> FlyingEnemyPositionRules;

	bool bShouldEnterGroupDefeated = false;

	UPROPERTY(BlueprintReadWrite)
	UDlgDialogue* WizardDialogueAfterGroupDefeated = nullptr;

	float GroupDefeatedDelayOverride = -1.0f;
};
