// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoAnalyticsComponent.h"

#include "EngineUtils.h"
#include "Misc/FileHelper.h"

#include "SplineLogic/SoPlayerSpline.h"
#include "Character/SoCharacter.h"
#include "Settings/SoGameSettings.h"
#include "SoAnalytics.h"
#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "Character/SoPlayerProgress.h"
#include "Levels/SoLevelHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
USoAnalyticsComponent::USoAnalyticsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::BeginPlay()
{
	Super::BeginPlay();
	SoOwner = CastChecked<ASoCharacter>(GetOwner());

	CurrentTimeSeconds = 0.f;
	WaitTimeSecondsPerformance = 0.f;
	WaitTimeSecondsCriticalPerformance = 0.f;
	WaitTimeSecondsGameplay = 0.f;
	LastTimeSecondsUpdateAverage = 0.f;
	bCollectGameAnalytics = false;
	Analytics = nullptr;

	// Analytics
	SetCanCollectAnalytics(USoGameSettings::Get().CanCollectGameAnalytics());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetCanCollectAnalytics(false);
	Super::EndPlay(EndPlayReason);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (!bCollectGameAnalytics)
		return;

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float UndilatedDeltaSeconds = USoDateTimeHelper::GetDeltaSeconds();
	FrameTickForAverageNum++;
	CurrentTimeSeconds += UndilatedDeltaSeconds;
	WaitTimeSecondsPerformance += UndilatedDeltaSeconds;
	WaitTimeSecondsCriticalPerformance += UndilatedDeltaSeconds;
	WaitTimeSecondsGameplay += UndilatedDeltaSeconds;

	// TODO Handle sending the rest of the events when the Map ends, because of the interval this might not happen immediately

	// Can do update
	if (WaitTimeSecondsPerformance > IntervalSecondsPerformance)
	{
		IntervalPerformanceFinished();
		WaitTimeSecondsPerformance = 0.f;
	}
	if (WaitTimeSecondsCriticalPerformance > IntervalSecondsCriticalPerformance)
	{
		IntervalCriticalPerformanceFinished();
		WaitTimeSecondsCriticalPerformance = 0.f;
	}
	if (WaitTimeSecondsGameplay > IntervalSecondsGameplay)
	{
		IntervalGameplayFinished();
		WaitTimeSecondsGameplay = 0.f;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::SetCanCollectAnalytics(bool bInCollect)
{
	if (!SoOwner || bCollectGameAnalytics == bInCollect)
		return;

	auto SetCollectDisabled = [this]()
	{
		// Flush
		FireAllRecordIntervals();
		if (SoOwner && SoOwner->GetPlayerProgress())
		{
			SoOwner->OnPlayerSplineChanged.RemoveDynamic(this, &Self::OnPlayerSplineChanged);
			SoOwner->GetPlayerProgress()->OnPreLoad().RemoveAll(this);
			SoOwner->GetPlayerProgress()->OnPreSave().RemoveAll(this);
		}
		bCollectGameAnalytics = false;
	};

	// Kill previous
	if (bCollectGameAnalytics && !bInCollect)
	{
		SetCollectDisabled();
		SetComponentTickEnabled(false);
		return;
	}

	// Normal set, first time?
	bCollectGameAnalytics = bInCollect;
	if (bCollectGameAnalytics)
	{
		// NOTE: Disable anlytics here for episodes if we want that?
		Analytics = USoGameInstance::Get(this).GetAnalytics();
		if (SoOwner && SoOwner->GetPlayerProgress())
		{
			SoOwner->OnPlayerSplineChanged.AddDynamic(this, &Self::OnPlayerSplineChanged);
			SoOwner->GetPlayerProgress()->OnPreLoad().AddUObject(this, &Self::OnPreLoadPlayerProgress);
			SoOwner->GetPlayerProgress()->OnPreSave().AddUObject(this, &Self::OnPreSavePlayerProgress);
		}
		else
		{
			UE_LOG(LogSoAnalytics, Error, TEXT("Disabling analytics because we can't get the SoOWner"));
			bCollectGameAnalytics = false;
		}
	}
	else
	{
		// Disabled
		SetCollectDisabled();
	}

	// Tick only when we can
	SetComponentTickEnabled(bCollectGameAnalytics);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::RecordGameplayMilestone(FName MilestoneName, bool bAttachPlayTime)
{
	if (!SoOwner || !SoOwner->GetPlayerProgress())
		return;

	// Map is current one from this world
	// TODO maybe edge cases when milestone is changing map?
	const FName MapFName = USoLevelHelper::GetMapNameFromObject(this);
	const FString MapName = MapFName.ToString();

	// Already exists, nothing to record
	FSoPlayerProgressStats& AllSessions = SoOwner->GetPlayerProgress()->GetStatsFromAllSession();
	if (AllSessions.HasMapMilestone(MapFName, MilestoneName))
	{
		UE_LOG(LogSoAnalytics,
			   Warning,
			   TEXT("RecordGameplayMilestone: MilestoneName = `%s` already recorded. Wrong MilestoneName in game maybe? Anyways, ignoring"),
			   *MilestoneName.ToString());
		return;
	}

	// Record new milestone
	if (bAttachPlayTime)
		AllSessions.AddMapMilestone(MapFName, MilestoneName, AllSessions.TotalPlayTimeSeconds);
	else
		AllSessions.AddMapMilestone(MapFName, MilestoneName);

	// Send analytics if enabled
	if (bCollectGameAnalytics && Analytics)
	{
		const FString MilestoneNameStr = MilestoneName.ToString();
		if (bAttachPlayTime)
			Analytics->RecordMilestone(MapName, MilestoneNameStr, AllSessions.TotalPlayTimeSeconds);
		else
			Analytics->RecordMilestone(MapName, MilestoneNameStr);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::OnPreLoadPlayerProgress()
{
	// Fire everything we can as the sessions will get cleared
	FireAllRecordIntervals();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::OnPreSavePlayerProgress()
{
	// Mostly useful for performance
	FireAllRecordIntervals();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::OnPlayerSplineChanged(const ASoSpline* OldSpline, const ASoSpline* NewSpline)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::IntervalPerformanceFinished(bool bFlushAll)
{
	if (!bCollectGameAnalytics || !Analytics || !SoOwner || !SoOwner->GetPlayerProgress())
		return;

	const float TimeSinceAverageUpdate = CurrentTimeSeconds - LastTimeSecondsUpdateAverage;
	if (TimeSinceAverageUpdate < 1.0f)
		return;

	// FMath::RoundToInt(FrameTickForAverageNum / IntervalSecondsPerformance);
	FSoPlayerProgressStats& AllSessions = SoOwner->GetPlayerProgress()->GetStatsFromAllSession();
	const int32 AverageFPSSinceUpdate = FMath::RoundToInt(FrameTickForAverageNum / TimeSinceAverageUpdate);

	// Reset for next interval
	FrameTickForAverageNum = 0;
	LastTimeSecondsUpdateAverage = CurrentTimeSeconds;

	// Record performance for each spline
	for (TActorIterator<ASoPlayerSpline> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		ASoPlayerSpline* PlayerSpline = *ActorItr;

		// On flush, simply flush all :shrug:
		const bool bHasAveragePerformanceStats = (bFlushAll && PlayerSpline->HasAveragePerformanceStats()) ||
												PlayerSpline->HasAveragePerformanceStats(SplineAccumulatePerformanceThresholdSeconds);
		if (!bHasAveragePerformanceStats)
			continue;

		const FName MapFName = GetSafeMapNameFromSpline(PlayerSpline);
		const FString MapName = MapFName.ToString();
		const FName SplineFName = PlayerSpline->GetSplineFName();
		const FString SplineName = SplineFName.ToString();

		// Average FPS
		const int32 AverageFPSSpline = FMath::RoundToInt(PlayerSpline->GetAverageFPS());
		AllSessions.UpdateMapSplineAverageFPS(MapFName, SplineFName, AverageFPSSpline);
		Analytics->RecordPerformanceSplineAverageFPS(MapName, SplineName, AverageFPSSpline);

		// Reset for next interval
		PlayerSpline->ResetAveragePerformanceStats();
	}

	AllSessions.UpdateTotalAverageFPS(AverageFPSSinceUpdate);
	Analytics->RecordPerformanceAverageFPS(AverageFPSSinceUpdate);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::IntervalCriticalPerformanceFinished(bool bFlushAll)
{
	if (!bCollectGameAnalytics || !Analytics || !SoOwner || !SoOwner->GetPlayerProgress())
		return;

	FSoPlayerProgressStats& AllSessions = SoOwner->GetPlayerProgress()->GetStatsFromAllSession();

	// Record performance for each spline
	TArray<FString> SplineMessages;
	for (TActorIterator<ASoPlayerSpline> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		ASoPlayerSpline* PlayerSpline = *ActorItr;
		if (!PlayerSpline->HasCriticalPerformanceStats())
			continue;

		const FName MapFName = GetSafeMapNameFromSpline(PlayerSpline);
		const FString MapName = MapFName.ToString();
		const FName SplineFName = PlayerSpline->GetSplineFName();
		const FString SplineName = SplineFName.ToString();

		// Critical FPS
		const FSoSplineCriticalFPSAreas& FPSAreas = PlayerSpline->GetCriticalFPSAreas();
		AllSessions.UpdateMapSplineCriticalFPSAreas(MapFName, SplineFName, FPSAreas);

		// Mark for sending
		if (AllSessions.CanMapSplineCriticalFPSAreasBeSent(MapFName, SplineFName))
		{
			// Group them
			FString Message;
			if (Analytics->BuildMessageForCriticalFPSAreas(MapName, SplineName, FPSAreas, SplineAccumulatePerformanceThresholdSeconds, Message))
			{
				SplineMessages.Add(Message);
				AllSessions.IncrementMapSplineCriticalFPSAreasSentRecordsNum(MapFName, SplineFName);
			}
		}

		// Reset for next interval
		PlayerSpline->ResetCriticalPerformanceStats();
		PlayerSpline->ResetCriticalFPSAreas();
	}

	// Send them all
	Analytics->RecordPerformanceSplineCriticalFPSAreas(SplineMessages);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsComponent::IntervalGameplayFinished(bool bFlushAll)
{
	if (!bCollectGameAnalytics || !Analytics || !SoOwner)
		return;

	USoPlayerProgress* PlayerProgress = SoOwner->GetPlayerProgress();
	if (!PlayerProgress)
		return;

	// Record the temp analytics
	FSoPlayerProgressStats& TempAnalyticsStats = PlayerProgress->GetTempAnalyticsStats();

	// Useful from current session
	Analytics->RecordDeathTotal(TempAnalyticsStats.TotalDeathNum);
	Analytics->RecordDeathWithSoulKeeperNum(TempAnalyticsStats.TotalDeathWithSoulKeeperNum);
	Analytics->RecordDeathWithCheckpointNum(TempAnalyticsStats.TotalDeathWithCheckpointNum);
	Analytics->RecordHPLostTotal(TempAnalyticsStats.TotalLostHp);
	Analytics->RecordHPLostByDamageType(TempAnalyticsStats.TotalLostHpByDamageTypeTable);
	Analytics->RecordPlayTime(TempAnalyticsStats.TotalPlayTimeSeconds);

	// Reset for next interval
	TempAnalyticsStats.ResetGameplayTotalVariables();

	// For each Map
	for (auto& KeyValue : TempAnalyticsStats.MapsProgressTable)
	{
		const FName MapFName = KeyValue.Key;

		// Record or each Spline in Map
		for (auto& ElemSpline : KeyValue.Value.Splines)
		{
			const FName SplineFName = ElemSpline.Key;
			FSoPlayerProgressSplineStats& SplineStats = ElemSpline.Value;
			if (bFlushAll || SplineStats.HasStatsForAtLeast(SplineAccumulateGamePlayThresholdSeconds))
			{
				const FString SplineName = SplineFName.ToString();
				const FString MapName = MapFName.ToString();
				Analytics->RecordSplineEnterNum(MapName, SplineName, SplineStats.EnterNum);
				Analytics->RecordSplineTimeSpent(MapName, SplineName, FMath::RoundToInt(SplineStats.TimeSpentSeconds));
				Analytics->RecordSplineDeaths(MapName, SplineName, SplineStats.DeathData);
				Analytics->RecordSplineHPLostTotal(MapName, SplineName, SplineStats.LostHp);
				Analytics->RecordSplineHPLostByDamageType(MapName, SplineName, SplineStats.LostHpByDamageTypeTable);

				// Reset for next interval
				SplineStats = {};
			}
		}
	}

	// Record spells stats
	for (const auto& Elem : TempAnalyticsStats.SpellsStatsTable)
	{
		Analytics->RecordSpellCasted(Elem.Key.ToString(), Elem.Value.CastNum);
	}
	TempAnalyticsStats.SpellsStatsTable.Empty();

	// Done one each spline
	//PlayerProgress->ResetTempAnalyticsStats();

	// Record spline first enter time for each Map
	// NOTE: CurrentSession does not contain this information or is useless for us.
	FSoPlayerProgressStats& AllSessions = PlayerProgress->GetStatsFromAllSession();
	for (auto& KeyValue : AllSessions.MapsProgressTable)
	{
		const FString MapName = KeyValue.Key.ToString();
		const TMap<FName, int32> SplinesFromCurrentSession = KeyValue.Value.GetAllSplinesFirstTimeEnterForSending();

		// Record each spline
		for (const auto& SplineElem : SplinesFromCurrentSession)
		{
			const FString SplineName = SplineElem.Key.ToString();
			const int32 TimeFirstEnterSeconds = SplineElem.Value;
			Analytics->RecordSplineTimeFirstEnter(MapName, SplineName, TimeFirstEnterSeconds);
		}

		KeyValue.Value.MarkAllSplinesFirstTimeEnterAsSent();
	}

	// Record Item Usage Times
	// NOTE: CurrentSession does not contain this information or is useless for us.
	for (auto& ItemElem : AllSessions.ItemsStatsTable)
	{
		const FName ItemFName = ItemElem.Key;
		if (ItemFName == NAME_None)
			continue;

		Analytics->RecordItemUsageTimes(ItemFName.ToString(), ItemElem.Value.Usages);
		ItemElem.Value.MarkAllItemsUsageAsSent();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoAnalyticsComponent::GetSafeMapNameFromSpline(const ASoPlayerSpline* PlayerSpline) const
{
	const FName MapName = USoLevelHelper::GetMapNameFromObject(PlayerSpline);
	if (MapName == NAME_None)
		return USoLevelHelper::GetMapNameFromObject(this);

	return MapName;
}
