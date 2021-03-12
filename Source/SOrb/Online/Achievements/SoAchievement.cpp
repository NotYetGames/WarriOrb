// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAchievement.h"

#if WARRIORB_WITH_ONLINE
#include "Interfaces/OnlineAchievementsInterface.h"
#endif // WARRIORB_WITH_ONLINE

#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoStringHelper.h"
#include "SoAchievementSettings.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "Math/UnrealMathUtility.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoAchievement, All, All)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName FSoAchievement::SteamStatTypeToName(const ESoSteamStatType Type)
{
	FString EnumValue;
	if (USoStringHelper::ConvertEnumToString<ESoSteamStatType>(TEXT("ESoSteamStatType"), Type, false, EnumValue))
		return FName(*EnumValue);

	return NAME_None;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const TSet<FName>& FSoAchievement::GetAllAchievementNames()
{
	static TSet<FName> AllNamesSet;
	if (AllNamesSet.Num() == 0)
	{
		if (USoPlatformHelper::IsDemo())
		{
			AllNamesSet.Append(GetDefault<USoAchievementSettings>()->GetDemoAchievementNames());
		}
		else
		{
			AllNamesSet.Append(GetDefault<USoAchievementSettings>()->GetAchievementNames());
		}
	}

	return AllNamesSet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoAchievement::WouldMakeItUnlocked(float FutureProgress) const
{
	FutureProgress = GetClampOfProgress(FutureProgress);
	return FMath::IsNearlyEqual(FutureProgress, GetMaxProgressAsFloat());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoAchievement::SetProgress(float InProgress)
{
	Progress = GetClampOfProgress(InProgress);
	UnlockTime = {};

	if (IsUnlocked())
		UnlockTime = FDateTime::UtcNow();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoAchievement::Unlock()
{
	SetProgress(GetMaxProgressAsFloat());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoAchievement::ToString() const
{
	return FString::Printf(
		TEXT("Name=%s, Progress=%f (IsUnlocked = %s), UnlockTime=%s, bSetBySteam = %s\n")
		TEXT("Title=%s, LockedDesc=%s, UnlockedDesc=%s"),
		*Name.ToString(),
		Progress,
		IsUnlocked() ? TEXT("true") : TEXT("false"),
		*UnlockTime.ToString(),
		bSetBySteam ? TEXT("true") : TEXT("false"),
		*Title.ToString(),
		*LockedDesc.ToString(),
		*UnlockedDesc.ToString()
	);
}
