// Copyright (c) Daniel Butum. All Rights Reserved.

#include "NotYetSteamModule.h"
#include "Core.h"
#include "Modules/ModuleManager.h"

#include "UObject/UObjectBase.h"
#include "NYSteamSubsystem.h"
#include "Templates/SharedPointer.h"

// From "OnlineSubsystemSteamModule.h"
// Only allow steam on desktop platforms
#define LOADING_STEAM_CLIENT_LIBRARY_DYNAMICALLY	(PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX)
#define LOADING_STEAM_SERVER_LIBRARY_DYNAMICALLY	((PLATFORM_WINDOWS && PLATFORM_32BITS) || (PLATFORM_LINUX && !IS_MONOLITHIC) || PLATFORM_MAC)
#define LOADING_STEAM_LIBRARIES_DYNAMICALLY			(LOADING_STEAM_CLIENT_LIBRARY_DYNAMICALLY || LOADING_STEAM_SERVER_LIBRARY_DYNAMICALLY)


DEFINE_LOG_CATEGORY_STATIC(LogNotYetSteamModule, All, All)

#define LOCTEXT_NAMESPACE "FNotYetSteamModule"

void FNotYetSteamModule::StartupModule()
{
	UE_LOG(LogNotYetSteamModule, Log, TEXT("StartupModule"));
	if (bInitializeAtStartup)
	{
		UE_LOG(LogNotYetSteamModule, Log, TEXT("StartupModule: bInitializeAtStartup=true calling Initialize"));
		Initialize();
	}
}

void FNotYetSteamModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (SteamSubsystem.IsValid())
	{
		SteamSubsystem->Shutdown();
		SteamSubsystem.Reset();
	}

#if LOADING_STEAM_LIBRARIES_DYNAMICALLY
	// Free the dll handle
	if (SteamClientLibraryHandle)
	{
		FPlatformProcess::FreeDllHandle(SteamClientLibraryHandle);
		SteamClientLibraryHandle = nullptr;
	}
#endif	//LOADING_STEAM_LIBRARIES_DYNAMICALLY

	UE_LOG(LogNotYetSteamModule, Log, TEXT("ShutdownModule"));
}

void FNotYetSteamModule::Initialize()
{
	if (SteamSubsystem.IsValid())
	{
		UE_LOG(LogNotYetSteamModule, Log, TEXT("Initialize: SteamSubsystem already initialized"));
		return;
	}

	SteamSubsystem = MakeShared<FNYSteamSubsystem, ESPMode::ThreadSafe>();

	// TOOD: remove this after we get rid of retarded unreal steam implementation
	// https://docs.unrealengine.com/en-US/Programming/Development/Tools/ConsoleManager/index.html
#if !UE_BUILD_SHIPPING
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OSS.SteamInitServerOnClient"));
	CVar->Set(0);
#endif

	// if (AreSteamDllsLoaded())
	// {
	// 	UE_LOG(LogNotYetSteamModule, Warning, TEXT("Initialize: Steam DLLs are not loaded"));
	// 	return;
	// }

	LoadSteamModules();
	if (SteamClientLibraryHandle)
	{
		UE_LOG(LogNotYetSteamModule, Log, TEXT("Initialize: Loaded Steam SDK"));
	}
	else
	{
		UE_LOG(LogNotYetSteamModule, Error, TEXT("Initialize: Failed to use own Steam third party library"));
	}
}

bool FNotYetSteamModule::AreSteamDllsLoaded() const
{
	// We assume it is statically linked on desktop and false otherwise (consoles)
	bool bLoadedClientDll = PLATFORM_DESKTOP;

	// Dynamicly linked
#if LOADING_STEAM_LIBRARIES_DYNAMICALLY
	bLoadedClientDll = SteamClientLibraryHandle != nullptr;
#endif

	return bLoadedClientDll;
}

void FNotYetSteamModule::LoadSteamModules()
{
	const FString RootSteamPath = GetDllPath();
	UE_LOG(LogNotYetSteamModule, Log, TEXT("LogNotYetSteamModule::LoadDll Loading From %s"), *RootSteamPath);

	FString SteamClientLibraryPath = TEXT("STEAM NOT SUPPORTED");
#if PLATFORM_WINDOWS

	#if PLATFORM_64BITS
		SteamClientLibraryPath = RootSteamPath + "steam_api64.dll";
	#else
		SteamClientLibraryPath = RootSteamPath + "steam_api.dll";
	#endif // PLATFORM_64BITS

#elif PLATFORM_LINUX
	SteamClientLibraryPath = RootSteamPath + "libsteam_api.so";
#endif	// PLATFORM_WINDOWS


#if LOADING_STEAM_LIBRARIES_DYNAMICALLY
	FPlatformProcess::PushDllDirectory(*RootSteamPath);
	SteamClientLibraryHandle = FPlatformProcess::GetDllHandle(*SteamClientLibraryPath);
	if (!SteamClientLibraryHandle)
	{
		const FString Message = TEXT("Couldn't load Steam DLL = ") + SteamClientLibraryPath;
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *Message, TEXT("Error"));
		UE_LOG(LogNotYetSteamModule, Error, TEXT("Failed to load Steam DLL = '%s'!"), *SteamClientLibraryPath);
	}
	FPlatformProcess::PopDllDirectory(*RootSteamPath);
#else
	UE_LOG(LogNotYetSteamModule, Warning, TEXT("Steam library = %s, should either be linked explicitly or this platform does not support steam."), *SteamClientLibraryPath);
#endif // LOADING_STEAM_LIBRARIES_DYNAMICALLY
}

FString FNotYetSteamModule::GetDllPath()
{
	//STEAM_SDK_ROOT_PATH
	const FString BasePath = FPaths::EngineDir() / STEAM_SDK_ROOT_PATH / STEAM_SDK_VER_PATH;
#if PLATFORM_WINDOWS

	#if PLATFORM_64BITS
		return BasePath / TEXT("Win64/");
	#else
		return BasePath / TEXT("Win32/");
	#endif	//PLATFORM_64BITS

#elif PLATFORM_LINUX

	#if PLATFORM_64BITS
		return BasePath / TEXT("x86_64-unknown-linux-gnu/");
	#else
		return BasePath / TEXT("i686-unknown-linux-gnu/");
	#endif	//PLATFORM_64BITS

#else
	return FString();
#endif	//PLATFORM_WINDOWS
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNotYetSteamModule, NotYetSteam)
