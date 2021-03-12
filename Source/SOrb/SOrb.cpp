// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SOrb.h"

#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "SaveGameSystem.h"
#include "PlatformFeatures.h"
#include "Serialization/MemoryReader.h"

#include "Localization/SoLocalization.h"
#include "Settings/Input/SoInputHelper.h"
#include "SoBeforeGame/Public/SoBeforeGameModule.h"
#include "Localization/SoLocalizationHelper.h"
#include "FMODStudioModule.h"

#if WARRIORB_WITH_SDL2
#include "INotYetSDL2Module.h"
#endif // WARRIORB_WITH_SDL2

#if WARRIORB_WITH_STEAM
#include "INotYetSteamModule.h"
#endif // WARRIORB_WITH_STEAM

// NOTE: the name (third param) seems to be deprecated
IMPLEMENT_PRIMARY_GAME_MODULE(FWarriorbGameModule, SOrb, "WarriOrb");

DEFINE_LOG_CATEGORY(LogSoGeneral);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::StartupModule()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: StartupModule"));

	// Listen to core delegates
	PreExitDelegateHandle = FCoreDelegates::OnPreExit.AddRaw(this, &Self::HandlePreExit);
	ExitDelegateHandle = FCoreDelegates::OnExit.AddRaw(this, &Self::HandleExit);
	ApplicationWillDeactivateDelegate = FCoreDelegates::ApplicationWillDeactivateDelegate.AddRaw(this, &Self::HandleApplicationWillDeactivate);
	ApplicationHasReactivatedDelegate = FCoreDelegates::ApplicationHasReactivatedDelegate.AddRaw(this, &Self::HandleApplicationHasReactivated);
	ApplicationWillEnterBackgroundDelegate = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddRaw(this, &Self::HandleApplicationWillEnterBackground);
	ApplicationHasEnteredForegroundDelegate = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddRaw(this, &Self::HandleApplicationHasEnteredForeground);

#if PLATFORM_SWITCH
	HandleSwitchConfigSerialization();
#endif

	Init();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::ShutdownModule()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: ShutdownModule"));

	if (PreExitDelegateHandle.IsValid())
	{
		FCoreDelegates::OnPreExit.Remove(PreExitDelegateHandle);
		PreExitDelegateHandle.Reset();
	}
	if (ExitDelegateHandle.IsValid())
	{
		FCoreDelegates::OnExit.Remove(ExitDelegateHandle);
		ExitDelegateHandle.Reset();
	}
	if (ApplicationWillDeactivateDelegate.IsValid())
	{
		FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(ApplicationWillDeactivateDelegate);
		ApplicationWillDeactivateDelegate.Reset();
	}
	if (ApplicationHasReactivatedDelegate.IsValid())
	{
		FCoreDelegates::ApplicationHasReactivatedDelegate.Remove(ApplicationHasReactivatedDelegate);
		ApplicationHasReactivatedDelegate.Reset();
	}
	if (ApplicationWillEnterBackgroundDelegate.IsValid())
	{
		FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(ApplicationWillEnterBackgroundDelegate);
		ApplicationWillEnterBackgroundDelegate.Reset();
	}
	if (ApplicationHasEnteredForegroundDelegate.IsValid())
	{
		FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ApplicationHasEnteredForegroundDelegate);
		ApplicationHasEnteredForegroundDelegate.Reset();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::Init()
{
	InitStringTables();
	InitInput();
	InitPlugins();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::InitStringTables()
{
	if (bInitStringTables)
		return;

	// Register String tables that are not already registered
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: InitStringTables"));
	FSoBeforeGameModule::Get().InitStringTables();
	bInitStringTables = true;
}

void FWarriorbGameModule::InitInput()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: InitInput"));
	if (!bInitStringTables)
	{
		UE_LOG(LogSoGeneral, Error, TEXT("WarriorbGame: Called InitInput before InitStringTables, some translations may be invalid"));
	}

	// Init the static variables
	// NOTE: the order here matters
	FSoInputKey::Init();
	FSoInputActionName::Init();
	FSoInputAxisName::Init();
	FSoInputActionNameAxisNameLink::Init();
	FSoInputConfigurableActionName::Init();
	USoLocalizationHelper::Init();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::InitPlugins()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: InitPlugins"));
	IFMODStudioModule::Get().RefreshSettings();

	// TODO why do we need to manually load it?
#if WARRIORB_WITH_SDL2
	if (!INotYetSDL2Module::LoadModule())
	{
		UE_LOG(LogSoGeneral, Error, TEXT("INotYetSDL2Module Failed :("));
	}
#endif // WARRIORB_WITH_SDL2

#if WARRIORB_WITH_STEAM
	if (!INotYetSteamModule::LoadModule())
	{
		UE_LOG(LogSoGeneral, Error, TEXT("INotYetSteamModule Failed :("));
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::HandleSwitchConfigSerialization()
{
	// set up save game system to load GameUserSettings before it's loaded from normal location
	FCoreDelegates::CountPreLoadConfigFileRespondersDelegate.AddLambda([](const TCHAR* Filename, int32& ResponderCount)
	{
		if (GInputIni.Len() > 0 && Filename == GInputIni)
		{
			ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
			if (SaveSystem->DoesSaveGameExist(TEXT("Input.ini"), 0))
			{
				ResponderCount++;
			}
		}
	});

	FCoreDelegates::PreLoadConfigFileDelegate.AddLambda([](const TCHAR* Filename, FString& LoadedContents)
	{
		if (GInputIni.Len() > 0 && Filename == GInputIni)
		{
			ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
			TArray<uint8> RawData;
			// load with the same game system
			if (SaveSystem->LoadGame(false, TEXT("Input.ini"), 0, RawData))
			{
				FMemoryReader Ar(RawData);
				Ar << LoadedContents;
			}
		}
	});

	FCoreDelegates::PreSaveConfigFileDelegate.AddLambda([](const TCHAR* Filename, const FString& ContentsToSave, int32& SavedCount)
	{
		if (GInputIni.Len() > 0 && Filename == GInputIni)
		{
			ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();

			TArray<uint8> RawData;
			FMemoryWriter Ar(RawData, true);
			Ar << (FString&)ContentsToSave;
			if (RawData.Num() > 0)
			{
				// save with the save game system
				if (SaveSystem->SaveGame(false, TEXT("Input.ini"), 0, RawData))
				{
					// if we succeeded, track it
					SavedCount++;
				}
			}
		}
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::HandlePreExit()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: HandlePreExit"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::HandleExit()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: HandleExit"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::HandleApplicationWillDeactivate()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: HandleApplicationWillDeactivate"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::HandleApplicationHasReactivated()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: HandleApplicationHasReactivated"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::HandleApplicationWillEnterBackground()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: HandleApplicationWillEnterBackground"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FWarriorbGameModule::HandleApplicationHasEnteredForeground()
{
	UE_LOG(LogSoGeneral, Log, TEXT("WarriorbGame: HandleApplicationHasEnteredForeground"));
}
