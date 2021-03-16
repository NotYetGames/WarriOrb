// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoGameEngine.h"

#include "Basic/SoGameMode.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoGameInstance.h"
#include "Settings/SoGameSettings.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoGameEngine, All, All)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameEngine::USoGameEngine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameEngine::Init(IEngineLoop* InEngineLoop)
{
	// FString String;
	// FFileHelper::LoadFileToString(String, TEXT("U:\\Warriorb\\Config\\Test.ini"));
	// const TCHAR* StrPtr = *String;
	// bool bIsPureANSII = SoIsPureAnsi(StrPtr);

	Super::Init(InEngineLoop);
	UE_LOG(LogSoGameEngine, Log, TEXT("Init"));

	if (FParse::Param(FCommandLine::Get(), TEXT("AssetsPrintAll")))
		USoPlatformHelper::PrintAllAssets();
	if (FParse::Param(FCommandLine::Get(), TEXT("AssetsPrintAllMaps")))
		USoPlatformHelper::PrintAllAssetsMaps();

	// Check if GPU is in blacklist
	//USoPlatformHelper::WarnIfGPUIsBlacklisted();

	// Move to particular monitor
	USoPlatformHelper::MoveGameToDefaultMonitor();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameEngine::Start()
{
	// Start GameInstance
	UE_LOG(LogSoGameEngine, Log, TEXT("Start"));
	Super::Start();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameEngine::PreExit()
{
	UE_LOG(LogSoGameEngine, Log, TEXT("PreExit"));

	// Save the current slot on shutdown
#if !WARRIORB_WITH_EDITOR
	// Find Current Game world
	bool bFoundAtLeastOneWorld = false;
	for (int32 WorldIndex = 0; WorldIndex < WorldList.Num(); ++WorldIndex)
	{
		UWorld* const World = WorldList[WorldIndex].World();
		// Invalid world or not a game world
		if (World == nullptr || !World->IsGameWorld())
			continue;

		if (USoGameInstance* SoGameInstance = USoGameInstance::GetInstance(World))
		{
			bFoundAtLeastOneWorld = true;
			SoGameInstance->SaveGameForCurrentState();
		}
		else
		{
			UE_LOG(LogSoGameEngine, Error, TEXT("USoGameEngine::PreExit Found a valid Game World but could not get the ASOrbGameMode"));
		}
	}

	if (bFoundAtLeastOneWorld)
	{
		UE_LOG(LogSoGameEngine, Verbose, TEXT("USoGameEngine::PreExit Found a valid Game World."));
	}
	else
	{
		UE_LOG(LogSoGameEngine, Error, TEXT("USoGameEngine::PreExit Could not find any valid Game World"));
	}
#endif

	// NOTE: This destroys all the worlds so we must call it at the end.
	Super::PreExit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameEngine::WorldAdded(UWorld* World)
{
	Super::WorldAdded(World);
	UE_LOG(LogSoGameEngine, Log, TEXT("WorldAdded"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameEngine::WorldDestroyed(UWorld* InWorld)
{
	Super::WorldDestroyed(InWorld);
	UE_LOG(LogSoGameEngine, Log, TEXT("WorldDestroyed"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameEngine::OnLostFocusPause(bool EnablePause)
{
	//ensure(false);
	Super::OnLostFocusPause(EnablePause);
}
