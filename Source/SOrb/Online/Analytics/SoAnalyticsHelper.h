// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoAnalytics.h"

#include "SoAnalyticsHelper.generated.h"

UCLASS()
class SORB_API USoAnalyticsHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Records a game play milestone for the current Map.
	 * If bAttachPlayTime it will also attach to this milestone the session total play seconds.
	 * Each milestone must be unique in each Map for it to be sent to the analytics server.
	 */
	UFUNCTION(BlueprintCallable, Category = SoAnalytics, meta = (WorldContext = "WorldContextObject"))
	static void RecordGameplayMilestone(const UObject* WorldContextObject, FName MilestoneName, bool bAttachPlayTime = true);

	/** Records game play milestone for the current Map with time - if it was not recorded before */
	UFUNCTION(BlueprintCallable, Category = SoAnalytics, meta = (WorldContext = "WorldContextObject"))
	static void RecordGameplayMilestoneIfItIsNot(const UObject* WorldContextObject, FName MilestoneName, bool bAttachPlayTime = true);

	/** Checks if a milestone is recorded for the current Map. */
	UFUNCTION(BlueprintPure, Category = SoAnalytics, meta = (WorldContext = "WorldContextObject"))
	static bool IsGameplayMilestoneRecorded(const UObject* WorldContextObject, FName MilestoneName);

	/** Recording the adding of gold. */
	UFUNCTION(BlueprintCallable, Category = SoAnalytics, meta = (WorldContext = "WorldContextObject"))
	static void RecordAddGold(const UObject* WorldContextObject, int32 GoldAmount, ESoResourceItemType ItemType, const FString& ItemID);

	/** Recording the loosing of gold. */
	UFUNCTION(BlueprintCallable, Category = SoAnalytics, meta = (WorldContext = "WorldContextObject"))
	static void RecordSubtractGold(const UObject* WorldContextObject, int32 GoldAmount, ESoResourceItemType ItemType, const FString& ItemID);
};
