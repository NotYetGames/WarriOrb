// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoAnalytics.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoStringHelper.h"
#include "SoAnalyticsProvider.h"
#include "CharacterBase/SoIMortalTypes.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "SaveFiles/Stats/SoPlayerProgressSplineStats.h"
#include "SaveFiles/Stats/SoPlayerProgressItemStats.h"
#include "Levels/SoLevelHelper.h"

DEFINE_LOG_CATEGORY(LogSoAnalytics);

// Static constants
const FString USoAnalytics::ATTRIBUTE_FlowType(TEXT("flowType"));
const FString USoAnalytics::ATTRIBUTE_FlowSink(TEXT("sink"));
const FString USoAnalytics::ATTRIBUTE_FlowSource(TEXT("source"));
const FString USoAnalytics::ATTRIBUTE_ItemType(TEXT("itemType"));
const FString USoAnalytics::ATTRIBUTE_Currency(TEXT("currency"));
const FString USoAnalytics::ATTRIBUTE_Message(TEXT("message"));
const FString USoAnalytics::ATTRIBUTE_Value(TEXT("value"));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::StartSession(bool bInCollectGameAnalytics, bool bInWaitForAnalyticsToSend, double PollWaitSeconds, double PollProcessEventsSeconds)
{
	ModifyCanCollectStatus(bInCollectGameAnalytics, bInWaitForAnalyticsToSend, PollWaitSeconds, PollProcessEventsSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::ModifyCanCollectStatus(bool bInCollectGameAnalytics, bool bInWaitForAnalyticsToSend, double PollWaitSeconds, double PollProcessEventsSeconds)
{
	if (bCollectGameAnalytics == bInCollectGameAnalytics)
		return;

	// Was collecting, now stop
	if (bCollectGameAnalytics && !bInCollectGameAnalytics)
	{
		UE_LOG(LogSoAnalytics, Display, TEXT("Stopping previous collect session. bCollectGameAnalytics = %d, bWaitForAnalyticsToSend = %d"), bInCollectGameAnalytics, bInWaitForAnalyticsToSend);
		EndSession();
		bCollectGameAnalytics = bInCollectGameAnalytics;
		bWaitForAnalyticsToSend = bInWaitForAnalyticsToSend;
		return;
	}

	// Start normal session
	bCollectGameAnalytics = bInCollectGameAnalytics;
	bWaitForAnalyticsToSend = bInWaitForAnalyticsToSend;
	UE_LOG(LogSoAnalytics, Display, TEXT("Normal session. bCollectGameAnalytics = %d, bWaitForAnalyticsToSend = %d"), bCollectGameAnalytics, bWaitForAnalyticsToSend);
	if (!bCollectGameAnalytics)
	{
		UE_LOG(LogSoAnalytics, Display, TEXT("Analytics is DISABLED because user opted out."));
		return;
	}

	if (bInWaitForAnalyticsToSend)
		FSoAnalyticsProvider::SetPollTimers(PollWaitSeconds, PollProcessEventsSeconds);

	if (FSoAnalyticsProvider::StartSession())
	{
		FSoAnalyticsProvider::SetCustomDimensions();
		UE_LOG(LogSoAnalytics, Display, TEXT("Analytics START new session"));
	}
	else
	{
		UE_LOG(LogSoAnalytics, Error, TEXT("USoAnalytics::StartSession: Failed to start"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::EndSession()
{
	if (bCollectGameAnalytics)
	{
		UE_LOG(LogSoAnalytics, Display, TEXT("Analytics END session"));
		FSoAnalyticsProvider::EndSession();
		FlushEventsAndWait();
	}

	bCollectGameAnalytics = false;
	bWaitForAnalyticsToSend = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::FlushEventsAndWait(bool bForceWait)
{
	if (!bCollectGameAnalytics)
		return;

	FSoAnalyticsProvider::FlushEvents();
	if (bForceWait || bWaitForAnalyticsToSend)
	{
		UE_LOG(LogSoAnalytics, Display, TEXT("Analytics WAITING for analytics to send"));
		FSoAnalyticsProvider::WaitForAnalyticsToSend();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordDesignEvent(const FString& EventId)
{
	if (!bCollectGameAnalytics)
		return;

	FSoAnalyticsProvider::RecordEvent(EventId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordDesignEvent(const FString& EventId, float Value)
{
	if (!bCollectGameAnalytics)
		return;

	// AttributeName is actually the EventName
	const FString FakeEventId = TEXT("NOT_USED_") + EventId;
	const FString ValueString = FString::SanitizeFloat(Value);
	FSoAnalyticsProvider::RecordEventWithAttribute(FakeEventId, EventId, ValueString);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordPerformanceAverageFPS(int32 FPS)
{
	if (!bCollectGameAnalytics || FPS <= 0)
		return;

	static const FString EventName(TEXT("Performance:AverageFPS"));
	RecordDesignEvent(EventName, static_cast<float>(FPS));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordPerformanceSplineAverageFPS(const FString& MapName, const FString& SplineName, int32 FPS)
{
	if (!bCollectGameAnalytics || MapName.IsEmpty() || SplineName.IsEmpty() || FPS <= 0)
		return;

	const FString EventName = FString::Printf(TEXT("Performance:%s:%s:AverageFPS"), *MapName, *SplineName);
	RecordDesignEvent(EventName, static_cast<float>(FPS));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAnalytics::BuildMessageForCriticalFPSAreas(const FString& MapName, const FString& SplineName, const FSoSplineCriticalFPSAreas& Areas,
	const float AccumulateThreshold, FString& OutMessage)
{
	bool bDidSendAny = false;
	if (!bCollectGameAnalytics || MapName.IsEmpty() || SplineName.IsEmpty() || !Areas.HasAreas())
		return bDidSendAny;

	OutMessage = FString::Printf(
		TEXT("Chapter = `%s`\n")
		TEXT("Spline = `%s`\n")
		TEXT("Areas (on spline):\n"),
		*MapName, *SplineName
	);

	// Send each area
	for (const FSoSplineCriticalFPSLocation& Area : Areas.Areas)
	{
		if (!Area.IsValid() || Area.TimeSeconds < AccumulateThreshold)
			continue;

		bDidSendAny = true;
		OutMessage += FString::Printf(
			TEXT("\tAverageDistance = %d, AverageCriticalFPS = %d, TimeSeconds = %d\n"),
			Area.AverageDistance, Area.AverageCriticalFPS, FMath::TruncToInt(Area.TimeSeconds)
		);
	}

	return bDidSendAny;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordPerformanceSplineCriticalFPSAreas(const TArray<FString>& SplineMessages)
{
	if (!bCollectGameAnalytics || SplineMessages.Num() == 0)
		return;

	FString Message = FString::Printf(
		TEXT("- Critical FPS drop happened -\n")
		TEXT("CriticalFPSThreshold = %f\n")
		TEXT("ThresholdDistanceSimilar = %d\n")
		TEXT("Platform:\n%s\n")
		TEXT("Splines:\n\n"),
		ASoPlayerSpline::CriticalFPSThreshold, FSoSplineCriticalFPSLocation::ThresholdDistanceSimilar,
		*USoPlatformHelper::ToStringPlatformContext()
	);

	// Send each spline
	for (const FString& SplineMessage : SplineMessages)
		if (!SplineMessage.IsEmpty())
			Message += FString::Printf(TEXT("%s\n\n"), *SplineMessage);

	RecordErrorEvent(ESoAnalyticsErrorType::EAE_Warning, Message);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordDeathTotal(int32 DeathNum)
{
	if (!bCollectGameAnalytics || DeathNum <= 0)
		return;

	static const FString EventName(TEXT("Deaths:All"));
	RecordDesignEvent(EventName, static_cast<float>(DeathNum));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordDeathWithSoulKeeperNum(int32 DeathNum)
{
	if (!bCollectGameAnalytics || DeathNum <= 0)
		return;

	static const FString EventName(TEXT("Deaths:WithSoulKeeper"));
	RecordDesignEvent(EventName, static_cast<float>(DeathNum));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordDeathWithCheckpointNum(int32 DeathNum)
{
	if (!bCollectGameAnalytics || DeathNum <= 0)
		return;

	static const FString EventName(TEXT("Deaths:WithCheckpoint"));
	RecordDesignEvent(EventName, static_cast<float>(DeathNum));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordHPLostTotal(int32 HPNum)
{
	if (!bCollectGameAnalytics || HPNum <= 0)
		return;

	static const FString EventName(TEXT("HPLost:All"));
	RecordDesignEvent(EventName, static_cast<float>(HPNum));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordHPLostByDamageType(const TMap<ESoDmgType, int32>& LostHpByDamageTypeTable)
{
	if (!bCollectGameAnalytics || LostHpByDamageTypeTable.Num() == 0)
		return;

	static const FString BaseEventName(TEXT("HPLost:"));
	InternalRecordHPLostByDamageType(BaseEventName, LostHpByDamageTypeTable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordPlayTime(float Seconds)
{
	if (!bCollectGameAnalytics || Seconds <= 0.f)
		return;

	static const FString EventName(TEXT("Gameplay:PlayTime"));
	RecordDesignEvent(EventName, USoDateTimeHelper::SecondsToMinutes(Seconds));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordSplineEnterNum(const FString& MapName, const FString& SplineName, int32 EnterNum)
{
	if (!bCollectGameAnalytics || MapName.IsEmpty() || SplineName.IsEmpty() || EnterNum <= 0)
		return;

	const FString EventName = FString::Printf(TEXT("Gameplay:%s:%s:EnterNumber"), *MapName, *SplineName);
	RecordDesignEvent(EventName, static_cast<float>(EnterNum));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordSplineTimeSpent(const FString& MapName, const FString& SplineName, float Seconds)
{
	if (!bCollectGameAnalytics || MapName.IsEmpty() || SplineName.IsEmpty() || Seconds <= 0.f)
		return;

	const FString EventName = FString::Printf(TEXT("Gameplay:%s:%s:TimeSpent"), *MapName, *SplineName);
	RecordDesignEvent(EventName, USoDateTimeHelper::SecondsToMinutes(Seconds));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordSplineTimeFirstEnter(const FString& MapName, const FString& SplineName, float Seconds)
{
	if (!bCollectGameAnalytics || MapName.IsEmpty() || SplineName.IsEmpty() || Seconds < 0.f)
		return;

	const FString EventName = FString::Printf(TEXT("Gameplay:%s:%s:TimeFirstEnter"), *MapName, *SplineName);
	RecordDesignEvent(EventName, USoDateTimeHelper::SecondsToMinutes(Seconds));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordSplineDeath(const FString& MapName, const FString& SplineName, const FSoSplineDeathContext& Context)
{
	if (!bCollectGameAnalytics || MapName.IsEmpty() || SplineName.IsEmpty())
		return;

	const FString BaseEventName = FString::Printf(TEXT("Deaths:%s:%s:"), *MapName, *SplineName);
	InternalRecordGameplaySplineDeath(BaseEventName, MapName, SplineName, Context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordSplineDeaths(const FString& MapName, const FString& SplineName, const TArray<FSoSplineDeathContext>& DeathData)
{
	if (!bCollectGameAnalytics || MapName.IsEmpty() || SplineName.IsEmpty() || DeathData.Num() == 0)
		return;

	const FString BaseEventName = FString::Printf(TEXT("Deaths:%s:%s:"), *MapName, *SplineName);
	for (const FSoSplineDeathContext& Context : DeathData)
		InternalRecordGameplaySplineDeath(BaseEventName, MapName, SplineName, Context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordSplineHPLostTotal(const FString& MapName, const FString& SplineName, int32 HPNum)
{
	if (!bCollectGameAnalytics || HPNum <= 0 || MapName.IsEmpty() || SplineName.IsEmpty())
		return;

	const FString EventName = FString::Printf(TEXT("HPLost:%s:%s:All"), *MapName, *SplineName);
	RecordDesignEvent(EventName, static_cast<float>(HPNum));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordSplineHPLostByDamageType(const FString& MapName, const FString& SplineName, const TMap<ESoDmgType, int32>& LostHpByDamageTypeTable)
{
	if (!bCollectGameAnalytics || LostHpByDamageTypeTable.Num() == 0 || MapName.IsEmpty() || SplineName.IsEmpty())
		return;

	const FString BaseEventName = FString::Printf(TEXT("HPLost:%s:%s:"), *MapName, *SplineName);
	InternalRecordHPLostByDamageType(BaseEventName, LostHpByDamageTypeTable);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::InternalRecordGameplaySplineDeath(const FString& BaseEventName, const FString& MapName, const FString& SplineName, const FSoSplineDeathContext& Context)
{
	if (!bCollectGameAnalytics || MapName.IsEmpty() || SplineName.IsEmpty())
		return;

	// Distance along spline
	{
		const FString EventName = BaseEventName + TEXT("Distance");
		RecordDesignEvent(EventName, static_cast<float>(Context.Distance));
	}

	// Absolute time of death
	{
		const FString EventName = BaseEventName + TEXT("Time");
		RecordDesignEvent(EventName, USoDateTimeHelper::SecondsToMinutes(Context.TimeSeconds));
	}

	// Has soul keeper
	if (Context.bSoulKeeperActive)
	{
		const FString EventName = BaseEventName + TEXT("SoulKeeperActive");
		RecordDesignEvent(EventName);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::InternalRecordHPLostByDamageType(const FString& BaseEventName, const TMap<ESoDmgType, int32>& LostHpByDamageTypeTable)
{
	if (!bCollectGameAnalytics || LostHpByDamageTypeTable.Num() == 0)
		return;

	for (const auto& KeyValue : LostHpByDamageTypeTable)
	{
		const ESoDmgType DamageType = KeyValue.Key;
		const int32 LostHP = KeyValue.Value;
		if (DamageType >= ESoDmgType::Max || LostHP <= 0)
			continue;

		// Send for each damage type
		const FString DamageName = USoStringHelper::DamageTypeToFriendlyString(DamageType);
		const FString EventName = BaseEventName + DamageName;
		RecordDesignEvent(EventName, static_cast<float>(LostHP));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordMilestone(const FString& MapName, const FString& MilestoneName, const float Seconds)
{
	if (!bCollectGameAnalytics || MapName.IsEmpty() || MilestoneName.IsEmpty() || Seconds < 0.f)
		return;

	const FString EventName = FString::Printf(TEXT("Milestones:%s:%s"), *MapName, *MilestoneName);
	RecordDesignEvent(EventName, USoDateTimeHelper::SecondsToMinutes(Seconds));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordMilestone(const FString& MapName, const FString& MilestoneName)
{
	if (!bCollectGameAnalytics || MapName.IsEmpty() || MilestoneName.IsEmpty())
		return;

	const FString EventName = FString::Printf(TEXT("Milestones:%s:%s"), *MapName, *MilestoneName);
	RecordDesignEvent(EventName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordItemUsageTime(const FString& ItemName, const FSoPlayerProgressItemUsageStats& Usage)
{
	if (!bCollectGameAnalytics || ItemName.IsEmpty() || !Usage.CanBeSent())
		return;

	const int32 TimeUsedSeconds = Usage.GetTimeUsedSeconds();
	if (TimeUsedSeconds > 0)
	{
		const FString EventName = FString::Printf(TEXT("Items:%s:TimeUsed"), *ItemName);
		RecordDesignEvent(EventName, USoDateTimeHelper::SecondsToMinutes(TimeUsedSeconds));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordItemUsageTimes(const FString& ItemName, const TArray<FSoPlayerProgressItemUsageStats>& Usages)
{
	if (!bCollectGameAnalytics || ItemName.IsEmpty() || Usages.Num() == 0)
		return;

	for (const FSoPlayerProgressItemUsageStats& Usage : Usages)
		RecordItemUsageTime(ItemName, Usage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordSpellCasted(const FString& SpellName, int32 CastAmount)
{
	if (!bCollectGameAnalytics || SpellName.IsEmpty() || CastAmount <= 0)
		return;

	const FString EventName = FString::Printf(TEXT("Spells:%s:CastCount"), *SpellName);
	RecordDesignEvent(EventName, static_cast<float>(CastAmount));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordUIEpisodeStart()
{
	if (!bCollectGameAnalytics)
		return;

	static const FString EventName(TEXT("UI:Episode:Start"));
	RecordDesignEvent(EventName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordUISaveLoad()
{
	if (!bCollectGameAnalytics)
		return;

	static const FString EventName(TEXT("UI:Save:Load"));
	RecordDesignEvent(EventName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordUISaveRemove()
{
	if (!bCollectGameAnalytics)
		return;

	static const FString EventName(TEXT("UI:Save:Remove"));
	RecordDesignEvent(EventName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordUISaveStartNewGame()
{
	if (!bCollectGameAnalytics)
		return;

	static const FString EventName(TEXT("UI:Save:StartNewGame"));
	RecordDesignEvent(EventName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordEpisodeCompleted(const FString& EpisodeName, float Seconds)
{
	if (!bCollectGameAnalytics || EpisodeName.IsEmpty() || Seconds < 1.f || !USoLevelHelper::IsValidEpisodeName(FName(*EpisodeName)))
		return;

	const FString EventName = FString::Printf(TEXT("Episode:%s:Progress:Completed"), *EpisodeName);
	RecordDesignEvent(EventName, USoDateTimeHelper::SecondsToMinutes(Seconds));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::RecordErrorEvent(ESoAnalyticsErrorType ErrorType, const FString& Message)
{
	if (!bCollectGameAnalytics)
		return;

	const FString Error = GetStringFromAnalyticsErrorType(ErrorType);

#if WARRIORB_WITH_ANALYTICS
	const TArray<FAnalyticsEventAttribute> Attributes = {
		{ ATTRIBUTE_Message, Message }
	};

	FSoAnalyticsProvider::RecordErrorWithAttributes(Error, Attributes);
#endif // WARRIORB_WITH_ANALYTICS
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::UpdateMapProgress(ESoAnalyticsProgressType ProgressType, const FString& MapName, float TimeSeconds)
{
	if (!bCollectGameAnalytics)
		return;

	const TArray<FString> ProgressHierarchy = { MapName };
	UpdateProgress(ProgressType, ProgressHierarchy, TimeSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::UpdateLevelProgress(ESoAnalyticsProgressType ProgressType, const FString& MapName, const FString& LevelName, float TimeSeconds)
{
	if (!bCollectGameAnalytics)
		return;

	const TArray<FString> ProgressHierarchy = { MapName, LevelName };
	UpdateProgress(ProgressType, ProgressHierarchy, TimeSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::UpdateSplineProgress(ESoAnalyticsProgressType ProgressType, const FString& MapName, const FString& LevelName, const FString& SplineName, float TimeSeconds)
{
	if (!bCollectGameAnalytics)
		return;

	const TArray<FString> ProgressHierarchy = { MapName, LevelName, SplineName };
	UpdateProgress(ProgressType, ProgressHierarchy, TimeSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::UpdateProgress(ESoAnalyticsProgressType ProgressType, const TArray<FString>& ProgressHierarchy, float TimeSeconds)
{
	if (!bCollectGameAnalytics)
		return;

#if WARRIORB_WITH_ANALYTICS
	check(ProgressHierarchy.Num() > 0);
	const FString ProgressString = GetStringFromAnalyticsProgressType(ProgressType);
	const FString TimeString = FString::SanitizeFloat(USoDateTimeHelper::SecondsToMinutes(TimeSeconds));
	const TArray<FAnalyticsEventAttribute> Attributes = {
		{ ATTRIBUTE_Value, TimeString }
	};

	FSoAnalyticsProvider::RecordProgressWithFullHierarchyAndAttributes(ProgressString, ProgressHierarchy, Attributes);
#endif // WARRIORB_WITH_ANALYTICS
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::AddResourceCurrency(const FSoResourceCurrency& ResourceCurrency)
{
	if (!bCollectGameAnalytics)
		return;

	// Add, source of money
	ModifyResourceCurrency(ATTRIBUTE_FlowSource, ResourceCurrency);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::SubtractResourceCurrency(const FSoResourceCurrency& ResourceCurrency)
{
	if (!bCollectGameAnalytics)
		return;

	// Subtract, sink of money
	ModifyResourceCurrency(ATTRIBUTE_FlowSink, ResourceCurrency);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalytics::ModifyResourceCurrency(const FString& FlowType, const FSoResourceCurrency& ResourceCurrency)
{
	if (!bCollectGameAnalytics)
		return;

	// Ignore 0 change
	if (ResourceCurrency.CurrencyQuantity == 0)
		return;

#if WARRIORB_WITH_ANALYTICS
	const TArray<FAnalyticsEventAttribute> Attributes = {
		{ ATTRIBUTE_FlowType, FlowType },
		{ ATTRIBUTE_ItemType, GetStringFromResourceItemType(ResourceCurrency.ItemType) },
		{ ATTRIBUTE_Currency, GetStringFromResourceCurrencyType(ResourceCurrency.CurrencyType) }
	};

	// itemQuantity is actually CurrencyQuantity
	FSoAnalyticsProvider::RecordItemPurchaseWithAttributes(ResourceCurrency.ItemID, FMath::Abs(ResourceCurrency.CurrencyQuantity), Attributes);
#endif // WARRIORB_WITH_ANALYTICS
}
