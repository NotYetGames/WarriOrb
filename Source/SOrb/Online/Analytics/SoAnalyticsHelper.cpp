// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoAnalyticsHelper.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "SoAnalyticsComponent.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerProgress.h"
#include "SaveFiles/Stats/SoPlayerProgressStats.h"
#include "Basic/SoGameInstance.h"
#include "Levels/SoLevelHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoAnalyticsHelper, All, All)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsHelper::RecordGameplayMilestone(const UObject* WorldContextObject, const FName MilestoneName, bool bAttachPlayTime)
{
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (Character == nullptr)
	{
		UE_LOG(LogSoAnalyticsHelper, Error, TEXT("USoAnalyticsHelper::RecordGameplayMileStone. Could not get character"));
		return;
	}

	USoAnalyticsComponent* AnalyticsComponent = Character->GetAnalyticsComponent();
	if (AnalyticsComponent == nullptr)
		return;

	AnalyticsComponent->RecordGameplayMilestone(MilestoneName, bAttachPlayTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsHelper::RecordGameplayMilestoneIfItIsNot(const UObject* WorldContextObject, FName MilestoneName, bool bAttachPlayTime)
{
	if (!IsGameplayMilestoneRecorded(WorldContextObject, MilestoneName))
		RecordGameplayMilestone(WorldContextObject, MilestoneName, bAttachPlayTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAnalyticsHelper::IsGameplayMilestoneRecorded(const UObject* WorldContextObject, FName MilestoneName)
{
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (Character == nullptr || Character->GetPlayerProgress() == nullptr)
		return false;

	const FName MapName = USoLevelHelper::GetMapNameFromObject(WorldContextObject);
	const FSoPlayerProgressStats& AllSessions = Character->GetPlayerProgress()->GetStatsFromAllSession();
	return AllSessions.HasMapMilestone(MapName, MilestoneName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsHelper::RecordAddGold(const UObject* WorldContextObject, int32 GoldAmount, ESoResourceItemType ItemType, const FString& ItemID)
{
	if (USoAnalytics* Analytics = USoGameInstance::Get(WorldContextObject).GetAnalytics())
		Analytics->AddGold(GoldAmount, ItemType, ItemID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAnalyticsHelper::RecordSubtractGold(const UObject* WorldContextObject, int32 GoldAmount, ESoResourceItemType ItemType, const FString& ItemID)
{
	if (USoAnalytics* Analytics = USoGameInstance::Get(WorldContextObject).GetAnalytics())
		Analytics->SubtractGold(GoldAmount, ItemType, ItemID);
}
