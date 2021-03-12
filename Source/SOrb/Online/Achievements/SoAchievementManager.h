// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "UObject/CoreOnline.h"

#if WARRIORB_WITH_ONLINE
#include "OnlineStats.h"
#include "OnlineSubsystem.h"
#endif // WARRIORB_WITH_ONLINE

#if WARRIORB_WITH_STEAM
#include "INYSteamSubsystem.h"
#endif // WARRIORB_WITH_STEAM

#include "SoAchievement.h"
#include "SoAchievementSettings.h"

#include "SoAchievementManager.generated.h"

class APlayerController;


USTRUCT(BlueprintType, Blueprintable)
struct FSoConditionalAchievement
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ConditionBoolTrueOnCharacter;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ConditionBoolFalseOnCharacter;
};


// Docs: https://docs.unrealengine.com/en-us/Programming/Online/AchievementsInterface
// See blueprints implementation:
// - UAchievementBlueprintLibrary
// - FOnlineSubsystemBPCallHelper
// - UAchievementWriteCallbackProxy
UCLASS(Blueprintable, BlueprintType)
class SORB_API USoAchievementManager : public UObject
{
	GENERATED_BODY()
public:
	USoAchievementManager(const FObjectInitializer& ObjectInitializer);
	~USoAchievementManager();

	UFUNCTION(BlueprintPure, DisplayName = "Get So Achievement Manager", Category="Achievements", meta = (WorldContext = "WorldContextObject"))
	static USoAchievementManager* GetInstance(const UObject* WorldContextObject);
	static USoAchievementManager& Get(const UObject* WorldContextObject)
	{
		check(IsValid(WorldContextObject));
		auto* Instance = GetInstance(WorldContextObject);
		check(IsValid(Instance));
		return *Instance;
	}

	static bool AreSteamAchievementsAvailable();

#if WARRIORB_WITH_STEAM
	static INYSteamStatsAndAchievementsPtr GetSteamAchievements();
#endif

	void Tick(float DeltaSeconds);
	bool Initialize(APlayerController* InController);
	bool IsInitialized() const { return PlayerControllerWeakPtr.IsValid() && IsUserIDValid(); }
	void Shutdown();

	// Removes all achievements, WARNING: Only call this in dev mode
	// All these work only in NON SHIPPING
	void ResetAchievements(); // this does not clean the player array
	void ResetAchievement(UObject* WorldContextObject, FName AchievementName);

	UFUNCTION(BlueprintCallable, Category=">Achievements", meta = (WorldContext = "WorldContextObject"))
	void UnlockAchievement(UObject* WorldContextObject, FName AchievementName);

	UFUNCTION(BlueprintCallable, Category = ">Achievements", meta = (WorldContext = "WorldContextObject"))
	static void UnlockAchievementIfSatisfied(UObject* WorldContextObject, const FSoConditionalAchievement& Achievement);

	bool IsProgressAchievement(FName AchievementName) const;
	void ShowAchievementProgress(FName AchievementName) const;
	bool IsValidAchievementName(FName AchievementName) const { return FSoAchievement::IsValidName(AchievementName); }

	bool IsUserIDValid() const { return UserIDPtr.IsValid() && UserIDPtr->IsValid(); }

	// Are we recording achievements in  offline mode
	bool IsInOfflineMode() const;

private:
	// Steam variants
	void SetSteamAchievementProgressForName(FName AchievementName, float CompletionRatio);

	// Sanity checks

	void CheckSteamDataIntegrity();

	// Checks if the current state is valid, aka we can do some operation
	bool ValidateCurrentState(bool bLog = true) const;

	// NOTE: This is async
	void CacheSteamAchievements();
	void CacheAchievements();

#if WARRIORB_WITH_STEAM
	// Internal callback when the achievement query completes
	void OnSteamUserStatsReceived(FNYSteamUserStatsReceived_Result Result);

	// Internal callback when the achievement is written
	void OnSteamAchievementStored(FNYSteamUserAchievementStored_Result Result);
#endif // WARRIORB_WITH_STEAM

protected:
	// All the Achievement subsystems:
#if WARRIORB_WITH_STEAM
	INYSteamStatsAndAchievementsPtr SteamAchievementsInterface;
#endif // WARRIORB_WITH_STEAM

	// Holds the player controller weak object
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr = nullptr;

	// Our Net user ID
	TSharedPtr<const FUniqueNetId> UserIDPtr;
};
