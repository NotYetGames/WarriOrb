// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#include "FMODStudioModule.h"
#include "FMODSettings.h"
#include "FMODAudioComponent.h"
#include "FMODBlueprintStatics.h"
#include "FMODAssetTable.h"
#include "FMODFileCallbacks.h"
#include "FMODBankUpdateNotifier.h"
#include "FMODUtils.h"
#include "FMODEvent.h"
#include "FMODListener.h"
#include "FMODSnapshotReverb.h"

#include "Async/Async.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
//#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Containers/Ticker.h"
#include "Misc/Paths.h"
#include "Runtime/Media/Public/IMediaClock.h"
#include "Runtime/Media/Public/IMediaClockSink.h"
#include "Runtime/Media/Public/IMediaModule.h"
#include "TimerManager.h"

#include "fmod_studio.hpp"
#include "fmod_errors.h"
#include "FMODStudioPrivatePCH.h"

#if PLATFORM_PS4
#include "FMODPlatformLoadDll_PS4.h"
#elif PLATFORM_XBOXONE
#include "FMODPlatformLoadDll_XBoxOne.h"
#elif PLATFORM_SWITCH
#include "FMODPlatformLoadDll_Switch.h"
#else
#include "FMODPlatformLoadDll_Generic.h"
#endif

#define LOCTEXT_NAMESPACE "FMODStudio"

DEFINE_LOG_CATEGORY(LogFMOD);

DECLARE_STATS_GROUP(TEXT("FMOD"), STATGROUP_FMOD, STATCAT_Advanced);
DECLARE_FLOAT_COUNTER_STAT(TEXT("FMOD CPU - Mixer"), STAT_FMOD_CPUMixer, STATGROUP_FMOD);
DECLARE_FLOAT_COUNTER_STAT(TEXT("FMOD CPU - Studio"), STAT_FMOD_CPUStudio, STATGROUP_FMOD);
DECLARE_MEMORY_STAT(TEXT("FMOD Memory - Current"), STAT_FMOD_Current_Memory, STATGROUP_FMOD);
DECLARE_MEMORY_STAT(TEXT("FMOD Memory - Max"), STAT_FMOD_Max_Memory, STATGROUP_FMOD);
DECLARE_DWORD_COUNTER_STAT(TEXT("FMOD Channels - Total"), STAT_FMOD_Total_Channels, STATGROUP_FMOD);
DECLARE_DWORD_COUNTER_STAT(TEXT("FMOD Channels - Real"), STAT_FMOD_Real_Channels, STATGROUP_FMOD);

const TCHAR *FMODSystemContextNames[EFMODSystemContext::Max] = {
    TEXT("Auditioning"), TEXT("Runtime"), TEXT("Editor"),
};

void *F_CALLBACK FMODMemoryAlloc(unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr)
{
    return FMemory::Malloc(size);
}
void *F_CALLBACK FMODMemoryRealloc(void *ptr, unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr)
{
    return FMemory::Realloc(ptr, size);
}
void F_CALLBACK FMODMemoryFree(void *ptr, FMOD_MEMORY_TYPE type, const char *sourcestr)
{
    FMemory::Free(ptr);
}

struct FFMODSnapshotEntry
{
    FFMODSnapshotEntry(UFMODSnapshotReverb *InSnapshot = nullptr, FMOD::Studio::EventInstance *InInstance = nullptr)
        : Snapshot(InSnapshot)
        , Instance(InInstance)
        , StartTime(0.0)
        , FadeDuration(0.0f)
        , FadeIntensityStart(0.0f)
        , FadeIntensityEnd(0.0f)
    {
    }

    float CurrentIntensity() const
    {
        double CurrentTime = FApp::GetCurrentTime();
        if (StartTime + FadeDuration <= CurrentTime)
        {
            return FadeIntensityEnd;
        }
        else
        {
            float Factor = (CurrentTime - StartTime) / FadeDuration;
            return FMath::Lerp<float>(FadeIntensityStart, FadeIntensityEnd, Factor);
        }
    }

    void FadeTo(float Target, float Duration)
    {
        float StartIntensity = CurrentIntensity();

        StartTime = FApp::GetCurrentTime();
        FadeDuration = Duration;
        FadeIntensityStart = StartIntensity;
        FadeIntensityEnd = Target;
    }

    UFMODSnapshotReverb *Snapshot;
    FMOD::Studio::EventInstance *Instance;
    double StartTime;
    float FadeDuration;
    float FadeIntensityStart;
    float FadeIntensityEnd;
};

class FFMODStudioSystemClockSink : public IMediaClockSink
{
public:
    DECLARE_DELEGATE_RetVal(void, FUpdateListenerPosition);

    FFMODStudioSystemClockSink(FMOD::Studio::System *SystemIn)
        : System(SystemIn)
        , LastResult(FMOD_OK)
    {
    }

    virtual void TickRender(FTimespan DeltaTime, FTimespan Timecode) override
    {
        if (System)
        {
            if (UpdateListenerPosition.IsBound())
            {
                UpdateListenerPosition.Execute();
            }

            LastResult = System->update();
        }
    }

    void SetUpdateListenerPositionDelegate(FUpdateListenerPosition UpdateListenerPositionIn) { UpdateListenerPosition = UpdateListenerPositionIn; }

    void OnDestroyStudioSystem() { System = nullptr; }

    FMOD::Studio::System *System;
    FMOD_RESULT LastResult;
    FUpdateListenerPosition UpdateListenerPosition;
};

class FFMODStudioModule : public IFMODStudioModule
{
public:
    /** IModuleInterface implementation */
    FFMODStudioModule()
        : AuditioningInstance(nullptr)
        , ListenerCount(1)
        , bSimulating(false)
        , bIsInPIE(false)
        , bUseSound(true)
        , bListenerMoved(true)
		, bAllowLiveUpdate(WITH_EDITOR)
        , bBanksLoaded(false)
        , LowLevelLibHandle(nullptr)
        , StudioLibHandle(nullptr)
        , bMixerPaused(false)
        , MemPool(nullptr)
    {
        for (int i = 0; i < EFMODSystemContext::Max; ++i)
        {
            StudioSystem[i] = nullptr;
        }
    }

    void HandleApplicationWillDeactivate()
    {
        AsyncTask(ENamedThreads::GameThread, [&]() { SetSystemPaused(true); });
    }
    void HandleApplicationHasReactivated()
    {
        AsyncTask(ENamedThreads::GameThread, [&]() { SetSystemPaused(false); });
    }

    virtual void StartupModule() override;
    virtual void PostLoadCallback() override;
    virtual void ShutdownModule() override;

    FString GetDllPath(const TCHAR *ShortName, bool bExplicitPath, bool bUseLibPrefix);
    void *LoadDll(const TCHAR *ShortName);

    bool LoadLibraries();

    void LoadBanks(EFMODSystemContext::Type Type);

    /** Called when a newer version of the bank files was detected */
    void HandleBanksUpdated();

    void CreateStudioSystem(EFMODSystemContext::Type Type);
    void DestroyStudioSystem(EFMODSystemContext::Type Type);

    bool Tick(float DeltaTime);

    void UpdateViewportPosition();

    virtual FMOD::Studio::System *GetStudioSystem(EFMODSystemContext::Type Context) override;
    virtual FMOD::Studio::EventDescription *GetEventDescription(const UFMODEvent *Event, EFMODSystemContext::Type Type) override;
    virtual FMOD::Studio::EventInstance *CreateAuditioningInstance(const UFMODEvent *Event) override;
    virtual void StopAuditioningInstance() override;

    virtual void SetListenerPosition(int ListenerIndex, UWorld *World, const FTransform &ListenerTransform, float DeltaSeconds) override;
    virtual void FinishSetListenerPosition(int ListenerCount, float DeltaSeconds) override;

    virtual const FFMODListener &GetNearestListener(const FVector &Location) override;

    virtual bool HasListenerMoved() override;

    virtual void RefreshSettings();

    virtual void SetSystemPaused(bool paused) override;

    virtual void SetInPIE(bool bInPIE, bool simulating) override;

    virtual UFMODAsset *FindAssetByName(const FString &Name) override;
    virtual UFMODEvent *FindEventByName(const FString &Name) override;

    FSimpleMulticastDelegate BanksReloadedDelegate;
    virtual FSimpleMulticastDelegate &BanksReloadedEvent() override { return BanksReloadedDelegate; }

    virtual TArray<FString> GetFailedBankLoads(EFMODSystemContext::Type Context) override { return FailedBankLoads[Context]; }

    virtual TArray<FString> GetRequiredPlugins() override { return RequiredPlugins; }

    virtual void AddRequiredPlugin(const FString &Plugin)
    {
        if (!RequiredPlugins.Contains(Plugin))
        {
            RequiredPlugins.Add(Plugin);
        }
    }

    virtual bool UseSound() override { return bUseSound; }

    virtual bool LoadPlugin(EFMODSystemContext::Type Context, const TCHAR *ShortName) override;

    virtual void LogError(int result, const char *function) override;

    virtual bool AreBanksLoaded() override;

    void ResetInterpolation();

    /** The studio system handle. */
    FMOD::Studio::System *StudioSystem[EFMODSystemContext::Max];
    FMOD::Studio::EventInstance *AuditioningInstance;

    /** The delegate to be invoked when this profiler manager ticks. */
    FTickerDelegate OnTick;

    /** IMediaClockSink wrappers for Studio Systems */
    TSharedPtr<FFMODStudioSystemClockSink, ESPMode::ThreadSafe> ClockSinks[EFMODSystemContext::Max];

    /** Handle for registered TickDelegate. */
    FDelegateHandle TickDelegateHandle;

    /** Table of assets with name and guid */
    FFMODAssetTable AssetTable;

    /** Periodically checks for updates of the strings.bank file */
    FFMODBankUpdateNotifier BankUpdateNotifier;

    /** List of failed bank files */
    TArray<FString> FailedBankLoads[EFMODSystemContext::Max];

    /** List of required plugins we found when loading banks. */
    TArray<FString> RequiredPlugins;

/** Listener information */
#if FMOD_VERSION >= 0x00010600
    static const int MAX_LISTENERS = FMOD_MAX_LISTENERS;
#else
    static const int MAX_LISTENERS = 1;
#endif
    FFMODListener Listeners[MAX_LISTENERS];
    int ListenerCount;

    /** Current snapshot applied via reverb zones*/
    TArray<FFMODSnapshotEntry> ReverbSnapshots;

    /** True if simulating */
    bool bSimulating;

    /** True if in PIE */
    bool bIsInPIE;

    /** True if we want sound enabled */
    bool bUseSound;

    /** True if we the listener has moved and may have changed audio settings*/
    bool bListenerMoved;

    /** True if we allow live update */
    bool bAllowLiveUpdate;

    bool bBanksLoaded;

    /** Dynamic library */
    FString BaseLibPath;
    void *LowLevelLibHandle;
    void *StudioLibHandle;

    /** True if the mixer has been paused by application deactivation */
    bool bMixerPaused;

    /** You can also supply a pool of memory for FMOD to work with and it will do so with no extra calls to malloc or free. */
    void *MemPool;

    bool bLoadAllSampleData;
};

IMPLEMENT_MODULE(FFMODStudioModule, FMODStudio)

void FFMODStudioModule::LogError(int result, const char *function)
{
    FString ErrorStr(ANSI_TO_TCHAR(FMOD_ErrorString((FMOD_RESULT)result)));
    FString FunctionStr(ANSI_TO_TCHAR(function));
    UE_LOG(LogFMOD, Error, TEXT("'%s' returned '%s'"), *FunctionStr, *ErrorStr);
}

bool FFMODStudioModule::LoadPlugin(EFMODSystemContext::Type Context, const TCHAR *ShortName)
{
    UE_LOG(LogFMOD, Log, TEXT("Loading plugin '%s'"), ShortName);

    static const int ATTEMPT_COUNT = 2;
    static const TCHAR *AttemptPrefixes[ATTEMPT_COUNT] = {
        TEXT(""),
#if PLATFORM_64BITS
        TEXT("64")
#else
        TEXT("32")
#endif
    };

    FMOD::System *LowLevelSystem = nullptr;
    verifyfmod(StudioSystem[Context]->getLowLevelSystem(&LowLevelSystem));

    FMOD_RESULT PluginLoadResult;

    for (int useLib = 0; useLib < 2; ++useLib)
    {
        for (int attempt = 0; attempt < 2; ++attempt)
        {
            // Try to load combinations of 64/32 suffix and lib prefix for relevant platforms
            FString AttemptName = FString(ShortName) + AttemptPrefixes[attempt];
            FString PluginPath = GetDllPath(*AttemptName, true, useLib != 0);

            UE_LOG(LogFMOD, Log, TEXT("Trying to load plugin file at location: %s"), *PluginPath);

#if defined(PLATFORM_UWP) && PLATFORM_UWP
            FPaths::MakePathRelativeTo(PluginPath, *(FPaths::RootDir() + TEXT("/")));
#endif

            unsigned int Handle = 0;
            PluginLoadResult = LowLevelSystem->loadPlugin(TCHAR_TO_UTF8(*PluginPath), &Handle, 0);
            if (PluginLoadResult == FMOD_OK)
            {
                UE_LOG(LogFMOD, Log, TEXT("Loaded plugin %s"), ShortName);
                return true;
            }
        }
    }
    UE_LOG(LogFMOD, Error, TEXT("Failed to load plugin '%s', sounds may not play"), ShortName);
    return false;
}

void *FFMODStudioModule::LoadDll(const TCHAR *ShortName)
{
    FString LibPath = GetDllPath(ShortName, false, true);

    void *Handle = nullptr;
    UE_LOG(LogFMOD, Log, TEXT("FFMODStudioModule::LoadDll: Loading %s"), *LibPath);
    // Unfortunately Unreal's platform loading code hasn't been implemented on all platforms so we wrap it
    Handle = FMODPlatformLoadDll(*LibPath);
#if WITH_EDITOR
    if (!Handle && !FApp::IsUnattended())
    {
        FString Message = TEXT("Couldn't load FMOD DLL ") + LibPath;
        FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *Message, TEXT("Error"));
    }
#endif
    if (!Handle)
    {
        UE_LOG(LogFMOD, Error, TEXT("Failed to load FMOD DLL '%s', FMOD sounds will not play!"), *LibPath);
    }
    return Handle;
}

FString FFMODStudioModule::GetDllPath(const TCHAR *ShortName, bool bExplicitPath, bool bUseLibPrefix)
{
    const TCHAR *LibPrefixName = (bUseLibPrefix ? TEXT("lib") : TEXT(""));
#if PLATFORM_MAC
    return FString::Printf(TEXT("%s/Mac/%s%s.dylib"), *BaseLibPath, LibPrefixName, ShortName);
#elif PLATFORM_PS4
    const TCHAR *DirPrefix = (bExplicitPath ? TEXT("/app0/prx/") : TEXT(""));
    return FString::Printf(TEXT("%s%s%s.prx"), DirPrefix, LibPrefixName, ShortName);
#elif PLATFORM_XBOXONE
    return FString::Printf(TEXT("%s/XBoxOne/%s.dll"), *BaseLibPath, ShortName);
#elif PLATFORM_ANDROID
    return FString::Printf(TEXT("%s%s.so"), LibPrefixName, ShortName);
#elif PLATFORM_LINUX
    return FString::Printf(TEXT("%s%s.so"), LibPrefixName, ShortName);
#elif PLATFORM_WINDOWS
#if PLATFORM_64BITS
    return FString::Printf(TEXT("%s/Win64/%s.dll"), *BaseLibPath, ShortName);
#else
    return FString::Printf(TEXT("%s/Win32/%s.dll"), *BaseLibPath, ShortName);
#endif
#elif defined(PLATFORM_UWP) && PLATFORM_UWP
    return FString::Printf(TEXT("%s/UWP64/%s.dll"), *BaseLibPath, ShortName);
#else
    UE_LOG(LogFMOD, Error, TEXT("Unsupported platform for dynamic libs"));
    return "";
#endif
}

bool FFMODStudioModule::LoadLibraries()
{
#if PLATFORM_IOS || PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_SWITCH
    return true; // Nothing to do on those platforms
#elif PLATFORM_HTML5
    UE_LOG(LogFMOD, Error, TEXT("FMOD Studio not supported on HTML5"));
    return false; // Explicitly don't support this
#else
    UE_LOG(LogFMOD, Verbose, TEXT("FFMODStudioModule::LoadLibraries"));

#if defined(FMODSTUDIO_LINK_DEBUG)
    FString ConfigName = TEXT("D");
#elif defined(FMODSTUDIO_LINK_LOGGING)
    FString ConfigName = TEXT("L");
#elif defined(FMODSTUDIO_LINK_RELEASE)
    FString ConfigName = TEXT("");
#else
#error FMODSTUDIO_LINK not defined
#endif

#if PLATFORM_WINDOWS && PLATFORM_64BITS
    ConfigName += TEXT("64");
#elif defined(PLATFORM_UWP) && PLATFORM_UWP
    ConfigName += TEXT("_X64");
#endif

    FString LowLevelName = FString(TEXT("fmod")) + ConfigName;
    FString StudioName = FString(TEXT("fmodstudio")) + ConfigName;
    LowLevelLibHandle = LoadDll(*LowLevelName);
    StudioLibHandle = LoadDll(*StudioName);
    return (LowLevelLibHandle != nullptr && StudioLibHandle != nullptr);
#endif
}

void FFMODStudioModule::StartupModule()
{
    UE_LOG(LogFMOD, Log, TEXT("FFMODStudioModule startup"));
    BaseLibPath = IPluginManager::Get().FindPlugin(TEXT("FMODStudio"))->GetBaseDir() + TEXT("/Binaries");
    UE_LOG(LogFMOD, Log, TEXT(" Lib path = '%s'"), *BaseLibPath);

    if (FParse::Param(FCommandLine::Get(), TEXT("nosound")) || FApp::IsBenchmarking() || IsRunningDedicatedServer() || IsRunningCommandlet())
    {
        bUseSound = false;
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("noliveupdate")))
    {
        bAllowLiveUpdate = false;
    }

    if (LoadLibraries())
    {
        verifyfmod(FMOD::Debug_Initialize(FMOD_DEBUG_LEVEL_WARNING, FMOD_DEBUG_MODE_CALLBACK, FMODLogCallback));

        const UFMODSettings &Settings = *GetDefault<UFMODSettings>();
#if PLATFORM_IOS || PLATFORM_ANDROID
        int size = Settings.MemoryPoolSizes.Mobile;
#elif PLATFORM_PS4
        int size = Settings.MemoryPoolSizes.PS4;
#elif PLATFORM_XBOXONE
        int size = Settings.MemoryPoolSizes.XboxOne;
#elif PLATFORM_SWITCH
        int size = Settings.MemoryPoolSizes.Switch;
#else
        int size = Settings.MemoryPoolSizes.Desktop;
#endif
        if (!GIsEditor && size > 0)
        {
            MemPool = FMemory::Malloc(size);
            verifyfmod(FMOD::Memory_Initialize(MemPool, size, nullptr, nullptr, nullptr));
        }
        else
        {
            verifyfmod(FMOD::Memory_Initialize(0, 0, FMODMemoryAlloc, FMODMemoryRealloc, FMODMemoryFree));
        }

        verifyfmod(FMODPlatformSystemSetup());

        AcquireFMODFileSystem();

        // Create sandbox system just for asset loading
        AssetTable.Create();
        RefreshSettings();

        if (GIsEditor)
        {
            CreateStudioSystem(EFMODSystemContext::Auditioning);
            CreateStudioSystem(EFMODSystemContext::Editor);
        }
        else
        {
            AssetTable.Destroy(); // Don't need this copy around since we don't hot reload

            SetInPIE(true, false);
        }
    }

    OnTick = FTickerDelegate::CreateRaw(this, &FFMODStudioModule::Tick);
    TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(OnTick);

    if (GIsEditor)
    {
        BankUpdateNotifier.BanksUpdatedEvent.AddRaw(this, &FFMODStudioModule::HandleBanksUpdated);
    }
}

inline FMOD_SPEAKERMODE ConvertSpeakerMode(EFMODSpeakerMode::Type Mode)
{
    switch (Mode)
    {
        case EFMODSpeakerMode::Stereo:
            return FMOD_SPEAKERMODE_STEREO;
        case EFMODSpeakerMode::Surround_5_1:
            return FMOD_SPEAKERMODE_5POINT1;
        case EFMODSpeakerMode::Surround_7_1:
            return FMOD_SPEAKERMODE_7POINT1;
        default:
            check(0);
            return FMOD_SPEAKERMODE_DEFAULT;
    };
}

void FFMODStudioModule::CreateStudioSystem(EFMODSystemContext::Type Type)
{
    DestroyStudioSystem(Type);
    if (!bUseSound)
    {
        return;
    }

    UE_LOG(LogFMOD, Verbose, TEXT("CreateStudioSystem for context %s"), FMODSystemContextNames[Type]);

    const UFMODSettings &Settings = *GetDefault<UFMODSettings>();
    bLoadAllSampleData = Settings.bLoadAllSampleData;

    FMOD_SPEAKERMODE OutputMode = ConvertSpeakerMode(Settings.OutputFormat);
    FMOD_STUDIO_INITFLAGS StudioInitFlags = FMOD_STUDIO_INIT_NORMAL;
    FMOD_INITFLAGS InitFlags = FMOD_INIT_NORMAL;

#if (defined(FMODSTUDIO_LINK_DEBUG) || defined(FMODSTUDIO_LINK_LOGGING))
    bool liveUpdateEnabledForType = ((Type == EFMODSystemContext::Auditioning) && Settings.bEnableEditorLiveUpdate) ||
                                    ((Type == EFMODSystemContext::Runtime) && Settings.bEnableLiveUpdate);
    if (liveUpdateEnabledForType && bAllowLiveUpdate)
    {
        UE_LOG(LogFMOD, Verbose, TEXT("Enabling live update"));
        StudioInitFlags |= FMOD_STUDIO_INIT_LIVEUPDATE;
    }
#endif
    if (Type == EFMODSystemContext::Auditioning || Type == EFMODSystemContext::Editor)
    {
        StudioInitFlags |= FMOD_STUDIO_INIT_ALLOW_MISSING_PLUGINS;
    }

    verifyfmod(FMOD::Studio::System::create(&StudioSystem[Type]));
    FMOD::System *lowLevelSystem = nullptr;
    verifyfmod(StudioSystem[Type]->getLowLevelSystem(&lowLevelSystem));

    int DriverIndex = 0;
    if (!Settings.InitialOutputDriverName.IsEmpty())
    {
        int DriverCount = 0;
        verifyfmod(lowLevelSystem->getNumDrivers(&DriverCount));
        for (int id = 0; id < DriverCount; ++id)
        {
            char DriverNameUTF8[256] = {};
            verifyfmod(lowLevelSystem->getDriverInfo(id, DriverNameUTF8, sizeof(DriverNameUTF8), 0, 0, 0, 0));
            FString DriverName(UTF8_TO_TCHAR(DriverNameUTF8));
            UE_LOG(LogFMOD, Log, TEXT("Driver %d: %s"), id, *DriverName);
            if (DriverName.Contains(Settings.InitialOutputDriverName))
            {
                UE_LOG(LogFMOD, Log, TEXT("Selected driver %d"), id);
                DriverIndex = id;
            }
        }
        verifyfmod(lowLevelSystem->setDriver(DriverIndex));
    }

    FTCHARToUTF8 WavWriterDestUTF8(*Settings.WavWriterPath);
    void *InitData = nullptr;
    if (Type == EFMODSystemContext::Runtime && Settings.WavWriterPath.Len() > 0)
    {
        UE_LOG(LogFMOD, Log, TEXT("Running with Wav Writer: %s"), *Settings.WavWriterPath);
        verifyfmod(lowLevelSystem->setOutput(FMOD_OUTPUTTYPE_WAVWRITER));
        InitData = (void *)WavWriterDestUTF8.Get();
    }

    int SampleRate = Settings.SampleRate;
    if (Settings.bMatchHardwareSampleRate)
    {
        int DefaultSampleRate = 0;
        verifyfmod(lowLevelSystem->getSoftwareFormat(&DefaultSampleRate, 0, 0));
        int SystemSampleRate = 0;
        verifyfmod(lowLevelSystem->getDriverInfo(DriverIndex, nullptr, 0, nullptr, &SystemSampleRate, nullptr, nullptr));
        UE_LOG(LogFMOD, Log, TEXT("Default sample rate = %d"), DefaultSampleRate);
        UE_LOG(LogFMOD, Log, TEXT("System sample rate = %d"), SystemSampleRate);
        if (DefaultSampleRate >= 44100 && DefaultSampleRate <= 48000 && SystemSampleRate >= 44100 && SystemSampleRate <= 48000)
        {
            UE_LOG(LogFMOD, Log, TEXT("Matching system sample rate %d"), SystemSampleRate);
            SampleRate = SystemSampleRate;
        }
    }

    verifyfmod(lowLevelSystem->setSoftwareFormat(SampleRate, OutputMode, 0));
    verifyfmod(lowLevelSystem->setSoftwareChannels(Settings.RealChannelCount));
    AttachFMODFileSystem(lowLevelSystem, Settings.FileBufferSize);

    if (Settings.DSPBufferLength > 0 && Settings.DSPBufferCount > 0)
    {
        verifyfmod(lowLevelSystem->setDSPBufferSize(Settings.DSPBufferLength, Settings.DSPBufferCount));
    }

    FMOD_ADVANCEDSETTINGS advSettings = { 0 };
    advSettings.cbSize = sizeof(FMOD_ADVANCEDSETTINGS);
    if (Settings.bVol0Virtual)
    {
        advSettings.vol0virtualvol = Settings.Vol0VirtualLevel;
        InitFlags |= FMOD_INIT_VOL0_BECOMES_VIRTUAL;
    }
#if PLATFORM_IOS || PLATFORM_ANDROID || PLATFORM_SWITCH
    advSettings.maxFADPCMCodecs = Settings.RealChannelCount;
#elif PLATFORM_PS4
    advSettings.maxAT9Codecs = Settings.RealChannelCount;
#elif PLATFORM_XBOXONE
    advSettings.maxXMACodecs = Settings.RealChannelCount;
#else
    advSettings.maxVorbisCodecs = Settings.RealChannelCount;
#endif
    if (Type == EFMODSystemContext::Runtime)
    {
        advSettings.profilePort = Settings.LiveUpdatePort;
    }
    else if (Type == EFMODSystemContext::Auditioning)
    {
        advSettings.profilePort = Settings.EditorLiveUpdatePort;
    }
    advSettings.randomSeed = FMath::Rand();
    verifyfmod(lowLevelSystem->setAdvancedSettings(&advSettings));

    FMOD_STUDIO_ADVANCEDSETTINGS advStudioSettings = { 0 };
    advStudioSettings.cbsize = sizeof(advStudioSettings);
    advStudioSettings.studioupdateperiod = Settings.StudioUpdatePeriod;
    verifyfmod(StudioSystem[Type]->setAdvancedSettings(&advStudioSettings));

    verifyfmod(StudioSystem[Type]->initialize(Settings.TotalChannelCount, StudioInitFlags, InitFlags, InitData));

    for (FString PluginName : Settings.PluginFiles)
    {
        if (!PluginName.IsEmpty())
            LoadPlugin(Type, *PluginName);
    }

    if (Type == EFMODSystemContext::Runtime)
    {
        // Add interrupt callbacks for Mobile
        FCoreDelegates::ApplicationWillDeactivateDelegate.AddRaw(this, &FFMODStudioModule::HandleApplicationWillDeactivate);
        FCoreDelegates::ApplicationHasReactivatedDelegate.AddRaw(this, &FFMODStudioModule::HandleApplicationHasReactivated);
    }

    IMediaModule *MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");

    if (MediaModule != nullptr)
    {
        ClockSinks[Type] = MakeShared<FFMODStudioSystemClockSink, ESPMode::ThreadSafe>(StudioSystem[Type]);

        if (Type == EFMODSystemContext::Runtime)
        {
            ClockSinks[Type]->SetUpdateListenerPositionDelegate(FTimerDelegate::CreateRaw(this, &FFMODStudioModule::UpdateViewportPosition));
        }

        MediaModule->GetClock().AddSink(ClockSinks[Type].ToSharedRef());
    }
}

void FFMODStudioModule::DestroyStudioSystem(EFMODSystemContext::Type Type)
{
    UE_LOG(LogFMOD, Verbose, TEXT("DestroyStudioSystem for context %s"), FMODSystemContextNames[Type]);

    if (ClockSinks[Type].IsValid())
    {
        // Calling through the shared ptr enforces thread safety with the media clock
        ClockSinks[Type]->OnDestroyStudioSystem();

        IMediaModule *MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");

        if (MediaModule != nullptr)
        {
            MediaModule->GetClock().RemoveSink(ClockSinks[Type].ToSharedRef());
        }

        ClockSinks[Type].Reset();
    }

    // Unload all events and banks to remove warning spam when using split banks
    if (StudioSystem[Type] && bLoadAllSampleData)
    {
        int bankCount;
        verifyfmod(StudioSystem[Type]->getBankCount(&bankCount));
        if (bankCount > 0)
        {
            TArray<FMOD::Studio::Bank *> bankArray;
            TArray<FMOD::Studio::EventDescription *> eventArray;
            TArray<FMOD::Studio::EventInstance *> instanceArray;

            bankArray.SetNumUninitialized(bankCount, false);
            verifyfmod(StudioSystem[Type]->getBankList(bankArray.GetData(), bankCount, &bankCount));
            for (int i = 0; i < bankCount; i++)
            {
                int eventCount;
                verifyfmod(bankArray[i]->getEventCount(&eventCount));
                if (eventCount > 0)
                {
                    eventArray.SetNumUninitialized(eventCount, false);
                    verifyfmod(bankArray[i]->getEventList(eventArray.GetData(), eventCount, &eventCount));
                    for (int j = 0; j < eventCount; j++)
                    {
                        int instanceCount;
                        verifyfmod(eventArray[j]->getInstanceCount(&instanceCount));
                        if (instanceCount > 0)
                        {
                            instanceArray.SetNumUninitialized(instanceCount, false);
                            verifyfmod(eventArray[j]->getInstanceList(instanceArray.GetData(), instanceCount, &instanceCount));
                            for (int k = 0; k < instanceCount; k++)
                            {
                                verifyfmod(instanceArray[k]->stop(FMOD_STUDIO_STOP_IMMEDIATE));
                                verifyfmod(instanceArray[k]->release());
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < bankCount; i++)
            {
                FMOD_STUDIO_LOADING_STATE state;
                bankArray[i]->getSampleLoadingState(&state);
                if (state == FMOD_STUDIO_LOADING_STATE_LOADED)
                {
                    verifyfmod(bankArray[i]->unloadSampleData());
                }
            }
        }
    }

    if (StudioSystem[Type])
    {
        verifyfmod(StudioSystem[Type]->release());
        StudioSystem[Type] = nullptr;
    }
}

bool FFMODStudioModule::Tick(float DeltaTime)
{
    if (GIsEditor)
    {
        BankUpdateNotifier.Update();
    }

    if (ClockSinks[EFMODSystemContext::Auditioning].IsValid())
    {
        verifyfmod(ClockSinks[EFMODSystemContext::Auditioning]->LastResult);
    }
    if (ClockSinks[EFMODSystemContext::Runtime].IsValid())
    {
        FMOD_STUDIO_CPU_USAGE Usage = {};
        StudioSystem[EFMODSystemContext::Runtime]->getCPUUsage(&Usage);
        SET_FLOAT_STAT(STAT_FMOD_CPUMixer, Usage.dspusage);
        SET_FLOAT_STAT(STAT_FMOD_CPUStudio, Usage.studiousage);

        int currentAlloc, maxAlloc;
        FMOD::Memory_GetStats(&currentAlloc, &maxAlloc, false);
        SET_MEMORY_STAT(STAT_FMOD_Current_Memory, currentAlloc);
        SET_MEMORY_STAT(STAT_FMOD_Max_Memory, maxAlloc);

        int channels, realChannels;
        FMOD::System *lowlevel;
        StudioSystem[EFMODSystemContext::Runtime]->getLowLevelSystem(&lowlevel);
        lowlevel->getChannelsPlaying(&channels, &realChannels);
        SET_DWORD_STAT(STAT_FMOD_Real_Channels, realChannels);
        SET_DWORD_STAT(STAT_FMOD_Total_Channels, channels);

        verifyfmod(ClockSinks[EFMODSystemContext::Runtime]->LastResult);
    }
    if (ClockSinks[EFMODSystemContext::Editor].IsValid())
    {
        verifyfmod(ClockSinks[EFMODSystemContext::Editor]->LastResult);
    }
    return true;
}

void FFMODStudioModule::UpdateViewportPosition()
{
    if (bSimulating)
    {
        return;
    }
    int ListenerIndex = 0;

    UWorld *ViewportWorld = nullptr;
    if (GEngine && GEngine->GameViewport)
    {
        ViewportWorld = GEngine->GameViewport->GetWorld();
    }

    bool bCameraCut = false; // Not sure how to get View->bCameraCut from here
    float DeltaSeconds = ((bCameraCut || !ViewportWorld) ? 0.f : ViewportWorld->GetDeltaSeconds());

    bListenerMoved = false;

    if (IsValid(ViewportWorld))
    {
        for (FConstPlayerControllerIterator Iterator = ViewportWorld->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            APlayerController *PlayerController = Iterator->Get();
            if (PlayerController)
            {
                ULocalPlayer *LocalPlayer = PlayerController->GetLocalPlayer();
                if (LocalPlayer)
                {
                    FVector Location;
                    FVector ProjFront;
                    FVector ProjRight;
                    PlayerController->GetAudioListenerPosition(/*out*/ Location, /*out*/ ProjFront, /*out*/ ProjRight);
                    FVector ProjUp = FVector::CrossProduct(ProjFront, ProjRight);

                    FTransform ListenerTransform(FRotationMatrix::MakeFromXY(ProjFront, ProjRight));
                    ListenerTransform.SetTranslation(Location);
                    ListenerTransform.NormalizeRotation();

                    SetListenerPosition(ListenerIndex, ViewportWorld, ListenerTransform, DeltaSeconds);

                    ListenerIndex++;
                }
            }
        }
        FinishSetListenerPosition(ListenerIndex, DeltaSeconds);
    }
}

bool FFMODStudioModule::HasListenerMoved()
{
    return bListenerMoved;
}

void FFMODStudioModule::ResetInterpolation()
{
    for (FFMODListener &Listener : Listeners)
    {
        Listener = FFMODListener();
    }
}

const FFMODListener &FFMODStudioModule::GetNearestListener(const FVector &Location)
{
    float BestDistSq = FLT_MAX;
    int BestListener = 0;
    for (int i = 0; i < ListenerCount; ++i)
    {
        const float DistSq = FVector::DistSquared(Location, Listeners[i].Transform.GetTranslation());
        if (DistSq < BestDistSq)
        {
            BestListener = i;
            BestDistSq = DistSq;
        }
    }
    return Listeners[BestListener];
}

// Partially copied from FAudioDevice::SetListener
void FFMODStudioModule::SetListenerPosition(int ListenerIndex, UWorld *World, const FTransform &ListenerTransform, float DeltaSeconds)
{
    FMOD::Studio::System *System = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
    if (System && ListenerIndex < MAX_LISTENERS)
    {
        FVector ListenerPos = ListenerTransform.GetTranslation();

        FInteriorSettings *InteriorSettings =
            (FInteriorSettings *)alloca(sizeof(FInteriorSettings)); // FinteriorSetting::FInteriorSettings() isn't exposed (possible UE4 bug???)
        AAudioVolume *Volume = World->GetAudioSettings(ListenerPos, NULL, InteriorSettings);

        Listeners[ListenerIndex].Velocity =
            DeltaSeconds > 0.f ? (ListenerTransform.GetTranslation() - Listeners[ListenerIndex].Transform.GetTranslation()) / DeltaSeconds :
                                 FVector::ZeroVector;

        Listeners[ListenerIndex].Transform = ListenerTransform;

        Listeners[ListenerIndex].ApplyInteriorSettings(Volume, *InteriorSettings);

        // We are using a direct copy of the inbuilt transforms but the directions come out wrong.
        // Several of the audio functions use GetFront() for right, so we do the same here.
        const FVector Up = Listeners[ListenerIndex].GetUp();
        const FVector Right = Listeners[ListenerIndex].GetFront();
        const FVector Forward = Right ^ Up;

        FMOD_3D_ATTRIBUTES Attributes = { { 0 } };
        Attributes.position = FMODUtils::ConvertWorldVector(ListenerPos);
        Attributes.forward = FMODUtils::ConvertUnitVector(Forward);
        Attributes.up = FMODUtils::ConvertUnitVector(Up);
        Attributes.velocity = FMODUtils::ConvertWorldVector(Listeners[ListenerIndex].Velocity);

#if FMOD_VERSION >= 0x00010600
        // Expand number of listeners dynamically
        if (ListenerIndex >= ListenerCount)
        {
            Listeners[ListenerIndex] = FFMODListener();
            ListenerCount = ListenerIndex + 1;
            verifyfmod(System->setNumListeners(ListenerCount));
        }
        verifyfmod(System->setListenerAttributes(ListenerIndex, &Attributes));
#else
        verifyfmod(System->setListenerAttributes(&Attributes));
#endif

        bListenerMoved = true;
    }
}

void FFMODStudioModule::FinishSetListenerPosition(int NumListeners, float DeltaSeconds)
{
    FMOD::Studio::System *System = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
    if (!System || NumListeners < 1)
    {
        return;
    }

    if (System && NumListeners < ListenerCount)
    {
        ListenerCount = NumListeners;
#if FMOD_VERSION >= 0x00010600
        verifyfmod(System->setNumListeners(ListenerCount));
#endif
    }

    for (int i = 0; i < ListenerCount; ++i)
    {
        Listeners[i].UpdateCurrentInteriorSettings();
    }

    // Apply a reverb snapshot from the listener position(s)
    AAudioVolume *BestVolume = nullptr;
    for (int i = 0; i < ListenerCount; ++i)
    {
        AAudioVolume *CandidateVolume = Listeners[i].Volume;

        if (BestVolume == nullptr || (IsValid(CandidateVolume) && IsValid(BestVolume) && CandidateVolume->GetPriority() > BestVolume->GetPriority()))
        {
            BestVolume = CandidateVolume;
        }
    }
    UFMODSnapshotReverb *NewSnapshot = nullptr;

    if (IsValid(BestVolume) && BestVolume->GetReverbSettings().bApplyReverb)
    {
        NewSnapshot = Cast<UFMODSnapshotReverb>(BestVolume->GetReverbSettings().ReverbEffect);
    }

    if (NewSnapshot != nullptr)
    {
        FString NewSnapshotName = FMODUtils::LookupNameFromGuid(System, NewSnapshot->AssetGuid);
        UE_LOG(LogFMOD, Verbose, TEXT("Starting new snapshot '%s'"), *NewSnapshotName);

        // Try to steal old entry
        FFMODSnapshotEntry SnapshotEntry;
        int SnapshotEntryIndex = -1;
        for (int i = 0; i < ReverbSnapshots.Num(); ++i)
        {
            if (ReverbSnapshots[i].Snapshot == NewSnapshot)
            {
                UE_LOG(LogFMOD, Verbose, TEXT("Re-using old entry with intensity %f"), ReverbSnapshots[i].CurrentIntensity());
                SnapshotEntryIndex = i;
                break;
            }
        }
        // Create new instance
        if (SnapshotEntryIndex == -1)
        {
            UE_LOG(LogFMOD, Verbose, TEXT("Creating new instance"));

            FMOD::Studio::ID Guid = FMODUtils::ConvertGuid(NewSnapshot->AssetGuid);
            FMOD::Studio::EventInstance *NewInstance = nullptr;
            FMOD::Studio::EventDescription *EventDesc = nullptr;
            System->getEventByID(&Guid, &EventDesc);
            if (EventDesc)
            {
                EventDesc->createInstance(&NewInstance);
                if (NewInstance)
                {
                    NewInstance->setParameterValue("Intensity", 0.0f);
                    NewInstance->start();
                }
            }

            SnapshotEntryIndex = ReverbSnapshots.Num();
            ReverbSnapshots.Push(FFMODSnapshotEntry(NewSnapshot, NewInstance));
        }
        // Fade up
        if (ReverbSnapshots[SnapshotEntryIndex].FadeIntensityEnd == 0.0f)
        {
            ReverbSnapshots[SnapshotEntryIndex].FadeTo(BestVolume->GetReverbSettings().Volume, BestVolume->GetReverbSettings().FadeTime);
        }
    }
    // Fade out all other entries
    for (int i = 0; i < ReverbSnapshots.Num(); ++i)
    {
        UE_LOG(LogFMOD, Verbose, TEXT("Ramping intensity (%f,%f) -> %f"), ReverbSnapshots[i].FadeIntensityStart, ReverbSnapshots[i].FadeIntensityEnd,
            ReverbSnapshots[i].CurrentIntensity());
        ReverbSnapshots[i].Instance->setParameterValue("Intensity", 100.0f * ReverbSnapshots[i].CurrentIntensity());

        if (ReverbSnapshots[i].Snapshot != NewSnapshot)
        {
            // Start fading out if needed
            if (ReverbSnapshots[i].FadeIntensityEnd != 0.0f)
            {
                ReverbSnapshots[i].FadeTo(0.0f, ReverbSnapshots[i].FadeDuration);
            }
            // Finish fading out and remove
            else if (ReverbSnapshots[i].CurrentIntensity() == 0.0f)
            {
                UE_LOG(LogFMOD, Verbose, TEXT("Removing snapshot"));

                ReverbSnapshots[i].Instance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
                ReverbSnapshots[i].Instance->release();
                ReverbSnapshots.RemoveAt(i);
                --i; // removed entry, redo current index for next one
            }
        }
    }
}

void FFMODStudioModule::RefreshSettings()
{
    AssetTable.Refresh();
    if (GIsEditor)
    {
        const UFMODSettings &Settings = *GetDefault<UFMODSettings>();
        BankUpdateNotifier.SetFilePath(Settings.GetMasterStringsBankPath());
    }
}

void FFMODStudioModule::SetInPIE(bool bInPIE, bool simulating)
{
    bIsInPIE = bInPIE;
    bSimulating = simulating;
    bListenerMoved = true;
    ResetInterpolation();

    if (GIsEditor)
    {
        BankUpdateNotifier.EnableUpdate(!bInPIE);
    }

    FMOD_DEBUG_FLAGS flags;

    if (bInPIE)
    {
        if (StudioSystem[EFMODSystemContext::Auditioning])
        {
            // We currently don't tear down auditioning system but we do stop the playing event.
            if (AuditioningInstance)
            {
                AuditioningInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
                AuditioningInstance = nullptr;
            }
            // Also make sure banks are finishing loading so they aren't grabbing file handles.
            StudioSystem[EFMODSystemContext::Auditioning]->flushCommands();
        }

        // TODO: Stop sounds for the Editor system? What should happen if the user previews a sequence with transport
        // controls then starts a PIE session? What does happen?

        UE_LOG(LogFMOD, Log, TEXT("Creating runtime Studio System"));
        ListenerCount = 1;
        CreateStudioSystem(EFMODSystemContext::Runtime);

        UE_LOG(LogFMOD, Log, TEXT("Loading Banks"));
        LoadBanks(EFMODSystemContext::Runtime);

        const UFMODSettings &Settings = *GetDefault<UFMODSettings>();
        flags = Settings.LoggingLevel;
    }
    else
    {
        ReverbSnapshots.Reset();
        DestroyStudioSystem(EFMODSystemContext::Runtime);
        flags = FMOD_DEBUG_LEVEL_WARNING;
    }

    verifyfmod(FMOD::Debug_Initialize(flags, FMOD_DEBUG_MODE_CALLBACK, FMODLogCallback));

}

UFMODAsset *FFMODStudioModule::FindAssetByName(const FString &Name)
{
    return AssetTable.FindByName(Name);
}

UFMODEvent *FFMODStudioModule::FindEventByName(const FString &Name)
{
    UFMODAsset *Asset = AssetTable.FindByName(Name);
    return Cast<UFMODEvent>(Asset);
}

void FFMODStudioModule::SetSystemPaused(bool paused)
{
    if (StudioSystem[EFMODSystemContext::Runtime])
    {
        if (bMixerPaused != paused)
        {
            FMOD::System *LowLevelSystem = nullptr;
            verifyfmod(StudioSystem[EFMODSystemContext::Runtime]->getLowLevelSystem(&LowLevelSystem));

            // Resume mixer before making calls for Android in particular
            if (!paused)
            {
                LowLevelSystem->mixerResume();
            }

            FMOD::ChannelGroup *MasterChannelGroup = nullptr;
            verifyfmod(LowLevelSystem->getMasterChannelGroup(&MasterChannelGroup));
            verifyfmod(MasterChannelGroup->setPaused(paused));

            if (paused)
            {
                LowLevelSystem->mixerSuspend();
            }
        }

        bMixerPaused = paused;
    }
}

void FFMODStudioModule::PostLoadCallback()
{
}

void FFMODStudioModule::ShutdownModule()
{
    UE_LOG(LogFMOD, Verbose, TEXT("FFMODStudioModule shutdown"));

    DestroyStudioSystem(EFMODSystemContext::Auditioning);
    DestroyStudioSystem(EFMODSystemContext::Runtime);
    DestroyStudioSystem(EFMODSystemContext::Editor);

    ReleaseFMODFileSystem();

    if (MemPool)
        FMemory::Free(MemPool);

    if (GIsEditor)
    {
        BankUpdateNotifier.BanksUpdatedEvent.RemoveAll(this);
    }

    if (UObjectInitialized())
    {
        // Unregister tick function.
        FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
    }

    UE_LOG(LogFMOD, Verbose, TEXT("FFMODStudioModule unloading dynamic libraries"));
    if (StudioLibHandle)
    {
        FPlatformProcess::FreeDllHandle(StudioLibHandle);
        StudioLibHandle = nullptr;
    }
    if (LowLevelLibHandle)
    {
        FPlatformProcess::FreeDllHandle(LowLevelLibHandle);
        LowLevelLibHandle = nullptr;
    }
    UE_LOG(LogFMOD, Verbose, TEXT("FFMODStudioModule finished unloading"));
}

struct NamedBankEntry
{
    NamedBankEntry()
        : Bank(nullptr)
    {
    }
    NamedBankEntry(const FString &InName, FMOD::Studio::Bank *InBank, FMOD_RESULT InResult)
        : Name(InName)
        , Bank(InBank)
        , Result(InResult)
    {
    }

    FString Name;
    FMOD::Studio::Bank *Bank;
    FMOD_RESULT Result;
};

bool FFMODStudioModule::AreBanksLoaded()
{
    return bBanksLoaded;
}

void FFMODStudioModule::LoadBanks(EFMODSystemContext::Type Type)
{
    const UFMODSettings &Settings = *GetDefault<UFMODSettings>();

    FailedBankLoads[Type].Reset();
    if (Type == EFMODSystemContext::Auditioning || Type == EFMODSystemContext::Editor)
    {
        RequiredPlugins.Reset();
    }

    if (StudioSystem[Type] != nullptr && Settings.IsBankPathSet())
    {
        UE_LOG(LogFMOD, Verbose, TEXT("LoadBanks for context %s"), FMODSystemContextNames[Type]);

        /*
			Queue up all banks to load asynchronously then wait at the end.
		*/
        bool bLoadAllBanks = ((Type == EFMODSystemContext::Auditioning) || (Type == EFMODSystemContext::Editor) || Settings.bLoadAllBanks);
        bool bLoadSampleData = ((Type == EFMODSystemContext::Runtime) && Settings.bLoadAllSampleData);
        bool bLockAllBuses = ((Type == EFMODSystemContext::Runtime) && Settings.bLockAllBuses);
        FMOD_STUDIO_LOAD_BANK_FLAGS BankFlags = (bLockAllBuses ? FMOD_STUDIO_LOAD_BANK_NORMAL : FMOD_STUDIO_LOAD_BANK_NONBLOCKING);

        // Always load the master bank at startup
        FString MasterBankPath = Settings.GetMasterBankPath();
        UE_LOG(LogFMOD, Verbose, TEXT("Loading master bank: %s"), *MasterBankPath);

        TArray<NamedBankEntry> BankEntries;

        FMOD::Studio::Bank *MasterBank = nullptr;
        FMOD_RESULT Result;
        Result = StudioSystem[Type]->loadBankFile(TCHAR_TO_UTF8(*MasterBankPath), BankFlags, &MasterBank);
        BankEntries.Add(NamedBankEntry(MasterBankPath, MasterBank, Result));

        if (Result == FMOD_OK)
        {
            FMOD::Studio::Bank *MasterAssetsBank = nullptr;
            FString MasterAssetsBankPath = Settings.GetMasterAssetsBankPath();
            if (FPaths::FileExists(MasterAssetsBankPath))
            {
                Result = StudioSystem[Type]->loadBankFile(TCHAR_TO_UTF8(*MasterAssetsBankPath), BankFlags, &MasterAssetsBank);
                BankEntries.Add(NamedBankEntry(MasterAssetsBankPath, MasterAssetsBank, Result));
            }
        }

        if (Result == FMOD_OK)
        {
            // Auditioning needs string bank to get back full paths from events
            // Runtime could do without it, but if we load it we can look up guids to names which is helpful
            {
                FString StringsBankPath = Settings.GetMasterStringsBankPath();
                UE_LOG(LogFMOD, Verbose, TEXT("Loading strings bank: %s"), *StringsBankPath);
                FMOD::Studio::Bank *StringsBank = nullptr;
                Result = StudioSystem[Type]->loadBankFile(TCHAR_TO_UTF8(*StringsBankPath), BankFlags, &StringsBank);
                BankEntries.Add(NamedBankEntry(StringsBankPath, StringsBank, Result));
            }

            // Optionally load all banks in the directory
            if (bLoadAllBanks)
            {
                UE_LOG(LogFMOD, Verbose, TEXT("Loading all banks"));
                TArray<FString> BankFiles;
                Settings.GetAllBankPaths(BankFiles);
                for (const FString &OtherFile : BankFiles)
                {
                    if (Settings.SkipLoadBankName.Len() && OtherFile.Contains(Settings.SkipLoadBankName))
                    {
                        UE_LOG(LogFMOD, Log, TEXT("Skipping bank: %s"), *OtherFile);
                        continue;
                    }
                    UE_LOG(LogFMOD, Log, TEXT("Loading bank: %s"), *OtherFile);

                    FMOD::Studio::Bank *OtherBank;
                    Result = StudioSystem[Type]->loadBankFile(TCHAR_TO_UTF8(*OtherFile), BankFlags, &OtherBank);
                    BankEntries.Add(NamedBankEntry(OtherFile, OtherBank, Result));
                }
            }

            // Optionally lock all buses to make sure they are created
            if (Settings.bLockAllBuses)
            {
                UE_LOG(LogFMOD, Verbose, TEXT("Locking all buses"));
                int BusCount = 0;
                verifyfmod(MasterBank->getBusCount(&BusCount));
                if (BusCount != 0)
                {
                    TArray<FMOD::Studio::Bus *> BusList;
                    BusList.AddZeroed(BusCount);
                    verifyfmod(MasterBank->getBusList(BusList.GetData(), BusCount, &BusCount));
                    BusList.SetNum(BusCount);
                    for (int BusIdx = 0; BusIdx < BusCount; ++BusIdx)
                    {
                        verifyfmod(BusList[BusIdx]->lockChannelGroup());
                    }
                }
            }
        }

        // Wait for all banks to load.
        StudioSystem[Type]->flushCommands();

        for (NamedBankEntry &Entry : BankEntries)
        {
            if (Entry.Result == FMOD_OK)
            {
                FMOD_STUDIO_LOADING_STATE BankLoadingState = FMOD_STUDIO_LOADING_STATE_ERROR;
                Entry.Result = Entry.Bank->getLoadingState(&BankLoadingState);
                if (BankLoadingState == FMOD_STUDIO_LOADING_STATE_ERROR)
                {
                    Entry.Bank->unload();
                    Entry.Bank = nullptr;
                }
                else if (bLoadSampleData)
                {
                    verifyfmod(Entry.Bank->loadSampleData());
                }
            }
            if (Entry.Bank == nullptr || Entry.Result != FMOD_OK)
            {
                FString ErrorMessage;
                if (!FPaths::FileExists(Entry.Name))
                {
                    ErrorMessage = "File does not exist";
                }
                else
                {
                    ErrorMessage = UTF8_TO_TCHAR(FMOD_ErrorString(Entry.Result));
                }
                UE_LOG(LogFMOD, Warning, TEXT("Failed to bank: %s (%s)"), *Entry.Name, *ErrorMessage);
                FailedBankLoads[Type].Add(FString::Printf(TEXT("%s (%s)"), *FPaths::GetBaseFilename(Entry.Name), *ErrorMessage));
            }
        }
    }

    bBanksLoaded = true;
}

void FFMODStudioModule::HandleBanksUpdated()
{
    UE_LOG(LogFMOD, Verbose, TEXT("Refreshing auditioning system"));

    DestroyStudioSystem(EFMODSystemContext::Auditioning);

    AssetTable.Refresh();

    CreateStudioSystem(EFMODSystemContext::Auditioning);
    LoadBanks(EFMODSystemContext::Auditioning);

    DestroyStudioSystem(EFMODSystemContext::Editor);
    CreateStudioSystem(EFMODSystemContext::Editor);
    LoadBanks(EFMODSystemContext::Editor);

    BanksReloadedDelegate.Broadcast();
}

FMOD::Studio::System *FFMODStudioModule::GetStudioSystem(EFMODSystemContext::Type Context)
{
    if (Context == EFMODSystemContext::Max)
    {
        Context = (bIsInPIE ? EFMODSystemContext::Runtime : EFMODSystemContext::Auditioning);
    }
    return StudioSystem[Context];
}

FMOD::Studio::EventDescription *FFMODStudioModule::GetEventDescription(const UFMODEvent *Event, EFMODSystemContext::Type Context)
{
    if (Context == EFMODSystemContext::Max)
    {
        Context = (bIsInPIE ? EFMODSystemContext::Runtime : EFMODSystemContext::Auditioning);
    }
    if (StudioSystem[Context] != nullptr && IsValid(Event) && Event->AssetGuid.IsValid())
    {
        FMOD::Studio::ID Guid = FMODUtils::ConvertGuid(Event->AssetGuid);
        FMOD::Studio::EventDescription *EventDesc = nullptr;
        StudioSystem[Context]->getEventByID(&Guid, &EventDesc);
        return EventDesc;
    }
    return nullptr;
}

FMOD::Studio::EventInstance *FFMODStudioModule::CreateAuditioningInstance(const UFMODEvent *Event)
{
    StopAuditioningInstance();
    if (IsValid(Event))
    {
        FMOD::Studio::EventDescription *EventDesc = GetEventDescription(Event, EFMODSystemContext::Auditioning);
        if (EventDesc)
        {
            FMOD_RESULT Result = EventDesc->createInstance(&AuditioningInstance);
            if (Result == FMOD_OK)
            {
                return AuditioningInstance;
            }
        }
    }
    return nullptr;
}

void FFMODStudioModule::StopAuditioningInstance()
{
    if (AuditioningInstance)
    {
        // Don't bother checking for errors just in case auditioning is already shutting down
        AuditioningInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
        AuditioningInstance->release();
        AuditioningInstance = nullptr;
    }
}

#undef LOCTEXT_NAMESPACE
