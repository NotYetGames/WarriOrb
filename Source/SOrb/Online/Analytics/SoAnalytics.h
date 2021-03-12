// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CharacterBase/SoIMortalTypes.h"

#include "SoAnalytics.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogSoAnalytics, All, All);

struct FSoPlayerProgressItemUsageStats;
struct FSoSplineDeathContext;
struct FSoSplineStats;
struct FSoPlayerProgressStats;
struct FSoSplineCriticalFPSAreas;

////////////////////////////////////////////////////////////////////////////////////////
// NOTE this must be kept in sync with the GameAnalytics settings
UENUM(BlueprintType)
enum class ESoResourceCurrencyType : uint8
{
	ERC_Gold = 0		UMETA(DisplayName = "Gold")
};

////////////////////////////////////////////////////////////////////////////////////////
// NOTE this must be kept in sync with the GameAnalytics settings
UENUM(BlueprintType)
enum class ESoResourceItemType : uint8
{
	ERI_BuyTrader = 0		UMETA(DisplayName = "Buy from Trader"),
	ERI_SellTrader 			UMETA(DisplayName = "Sell Trader"),
	ERI_MonsterChest 		UMETA(DisplayName = "Monster Chest"),
	ERI_Quest 				UMETA(DisplayName = "Quest"),
};


////////////////////////////////////////////////////////////////////////////////////////
// Our own version of EGAErrorSeverity
UENUM()
enum class ESoAnalyticsErrorType : uint8
{
	EAE_Undefined = 0,
	EAE_Debug,
	EAE_Info,
	EAE_Warning,
	EAE_Error,
	EAE_Critical,
};

////////////////////////////////////////////////////////////////////////////////////////
// Our own version of EGAProgressionStatus
UENUM()
enum class ESoAnalyticsProgressType : uint8
{
	EAP_Undefined = 0,
	EAE_Start,
	EAE_Complete,
	EAE_Fail,
};

////////////////////////////////////////////////////////////////////////////////////////
// Defines all the info we need to record a currency
USTRUCT(BlueprintType)
struct FSoResourceCurrency
{
	GENERATED_USTRUCT_BODY()

public:
	FSoResourceCurrency() {}
	FSoResourceCurrency(const ESoResourceCurrencyType InCurrencyType, const int32 InCurrencyQuantity, const ESoResourceItemType InItemType, const FString& InItemID)
		: CurrencyType(InCurrencyType), CurrencyQuantity(InCurrencyQuantity), ItemType(InItemType), ItemID(InItemID) {}

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ESoResourceCurrencyType CurrencyType = ESoResourceCurrencyType::ERC_Gold;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CurrencyQuantity = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ESoResourceItemType ItemType = ESoResourceItemType::ERI_BuyTrader;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ItemID;
};

/**
 * Handles the collection of events for Warriorb with the consent of the user.
 * Handles correct translation + conversion of seconds to the appropriate time stamp
 * Note you should only record events and stuff from the current session.
 */
UCLASS()
class SORB_API USoAnalytics : public UObject
{
	GENERATED_BODY()
public:
	// Start/End session
	void StartSession(bool bInCollectGameAnalytics, bool bInWaitForAnalyticsToSend, double PollWaitSeconds, double PollProcessEventsSeconds);
	void ModifyCanCollectStatus(bool bInCollectGameAnalytics, bool bInWaitForAnalyticsToSend, double PollWaitSeconds, double PollProcessEventsSeconds);
	void EndSession();
	void FlushEventsAndWait(bool bForceWait = false);

	// Design events (everything else)
	// See:
	// 1. https://gameanalytics.com/docs/unreal4-sdk#design-event
	// 2. https://gameanalytics.com/docs/cpp-sdk#design
	// 3. https://gameanalytics.com/docs/ga-data#design-event
	// 4. https://gameanalytics.com/docs/implementing-your-tracking#design
	// 5. https://gameanalytics.com/docs/custom-events
	//
	void RecordDesignEvent(const FString& EventId);
	void RecordDesignEvent(const FString& EventId, float Value);

	// Performance
	void RecordPerformanceAverageFPS(int32 FPS);
	void RecordPerformanceSplineAverageFPS(const FString& MapName, const FString& SplineName, int32 FPS);

	// Critical FPS areas, is actually a warning error report
	bool BuildMessageForCriticalFPSAreas(const FString& MapName, const FString& SplineName, const FSoSplineCriticalFPSAreas& Areas,
		float AccumulateThreshold, FString& OutMessage);
	void RecordPerformanceSplineCriticalFPSAreas(const TArray<FString>& SplineMessages);

	// Death happened
	void RecordDeathTotal(int32 DeathNum);
	void RecordDeathWithSoulKeeperNum(int32 DeathNum);
	void RecordDeathWithCheckpointNum(int32 DeathNum);

	// HP lost
	void RecordHPLostTotal(int32 HPNum);
	void RecordHPLostByDamageType(const TMap<ESoDmgType, int32>& LostHpByDamageTypeTable);

	// Total play time
	void RecordPlayTime(float Seconds);

	// Spline stuff happened
	void RecordSplineEnterNum(const FString& MapName, const FString& SplineName, int32 EnterNum);
	void RecordSplineTimeSpent(const FString& MapName, const FString& SplineName, float Seconds);
	void RecordSplineTimeFirstEnter(const FString& MapName, const FString& SplineName, float Seconds);

	// Spline deaths
	void RecordSplineDeath(const FString& MapName, const FString& SplineName, const FSoSplineDeathContext& Context);
	void RecordSplineDeaths(const FString& MapName, const FString& SplineName, const TArray<FSoSplineDeathContext>& DeathData);

	// Spline HP lost
	void RecordSplineHPLostTotal(const FString& MapName, const FString& SplineName, int32 HPNum);
	void RecordSplineHPLostByDamageType(const FString& MapName, const FString& SplineName, const TMap<ESoDmgType, int32> & LostHpByDamageTypeTable);

	// Gameplay milestone
	// Eg: Tutorial 2 finished, Defeated boss, etc
	void RecordMilestone(const FString& MapName, const FString& MilestoneName, const float Seconds);
	void RecordMilestone(const FString& MapName, const FString& MilestoneName);

	// Record Item usage time
	void RecordItemUsageTime(const FString& ItemName, const FSoPlayerProgressItemUsageStats& Usage);
	void RecordItemUsageTimes(const FString& ItemName, const TArray<FSoPlayerProgressItemUsageStats>& Usages);

	// Record Spells stats
	void RecordSpellCasted(const FString& SpellName, int32 CastAmount);

	// Records UI events
	void RecordUIEpisodeStart();
	void RecordUISaveLoad();
	void RecordUISaveRemove();
	void RecordUISaveStartNewGame();

	// Episode
	void RecordEpisodeCompleted(const FString& EpisodeName, float Seconds);

	// Record an error.
	// See:
	// 1. https://gameanalytics.com/docs/unreal4-sdk#error-event
	// 2. https://gameanalytics.com/docs/cpp-sdk#error
	// 3. https://gameanalytics.com/docs/ga-data#error-event
	// 4. https://gameanalytics.com/docs/implementing-your-tracking#error
	//
	void RecordErrorEvent(ESoAnalyticsErrorType ErrorType, const FString& Message);

	// Update the level progress of the game
	// Can have the follow hierarchies (only up to 3 hierarchies).
	// 1. Chapter: Chapter, time
	//
	// NOT Useful:
	// 2. Level:   Chapter, Level, time
	// 3. Spline:  Chapter, Level, Spline, time
	//
	// See:
	// 1. https://gameanalytics.com/docs/unreal4-sdk#progression-event
	// 2. https://gameanalytics.com/docs/cpp-sdk#progression
	// 3. https://gameanalytics.com/docs/ga-data#progression-event
	// 4. https://gameanalytics.com/docs/implementing-your-tracking#progression
	//
	void UpdateMapProgress(ESoAnalyticsProgressType ProgressType, const FString& MapName, float TimeSeconds);
	void UpdateLevelProgress(ESoAnalyticsProgressType ProgressType, const FString& MapName, const FString& LevelName, float TimeSeconds);
	void UpdateSplineProgress(ESoAnalyticsProgressType ProgressType, const FString& MapName, const FString& LevelName, const FString& SplineName, float TimeSeconds);

	// Add/Reduce gold
	FORCEINLINE void AddGold(int32 GoldAmount, ESoResourceItemType ItemType, const FString& ItemID)
	{
		AddResourceCurrency(ESoResourceCurrencyType::ERC_Gold, GoldAmount, ItemType, ItemID);
	}

	FORCEINLINE void SubtractGold(int32 GoldAmount, ESoResourceItemType ItemType, const FString& ItemID)
	{
		SubtractResourceCurrency(ESoResourceCurrencyType::ERC_Gold, GoldAmount, ItemType, ItemID);
	}

	// Enum to string conversion
	// NOTE this must be kept in sync with the GameAnalytics settings
	FORCEINLINE static FString GetStringFromResourceCurrencyType(ESoResourceCurrencyType Currency)
	{
		switch (Currency)
		{
		case ESoResourceCurrencyType::ERC_Gold:
			return TEXT("Gold");
		default:
			return TEXT("UNKNOWN");
		}
	}
	FORCEINLINE static FString GetStringFromResourceItemType(ESoResourceItemType ItemType)
	{
		switch (ItemType)
		{
		case ESoResourceItemType::ERI_BuyTrader:
			return TEXT("BuyTrader");
		case ESoResourceItemType::ERI_SellTrader:
			return TEXT("SellTrader");
		case ESoResourceItemType::ERI_MonsterChest:
			return TEXT("MonsterChest");
		case ESoResourceItemType::ERI_Quest:
			return TEXT("Quest");
		default:
			return TEXT("UNKNOWN");
		}
	}
	FORCEINLINE static FString GetStringFromAnalyticsErrorType(ESoAnalyticsErrorType ErrorType)
	{
		switch (ErrorType)
		{
		case ESoAnalyticsErrorType::EAE_Undefined:
			return TEXT("undefined");
		case ESoAnalyticsErrorType::EAE_Debug :
			return TEXT("debug");
		case ESoAnalyticsErrorType::EAE_Info:
			return TEXT("info");
		case ESoAnalyticsErrorType::EAE_Warning:
			return TEXT("warning");
		case ESoAnalyticsErrorType::EAE_Error:
			return TEXT("error");
		case ESoAnalyticsErrorType::EAE_Critical:
			return TEXT("critical");
		default:
			return TEXT("UNKNOWN");
		}
	}
	FORCEINLINE static FString GetStringFromAnalyticsProgressType(ESoAnalyticsProgressType ProgressType)
	{
		switch (ProgressType)
		{
		case ESoAnalyticsProgressType::EAP_Undefined:
			return TEXT("undefined");
		case ESoAnalyticsProgressType::EAE_Start:
			return TEXT("start");
		case ESoAnalyticsProgressType::EAE_Complete:
			return TEXT("complete");
		case ESoAnalyticsProgressType::EAE_Fail:
			return TEXT("fail");
		default:
			return TEXT("UNKNOWN");
		}
	}

protected:
	// Add/subtract generic currency with item type and id.
	// See:
	// 1. https://gameanalytics.com/docs/unreal4-sdk#resource-event
	// 2. https://gameanalytics.com/docs/cpp-sdk#resource
	// 3. https://gameanalytics.com/docs/ga-data#resource-event
	// 4. https://gameanalytics.com/docs/implementing-your-tracking#resource
	//
	void AddResourceCurrency(const FSoResourceCurrency& ResourceCurrency);
	void SubtractResourceCurrency(const FSoResourceCurrency& ResourceCurrency);

	// Helper methods
	FORCEINLINE void AddResourceCurrency(ESoResourceCurrencyType Currency, int32 InCurrencyQuantity, ESoResourceItemType ItemType, const FString& ItemID)
	{
		AddResourceCurrency({ Currency, InCurrencyQuantity, ItemType, ItemID });
	}
	FORCEINLINE void SubtractResourceCurrency(ESoResourceCurrencyType Currency, int32 InCurrencyQuantity, ESoResourceItemType ItemType, const FString& ItemID)
	{
		SubtractResourceCurrency({ Currency, InCurrencyQuantity, ItemType, ItemID });
	}

	// Internal version
	void InternalRecordHPLostByDamageType(const FString& BaseEventName, const TMap<ESoDmgType, int32>& LostHpByDamageTypeTable);
	void InternalRecordGameplaySplineDeath(const FString& BaseEventName, const FString& MapName, const FString& SplineName, const FSoSplineDeathContext& Context);

	// Internal version of Add/Subtract currency
	void ModifyResourceCurrency(const FString& FlowType, const FSoResourceCurrency& ResourceCurrency);

	// Internal version of Update<>Progress
	void UpdateProgress(ESoAnalyticsProgressType ProgressType, const TArray<FString>& ProgressHierarchy, float TimeSeconds);

public:
	// Constants from https://gameanalytics.com/docs/unreal4-sdk
	static const FString ATTRIBUTE_FlowType;
	static const FString ATTRIBUTE_FlowSink;
	static const FString ATTRIBUTE_FlowSource;
	static const FString ATTRIBUTE_ItemType;
	static const FString ATTRIBUTE_Currency;
	static const FString ATTRIBUTE_Message;
	static const FString ATTRIBUTE_Value;

protected:
	// Can collect data
	UPROPERTY(VisibleAnywhere)
	bool bCollectGameAnalytics = false;

	UPROPERTY(VisibleAnywhere)
	bool bWaitForAnalyticsToSend = false;
};
