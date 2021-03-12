// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoAchievementManager.h"
#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "SoAchievement.h"

#if WARRIORB_WITH_ONLINE
#include "Interfaces/OnlineAchievementsInterface.h"
#include "OnlineSubsystemUtils/Classes/AchievementWriteCallbackProxy.h"
#endif // WARRIORB_WITH_ONLINE

#if WARRIORB_WITH_STEAM
#include "INotYetSteamModule.h"
#endif // WARRIORB_WITH_STEAM

#include "DlgDialogueParticipant.h"

#include "Online/SoOnlineHelper.h"
#include "SoAchievementSettings.h"
#include "SaveFiles/SoWorldState.h"
#include "SaveFiles/SoSaveParser.h"
#include "SaveFiles/SoSaveWriter.h"
#include "SaveFiles/SoSaveHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoAchievementManager, All, All)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAchievementManager::USoAchievementManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAchievementManager::~USoAchievementManager()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAchievementManager* USoAchievementManager::GetInstance(const UObject* WorldContextObject)
{
	return USoGameInstance::Get(WorldContextObject).GetAchievementManager();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAchievementManager::AreSteamAchievementsAvailable()
{
	return USoPlatformHelper::IsSteamInitialized();
}

#if WARRIORB_WITH_STEAM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INYSteamStatsAndAchievementsPtr USoAchievementManager::GetSteamAchievements()
{
	INYSteamSubsystemPtr Subsystem = INotYetSteamModule::Get().GetSteamSubsystem();
	if (Subsystem.IsValid())
	{
		return Subsystem->GetStatsAndAchievements();
	}

	return nullptr;
}
#endif // WARRIORB_WITH_STEAM

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::Tick(float DeltaSeconds)
{
	//if (!ValidateCurrentState(false))
	//	return;

	// Fill achievements once
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAchievementManager::Initialize(APlayerController* InController)
{
	UE_LOG(LogSoAchievementManager, Log, TEXT("Initialize"));

	PlayerControllerWeakPtr = InController;
	UserIDPtr = USoOnlineHelper::GetUniqueNetIDFromObject(InController);
	if (!IsUserIDValid())
	{
		UE_LOG(LogSoAchievementManager, Error, TEXT("Initialize: Cannot map local player to unique net ID"));
		return false;
	}
	UE_LOG(LogSoAchievementManager, Log, TEXT("Initialize: UniqueNetID = %s"), *UserIDPtr->ToDebugString());

	// Initialize from settings
	// AchievementDefinitions = GetDefault<USoAchievementSettings>()->AchievementDefinitions;

	// Steam achievements
#if WARRIORB_WITH_STEAM
	SteamAchievementsInterface = GetSteamAchievements();
	//if (AreSteamAchievementsAvailable() && SteamAchievementsInterface.IsValid())
	if (SteamAchievementsInterface.IsValid())
	{
		UE_LOG(LogSoAchievementManager, Log, TEXT("Initialize: Steam Achievements are available"));

		SteamAchievementsInterface->OnUserStatsReceived().AddUObject(this, &ThisClass::OnSteamUserStatsReceived);
		SteamAchievementsInterface->OnUserAchievementStored().AddUObject(this, &ThisClass::OnSteamAchievementStored);
	}
	else
	{
		SteamAchievementsInterface = nullptr;
		UE_LOG(LogSoAchievementManager, Error, TEXT("Initialize: Steam Achievements are not available"));
	}

	// Warn us if anything is wrong
	CheckSteamDataIntegrity();
#endif // WARRIORB_WITH_STEAM

	// Retrieve the data for our achievements
	CacheAchievements();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::Shutdown()
{
	PlayerControllerWeakPtr = nullptr;
	UserIDPtr.Reset();
	UE_LOG(LogSoAchievementManager, Log, TEXT("Shutdown"));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::CheckSteamDataIntegrity()
{
#if WARRIORB_WITH_STEAM
	UE_LOG(LogSoAchievementManager, Verbose, TEXT("CheckSteamDataIntegrity"));
	if (!SteamAchievementsInterface.IsValid())
		return;

	// check maybe if achievement name exists?
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::ResetAchievements()
{
	UE_LOG(LogSoAchievementManager, Log, TEXT("ResetAchievements"));

#if WARRIORB_WITH_STEAM
	if (SteamAchievementsInterface.IsValid())
	{
		SteamAchievementsInterface->ResetAllAchievements();
	}
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::ResetAchievement(UObject* WorldContextObject, FName AchievementName)
{
	if (AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(WorldContextObject))
		IDlgDialogueParticipant::Execute_ModifyBoolValue(Player, AchievementName, false);

	UE_LOG(LogSoAchievementManager, Log, TEXT("ResetAchievement = %s"), *AchievementName.ToString());
#if WARRIORB_WITH_STEAM
	if (SteamAchievementsInterface.IsValid())
	{
		SteamAchievementsInterface->ResetAchievement(AchievementName);
	}
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAchievementManager::IsProgressAchievement(FName AchievementName) const
{
	// const FSoAchievement* Achievement = LocalAchievementsStorage().Find(FSoAchievement::NameIDToType(AchievementName));
	// if (!Achievement)
	// 	return false;

	// return Achievement->IsProgressAchievement();
	checkNoEntry();
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::ShowAchievementProgress(FName AchievementName) const
{
	if (!IsValidAchievementName(AchievementName))
	{
		UE_LOG(LogSoAchievementManager, Warning, TEXT("ShowAchievementProgress: AchievementName = %s does not exist"), *AchievementName.ToString());
		return;
	}

	UE_LOG(LogSoAchievementManager, Log, TEXT("ShowAchievementProgress = %s"), *AchievementName.ToString());
#if WARRIORB_WITH_STEAM
	if (SteamAchievementsInterface.IsValid())
	{
		// const FSoAchievement& FoundAchievement = LocalAchievementsStorage().FindChecked(FSoAchievement::NameIDToType(AchievementName));
		//SteamAchievementsInterface->ShowAchievementProgress(AchievementName, FoundAchievement.GetProgress(), FoundAchievement.GetMaxProgress());
		checkNoEntry();
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::UnlockAchievement(UObject* WorldContextObject, FName AchievementName)
{
	// achievement already unlocked
	if (AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(WorldContextObject))
	{
		if (IDlgDialogueParticipant::Execute_GetBoolValue(Player, AchievementName))
			return;

		IDlgDialogueParticipant::Execute_ModifyBoolValue(Player, AchievementName, true);
	}

	const FString ThisContext = FString::Printf(TEXT("UnlockAchievement(AchievementName = %s)"), *AchievementName.ToString());
	UE_LOG(LogSoAchievementManager, Log, TEXT("%s"), *ThisContext);

#if WARRIORB_WITH_STEAM
	if (SteamAchievementsInterface.IsValid())
	{
		SteamAchievementsInterface->UnlockAchievement(AchievementName);
	}
	else
	{
		UE_LOG(LogSoAchievementManager, Error, TEXT("%s - steam is NOT available"), *ThisContext);
	}
#endif // WARRIORB_WITH_STEAM

#if PLATFORM_XBOXONE
	UAchievementWriteCallbackProxy* Proxy = UAchievementWriteCallbackProxy::WriteAchievementProgress(WorldContextObject,
		USoStaticHelper::GetPlayerController(WorldContextObject),
		AchievementName);
	Proxy->Activate();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::UnlockAchievementIfSatisfied(UObject* WorldContextObject, const FSoConditionalAchievement& Achievement)
{
	if (Achievement.Name == NAME_None || !IsValid(WorldContextObject))
		return;

	if (AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(WorldContextObject))
	{
		if (Achievement.ConditionBoolTrueOnCharacter != NAME_None && !IDlgDialogueParticipant::Execute_GetBoolValue(Player, Achievement.ConditionBoolTrueOnCharacter))
			return;

		if (Achievement.ConditionBoolFalseOnCharacter != NAME_None && IDlgDialogueParticipant::Execute_GetBoolValue(Player, Achievement.ConditionBoolFalseOnCharacter))
			return;

		if (USoAchievementManager* AchievementManager = USoAchievementManager::GetInstance(WorldContextObject))
		{
			AchievementManager->UnlockAchievement(WorldContextObject, Achievement.Name);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::SetSteamAchievementProgressForName(FName AchievementName, float CompletionRatio)
{
#if WARRIORB_WITH_STEAM
	checkNoEntry();
	// UE_LOG(LogSoAchievementManager, Log, TEXT("SetSteamAchievementProgressForName: Achievement = %s with CompletionRatio = %f"), *AchievementName.ToString(), CompletionRatio);
	// if (!SteamAchievementsInterface.IsValid())
	// 	return;

	// FSoAchievement* Achievement = LocalAchievementsStorage().Find(FSoAchievement::NameIDToType(AchievementName));
	// if (!Achievement)
	// {
	// 	UE_LOG(LogSoAchievementManager, Warning, TEXT("SetSteamAchievementProgressForName: AchievementName = %s does not exist"), *AchievementName.ToString());
	// 	return;
	// }
	// const bool bIsProgressAchivement = Achievement->IsProgressAchievement();

	// // Write stat
	// if (bIsProgressAchivement)
	// {
	// 	// For achievements with progress we must write to the corresponding stat for that
	// 	const FSoAchievementDefinition& Definition = Achievement->GetDefinition();
	// 	verify(Definition.IsProgressAchievement());

	// 	switch (Definition.SteamUserStatType)
	// 	{
	// 	case ESoSteamStatType::Int:
	// 		SteamAchievementsInterface->SetIntStat(Definition.SteamUserStatName, FMath::FloorToInt(CompletionRatio));
	// 		break;
	// 	case ESoSteamStatType::Float:
	// 		SteamAchievementsInterface->SetFloatStat(Definition.SteamUserStatName, CompletionRatio);
	// 		break;
	// 	default:
	// 		checkNoEntry();
	// 		break;
	// 	}
	// 	SteamAchievementsInterface->StoreStats();
	// }

	// // Write achievement
	// if (!bIsProgressAchivement || (bIsProgressAchivement && Achievement->WouldMakeItUnlocked(CompletionRatio)))
	// {
	// 	// Normal achievement
	// 	MutexWriteQueries.Lock();
	// 	NumInProgressQueriesWrite++;
	// 	MutexWriteQueries.Unlock();

	// 	auto WriteFinishedDelegate = FOnAchievementsWrittenDelegate::CreateLambda([this, AchievementName, CompletionRatio](const FUniqueNetId& UserID, bool bSuccess)
	// 	{
	// 		OnAchievementWritten(AchievementName, CompletionRatio, UserID, bSuccess);

	// 		// TOOD do we need to reset the writer object, this potentially leaks
	// 		//WriteObject.Reset();
	// 	});

	// 	SteamAchievementsInterface->SetAchievementProgress(*UserIDPtr, AchievementName, CompletionRatio, WriteFinishedDelegate);
	// }
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAchievementManager::ValidateCurrentState(bool bLog) const
{
	bool bHasOneValidSubsystem = false;
#if WARRIORB_WITH_STEAM
	if (SteamAchievementsInterface.IsValid())
	{
		bHasOneValidSubsystem = true;
	}
#endif // WARRIORB_WITH_STEAM

	if (!bHasOneValidSubsystem)
	{
		if (bLog)
			UE_LOG(LogSoAchievementManager, Warning, TEXT("Does not have any valid subsystem"));

		return false;
	}

	const bool bHasValidUserID = IsUserIDValid();
	if (!bHasValidUserID)
	{
		if (bLog)
			UE_LOG(LogSoAchievementManager, Warning, TEXT("Does not have a valid UserID"));

		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAchievementManager::IsInOfflineMode() const
{
	return !ValidateCurrentState(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::CacheSteamAchievements()
{
#if WARRIORB_WITH_STEAM
	UE_LOG(LogSoAchievementManager, Log, TEXT("CacheSteamAchievements"));
	if (!SteamAchievementsInterface.IsValid())
		return;

	SteamAchievementsInterface->RequestCurrentStats();
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::CacheAchievements()
{
	UE_LOG(LogSoAchievementManager, Log, TEXT("CacheAchievements"));
	CacheSteamAchievements();
}

#if WARRIORB_WITH_STEAM

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::OnSteamUserStatsReceived(FNYSteamUserStatsReceived_Result Result)
{
	if (Result.bSuccess)
	{
		UE_LOG(LogSoAchievementManager, Log, TEXT("OnSteamUserStatsReceived Finished for UserID = %llu"), Result.UserID);
	}
	else
	{
		UE_LOG(LogSoAchievementManager, Error, TEXT("OnSteamUserStatsReceived Failure for UserID = %llu"),  Result.UserID);
	}
	// FillAchievements();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAchievementManager::OnSteamAchievementStored(FNYSteamUserAchievementStored_Result Result)
{
	UE_LOG(
		LogSoAchievementManager,
		Log,
		TEXT("OnAchievementWritten Finished for AchievementName = %s, CurrentProgress = %d, bSuccess = %d"),
		*Result.AchievementName.ToString(), Result.CurrentProgress, Result.bSuccess
	);
}

#endif // WARRIORB_WITH_STEAM
