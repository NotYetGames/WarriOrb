// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Camera/CameraTypes.h"

#include "SoPlatformHelper.generated.h"

struct FDisplayMetrics;
struct FSoDisplayResolution;
class FAudioDevice;
class FWindowsApplication;
class IOnlineSubsystem;
enum class ESoInputDeviceType : uint8;
class UForceFeedbackEffect;
class UCameraShake;

// Information about the current engine version
USTRUCT(BlueprintType)
struct FSoEngineInfo
{
	GENERATED_USTRUCT_BODY()
public:
	FString ToString() const;

public:
	/** The depot name, indicate where the executables and symbols are stored. */
	UPROPERTY(BlueprintReadOnly)
	FString DepotName;

	/** Product version, based on FEngineVersion. */
	UPROPERTY(BlueprintReadOnly)
	FString EngineVersion;
};


// Information about the current OS
USTRUCT(BlueprintType)
struct FSoOperatingSystemInfo
{
	GENERATED_USTRUCT_BODY()
public:
	FString ToString() const;

public:
	UPROPERTY(BlueprintReadOnly)
	FString OSMajor;

	UPROPERTY(BlueprintReadOnly)
	FString OSMinor;

	UPROPERTY(BlueprintReadOnly)
	FString OSVersion;

	UPROPERTY(BlueprintReadOnly)
	bool bIs64Bits = false;
};


// Information about the current CPU
USTRUCT(BlueprintType)
struct FSoCPUInfo
{
	GENERATED_USTRUCT_BODY()
public:
	FString ToString() const;

public:
	// Does not include hyper threading
	UPROPERTY(BlueprintReadOnly)
	int32 CPUPhysicalCores = 0;

	// Includes hyper threading
	UPROPERTY(BlueprintReadOnly)
	int32 CPULogicalCores = 0;

	UPROPERTY(BlueprintReadOnly)
	FString CPUVendor;

	UPROPERTY(BlueprintReadOnly)
	FString CPUBrand;

	/**
	 * On x86(-64) platforms, uses cpuid instruction to get the CPU signature
	 *
	 *	Bits 0-3	Stepping ID
	 *	Bits 4-7	Model
	 *	Bits 8-11	Family
	 *	Bits 12-13	Processor type (Intel) / Reserved (AMD)
	 *	Bits 14-15	Reserved
	 *	Bits 16-19	Extended model
	 *	Bits 20-27	Extended family
	 *	Bits 28-31	Reserved
	 */
	uint32 CPUInfo = 0;
};


// Information about the current CPU
USTRUCT(BlueprintType)
struct FSoGPUInfo
{
	GENERATED_USTRUCT_BODY()
public:
	FString ToString() const;

public:
	// DirectX VendorId, 0 if not set
	uint32 GPUVendorID = 0;

	uint32 GPUDeviceID = 0;

	uint32 GPUDeviceRevision = 0;

	// e.g. "NVIDIA GeForce GTX 680" or "AMD Radeon R9 200 / HD 7900 Series"
	UPROPERTY(BlueprintReadOnly)
	FString DesktopGPUAdapter;

	// e.g. "NVIDIA GeForce GTX 680" or "AMD Radeon R9 200 / HD 7900 Series"
	UPROPERTY(BlueprintReadOnly)
	FString RenderingGPUAdapter;

	// e.g. "15.200.1062.1004"(AMD)
	// e.g. "9.18.13.4788"(NVIDIA) first number is Windows version (e.g. 7:Vista, 6:XP, 4:Me, 9:Win8(1), 10:Win7), last 5 have the UserDriver version encoded
	// also called technical version number (https://wiki.mozilla.org/Blocklisting/Blocked_Graphics_Drivers)
	UPROPERTY(BlueprintReadOnly)
	FString AdapterInternalDriverVersion;

	// e.g. "Catalyst 15.7.1"(AMD) or "Crimson 15.7.1"(AMD) or "347.88"(NVIDIA)
	// also called commercial version number (https://wiki.mozilla.org/Blocklisting/Blocked_Graphics_Drivers)
	UPROPERTY(BlueprintReadOnly)
	FString AdapterUserDriverVersion;

	// e.g. 3-13-2015
	UPROPERTY(BlueprintReadOnly)
	FString AdapterDriverDate;
};


// Information about the RAM and GPU memory
USTRUCT(BlueprintType)
struct FSoPlatformMemoryInfo
{
	GENERATED_USTRUCT_BODY()
public:
	FString ToString() const;

public:
	/** The amount of physical memory currently available, in MegaBytes. */
	double RAMAvailablePhysicalMB = 0.f;

	/** The amount of virtual memory currently available, in MegaBytes. */
	double RAMAvailableVirtualMB = 0.f;

	/** The amount of physical memory used by the process, in MegaBytes. */
	double RAMUsedPhysicalMB = 0.f;

	/** The peak amount of physical memory used by the process, in MegaBytes. */
	double RAMPeakUsedPhysicalMB = 0.f;

	/** Total amount of virtual memory used by the process, in MegaBytes. */
	double RAMUsedVirtualMB = 0.f;

	/** The peak amount of virtual memory used by the process, in MegaBytes. */
	double RAMPeakUsedVirtualMB = 0.f;

	/** Amount of memory allocated by textures. In MegaBytes. */
	double GPUTextureMemoryMB = 0.f;

	/** Amount of memory allocated by rendertargets. In MegaBytes. */
	double GPURenderTargetMemoryMB = 0.f;

	/** In percent. If non-zero, the texture pool size is a percentage of GTotalGraphicsMemory. */
	UPROPERTY(BlueprintReadOnly)
	int32 GPUPoolSizeVRAMPercentage = 0;
};

// Corresponds to the one in XboxOneMisc.h
UENUM(BlueprintType)
enum class ESoXboxOneConsoleType : uint8
{
	Invalid,
	XboxOne		UMETA(DisplayName = "Xbox One"),
	XboxOneS 	UMETA(DisplayName = "Xbox One S"),
	XboxOneX 	UMETA(DisplayName = "Xbox One X (Scorpio)")
};


UENUM(BlueprintType)
enum class ESoGamepadLayoutType : uint8
{
	Xbox = 0		UMETA(DisplayName = "Xbox"),
	PlayStation		UMETA(DisplayName = "PlayStation"),
	Switch			UMETA(DisplayName = "Nintendo Switch"),

	Num				UMETA(Hidden)
};


UCLASS()
class SORB_API USoPlatformHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Prints error to all outputs possible
	static void PrintErrorToAll(const UObject* WorldContextObject, const FString& Message);
	static void PrintToAll(const UObject* WorldContextObject, const FString& Message);

	// Prints the message to the developer console
	static void PrintToConsole(const UObject* WorldContextObject, const FString& Message);

	// Prints the message to the screen for TimeToDisplaySeconds
	static void PrintToScreen(const FString& Message, float TimeToDisplaySeconds = 5.f, FColor DisplayColor = FColor::White);

	// Safer version of LaunchURL
	// Opens the specified URL in the platform's web browser of choice
	UFUNCTION(BlueprintCallable, Category = ">Platform")
	static bool LaunchURL(const FString& URL, bool bCopyURLToClipboard = true);

	// Safe version of ExploreFolder
	UFUNCTION(BlueprintCallable, Category = ">Platform")
	static bool ExploreFolder(const FString& FolderPath);

	// By default in non shipping and non editor builds GC verify is on
	// https://answers.unrealengine.com/questions/177892/game-hitches-gc-mark-time.html
	// We disable it by default if no command line flags are given
	static void DisableVerifyGC(bool bCheckForCommandLineOverrides = true);

	//
	// Disables/Enables all AI Logging
	// determines whether AI logging should be processed or not
	//

	static void SetIsAILoggingEnabled(bool bEnabled, bool bCheckForCommandLineOverrides);
	static void DisableAILogging(bool bCheckForCommandLineOverrides = true)
	{
		SetIsAILoggingEnabled(false, bCheckForCommandLineOverrides);
	}
	static void EnableAILogging(bool bCheckForCommandLineOverrides = true)
	{
		SetIsAILoggingEnabled(true, bCheckForCommandLineOverrides);
	}

	//
	// Disables/Enables all screen messages
	//

	static void SetAreScreenMessagesEnabled(bool bEnabled, bool bCheckForCommandLineOverrides);
	static void DisableScreenMessages(bool bCheckForCommandLineOverrides = true)
	{
		SetAreScreenMessagesEnabled(false, bCheckForCommandLineOverrides);
	}
	static void EnableScreenMessages(bool bCheckForCommandLineOverrides = true)
	{
		SetAreScreenMessagesEnabled(true, bCheckForCommandLineOverrides);
	}

	//
	// Disables/Enables Mouse support
	//

	static void SetIsMouseEnabled(const UObject* WorldContextObject, bool bEnabled);
	static void SetIsMouseAllowed(const UObject* WorldContextObject, bool bAllowed);

	UFUNCTION(BlueprintPure, Category = ">Mouse", meta = (WorldContext = "WorldContextObject"))
	static bool IsMouseAllowed(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = ">Mouse", meta = (WorldContext = "WorldContextObject"))
	static void DisallowMouse(const UObject* WorldContextObject)
	{
		SetIsMouseAllowed(WorldContextObject, false);
	}

	UFUNCTION(BlueprintCallable, Category = ">Mouse", meta = (WorldContext = "WorldContextObject"))
	static void AllowMouse(const UObject* WorldContextObject)
	{
		SetIsMouseAllowed(WorldContextObject, true);
	}

	//
	// Screen/Game
	//

	// Moves the game window to the default screen of the user
	static void MoveGameToDefaultMonitor();

	/**
	 * Gets the list of support fullscreen resolutions.
	 * @return true if successfully queried the device for available resolutions.
	 */
	static bool GetSupportedFullscreenResolutions(TArray<FSoDisplayResolution>& OutResolutions);

	/**
	 * Gets the list of windowed resolutions which are convenient for the current primary display size.
	 * @return true if successfully queried the device for available resolutions.
	 */
	static bool GetConvenientWindowedResolutions(TArray<FSoDisplayResolution>& OutResolutions);

	// Gets the refresh of all the displays as a set of ints
	static bool GetDisplaysRefreshRates(TSet<int32>& OutRefreshRates);

	// Gets info about all the displays
	static bool GetDisplaysMetrics(FDisplayMetrics& OutDisplayMetrics);

	/** Gets the AudioDevice from the world if it supports audio and it has an audio device. */
	static FAudioDevice* GetAudioDevice(const UObject* WorldContextObject);

	/**
	 * Creates the folders for this FolderPath.
	 * Can be a RootDirectory/Directory or RootDirectory/Directory/Somefile.txt
	 * in Both cases it will create the RootDirectory/Directory path
	 */
	static bool CreateDirectoryRecursively(const FString& FolderPath);

	// What it says, quits the game
	static void QuitGame(const UObject* WorldContextObject = nullptr);

	// Is the game in background?
	static bool IsGameInBackground(UObject* WorldContextObject = nullptr) { return !IsGameInForeground(WorldContextObject); }

	// Is the game in foreground, aka focused
	static bool IsGameInForeground(UObject* WorldContextObject = nullptr);

	// Sets the pause state of the game
	static bool SetGamePaused(const UObject* WorldContextObject, bool bPaused);

	// Returns the game's paused state
	static bool IsGamePaused(const UObject* WorldContextObject);

	// Sets the global time dilation
	static void SetGlobalTimeDilation(const UObject* WorldContextObject, float TimeDilation);

	// Gets the global time dilation
	static float GetGlobalTimeDilation(const UObject* WorldContextObject);

	// Are all game windows hidden?
	static bool AreAllGameWindowsHidden();

	// Gets the Unreal Engine version information
	static const FSoEngineInfo& GetUnrealEngineInfo();

	// Gets information about the operating system
	static FSoOperatingSystemInfo GetOperatingSystemInfo();

	//
	// Platform
	//

	// Helper method, to get all the info about the game that is not character related
	static FString ToStringPlatformContext();

	// Your GPU sucks
	static void WarnIfGPUIsBlacklisted();

	// Gets information about the CPU
	static FSoCPUInfo GetCPUInfo();

	// Gets information about the CPU
	static FSoGPUInfo GetGPUInfo();

	// Gets information about the memory
	static FSoPlatformMemoryInfo GetMemoryInfo();

	// Gets whether user settings should override the resolution or not
	static bool HasFixedResolution();

	// Gets whether user settings should override the VSync or not
	// Usually only true in editor and consoles (xbox, ps4, switch)
	static bool HasFixedVSync();

	// Gets whether this platform supports windowed mode rendering.
	static bool SupportsWindowedMode();

	// Whether the platform allows an application to quit to the OS
	// Apparently only false on Switch
	static bool SupportsQuit();

	// Returns the root of the packaged game
	static FString GetGameRootPath();

	/**
	 * Returns the string name of the current platform, to perform different behavior based on platform.
	 * (Platform names include Windows, Mac, IOS, Android, PS4, Switch, XboxOne, HTML5, Linux)
	 */
	static FString GetPlatformName();

	// The platform has shit hardware
	UFUNCTION(BlueprintPure, Category = ">Platform", DisplayName = "Does Platform Has Weak Hardware")
	static bool HasWeakHardware();

	// Some consoles has fixed
	static bool HasFixedWeakHardware()
	{
		return IsSwitch()
			|| IsXboxOneConsoleType()
			|| IsXboxOneSConsoleType();
	}

	UFUNCTION(BlueprintPure, Category = ">Platform", DisplayName = "Is Platform Switch")
	static bool IsSwitch() { return PLATFORM_SWITCH; }

	UFUNCTION(BlueprintPure, Category = ">Platform", DisplayName = "Is Platform Xbox One")
	static bool IsXboxOne() { return PLATFORM_XBOXONE; }

	UFUNCTION(BlueprintPure, Category = ">Platform", DisplayName = "Is Platform Xbox One && Console == Xbox One")
	static bool IsXboxOneConsoleType() { return GetXboxOneConsoleType() == ESoXboxOneConsoleType::XboxOne; }

	UFUNCTION(BlueprintPure, Category = ">Platform", DisplayName = "Is Platform Xbox One && Console == Xbox One S")
	static bool IsXboxOneSConsoleType() { return GetXboxOneConsoleType() == ESoXboxOneConsoleType::XboxOneS; }

	UFUNCTION(BlueprintPure, Category = ">Platform", DisplayName = "Is Platform Xbox One && Console == Xbox One X")
	static bool IsXboxOneXConsoleType() { return GetXboxOneConsoleType() == ESoXboxOneConsoleType::XboxOneX; }

	UFUNCTION(BlueprintPure, Category = ">Platform")
	static ESoXboxOneConsoleType GetXboxOneConsoleType();

	UFUNCTION(BlueprintPure, Category = ">Platform", DisplayName = "Is Platform Linux")
	static bool IsLinux() { return PLATFORM_LINUX; }

	UFUNCTION(BlueprintPure, Category = ">Platform", DisplayName = "Is Platform Windows")
	static bool IsWindows() { return PLATFORM_WINDOWS; }

	UFUNCTION(BlueprintPure, Category = ">Platform", DisplayName = "Is Platform Mac")
	static bool IsMac() { return PLATFORM_MAC; }

	//
	// Build
	//

	// Helper method to get the string of some compiled options like WITH_EDITOr and such, WITH_STATS
	static const FString& ToStringCompileFlags();

	// Gets the build version of this game (Warriorb) duh.
	UFUNCTION(BlueprintPure, Category = ">Build")
	static const FString& GetGameBuildVersion();

	// Gets git branch
	UFUNCTION(BlueprintPure, Category = ">Build")
	static const FString& GetGameBuildBranch();

	// Gets git commit hash
	UFUNCTION(BlueprintPure, Category = ">Build")
	static const FString& GetGameBuildCommit();

	// Format: BuildVersion - BuildCommit BuildBanch
	UFUNCTION(BlueprintPure, Category = ">Build")
	static FString GetGameBuildAll();

	UFUNCTION(BlueprintPure, Category = ">Build")
	static bool IsDemo() { return WARRIORB_DEMO; }

	// The opposite of IsDemo
	UFUNCTION(BlueprintPure, Category = ">Build")
	static bool IsNormalGame() { return !IsDemo(); }

	//
	// Steam
	//

	UFUNCTION(BlueprintPure, Category = ">Build")
	static bool IsWithSteam() { return WARRIORB_WITH_STEAM; }

	static bool InitializedSteam();

	// NOTE: this returns a string in the format specified here https://partner.steamgames.com/doc/store/localization#8
	static FString GetSteamCurrentGameLanguage();

	// Queries current app
	static FString GetCurrentSteamAppID();

	UFUNCTION(BlueprintPure, Category = ">Steam")
	static int32 GetSteamAppID(bool bDemo)
	{
		return bDemo ? WARRIORB_STEAM_DEMO_APP_ID : WARRIORB_STEAM_APP_ID;
	}

	static bool IsSteamInitialized();

	static bool OpenSteamOverlayToStore(uint32 AppID, bool bAddToCart = false, bool bShowCart = false);

	// Seems kinda stupid because if you have the controller connected and you launch the game from the NON big
	// picture interface this will still report as true
	static bool IsSteamBigPicture();

	// NOTE: only works on windows and if the check is enabled
	UFUNCTION(BlueprintPure, Category = ">Steam")
	static bool IsSteamBuildPirated();

	//
	// Discord
	//

	static int64 GetDiscordClientID(bool bDemo)
	{
		return bDemo ? WARRIORB_DISCORD_DEMO_CLIENT_ID : WARRIORB_DISCORD_CLIENT_ID;
	}

	//
	// Assets
	//

	UFUNCTION(Exec)
	static void PrintAllAssets();

	UFUNCTION(Exec)
	static void PrintAllAssetsMaps();

	//
	// Keyboard
	//

	UFUNCTION(BlueprintPure, Category = ">AuraSDK")
	static bool UseAsusAuraSDK() { return PLATFORM_WINDOWS && WARRIORB_USE_ASUS_AURA_SDK; }

	//
	// Camera
	//

	/**
	 * Play Camera Shake
	 * @param Shake - Camera shake animation to play
	 * @param Scale - Scalar defining how "intense" to play the anim
	 * @param PlaySpace - Which coordinate system to play the shake in (used for CameraAnims within the shake).
	 * @param UserPlaySpaceRot - Matrix used when PlaySpace = CAPS_UserDefined
	 */
	UFUNCTION(BlueprintCallable, Category=">Camera", meta = (WorldContext = "WorldContextObject"))
	static void PlayCameraShake(
		const UObject* WorldContextObject,
		TSubclassOf<UCameraShake> Shake,
		float Scale = 1.f,
		ECameraAnimPlaySpace::Type PlaySpace = ECameraAnimPlaySpace::CameraLocal,
		FRotator UserPlaySpaceRot = FRotator::ZeroRotator
	);


	//
	// Gamepad
	//

	static bool IsAnyGamepadConnected() { return NumGamepads() > 0; }
	static FString GetFirstGamepadName();
	static FString GetGamepadName(int32 JoystickIndex);
	static ESoGamepadLayoutType GetGamepadLayoutTypeFromName(const FString& GamepadName);
	static FString GamepadLayoutToString(ESoGamepadLayoutType Layout);
	static bool IsJoystickIndexAGamepad(int32 JoystickIndex);
	static int32 NumGamepads();

	// Does the current platform has a hard coded game controller
	static bool HasHardcodedGamepad()
	{
		return PLATFORM_XBOXONE || PLATFORM_SWITCH || PLATFORM_PS4;
	}

	// Get the hard coded game controller type
	static ESoInputDeviceType GetHardcodedGamepadType();

	// Should we use the UI for console?
	UFUNCTION(BlueprintPure, Category = ">Platform")
	static bool IsConsole()
	{
		return PLATFORM_XBOXONE || PLATFORM_SWITCH || PLATFORM_PS4 || WARRIORB_CONSOLE_TEST;
	}

	UFUNCTION(BlueprintPure, Category = ">Platform")
	static bool CanHaveKeyboard()
	{
		if (WARRIORB_NO_KEYBOARD_TEST)
			return false;

		return PLATFORM_DESKTOP || PLATFORM_XBOXONE;
	}

	//
	// Gamepad force feedback (aka vibration)
	//

	/**
	 * Play a force feedback pattern on the player's controller
	 * @param	ForceFeedbackEffect		The force feedback pattern to play
	 * @param	bLooping				Whether the pattern should be played repeatedly or be a single one shot
	 * @param	bIgnoreTimeDilation		Whether the pattern should ignore time dilation
	 * @param	bPlayWhilePaused		Whether the pattern should continue to play while the game is paused
	 * @param	Tag						A tag that allows stopping of an effect.  If another effect with this Tag is playing, it will be stopped and replaced
	 */
	UFUNCTION(BlueprintCallable, Category = ">Gamepad|ForceFeedback", meta = (WorldContext = "WorldContextObject"))
	static void PlayForceFeedback(
		const UObject* WorldContextObject,
		UForceFeedbackEffect* ForceFeedbackEffect,
		FName Tag = NAME_None,
		bool bLooping = false,
		bool bIgnoreTimeDilation = false,
		bool bPlayWhilePaused = false
	);

	static void SetForceFeedbackScale(const UObject* WorldContextObject, float NewValue);
	static float GetForceFeedbackScale(const UObject* WorldContextObject);

	//
	// Display Gamma
	//

	static void SetDisplayGamma(float NewGamma);
	static float GetDisplayGamma();

	//
	// OS specific
	//

#if PLATFORM_WINDOWS
	static TSharedPtr<FWindowsApplication> GetWindowsApplication();
#endif
};
