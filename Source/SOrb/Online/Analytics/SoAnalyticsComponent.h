// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SplineLogic/SoPlayerSpline.h"

#include "SoAnalyticsComponent.generated.h"

class ASoCharacter;
class USoAnalytics;

// Actor component that follows the player and updates the analytics sent
// This is used in additions to the USoPlayerProgress components
UCLASS(ClassGroup=(Custom))
class SORB_API USoAnalyticsComponent : public UActorComponent
{
	GENERATED_BODY()
	typedef USoAnalyticsComponent Self;
public:
	// Sets default values for this component's properties
	USoAnalyticsComponent();

	// UActorComponent Interface

	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Own methods

	// See comment in USoAnalyticsHelper::RecordGameplayMilestone
	UFUNCTION(BlueprintCallable, Category = Analytics)
	void RecordGameplayMilestone(FName MilestoneName, bool bAttachPlayTime);

	void FireAllRecordIntervals()
	{
		if (bCollectGameAnalytics && Analytics)
		{
			if (WaitTimeSecondsPerformance > 0.f)
			{
				IntervalPerformanceFinished(true);
				WaitTimeSecondsPerformance = 0.f;
			}
			if (WaitTimeSecondsCriticalPerformance > 0.f)
			{
				IntervalCriticalPerformanceFinished(true);
				WaitTimeSecondsCriticalPerformance = 0.f;
			}
			if (WaitTimeSecondsGameplay > 0.f)
			{
				IntervalGameplayFinished(true);
				WaitTimeSecondsGameplay = 0.f;
			}
		}
	}

	void SetCanCollectAnalytics(bool bInCollect);

protected:
	void OnPreLoadPlayerProgress();
	void OnPreSavePlayerProgress();

	UFUNCTION()
	void OnPlayerSplineChanged(const ASoSpline* OldSpline, const ASoSpline* NewSpline);

	void IntervalPerformanceFinished(bool bFlushAll = false);
	void IntervalCriticalPerformanceFinished(bool bFlushAll = false);
	void IntervalGameplayFinished(bool bFlushAll = false);

	FName GetSafeMapNameFromSpline(const ASoPlayerSpline* PlayerSpline) const;

protected:
	// Our master
	UPROPERTY(BlueprintReadOnly)
	ASoCharacter* SoOwner = nullptr;

	UPROPERTY(BlueprintReadOnly)
	USoAnalytics* Analytics = nullptr;

	// Can collect data
	UPROPERTY(VisibleAnywhere)
	bool bCollectGameAnalytics = false;

	// NOTE: These variables get set/reset at every IntervalSecondsPerformance

	// Performance variables
	int32 FrameTickForAverageNum = 0;

	float LastTimeSecondsUpdateAverage = 0.f;
	float WaitTimeSecondsPerformance = 0.f;
	float WaitTimeSecondsCriticalPerformance = 0.f;
	float WaitTimeSecondsGameplay = 0.f;

	// Holds the absolute current time
	float CurrentTimeSeconds = 0.f;

public:
	// At what interval are we submitting the performance data at, every 2 minutes, itsh
	static constexpr float IntervalSecondsPerformance = 60.f * 2.1f;

	// At what interval are we submitting the critical performance data at, once every 30 minutes, itsh
	static constexpr float IntervalSecondsCriticalPerformance = 60.f * 27.9f;

	// Interval we are submitting our gameplay data at.
	static constexpr float IntervalSecondsGameplay = 60.f;

	// Will only send spline performance data if it has at least this amount of seconds spent on it
	static constexpr float SplineAccumulatePerformanceThresholdSeconds = 5.0f;

	// Will only send spline gameplay data if it has at least this amount of seconds spent on it
	static constexpr float SplineAccumulateGamePlayThresholdSeconds = 2.0f;
};
