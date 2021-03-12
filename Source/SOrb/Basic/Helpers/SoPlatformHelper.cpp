// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoPlatformHelper.h"

#include "Kismet/KismetSystemLibrary.h"
#include "RHI.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformFilemanager.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericApplication.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Slate/SceneViewport.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/EngineVersion.h"
#include "AssetRegistryModule.h"

#include "Settings/SoDisplaySettingsTypes.h"
#include "Settings/SoGameSettings.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "SoStringHelper.h"
#include "Basic/SoGameSingleton.h"
#include "Character/SoPlayerController.h"
#include "HAL/FileManagerGeneric.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsApplication.h"
#endif // PLATFORM_WINDOWS

#if PLATFORM_XBOXONE
#include "XboxOne/XboxOneApplication.h"
#include "XboxOne/XboxOneMisc.h"
#endif // PLATFORM_XBOXONE

#if WARRIORB_WITH_SDL2
#include "NYSDL2_GamepadHelper.h"
#endif // WARRIORB_WITH_SDL2

#if WARRIORB_WITH_STEAM
#include "NYSteamHelper.h"
#include "INotYetSteamModule.h"
#endif

static const FName AssetRegistryName(TEXT("AssetRegistry"));

DEFINE_LOG_CATEGORY_STATIC(LogSoPlatformHelper, All, All)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEngineInfo::ToString() const
{
	return FString::Printf(
		TEXT("Engine:\n")
		TEXT("\tDepotName = `%s`\n")
		TEXT("\tEngineVersion = `%s`"),
		*DepotName, *EngineVersion);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoOperatingSystemInfo::ToString() const
{
	return FString::Printf(
		TEXT("OperatingSystem:\n")
		TEXT("\tOSVersion = `%s`\n")
		TEXT("\tOSMajor = `%s`, OSMinor = `%s`\n")
		TEXT("\tbIs64Bits = %d"),
		*OSVersion, *OSMajor, *OSMinor, bIs64Bits);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoCPUInfo::ToString() const
{
	return FString::Printf(
		TEXT("CPU:\n")
		TEXT("\tCPUPhysicalCores = %d, CPULogicalCores = %d\n")
		TEXT("\tCPUVendor = `%s`, CPUBrand = `%s`\n")
		TEXT("\tCPUInfo = %u"),
		CPUPhysicalCores, CPULogicalCores, *CPUVendor, *CPUBrand, CPUInfo);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoGPUInfo::ToString() const
{
	return FString::Printf(
		TEXT("GPU:\n")
		TEXT("\tGPUVendorID = %u, GPUDeviceID = %u\n")
		TEXT("\tGPUDeviceRevision = %u\n")
		TEXT("\tDesktopGPUAdapter = `%s`\n")
		TEXT("\tRenderingGPUAdapter = `%s`\n")
		TEXT("\tAdapterInternalDriverVersion = `%s`, AdapterUserDriverVersion = `%s`, AdapterDriverDate = `%s`"),
		GPUVendorID, GPUDeviceID, GPUDeviceRevision, *DesktopGPUAdapter,
		*RenderingGPUAdapter, *AdapterInternalDriverVersion, *AdapterUserDriverVersion,
		*AdapterDriverDate);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoPlatformMemoryInfo::ToString() const
{
	return FString::Printf(
		TEXT("Memory:\n")
		TEXT("\tRAMAvailablePhysical = %llu MB\n")
		TEXT("\tRAMAvailableVirtual = %llu MB\n")
		TEXT("\tRAMUsedPhysical = %llu MB\n")
		TEXT("\tRAMPeakUsedPhysical = %llu MB\n")
		TEXT("\tRAMUsedVirtual = %llu MB\n")
		TEXT("\tRAMPeakUsedVirtual = %llu MB\n")
		TEXT("\tGPUTextureMemory = %llu MB\n")
		TEXT("\tGPURenderTargetMemory = %llu MB\n")
		TEXT("\tGPUPoolSizeVRAMPercentage = %d %%"),
		USoMathHelper::TruncDoubleToUint64(RAMAvailablePhysicalMB), USoMathHelper::TruncDoubleToUint64(RAMAvailableVirtualMB), USoMathHelper::TruncDoubleToUint64(RAMUsedPhysicalMB),
		USoMathHelper::TruncDoubleToUint64(RAMPeakUsedPhysicalMB), USoMathHelper::TruncDoubleToUint64(RAMUsedVirtualMB), USoMathHelper::TruncDoubleToUint64(RAMPeakUsedVirtualMB),
		USoMathHelper::TruncDoubleToUint64(GPUTextureMemoryMB), USoMathHelper::TruncDoubleToUint64(GPURenderTargetMemoryMB), GPUPoolSizeVRAMPercentage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::PrintErrorToAll(const UObject* WorldContextObject, const FString& Message)
{
	UE_LOG(LogSoPlatformHelper, Error, TEXT("%s"), *Message);
	PrintToConsole(WorldContextObject, Message);
	PrintToScreen(Message, 10.f, FColor::Red);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::PrintToAll(const UObject* WorldContextObject, const FString& Message)
{
	UE_LOG(LogSoPlatformHelper, Log, TEXT("%s"), *Message);
	PrintToConsole(WorldContextObject, Message);
	PrintToScreen(Message, 10.f, FColor::White);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::PrintToConsole(const UObject* WorldContextObject, const FString& Message)
{
	if (APlayerController* PlayerController = USoStaticHelper::GetPlayerController(WorldContextObject))
		PlayerController->ClientMessage(Message);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::PrintToScreen(const FString& Message, float TimeToDisplaySeconds, FColor DisplayColor)
{
	if (!GEngine)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("PrintToScreen: GEngine == nullptr"));
		return;
	}

	const bool bPreviousValue = GAreScreenMessagesEnabled;
	GAreScreenMessagesEnabled = true;
	GEngine->AddOnScreenDebugMessage(-1, TimeToDisplaySeconds, DisplayColor, Message);
	GAreScreenMessagesEnabled = bPreviousValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::LaunchURL(const FString& URL, bool bCopyURLToClipboard)
{
	if (!URL.IsEmpty() && bCopyURLToClipboard)
		FPlatformApplicationMisc::ClipboardCopy(*URL);

	if (URL.IsEmpty() || !FPlatformProcess::CanLaunchURL(*URL))
	{
		UE_LOG(
			LogSoPlatformHelper,
			Warning,
			TEXT("Can not launch URL = `%s`. Either because the URL is empty or the platform can not launch it"),
			*URL
		);
		return false;
	}

	FString Error;
	FPlatformProcess::LaunchURL(*URL, nullptr, &Error);
	if (!Error.IsEmpty())
	{
		UE_LOG(
			LogSoPlatformHelper,
			Warning,
			TEXT("Error happened while launching URL = `%s`, Error = `%s`"),
			*URL, *Error
		);
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::ExploreFolder(const FString& FolderPath)
{
	if (FolderPath.IsEmpty())
		return false;

#if PLATFORM_DESKTOP
	FPlatformProcess::ExploreFolder(*FolderPath);
	return true;
#else
	// consoles most likely, ignore
	return false;
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::DisableVerifyGC(bool bCheckForCommandLineOverrides)
{
	// Already disabled, nothing to do
	if (!GShouldVerifyGCAssumptions)
	{
		UE_LOG(LogSoPlatformHelper, Verbose, TEXT("DisableVerifyGC: GShouldVerifyGCAssumptions IS ALREADY false"));
		return;
	}

	// Is set by the command line flags, ignore, as the user command line has higher precedence
	if (bCheckForCommandLineOverrides)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("VERIFYGC"))
		 || FParse::Param(FCommandLine::Get(), TEXT("NOVERIFYGC")))
		 {
			UE_LOG(
				LogSoPlatformHelper,
				Verbose,
				TEXT("DisableVerifyGC: Ignoring because command line flags were specified: -VERIFYGC or -NOVERIFYGC")
			);
			return;
		 }
	}

	UE_LOG(LogSoPlatformHelper, Log, TEXT("DisableVerifyGC: GShouldVerifyGCAssumptions = 0"));
	GShouldVerifyGCAssumptions = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::SetIsAILoggingEnabled(bool bEnabled, bool bCheckForCommandLineOverrides)
{
	if (!GEngine)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("SetIsAILoggingEnabledEnabled: Can't get valid GEngine"));
		return;
	}

	// Nothing to do
	const bool bIsAILoggingEnabled = !GEngine->bDisableAILogging;
	if (bIsAILoggingEnabled == bEnabled)
	{
		UE_LOG(
			LogSoPlatformHelper,
			Verbose,
			TEXT("SetIsAILoggingEnabled: GEngine->bDisableAILogging IS ALREADY %d"),
			!bIsAILoggingEnabled
		);
	}

	// Is set by the command line flags, ignore, as the user command line has higher precedence
	if (bCheckForCommandLineOverrides)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("noailogging"))
		 || FParse::Param(FCommandLine::Get(), TEXT("enableailogging")))
		 {
			UE_LOG(
				LogSoPlatformHelper,
				Verbose,
				TEXT("SetIsAILoggingEnabled: Ignoring because command line flags were specified: -noailogging or -enableailogging")
			);
			return;
		 }
	}

	const bool bNewValue = !bEnabled;
	UE_LOG(LogSoPlatformHelper, Log, TEXT("SetIsAILoggingEnabled: GEngine->bDisableAILogging = %d"), bNewValue);
	GEngine->bDisableAILogging = bNewValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::SetAreScreenMessagesEnabled(bool bEnabled, bool bCheckForCommandLineOverrides)
{
	/**
	*	Global value indicating on-screen warnings/message should be displayed.
	*	Disabled via console command "DISABLEALLSCREENMESSAGES"
	*	Enabled via console command "ENABLEALLSCREENMESSAGES"
	*	Toggled via console command "TOGGLEALLSCREENMESSAGES"
	*/

	// Nothing to do
	if (GAreScreenMessagesEnabled == bEnabled)
	{
		UE_LOG(
			LogSoPlatformHelper,
			Verbose,
			TEXT("SetAreScreenMessagesEnabled: GAreScreenMessagesEnabled IS ALREADY %d"),
			bEnabled
		);
		return;
	}

	// Is set by the command line flags, ignore, as the user command line has higher precedence
	if (bCheckForCommandLineOverrides && FParse::Param(FCommandLine::Get(),TEXT("NOSCREENMESSAGES")))
	{
		UE_LOG(
			LogSoPlatformHelper,
			Verbose,
			TEXT("SetAreScreenMessagesEnabled: Ignoring because command line flag was specified: -NOSCREENMESSAGES")
		);
	}

	UE_LOG(LogSoPlatformHelper, Log, TEXT("SetAreScreenMessagesEnabled: GAreScreenMessagesEnabled = %d"), GAreScreenMessagesEnabled);
	GAreScreenMessagesEnabled = bEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::SetIsMouseEnabled(const UObject* WorldContextObject, bool bEnabled)
{
	ASoPlayerController* PlayerController = ASoPlayerController::GetInstance(WorldContextObject);
	if (!PlayerController)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("SetIsMouseEnabled: Can't get valid ASoPlayerController"));
		return;
	}

	PlayerController->SetIsMouseEnabled(bEnabled);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::SetIsMouseAllowed(const UObject* WorldContextObject, bool bAllowed)
{
	ASoPlayerController* PlayerController = ASoPlayerController::GetInstance(WorldContextObject);
	if (!PlayerController)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("SetIsMouseAllowed: Can't get valid ASoPlayerController"));
		return;
	}

	PlayerController->SetIsMouseAllowed(bAllowed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::IsMouseAllowed(const UObject* WorldContextObject)
{
	ASoPlayerController* PlayerController = ASoPlayerController::GetInstance(WorldContextObject);
	if (!PlayerController)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("IsMouseAllowed: Can't get valid ASoPlayerController"));
		return false;
	}

	return PlayerController->IsMouseAllowed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::MoveGameToDefaultMonitor()
{
#if PLATFORM_DESKTOP
	auto* GameSettings = USoGameSettings::GetInstance();
	if (!GameSettings)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("MoveGameToDefaultMonitor: GameSettings == nullptr"));
		return;
	}

	// Start on the main display by default
	int32 MonitorNumber = 0;
	FParse::Value(FCommandLine::Get(), TEXT("monitor="), MonitorNumber);

	UE_LOG(LogSoPlatformHelper, Log, TEXT("Using MonitorNumber = %d"), MonitorNumber);
	FMonitorInfo MonitorInfo;
	if (GameSettings->MoveToMonitor(MonitorNumber, MonitorInfo))
	{
		UE_LOG(
			LogSoPlatformHelper,
			Log,
			TEXT("Moved to MonitorNumber = %d, Name = `%s`, ID = `%s`, NativeWidth = %d, NativeHeight = %d, bIsPrimary = %d"),
			MonitorNumber, *MonitorInfo.Name, *MonitorInfo.ID, MonitorInfo.NativeWidth, MonitorInfo.NativeHeight, MonitorInfo.bIsPrimary
		);
	}
	else
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("FAILED to Move to MonitorNumber = %d"), MonitorNumber);
	}
#endif // PLATFORM_DESKTOP
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::GetSupportedFullscreenResolutions(TArray<FSoDisplayResolution>& OutResolutions)
{
	TArray<FIntPoint> SupportedResolutions;
	if (!UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedResolutions))
		return false;

	for (const FIntPoint& SupportedResolution : SupportedResolutions)
		OutResolutions.Emplace(SupportedResolution);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::GetConvenientWindowedResolutions(TArray<FSoDisplayResolution>& OutResolutions)
{
	TArray<FIntPoint> SupportedResolutions;
	if (!UKismetSystemLibrary::GetConvenientWindowedResolutions(SupportedResolutions))
		return false;

	for (const FIntPoint& SupportedResolution : SupportedResolutions)
		OutResolutions.Emplace(SupportedResolution);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::GetDisplaysRefreshRates(TSet<int32>& OutRefreshRates)
{
	FScreenResolutionArray SupportedResolutions;
	if (RHIGetAvailableResolutions(SupportedResolutions, false))
		for (const FScreenResolutionRHI& Resolution : SupportedResolutions)
			OutRefreshRates.Add(Resolution.RefreshRate);

	return SupportedResolutions.Num() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::GetDisplaysMetrics(FDisplayMetrics& OutDisplayMetrics)
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetInitialDisplayMetrics(OutDisplayMetrics);
		return true;
	}
	if (FApp::CanEverRender())
	{
		FDisplayMetrics::RebuildDisplayMetrics(OutDisplayMetrics);
		return true;
	}

	UE_LOG(LogSoPlatformHelper, Error, TEXT("GetDisplaysMetrics: Can't get valid FDisplayMetrics"));
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FAudioDevice* USoPlatformHelper::GetAudioDevice(const UObject* WorldContextObject)
{
	if (!GEngine)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("GetAudioDevice: GEngine == nullptr"));
		return nullptr;
	}
	if (!GEngine->UseSound())
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("GetAudioDevice: GEngine does not use sound or it AudioDeviceManager is not initialized :O"));
		return nullptr;
	}

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(ThisWorld))
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("GetAudioDevice: UWorld == nullptr"));
		return nullptr;
	}
	if (!ThisWorld->bAllowAudioPlayback)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("GetAudioDevice: UWorld.bAllowAudioPlayback = false"));
		return nullptr;
	}

	return ThisWorld->GetAudioDevice();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::CreateDirectoryRecursively(const FString& FolderPath)
{
	// Makes sure it is a directory, gets the directory path
	const FString CreateFolder = FPaths::GetPath(FolderPath);
	return FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*CreateFolder);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::QuitGame(const UObject* WorldContextObject)
{
	UE_LOG(LogSoPlatformHelper, Log, TEXT("QuitGame"));
#if WITH_EDITOR
	// NOTE: the console command this seems to be for all platforms, consoles too
	// Console command is used inside UGameEngine::Exec. and ULocalPlayer::Exec
	if (APlayerController* PlayerController = USoStaticHelper::GetPlayerController(WorldContextObject))
	{
		PlayerController->ConsoleCommand(TEXT("quit"));
	}
	else
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("QuitGame: APlayerController == nullptr"));
	}
#else
	FPlatformMisc::RequestExit(false);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::IsGameInForeground(UObject* WorldContextObject)
{
//#if WITH_EDITOR
	if (!GEngine)
		return false;

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld)
	{
		// Find world
		for (TObjectIterator<AActor> Itr; Itr; ++Itr)
		{
			ThisWorld = Itr->GetWorld();
			if (ThisWorld && ThisWorld->WorldType != EWorldType::EditorPreview  &&
				ThisWorld->WorldType != EWorldType::Editor)
			{
				break;
			}

			// Reset :(
			ThisWorld = nullptr;
		}
	}

	// Find the first viewport of the first window and see if it is in foreground
	// TODO if this is the correct way on all platforms
	if (ThisWorld)
		if (const ULocalPlayer* LocalPlayer = ThisWorld->GetFirstLocalPlayerFromController())
			if (const UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
				if (const FSceneViewport* Viewport = LocalPlayer->ViewportClient->GetGameViewport())
					return Viewport->IsForegroundWindow();

	// last chance
	return FPlatformApplicationMisc::IsThisApplicationForeground();
//#else
//	// Can't use this in editor builds as it will just reference the Editor
//	return FPlatformApplicationMisc::IsThisApplicationForeground();
//#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::SetGamePaused(const UObject* WorldContextObject, bool bPaused)
{
	UE_LOG(LogSoPlatformHelper, Log, TEXT("SetGamePaused: bPaused = %d"), bPaused);

	// Sets the Pause in the GameMode/World
	if (APlayerController* PlayerController = USoStaticHelper::GetPlayerController(WorldContextObject))
		return PlayerController->SetPause(bPaused);

	UE_LOG(LogSoPlatformHelper, Error, TEXT("SetGamePaused: APlayerController == nullptr"));
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::IsGamePaused(const UObject* WorldContextObject)
{
	if (!GEngine)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("IsGamePaused: GEngine == nullptr"));
		return false;
	}

	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		return World->IsPaused();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::SetGlobalTimeDilation(const UObject* WorldContextObject, float TimeDilation)
{
	if (!GEngine)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("SetGlobalTimeDilation: GEngine == nullptr"));
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("SetGlobalTimeDilation: UWorld == nullptr"));
		return;
	}

	AWorldSettings* WorldSettings = World->GetWorldSettings();
	if (!WorldSettings)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("SetGlobalTimeDilation: AWorldSettings == nullptr"));
		return;
	}

	// Values are the same, ignore
	if (FMath::IsNearlyEqual(WorldSettings->TimeDilation, TimeDilation))
		return;

	const float ActualTimeDilation = WorldSettings->SetTimeDilation(TimeDilation);
	if (FMath::IsNearlyEqual(TimeDilation, ActualTimeDilation))
	{
		UE_LOG(LogSoPlatformHelper, Log, TEXT("SetGlobalTimeDilation: TimeDilation = %f"), TimeDilation);
	}
	else
	{
		UE_LOG(
			LogSoPlatformHelper,
			Warning,
			TEXT("Time Dilation must be between %f and %f. Clamped value to that range."),
			WorldSettings->MinGlobalTimeDilation, WorldSettings->MaxGlobalTimeDilation
		);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoPlatformHelper::GetGlobalTimeDilation(const UObject* WorldContextObject)
{
	if (!GEngine)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("GetGlobalTimeDilation: GEngine == nullptr"));
		return Default;
	}

	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		if (const AWorldSettings* WorldSettings = World->GetWorldSettings())
			return WorldSettings->TimeDilation;

	return USoGameSettings::DefaultGameSpeed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::AreAllGameWindowsHidden()
{
	static auto IsWindowHidden = [](TSharedRef<SWindow> Window)
	{
		return Window->IsWindowMinimized() || !Window->IsVisible();
	};

	// Check all modals, DlgDataDisplay
	const TArray<TSharedRef<SWindow>> AllWindows = FSlateApplication::Get().GetInteractiveTopLevelWindows();
	bool bAllHidden = true;
	for (const TSharedRef<SWindow>& Window : AllWindows)
	{
		if (!IsWindowHidden(Window))
		{
			bAllHidden = false;
			break;
		}
	}

	// Check top level
	TSharedPtr<SWindow> MainWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	const bool bIsMainWindowHidden = MainWindow.IsValid() ? IsWindowHidden(MainWindow.ToSharedRef()) : false;

	return bAllHidden && bIsMainWindowHidden;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoEngineInfo& USoPlatformHelper::GetUnrealEngineInfo()
{
	static const FEngineVersion& EngineVersion = FEngineVersion::Current();
	static const FSoEngineInfo EngineInfo = { EngineVersion.GetBranch(), EngineVersion.ToString() };
	return EngineInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tries opening and parsing the specified release file in /etc to find information about the distro used.
bool ReadEtcReleaseFile(const FString& FilePath, FString& OutOSVersion)
{
	if (FilePath.IsEmpty())
		return false;

	TArray<FString> FileLines;
	if (!FFileHelper::LoadFileToStringArray(FileLines, *FilePath))
		return false;

	FString Name, VersionID;
	for (const FString& Line : FileLines)
	{
		if (Line.IsEmpty())
			continue;

		TArray<FString> LineParts;
		if (Line.ParseIntoArray(LineParts, TEXT("="), true) != 2)
			continue;

		if (LineParts[0] == TEXT("NAME"))
			Name = LineParts[1].Replace(TEXT("\""), TEXT(""));
		else if (LineParts[0] == TEXT("VERSION_ID"))
			VersionID = LineParts[1].Replace(TEXT("\""), TEXT(""));
	}

	if (!Name.IsEmpty() && !VersionID.IsEmpty())
	{
		OutOSVersion = Name + " " + VersionID;
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoOperatingSystemInfo USoPlatformHelper::GetOperatingSystemInfo()
{
	FSoOperatingSystemInfo Info;

	// NOTE: Linux special support was added because it does not have support from the engine
#if PLATFORM_LINUX
	struct utsname UnameData;
	if (uname(&UnameData) == 0)
	{
		const FString sysname(UTF8_TO_TCHAR(UnameData.sysname));
		const FString version(UTF8_TO_TCHAR(UnameData.version));

		/// Ignore data after "-", since it could identify a system (self compiled kernels).
		FString release(UTF8_TO_TCHAR(UnameData.release));
		TArray<FString> ReleaseParts;
		if (release.ParseIntoArray(ReleaseParts, TEXT("-"), true) > 0)
			release = ReleaseParts[0];


		Info.OSMajor = sysname + " " + release;
		Info.OSMinor = version;
	}

	// Fill OSVersion from https://www.freedesktop.org/software/systemd/man/os-release.html
	// First try the standard /etc/os-release. Then check for older versions
	// Could check for older versions, but meh
	// https://github.com/supertuxkart/stk-code/blob/master/src/config/hardware_stats.cpp#L172
	if (!ReadEtcReleaseFile(TEXT("/etc/os-release"), Info.OSVersion))
		ReadEtcReleaseFile(TEXT("/etc/redhat-release"), Info.OSVersion);

	if (Info.OSVersion.IsEmpty())
	{
		Info.OSVersion = TEXT("Linux unknown");
	}
#else
	FPlatformMisc::GetOSVersions(Info.OSMajor, Info.OSMinor);
	Info.OSVersion = FPlatformMisc::GetOSVersion();
#endif

	Info.bIs64Bits = FPlatformMisc::Is64bitOperatingSystem();
	return Info;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoPlatformHelper::ToStringPlatformContext()
{
	// Add Info about the system
	const FSoEngineInfo& EngineInfo = GetUnrealEngineInfo();
	const FSoOperatingSystemInfo OSInfo = GetOperatingSystemInfo();
	const FSoCPUInfo CPUInfo = GetCPUInfo();
	const FSoGPUInfo GPUInfo = GetGPUInfo();
	const FSoPlatformMemoryInfo MemoryInfo = GetMemoryInfo();

	FString StringGameSettings = TEXT("COULD NOT RETRIEVE GAME SETTINGS");
	if (const auto* GameSettings = USoGameSettings::GetInstance())
		StringGameSettings = GameSettings->ToString();

	const FString CommandLine = FCommandLine::IsInitialized() ? FCommandLine::GetOriginalForLogging() : TEXT("");
	const FString Output = FString::Printf(
		TEXT("GAME Build version = %s, Branch = %s, Commit = %s\n")
		TEXT("GameRootPath = %s"),
		TEXT("UserName = `%s`\n")
		TEXT("DefaultLocale = `%s`\n")
		TEXT("CommandLine = `%s`\n")
		TEXT("\n%s\n") TEXT("\n%s\n")
		TEXT("\n%s\n") TEXT("\n%s\n")
		TEXT("\n%s\n") TEXT("\n%s\n")
		TEXT("\n%s\n"),
		*GetGameBuildVersion(), *GetGameBuildBranch(), *GetGameBuildCommit(),
		*GetGameRootPath(),
		FPlatformProcess::UserName(),
		*FPlatformMisc::GetDefaultLocale(),
		*CommandLine,
		*USoPlatformHelper::ToStringCompileFlags(), *EngineInfo.ToString(),
		*OSInfo.ToString(), *CPUInfo.ToString(),
		*GPUInfo.ToString(), *MemoryInfo.ToString(),
		*StringGameSettings
	);

	return Output;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::WarnIfGPUIsBlacklisted()
{
#if PLATFORM_DESKTOP
	const bool bNoGPUBlacklistBox = FParse::Param(FCommandLine::Get(), TEXT("NoGPUBlacklistBox"));

	const FSoGPUInfo GPUInfo = GetGPUInfo();
	FString BlacklistedGPUName;

	// NOTE: don't use the DesktopGPUAdapter because on dual graphics this will be false positive
	if (USoGameSingleton::IsBlacklistedGPUName(GPUInfo.RenderingGPUAdapter))
	{
		BlacklistedGPUName = GPUInfo.RenderingGPUAdapter;
	}
	if (!BlacklistedGPUName.IsEmpty())
	{
		const FString Message = FString::Printf(
			TEXT("Your GPU = %s is not supported because it is under the minimum requirements.\n\nSome things might not work!\n\nHere be dragons!!!"),
			*BlacklistedGPUName
		);

		UE_LOG(LogSoPlatformHelper, Warning, TEXT("%s"), *Message);

		// Do not display message because if user said not to display it or if we are in steam big picture mode
		if (!bNoGPUBlacklistBox && !IsSteamBigPicture())
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *Message, TEXT("Your GPU is not supported!!!"));
	}
#endif // PLATFORM_DESKTOP
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoCPUInfo USoPlatformHelper::GetCPUInfo()
{
	FSoCPUInfo Info;
	Info.CPUPhysicalCores = FPlatformMisc::NumberOfCores();
	Info.CPULogicalCores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
	Info.CPUVendor = FPlatformMisc::GetCPUVendor();
	Info.CPUBrand = FPlatformMisc::GetCPUBrand();
	Info.CPUInfo = FPlatformMisc::GetCPUInfo();
	return Info;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoGPUInfo USoPlatformHelper::GetGPUInfo()
{
	FSoGPUInfo Info;
	Info.GPUVendorID = GRHIVendorId;
	Info.GPUDeviceID = GRHIDeviceId;
	Info.GPUDeviceRevision = GRHIDeviceRevision;
	Info.DesktopGPUAdapter = FPlatformMisc::GetPrimaryGPUBrand();
	Info.RenderingGPUAdapter = GRHIAdapterName;
	Info.AdapterInternalDriverVersion = GRHIAdapterInternalDriverVersion;
	Info.AdapterUserDriverVersion = GRHIAdapterUserDriverVersion;
	Info.AdapterDriverDate = GRHIAdapterDriverDate;
	return Info;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoPlatformMemoryInfo USoPlatformHelper::GetMemoryInfo()
{
	FSoPlatformMemoryInfo Info;
	const FPlatformMemoryStats RAMStats = FPlatformMemory::GetStats();
	Info.RAMAvailablePhysicalMB = USoMathHelper::BytesToMegaBytes(RAMStats.AvailablePhysical);
	Info.RAMAvailableVirtualMB = USoMathHelper::BytesToMegaBytes(RAMStats.AvailableVirtual);
	Info.RAMUsedPhysicalMB = USoMathHelper::BytesToMegaBytes(RAMStats.UsedPhysical);
	Info.RAMPeakUsedPhysicalMB = USoMathHelper::BytesToMegaBytes(RAMStats.PeakUsedPhysical);
	Info.RAMUsedVirtualMB = USoMathHelper::BytesToMegaBytes(RAMStats.UsedVirtual);
	Info.RAMPeakUsedVirtualMB = USoMathHelper::BytesToMegaBytes(RAMStats.PeakUsedVirtual);

	Info.GPUTextureMemoryMB = USoMathHelper::KiloBytesToMegaBytes(GCurrentTextureMemorySize);
	Info.GPURenderTargetMemoryMB = USoMathHelper::KiloBytesToMegaBytes(GCurrentRendertargetMemorySize);
	Info.GPUPoolSizeVRAMPercentage = GPoolSizeVRAMPercentage;

	return Info;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::HasFixedResolution()
{
	return FPlatformProperties::HasFixedResolution();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::HasFixedVSync()
{
	if (!GConfig)
		return false;

	// See UGameUserSettings::ApplyNonResolutionSettings
	// Basically this means that if you have in ConfigSection of your Engine.ini (and its overrides) r.VSync=<value> you can't override it at runtime
	// Also because ECVF_SetBySystemSettingsIni > ECVF_SetByGameSetting for the VSync console variable
	const FString ConfigSection = GIsEditor ? TEXT("SystemSettingsEditor") : TEXT("SystemSettings");

	int32 VSyncValue = 0;
	return GConfig->GetInt(*ConfigSection, TEXT("r.Vsync"), VSyncValue, GEngineIni);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::SupportsWindowedMode()
{
	return FPlatformProperties::SupportsWindowedMode();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::SupportsQuit()
{
	return FPlatformProperties::SupportsQuit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoPlatformHelper::GetPlatformName()
{
	return FPlatformProperties::IniPlatformName();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::HasWeakHardware()
{
	return HasFixedWeakHardware() || USoGameSettings::Get().IsWeakHardwareOptimizationForced();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoXboxOneConsoleType USoPlatformHelper::GetXboxOneConsoleType()
{
#if PLATFORM_XBOXONE
	switch (FXboxOneMisc::GetConsoleType())
	{
	case EXboxOneConsoleType::XboxOne:
		return ESoXboxOneConsoleType::XboxOne;
	case EXboxOneConsoleType::XboxOneS:
		return ESoXboxOneConsoleType::XboxOneS;
	case EXboxOneConsoleType::Scorpio:
		return ESoXboxOneConsoleType::XboxOneX;
	default:
		return ESoXboxOneConsoleType::Invalid;
	}
#endif // PLATFORM_XBOXONE

	return ESoXboxOneConsoleType::Invalid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FString& USoPlatformHelper::ToStringCompileFlags()
{
	static const FString Output = FString::Printf(
		TEXT("Compiled flags:\n")
		TEXT("\tWARRIORB_NON_EDITOR_TEST = %d, WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST = %d\n")
		TEXT("\tWITH_EDITOR = %d, WARRIORB_WITH_EDITOR = %d\n")

		TEXT("\tWARRIORB_WITH_ANALYTICS = %d, WARRIORB_WITH_GAMEANALYTICS = %d, WARRIORB_COLLECT_ANALYTICS = %d\n")
		TEXT("\tWARRIORB_WITH_ONLINE = %d, WARRIORB_DEMO = %d\n")
		TEXT("\tWARRIORB_WITH_STEAM = %d, WARRIORB_STEAM_APP_ID = %d, WARRIORB_RELAUNCH_IN_STEAM = %d\n")
		TEXT("\tWARRIORB_WITH_DISCORD = %d, WARRIORB_DISCORD_CLIENT_ID = %lld\n")
		TEXT("\tWARRIORB_WITH_SDL2 = %d, WARRIORB_USE_ASUS_AURA_SDK = %d\n")

		TEXT("\tWARRIORB_USE_CHARACTER_SHADOW = %d, WARRIORB_USE_UNREAL_SAVE_SYSTEM = %d, WARRIORB_USE_ARROW_PRESET_AS_DEFAULT = %d, WARRIORB_ALLOW_CONSOLE_CHEATS_DEFAULT = %d\n")
		TEXT("\tWARRIORB_WITH_VIDEO_DEMO = %d, WARRIORB_WITH_VIDEO_INTRO= = %d\n")

		TEXT("\tALLOW_DEBUG_FILES = %d, ALLOW_CONSOLE = %d, NO_LOGGING = %d\n")
		TEXT("\tSTATS = %d, FORCE_USE_STATS = %d\n")
		TEXT("\tDO_CHECK = %d, USE_LOGGING_IN_SHIPPING = %d, USE_CHECKS_IN_SHIPPING= %d, ALLOW_CONSOLE_IN_SHIPPING = %d\n"),

		WARRIORB_NON_EDITOR_TEST, WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST,
		WITH_EDITOR, WARRIORB_WITH_EDITOR,

		WARRIORB_WITH_ANALYTICS, WARRIORB_WITH_GAMEANALYTICS, WARRIORB_COLLECT_ANALYTICS,
		WARRIORB_WITH_ONLINE, WARRIORB_DEMO,
		WARRIORB_WITH_STEAM, GetSteamAppID(WARRIORB_DEMO), WARRIORB_RELAUNCH_IN_STEAM,
		WARRIORB_WITH_DISCORD, GetDiscordClientID(WARRIORB_DEMO),
		WARRIORB_WITH_SDL2, WARRIORB_USE_ASUS_AURA_SDK,

		WARRIORB_USE_CHARACTER_SHADOW, WARRIORB_USE_UNREAL_SAVE_SYSTEM, WARRIORB_USE_ARROW_PRESET_AS_DEFAULT, WARRIORB_ALLOW_CONSOLE_CHEATS_DEFAULT,
		WARRIORB_WITH_VIDEO_DEMO, WARRIORB_WITH_VIDEO_INTRO,

		ALLOW_DEBUG_FILES, ALLOW_CONSOLE, NO_LOGGING,
		STATS, FORCE_USE_STATS,
		DO_CHECK, USE_LOGGING_IN_SHIPPING, USE_CHECKS_IN_SHIPPING, ALLOW_CONSOLE_IN_SHIPPING
	);
	return Output;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FString& USoPlatformHelper::GetGameBuildVersion()
{
#ifndef WARRIORB_BUILD_VERSION
#error "WARRIORB_BUILD_VERSION is not set in the SOrb.Build.cs file"
#endif // !WARRIORB_BUILD_VERSION

	static const FString BuildVersion(TEXT(WARRIORB_BUILD_VERSION));
#if WITH_EDITOR
	static FString FinalBuildVersion = BuildVersion + TEXT(" Editor");
#else
	static FString FinalBuildVersion = BuildVersion;
#endif

	return FinalBuildVersion;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FString& USoPlatformHelper::GetGameBuildBranch()
{
#ifndef WARRIORB_BUILD_BRANCH
#error "WARRIORB_BUILD_BRANCH is not set in the SOrb.Build.cs file"
#endif // !WARRIORB_BUILD_BRANCH

	static const FString BuildBranch(TEXT(WARRIORB_BUILD_BRANCH));
	return BuildBranch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FString& USoPlatformHelper::GetGameBuildCommit()
{
#ifndef WARRIORB_BUILD_COMMIT
#error "WARRIORB_BUILD_COMMIT is not set in the SOrb.Build.cs file"
#endif // !WARRIORB_BUILD_COMMIT

	static const FString BuildCommit(TEXT(WARRIORB_BUILD_COMMIT));
	return BuildCommit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoPlatformHelper::GetGameBuildAll()
{
	return FString::Printf(TEXT("%s - %s (%s)"), *GetGameBuildVersion(), *GetGameBuildCommit(), *GetGameBuildBranch());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::InitializedSteam()
{
#if WARRIORB_WITH_STEAM
	return UNYSteamHelper::Initialize(GetSteamAppID(IsDemo()), WARRIORB_RELAUNCH_IN_STEAM);
#else
	return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoPlatformHelper::GetCurrentSteamAppID()
{
#if WARRIORB_WITH_STEAM
	INYSteamSubsystemPtr Subsystem = INotYetSteamModule::Get().GetSteamSubsystem();
	if (Subsystem.IsValid())
		return Subsystem->GetAppIDAsString();

	return TEXT("STEAM NOT OPENED");
#else
	return TEXT("DISABLED");
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoPlatformHelper::GetSteamCurrentGameLanguage()
{
#if WARRIORB_WITH_STEAM
	INYSteamSubsystemPtr Subsystem = INotYetSteamModule::Get().GetSteamSubsystem();
	if (Subsystem.IsValid())
		return Subsystem->GetCurrentGameLanguage();

	return TEXT("");
#else
	return TEXT("DISABLED");
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::IsSteamInitialized()
{
#if WARRIORB_WITH_STEAM
	INYSteamSubsystemPtr Subsystem = INotYetSteamModule::Get().GetSteamSubsystem();
	if (Subsystem.IsValid())
		return Subsystem->IsEnabled();
#endif
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::OpenSteamOverlayToStore(uint32 AppID, bool bAddToCart, bool bShowCart)
{
#if WARRIORB_WITH_STEAM
	INYSteamSubsystemPtr Subsystem = INotYetSteamModule::Get().GetSteamSubsystem();
	if (Subsystem.IsValid())
		return Subsystem->GetExternalUI()->ActivateGameOverlayToStore(AppID, bAddToCart, bShowCart);
#endif
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::IsSteamBigPicture()
{
	// https://partner.steamgames.com/doc/features/steam_controller/getting_started_for_devs
	return FPlatformMisc::GetEnvironmentVariable(TEXT("SteamTenfoot")).Len() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoPlatformHelper::GetGameRootPath()
{
	{
		const FString BaseDirPath = FPlatformProcess::BaseDir();
		const FString EnginePath = BaseDirPath / TEXT("../../../Engine/Binaries/");
		if (FPaths::DirectoryExists(EnginePath))
		{
			FString GameRootPath = BaseDirPath / TEXT("../../../");
			FPaths::CollapseRelativeDirectories(GameRootPath);
			return GameRootPath;
		}
	}

	// UE_LOG(LogSoPlatformHelper, Error, TEXT("GetPackagedGameRootPath: BaseDirPath = %s ||  LaunchDirPath = %s"), *BaseDirPath, *LaunchDirPath);
	//
	// // Ran the launcher binary
	// const FString LaunchDirPath = FPaths::LaunchDir();
	//
	// // Ran directly the binary
	// if (FPaths::DirectoryExists(LaunchDirPath / TEXT("../../../Engine/Binaries/")))
	// 	return LaunchDirPath / TEXT("../../../");
	//
	// // In root already
	// if (FPaths::DirectoryExists(LaunchDirPath / TEXT("Engine/Binaries/")))
	// 	return LaunchDirPath;

	return TEXT("");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::IsSteamBuildPirated()
{
	// Cache
	static bool bIsSet = false;
	static bool bIsPirated = false;
	if (bIsSet)
		return bIsPirated;

#if WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES
	bool bPathIsPirated = false;
	bool bHashIsPirated = false;

	// Try path
	const FString EngineSteamWorksPath = GetGameRootPath() / TEXT("Engine/Binaries/ThirdParty/Steamworks/Steamv139/Win64/");
	if (FPaths::DirectoryExists(EngineSteamWorksPath))
	{
		// UE_LOG(LogSoPlatformHelper, Log, TEXT("IsSteamBuildPirated: EngineSteamWorksPath = %s"), *EngineSteamWorksPath);
		TArray<FString> Files;
		FFileManagerGeneric::Get().FindFiles(Files, *EngineSteamWorksPath, nullptr);
		for (int32 Index = 0; Index < Files.Num(); Index++)
		{
			//UE_LOG(LogSoPlatformHelper, Log, TEXT("IsSteamBuildPirated: File = %s"), *Files[Index]);
			if (Files[Index].Equals(TEXT("steam_api64.cdx"), ESearchCase::IgnoreCase))
			{
				bPathIsPirated = true;
			}
			if (Files[Index].Equals(TEXT("steam_emu.ini"), ESearchCase::IgnoreCase))
			{
				bPathIsPirated = true;
			}
		}

		// This does not work because they hook some shit onto us
		// if (FPaths::FileExists(EngineSteamWorksPath / TEXT("steam_api64.cdx")))
		// {
		// 	UE_LOG(LogSoPlatformHelper, Log, TEXT("IsSteamBuildPirated: steam_api64.cdx"));
		// 	bPathIsPirated = true;
		// }
		// else if (FPaths::FileExists(EngineSteamWorksPath / TEXT("steam_emu.ini")))
		// {
		// 	UE_LOG(LogSoPlatformHelper, Log, TEXT("IsSteamBuildPirated: steam_emu.ini"));
		// 	bPathIsPirated = true;
		// }
	}

	// Try Hash
	static const FString SteamLibraryWindowsAPI64Sha1Sum(TEXT(WARRIORB_STEAM_LIBRARY_WINDOWS_API64_SHA1_SUM));
	if (!SteamLibraryWindowsAPI64Sha1Sum.IsEmpty())
	{
		const FString CurrentHashSum = UNYSteamHelper::GetDllAPI64Sha1Sum();
		if (!CurrentHashSum.IsEmpty())
		{
			//UE_LOG(LogSoPlatformHelper, Log, TEXT("IsSteamBuildPirated: CurrentHashSum = %s"), *CurrentHashSum);
			bHashIsPirated = !SteamLibraryWindowsAPI64Sha1Sum.Equals(CurrentHashSum, ESearchCase::IgnoreCase);
		}
	}

	bIsSet = true;
	bIsPirated = bPathIsPirated || bHashIsPirated;
#else
	bIsSet = true;
	bIsPirated = false;
#endif

	return bIsPirated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::PrintAllAssets()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryName);
	TArray<FAssetData> Assets;

	AssetRegistryModule.Get().GetAllAssets(Assets, false);

	GLog->Log(TEXT("\n\nAllAssets:\n\n"));
	for (const FAssetData& Asset : Assets)
	{
		GLog->Logf(TEXT("%s"), *Asset.ObjectPath.ToString());
	}
	GLog->Log(TEXT("\n\nEND AllAssets\n\n"));
	GLog->Flush();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::PrintAllAssetsMaps()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryName);
	TArray<FAssetData> Assets;

	AssetRegistryModule.Get().GetAssetsByClass(UWorld::StaticClass()->GetFName(), Assets, true);

	GLog->Log(TEXT("\n\nMaps AllAssets:\n\n"));
	for (const FAssetData& Asset : Assets)
	{
		GLog->Logf(TEXT("%s"), *Asset.ObjectPath.ToString());
	}
	GLog->Log(TEXT("\n\nEND Maps AllAssets\n\n"));
	GLog->Flush();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoPlatformHelper::GetFirstGamepadName()
{
#if WARRIORB_WITH_SDL2
	// If only one and remapping software running we can be sure this is the one?
	// Maybe this is not such a great idea what if the user has an xbox controller connected but the remapping software is runningn
	if (NumGamepads() == 1 && UNYSDL2_GamepadHelper::IsPS4RemapperRunning())
	{
		return TEXT("PS4 Remapping software running");
	}

	return UNYSDL2_GamepadHelper::GetFirstGamepadName();
#else
	return GetGamepadName(INDEX_NONE);
#endif // WARRIORB_WITH_SDL2
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoPlatformHelper::GetGamepadName(int32 JoystickIndex)
{
#if WARRIORB_WITH_SDL2
	// If only one and remapping software running we can be sure this is the one?
	// Maybe this is not such a great idea what if the user has an xbox controller connected but the remapping software is runningn
	if (NumGamepads() == 1 && UNYSDL2_GamepadHelper::IsPS4RemapperRunning())
	{
		return TEXT("PS4 Remapping software running");
	}

	return UNYSDL2_GamepadHelper::GetGamepadName(JoystickIndex);
#elif PLATFORM_XBOXONE
	return TEXT("XboxOne Controller");
#elif PLATFORM_SWITCH
	return TEXT("Switch Controller");
#else
	return TEXT("Unknown");
#endif // WARRIORB_WITH_SDL2
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoGamepadLayoutType USoPlatformHelper::GetGamepadLayoutTypeFromName(const FString& GamepadName)
{
	if (GamepadName.Contains(TEXT("PS3")) || GamepadName.Contains(TEXT("PS4")))
	{
		return ESoGamepadLayoutType::PlayStation;
	}
	if (GamepadName.Contains(TEXT("Switch")))
	{
		return ESoGamepadLayoutType::Switch;
	}

	return ESoGamepadLayoutType::Xbox;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoPlatformHelper::GamepadLayoutToString(ESoGamepadLayoutType Layout)
{
	FString EnumValue;
	USoStringHelper::ConvertEnumToString<ESoGamepadLayoutType>(TEXT("ESoGamepadLayoutType"), Layout, false, EnumValue);
	return EnumValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlatformHelper::IsJoystickIndexAGamepad(int32 JoystickIndex)
{
#if WARRIORB_WITH_SDL2
	return UNYSDL2_GamepadHelper::IsJoystickIndexAGamepad(JoystickIndex);
#else
	// NOTE: this ignores the input
	return FSlateApplication::Get().IsGamepadAttached();
#endif // WARRIORB_WITH_SDL2
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoPlatformHelper::NumGamepads()
{
#if WARRIORB_WITH_SDL2
	return UNYSDL2_GamepadHelper::NumGamepads();
#else
	// NOTE: always reports either 0 or 1
	return FSlateApplication::Get().IsGamepadAttached();
#endif // WARRIORB_WITH_SDL2
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::PlayCameraShake(
	const UObject* WorldContextObject, TSubclassOf<UCameraShake> Shake,
	float Scale, ECameraAnimPlaySpace::Type PlaySpace, FRotator UserPlaySpaceRot)
{
	APlayerController* PlayerController = USoStaticHelper::GetPlayerController(WorldContextObject);
	if (!PlayerController)
		return;

	PlayerController->ClientPlayCameraShake(Shake, Scale, PlaySpace,UserPlaySpaceRot);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::PlayForceFeedback(
	const UObject* WorldContextObject, UForceFeedbackEffect* ForceFeedbackEffect,
	FName Tag, bool bLooping, bool bIgnoreTimeDilation, bool bPlayWhilePaused)
{
	if (!ForceFeedbackEffect)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("PlayForceFeedback: ForceFeedbackEffect == nullptr"));
		return;
	}

	// Disabled by settings
	if (!USoGameSettings::Get().IsVibrationEnabled())
		return;

	APlayerController* PlayerController = USoStaticHelper::GetPlayerController(WorldContextObject);
	if (!PlayerController)
		return;

	FForceFeedbackParameters Params;
	Params.Tag = Tag;
	Params.bLooping = bLooping;
	Params.bIgnoreTimeDilation = bIgnoreTimeDilation;
	Params.bPlayWhilePaused = bPlayWhilePaused;
	PlayerController->ClientPlayForceFeedback(ForceFeedbackEffect, Params);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::SetForceFeedbackScale(const UObject* WorldContextObject, float NewValue)
{
	APlayerController* PlayerController = USoStaticHelper::GetPlayerController(WorldContextObject);
	if (!PlayerController)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("SetForceFeedbackScale: PlayerController == nullptr"));
		return;
	}

	// Values are the same, ignore
	if (FMath::IsNearlyEqual(PlayerController->ForceFeedbackScale, NewValue))
		return;

	PlayerController->ForceFeedbackScale = NewValue;
	UE_LOG(LogSoPlatformHelper, Log, TEXT("SetForceFeedbackScale: NewValue = %f"), PlayerController->ForceFeedbackScale);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoPlatformHelper::GetForceFeedbackScale(const UObject* WorldContextObject)
{
	APlayerController* PlayerController = USoStaticHelper::GetPlayerController(WorldContextObject);
	if (!PlayerController)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("GetForceFeedbackScale: PlayerController == nullptr"));
		return USoGameSettings::DefaultVibrationScale;
	}

	return PlayerController->ForceFeedbackScale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlatformHelper::SetDisplayGamma(float NewGamma)
{
	if (!GEngine)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("SetDisplayGamma: GEngine == nullptr"));
		return;
	}

	// Values are the same, ignore
	if (FMath::IsNearlyEqual(GEngine->DisplayGamma, NewGamma))
		return;

	GEngine->DisplayGamma = NewGamma;
	UE_LOG(LogSoPlatformHelper, Log, TEXT("SetDisplayGamma: NewGamma = %f"), GEngine->DisplayGamma);
}

/////////////////////////////////////////SetForceFeedbackScale///////////////////////////////////////////////////////////////////////////////
float USoPlatformHelper::GetDisplayGamma()
{
	if (!GEngine)
	{
		UE_LOG(LogSoPlatformHelper, Error, TEXT("GetDisplayGamma: GEngine == nullptr"));
		return USoGameSettings::DefaultBrightness;
	}

	return GEngine->GetDisplayGamma();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputDeviceType USoPlatformHelper::GetHardcodedGamepadType()
{
#if PLATFORM_XBOXONE
	return ESoInputDeviceType::Gamepad_Xbox;
#elif PLATFORM_SWITCH
	return ESoInputDeviceType::Gamepad_Switch;
#elif PLATFORM_PS4
	return ESoInputDeviceType::Gamepad_PlayStation;
#else
	return ESoInputDeviceType::Keyboard;
#endif
}

#if PLATFORM_WINDOWS

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedPtr<FWindowsApplication> USoPlatformHelper::GetWindowsApplication()
{
	if (FSlateApplication::IsInitialized())
	{
		TSharedPtr<GenericApplication> GenericApp = FSlateApplication::Get().GetPlatformApplication();
		if (GenericApp.IsValid())
		{
			return StaticCastSharedPtr<FWindowsApplication>(GenericApp);
		}
	}

	return nullptr;
}

#endif // PLATFORM_WINDOWS
