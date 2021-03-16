#include "UEGameAnalytics.h"
#include "Misc/ConfigCacheIni.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "GameAnalyticsProvider.h"
#include "GameAnalytics.h"
#if PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include "../GA-SDK-CPP/GameAnalytics.h"
#elif PLATFORM_HTML5
#include "../GA-SDK-HTML5/GameAnalytics.h"
#endif

#include "Misc/ConfigCacheIni.h"
#include "Misc/EngineVersion.h"

#define GA_VERSION TEXT("3.0.6")

DEFINE_LOG_CATEGORY(LogGameAnalyticsAnalytics)

IMPLEMENT_MODULE(FAnalyticsGameAnalytics, GameAnalytics)

FGameAnalyticsBuildOverride FAnalyticsGameAnalytics::BuildOverride;

void FAnalyticsGameAnalytics::StartupModule()
{
    UE_LOG(LogGameAnalyticsAnalytics, Display, TEXT("FAnalyticsGameAnalytics Constructor"));
    GameAnalyticsProvider = MakeShareable(new FAnalyticsProviderGameAnalytics());
}

void FAnalyticsGameAnalytics::ShutdownModule()
{
    if (GameAnalyticsProvider.IsValid())
    {
        UE_LOG(LogGameAnalyticsAnalytics, Display, TEXT("FAnalyticsGameAnalytics Destructor"));
        GameAnalyticsProvider->EndSession();
    }
}

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 13
TSharedPtr<IAnalyticsProvider> FAnalyticsGameAnalytics::CreateAnalyticsProvider(const FAnalyticsProviderConfigurationDelegate& GetConfigValue) const
{
    return GameAnalyticsProvider;
}
#else
TSharedPtr<IAnalyticsProvider> FAnalyticsGameAnalytics::CreateAnalyticsProvider(const FAnalytics::FProviderConfigurationDelegate& GetConfigValue) const
{
    return GameAnalyticsProvider;
}
#endif


FAnalyticsGameAnalytics::FGameAnalyticsProjectSettings FAnalyticsGameAnalytics::LoadProjectSettings()
{
    FGameAnalyticsProjectSettings Settings;
    if (!GConfig)
    {
        return Settings;
    }

	const FString Section = TEXT("/Script/GameAnalyticsEditor.GameAnalyticsProjectSettings");
    const bool bUseBuildOverride = BuildOverride.IsBound();

	// Windows
    if (!GConfig->GetString(*Section, TEXT("AndroidGameKey"), Settings.AndroidGameKey, GetIniName()))
    {
        Settings.AndroidGameKey = "";
    }
    if (!GConfig->GetString(*Section, TEXT("AndroidSecretKey"), Settings.AndroidSecretKey, GetIniName()))
    {
        Settings.AndroidSecretKey = "";
    }
    if (!bUseBuildOverride && !GConfig->GetString(*Section, TEXT("AndroidBuild"), Settings.AndroidBuild, GetIniName()))
    {
        Settings.AndroidBuild = "0.1";
    }

	// IOS
    if (!GConfig->GetString(*Section, TEXT("IosGameKey"), Settings.IosGameKey, GetIniName()))
    {
        Settings.IosGameKey = "";
    }
    if (!GConfig->GetString(*Section, TEXT("IosSecretKey"), Settings.IosSecretKey, GetIniName()))
    {
        Settings.IosSecretKey = "";
    }
    if (!bUseBuildOverride && !GConfig->GetString(*Section, TEXT("IosBuild"), Settings.IosBuild, GetIniName()))
    {
        Settings.IosBuild = "0.1";
    }

	// Mac
    if (!GConfig->GetString(*Section, TEXT("MacGameKey"), Settings.MacGameKey, GetIniName()))
    {
        Settings.MacGameKey = "";
    }
    if (!GConfig->GetString(*Section, TEXT("MacSecretKey"), Settings.MacSecretKey, GetIniName()))
    {
        Settings.MacSecretKey = "";
    }
    if (!bUseBuildOverride && !GConfig->GetString(*Section, TEXT("MacBuild"), Settings.MacBuild, GetIniName()))
    {
        Settings.MacBuild = "0.1";
    }

	// Windows
    if (!GConfig->GetString(*Section, TEXT("WindowsGameKey"), Settings.WindowsGameKey, GetIniName()))
    {
        Settings.WindowsGameKey = "";
    }
    if (!GConfig->GetString(*Section, TEXT("WindowsSecretKey"), Settings.WindowsSecretKey, GetIniName()))
    {
        Settings.WindowsSecretKey = "";
    }
    if (!bUseBuildOverride && !GConfig->GetString(*Section, TEXT("WindowsBuild"), Settings.WindowsBuild, GetIniName()))
    {
        Settings.WindowsBuild = "0.1";
    }

	// Linux
    if (!GConfig->GetString(*Section, TEXT("LinuxGameKey"), Settings.LinuxGameKey, GetIniName()))
    {
        Settings.LinuxGameKey = "";
    }
    if (!GConfig->GetString(*Section, TEXT("LinuxSecretKey"), Settings.LinuxSecretKey, GetIniName()))
    {
        Settings.LinuxSecretKey = "";
    }
    if (!bUseBuildOverride && !GConfig->GetString(*Section, TEXT("LinuxBuild"), Settings.LinuxBuild, GetIniName()))
    {
        Settings.LinuxBuild = "0.1";
    }

	// HTML5
    if (!GConfig->GetString(*Section, TEXT("Html5GameKey"), Settings.Html5GameKey, GetIniName()))
    {
        Settings.Html5GameKey = "";
    }
    if (!GConfig->GetString(*Section, TEXT("Html5SecretKey"), Settings.Html5SecretKey, GetIniName()))
    {
        Settings.Html5SecretKey = "";
    }
    if (!bUseBuildOverride && !GConfig->GetString(*Section, TEXT("Html5Build"), Settings.Html5Build, GetIniName()))
    {
        Settings.Html5Build = "0.1";
    }

    // Overridden by the game, same version for everyone
    if (bUseBuildOverride)
    {
        const FString& BuildVersion = BuildOverride.Execute();
        Settings.AndroidBuild = BuildVersion;
        Settings.IosBuild = BuildVersion;
        Settings.MacBuild = BuildVersion;
        Settings.WindowsBuild = BuildVersion;
        Settings.LinuxBuild = BuildVersion;
        Settings.Html5Build = BuildVersion;
    }

    if (!GConfig->GetBool(*Section, TEXT("UseManualSessionHandling"), Settings.UseManualSessionHandling, GetIniName()))
    {
        Settings.UseManualSessionHandling = false;
    }

	// InfoLogBuild
	if (FParse::Param(FCommandLine::Get(), TEXT("GAInfoLogBuild")))
	{
		Settings.InfoLogBuild = true;
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("GANoInfoLogBuild")))
	{
		Settings.InfoLogBuild = false;
	}
    else if (!GConfig->GetBool(*Section, TEXT("InfoLogBuild"), Settings.InfoLogBuild, GetIniName()))
    {
        Settings.InfoLogBuild = true;
    }

	// VerboseLogBuild
	if (FParse::Param(FCommandLine::Get(), TEXT("GAVerboseLogBuild")))
	{
		Settings.VerboseLogBuild = true;
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("GANoVerboseLogBuild")))
	{
		Settings.VerboseLogBuild = false;
	}
	else if (!GConfig->GetBool(*Section, TEXT("VerboseLogBuild"), Settings.VerboseLogBuild, GetIniName()))
    {
        Settings.VerboseLogBuild = false;
    }

    GConfig->GetArray(*Section, TEXT("+CustomDimensions01"), Settings.CustomDimensions01, GetIniName());
    GConfig->GetArray(*Section, TEXT("+CustomDimensions02"), Settings.CustomDimensions02, GetIniName());
    GConfig->GetArray(*Section, TEXT("+CustomDimensions03"), Settings.CustomDimensions03, GetIniName());
    GConfig->GetArray(*Section, TEXT("+ResourceCurrencies"), Settings.ResourceCurrencies, GetIniName());
    GConfig->GetArray(*Section, TEXT("+ResourceItemTypes"), Settings.ResourceItemTypes, GetIniName());

    return Settings;
}


#if !UE_BUILD_SHIPPING
/**
* Verifies that the event name matches GameAnalytics's 32 char limit and warns if it doesn't
*/
inline void CheckEventNameLen(const FString& EventName)
{
    const int32 Length = EventName.Len();
    if (Length > 32)
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("GameAnalytics event name is too long: %s and will be truncated by GameAnalytics. 32 character max limit."), *EventName);
    }
    else if (Length == 0)
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("GameAnalytics event name is empty!"));
    }
}

#define WarnIfEventNameIsWrongLength(x) CheckEventNameLen(x)
#else
// Have these compile out
#define WarnIfEventNameIsWrongLength(x)
#endif



// Provider
FAnalyticsProviderGameAnalytics::FAnalyticsProviderGameAnalytics() :
    bHasSessionStarted(false),
    bHasWrittenFirstEvent(false),
    Age(0)
{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 18
    AnalyticsFilePath = FPaths::ProjectSavedDir() + TEXT("Analytics/");
#else
    AnalyticsFilePath = FPaths::GameSavedDir() + TEXT("Analytics/");
#endif
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 15
    UserId = FPlatformMisc::GetDeviceId();
#else
    UserId = FPlatformMisc::GetUniqueDeviceId();
#endif
}

FAnalyticsProviderGameAnalytics::~FAnalyticsProviderGameAnalytics()
{
    //UE_LOG(LogGameAnalyticsAnalytics, Display, TEXT("FAnalyticsGameAnalytics ~FAnalyticsProviderGameAnalytics"));
    if (bHasSessionStarted)
    {
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
        // Empty
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
        gameanalytics::GameAnalytics::onQuit();
#else
		EndSession();
#endif
	}
}

bool FAnalyticsProviderGameAnalytics::StartSession(const TArray<FAnalyticsEventAttribute>& Attributes)
{
    //UGameAnalytics::logGAStateInfo(TEXT("FAnalyticsProviderGameAnalytics::StartSession"));
    if (!bHasSessionStarted)
    {
        UE_LOG(LogGameAnalyticsAnalytics, Verbose, TEXT("TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE = %d"), TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE);
#if WITH_EDITOR && TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
        UGameAnalytics::setThreadAndEventTimers(0.1, 0.05);
#endif
        ProjectSettings = FAnalyticsGameAnalytics::LoadProjectSettings();

#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 18
        UGameAnalytics::configureWritablePath(TCHAR_TO_ANSI(*FPaths::ProjectSavedDir()));
#else
        UGameAnalytics::configureWritablePath(TCHAR_TO_ANSI(*FPaths::GameSavedDir()));
#endif
#endif

        ////// Enable log
        UGameAnalytics::setEnabledInfoLog(ProjectSettings.InfoLogBuild);
        UGameAnalytics::setEnabledVerboseLog(ProjectSettings.VerboseLogBuild);

        ////// Configure engine version
        const FString EngineVersionString = FString::Printf(TEXT("unreal %d.%d.%d"), FEngineVersion::Current().GetMajor(), FEngineVersion::Current().GetMinor(), FEngineVersion::Current().GetPatch());
        UE_LOG(LogGameAnalyticsAnalytics, Verbose, TEXT("EngineVersionString = %s"), *EngineVersionString);
		UGameAnalytics::configureGameEngineVersion(TCHAR_TO_ANSI(*EngineVersionString));

        const FString SdkVersionString = FString::Printf(TEXT("unreal %s"), GA_VERSION);
        UE_LOG(LogGameAnalyticsAnalytics, Verbose, TEXT("SdkVersionString = %s"), *SdkVersionString);
        UGameAnalytics::configureSdkGameEngineVersion(TCHAR_TO_ANSI(*SdkVersionString));

        //// Configure build version
        FString BuildString;
#if PLATFORM_IOS
        BuildString = ProjectSettings.IosBuild;
#elif PLATFORM_ANDROID
        BuildString = ProjectSettings.AndroidBuild;
#elif PLATFORM_MAC
        BuildString = ProjectSettings.MacBuild;
#elif PLATFORM_WINDOWS
        BuildString = ProjectSettings.WindowsBuild;
#elif PLATFORM_LINUX
        BuildString = ProjectSettings.LinuxBuild;
#elif PLATFORM_HTML5
        BuildString = ProjectSettings.Html5Build;
#endif
        UE_LOG(LogGameAnalyticsAnalytics, Verbose, TEXT("BuildString = %s"), *BuildString);
        UGameAnalytics::configureBuild(TCHAR_TO_ANSI(*BuildString));

        ////// Configure available virtual currencies and item types
        if (ProjectSettings.ResourceCurrencies.Num() > 0)
        {
            std::vector<std::string> currencies;
            for (const FString& currency : ProjectSettings.ResourceCurrencies)
            {
                currencies.push_back(TCHAR_TO_ANSI(*currency));
            }
            UGameAnalytics::configureAvailableResourceCurrencies(currencies);
        }

        if (ProjectSettings.ResourceItemTypes.Num() > 0)
        {
            std::vector<std::string> resourceItem;
            for (const FString& item : ProjectSettings.ResourceItemTypes)
            {
                resourceItem.push_back(TCHAR_TO_ANSI(*item));
            }
            UGameAnalytics::configureAvailableResourceItemTypes(resourceItem);
        }

        // Configure available custom dimensions
        if (ProjectSettings.CustomDimensions01.Num() > 0)
        {
            std::vector<std::string> customDimension01;
            for (const FString& dimension01 : ProjectSettings.CustomDimensions01)
            {
                customDimension01.push_back(TCHAR_TO_ANSI(*dimension01));
            }
            UGameAnalytics::configureAvailableCustomDimensions01(customDimension01);
        }

        if (ProjectSettings.CustomDimensions02.Num() > 0)
        {
            std::vector<std::string> customDimension02;
            for (const FString& dimension02 : ProjectSettings.CustomDimensions02)
            {
                customDimension02.push_back(TCHAR_TO_ANSI(*dimension02));
            }
            UGameAnalytics::configureAvailableCustomDimensions02(customDimension02);
        }

        if (ProjectSettings.CustomDimensions03.Num() > 0)
        {
            std::vector<std::string> customDimension03;
            for (const FString& dimension03 : ProjectSettings.CustomDimensions03)
            {
                customDimension03.push_back(TCHAR_TO_ANSI(*dimension03));
            }
            UGameAnalytics::configureAvailableCustomDimensions03(customDimension03);
        }

        if(ProjectSettings.UseManualSessionHandling)
        {
            UGameAnalytics::setEnabledManualSessionHandling(ProjectSettings.UseManualSessionHandling);
        }

        const int32 AttrCount = Attributes.Num();
        FString gameKey;
        FString secretKey;
        if (AttrCount > 0)
        {
            // Load from user supplied arguments
            for (auto Attr : Attributes)
            {
#if PLATFORM_IOS
                if (Attr.AttrName == TEXT("ios_gameKey"))
                {
                    gameKey = Attr.ToString();
                }
                else if (Attr.AttrName == TEXT("ios_secretKey"))
                {
                    secretKey = Attr.ToString();
                }
#elif PLATFORM_ANDROID
                if (Attr.AttrName == TEXT("android_gameKey"))
                {
                    gameKey = Attr.ToString();
                }
                else if (Attr.AttrName == TEXT("android_secretKey"))
                {
                    secretKey = Attr.ToString();
                }
#elif PLATFORM_MAC
                if (Attr.AttrName == TEXT("mac_gameKey"))
                {
                    gameKey = Attr.ToString();
                }
                else if (Attr.AttrName == TEXT("mac_secretKey"))
                {
                    secretKey = Attr.ToString();
                }
#elif PLATFORM_WINDOWS
                if (Attr.AttrName == TEXT("windows_gameKey"))
                {
                    gameKey = Attr.ToString();
                }
                else if (Attr.AttrName == TEXT("windows_secretKey"))
                {
                    secretKey = Attr.ToString();
                }
#elif PLATFORM_LINUX
                if (Attr.AttrName == TEXT("linux_gameKey"))
                {
                    gameKey = Attr.ToString();
                }
                else if (Attr.AttrName == TEXT("linux_secretKey"))
                {
                    secretKey = Attr.ToString();
                }
#elif PLATFORM_HTML5
                if (Attr.AttrName == TEXT("html5_gameKey"))
                {
                    gameKey = Attr.ToString();
                }
                else if (Attr.AttrName == TEXT("html5_secretKey"))
                {
                    secretKey = Attr.ToString();
                }
#endif
            }
        }
        else
        {
            // Load from project settings
#if PLATFORM_IOS
            gameKey = ProjectSettings.IosGameKey;
            secretKey = ProjectSettings.IosSecretKey;
#elif PLATFORM_ANDROID
            gameKey = ProjectSettings.AndroidGameKey;
            secretKey = ProjectSettings.AndroidSecretKey;
#elif PLATFORM_MAC
            gameKey = ProjectSettings.MacGameKey;
            secretKey = ProjectSettings.MacSecretKey;
#elif PLATFORM_WINDOWS
            gameKey = ProjectSettings.WindowsGameKey;
            secretKey = ProjectSettings.WindowsSecretKey;
#elif PLATFORM_LINUX
            gameKey = ProjectSettings.LinuxGameKey;
            secretKey = ProjectSettings.LinuxSecretKey;
#elif PLATFORM_HTML5
            gameKey = ProjectSettings.Html5GameKey;
            secretKey = ProjectSettings.Html5SecretKey;
#endif
        }

        ////// Initialize (also calls startNewSession)
        UGameAnalytics::initialize(TCHAR_TO_ANSI(*gameKey), TCHAR_TO_ANSI(*secretKey));
        //UGameAnalytics::logGAStateInfo(TEXT("FAnalyticsProviderGameAnalytics::StartSession AFTER UGameAnalytics::initialize"));
        bHasSessionStarted = true;

    }
    else if (ProjectSettings.UseManualSessionHandling)
    {
        UGameAnalytics::startSession();
    }

    return bHasSessionStarted;
}

void FAnalyticsProviderGameAnalytics::EndSession()
{
    if (bHasSessionStarted)
    {
        //UGameAnalytics::logGAStateInfo(TEXT("FAnalyticsProviderGameAnalytics::EndSession"));
        if(ProjectSettings.UseManualSessionHandling)
        {
            UGameAnalytics::endSession();
        }
        else
        {
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
            // Empty
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
            gameanalytics::GameAnalytics::onSuspend();
#elif PLATFORM_HTML5
            js_onStop();
#else
            UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::EndSession ignored."));
#endif
        }
    }

#if WITH_EDITOR && TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
    UE_LOG(LogGameAnalyticsAnalytics, Verbose, TEXT("waitUntilJobsAreDone"));
    UGameAnalytics::waitUntilJobsAreDone();
#endif
}

void FAnalyticsProviderGameAnalytics::FlushEvents()
{
    UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::FlushEvents ignored."));
}

void FAnalyticsProviderGameAnalytics::SetUserID(const FString& InUserID)
{
    if(!bHasSessionStarted)
    {
        UE_LOG(LogGameAnalyticsAnalytics, Display, TEXT("GameAnalytics::SetCustomId('%s')"), *InUserID);
        UGameAnalytics::configureUserId(TCHAR_TO_ANSI(*InUserID));
        UserId = InUserID;
    }
    else
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::SetCustomId Custom id can only be set when SDK is not initialized."));
    }
}

FString FAnalyticsProviderGameAnalytics::GetUserID() const
{
    return UserId;
}

FString FAnalyticsProviderGameAnalytics::GetSessionID() const
{
    UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::GetSessionID ignored."));
    return SessionId;
}

bool FAnalyticsProviderGameAnalytics::SetSessionID(const FString& InSessionID)
{
    UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::SetSessionID ignored."));
    return !bHasSessionStarted;
}

void FAnalyticsProviderGameAnalytics::RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes)
{
    const int32 AttrCount = Attributes.Num();
    if (AttrCount > 0)
    {
        // Send an event for each attribute
        for (auto Attr : Attributes)
        {
            if (Attr.AttrName == TEXT("custom1"))
            {
                UGameAnalytics::setCustomDimension01(TCHAR_TO_ANSI(*Attr.ToString()));
            }
            else if (Attr.AttrName == TEXT("custom2"))
            {
                UGameAnalytics::setCustomDimension02(TCHAR_TO_ANSI(*Attr.ToString()));
            }
            else if (Attr.AttrName == TEXT("custom3"))
            {
                UGameAnalytics::setCustomDimension03(TCHAR_TO_ANSI(*Attr.ToString()));
            }
            else if (Attr.AttrName == TEXT("facebook"))
            {
                UGameAnalytics::setFacebookId(TCHAR_TO_ANSI(*Attr.ToString()));
            }
            else
            {
                float AttrValue = FCString::Atof(*Attr.ToString());
                UGameAnalytics::addDesignEvent(TCHAR_TO_ANSI(*Attr.AttrName), AttrValue);
            }
        }
    }
    else if (EventName.Len() > 0)
    {
        // Send an event without value
        UGameAnalytics::addDesignEvent(TCHAR_TO_ANSI(*EventName));
    }
}

void FAnalyticsProviderGameAnalytics::SetAge(int InAge)
{
    UGameAnalytics::setBirthYear(InAge);
}

void FAnalyticsProviderGameAnalytics::SetGender(const FString& InGender)
{
    EGAGender gender = GetEnumValueFromString<EGAGender>("EGAGender", InGender.ToLower());

    if (gender == EGAGender(0))
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("SetGender: Error value must be either male or female."));
        return;
    }

    UGameAnalytics::setGender(gender);
}

void FAnalyticsProviderGameAnalytics::RecordError(const FString& Error)
{
    UE_LOG(LogGameAnalyticsAnalytics, Display, TEXT("GameAnalytics::RecordError(const FString& Error) - ignoring call"));
}

void FAnalyticsProviderGameAnalytics::RecordError(const FString& Error, const TArray<FAnalyticsEventAttribute>& Attributes)
{
    EGAErrorSeverity ErrorSeverity = GetEnumValueFromString<EGAErrorSeverity>("EGAErrorSeverity", Error.ToLower());

    if (ErrorSeverity == EGAErrorSeverity(0))
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("RecordError: Error value must be either debug, info, warning, error, critical. Error=%s"), *Error);
        return;
    }

    const int32 AttrCount = Attributes.Num();
    if (AttrCount > 0)
    {
        for (auto Attr : Attributes)
        {
            if (Attr.AttrName == TEXT("message"))
            {
                UGameAnalytics::addErrorEvent(ErrorSeverity, TCHAR_TO_ANSI(*Attr.ToString()));
            }
        }
    }
    else
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::RecordError wrong usage, for correct usage see: https://github.com/GameAnalytics/GA-SDK-UNREAL/wiki/Error%20Event"));
    }
}

void FAnalyticsProviderGameAnalytics::RecordProgress(const FString& ProgressType, const FString& ProgressHierarchy)
{
    EGAProgressionStatus ProgressionStatus = GetEnumValueFromString<EGAProgressionStatus>("EGAProgressionStatus", ProgressType.ToLower());

    if (ProgressionStatus == EGAProgressionStatus(0))
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("RecordProgress: ProgressType value must be either start, complete or fail. ProgressType=%s"), *ProgressType);
        return;
    }

    UGameAnalytics::addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*ProgressHierarchy));
}

void FAnalyticsProviderGameAnalytics::RecordProgress(const FString& ProgressType, const FString& ProgressHierarchy, const TArray<FAnalyticsEventAttribute>& Attributes)
{
    EGAProgressionStatus ProgressionStatus = GetEnumValueFromString<EGAProgressionStatus>("EGAProgressionStatus", ProgressType.ToLower());

    if (ProgressionStatus == EGAProgressionStatus(0))
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("RecordProgress: ProgressType value must be either start, complete or fail. ProgressType=%s"), *ProgressType);
        return;
    }

    bool useValue = false;

    for (auto Attr : Attributes)
    {
        if (Attr.AttrName == TEXT("value"))
        {
            int32 value = FCString::Atoi(*Attr.ToString());
            useValue = true;

            UGameAnalytics::addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*ProgressHierarchy), value);
            break;
        }
    }

    if (!useValue)
    {
        UGameAnalytics::addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*ProgressHierarchy));
    }
}

void FAnalyticsProviderGameAnalytics::RecordProgress(const FString& ProgressType, const TArray<FString>& ProgressHierarchy, const TArray<FAnalyticsEventAttribute>& Attributes)
{
    EGAProgressionStatus ProgressionStatus = GetEnumValueFromString<EGAProgressionStatus>("EGAProgressionStatus", ProgressType.ToLower());

    if (ProgressionStatus == EGAProgressionStatus(0))
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("RecordProgress: ProgressType value must be either start, complete or fail. ProgressType=%s"), *ProgressType);
        return;
    }

    const int32 ProgressHierarchyCount = ProgressHierarchy.Num();
    if(ProgressHierarchyCount > 0)
    {
        bool useValue = false;

        for (auto Attr : Attributes)
        {
            UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("RecordProgress: Attributes. AttrName=%s"), *Attr.AttrName);
            if (Attr.AttrName == TEXT("value"))
            {
                int32 value = FCString::Atoi(*Attr.ToString());
                useValue = true;

                if (ProgressHierarchyCount > 2)
                {
                    UGameAnalytics::addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*ProgressHierarchy[0]), TCHAR_TO_ANSI(*ProgressHierarchy[1]), TCHAR_TO_ANSI(*ProgressHierarchy[2]), value);
                }
                else if (ProgressHierarchyCount > 1)
                {
                    UGameAnalytics::addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*ProgressHierarchy[0]), TCHAR_TO_ANSI(*ProgressHierarchy[1]), value);
                }
                else
                {
                    UGameAnalytics::addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*ProgressHierarchy[0]), value);
                }
                break;
            }
        }

        if (ProgressHierarchyCount > 2)
        {
            if(!useValue)
            {
                UGameAnalytics::addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*ProgressHierarchy[0]), TCHAR_TO_ANSI(*ProgressHierarchy[1]), TCHAR_TO_ANSI(*ProgressHierarchy[2]));
            }
        }
        else if (ProgressHierarchyCount > 1)
        {
            if(!useValue)
            {
                UGameAnalytics::addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*ProgressHierarchy[0]), TCHAR_TO_ANSI(*ProgressHierarchy[1]));
            }
        }
        else
        {
            if(!useValue)
            {
                UGameAnalytics::addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*ProgressHierarchy[0]));
            }
        }
    }
    else
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::RecordProgress wrong usage, for correct usage see: https://github.com/GameAnalytics/GA-SDK-UNREAL/wiki/Progression%20Event"));
    }
}

void FAnalyticsProviderGameAnalytics::RecordItemPurchase(const FString& ItemId, const FString& Currency, int PerItemCost, int ItemQuantity)
{
    UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::RecordItemPurchase(const FString& ItemId, const FString& Currency, int PerItemCost, int ItemQuantity) ignored. Instead use: FAnalyticsProviderGameAnalytics::RecordItemPurchase(const FString& ItemId, int ItemQuantity, const TArray<FAnalyticsEventAttribute>& Attributes)"));
}

void FAnalyticsProviderGameAnalytics::RecordItemPurchase(const FString& ItemId, int ItemQuantity, const TArray<FAnalyticsEventAttribute>& Attributes)
{
    EGAResourceFlowType FlowType = EGAResourceFlowType::source;
    FString Currency;
    FString ItemType;

    const int32 AttrCount = Attributes.Num();
    if (AttrCount > 0)
    {
        for (auto Attr : Attributes)
        {
            if (Attr.AttrName == TEXT("flowType"))
            {
                FlowType = GetEnumValueFromString<EGAResourceFlowType>("EGAResourceFlowType", Attr.ToString().ToLower());

                if (FlowType == EGAResourceFlowType(0))
                {
                    UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("RecordItemPurchaseError: FlowType value must be either sink or source. flowType=%s"), *Attr.ToString());
                    return;
                }
            }
            else if (Attr.AttrName == TEXT("currency"))
            {
                Currency = Attr.ToString();
            }
            else if (Attr.AttrName == TEXT("itemType"))
            {
                ItemType = Attr.ToString();
            }
        }

        if (!Currency.IsEmpty() && !ItemType.IsEmpty())
        {
            UGameAnalytics::addResourceEvent(FlowType, TCHAR_TO_ANSI(*Currency), ItemQuantity, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId));
        }
        else
        {
            UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::RecordItemPurchase wrong usage, for correct usage see: https://github.com/GameAnalytics/GA-SDK-UNREAL/wiki/Resource%20Event"));
        }
    }
    else
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::RecordItemPurchase wrong usage, for correct usage see: https://github.com/GameAnalytics/GA-SDK-UNREAL/wiki/Resource%20Event"));
    }
}

void FAnalyticsProviderGameAnalytics::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const FString& RealCurrencyType, float RealMoneyCost, const FString& PaymentProvider)
{
    UE_LOG(LogGameAnalyticsAnalytics, Display, TEXT("FAnalyticsProviderGameAnalytics::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const FString& RealCurrencyType, float RealMoneyCost, const FString& PaymentProvider) - ignoring call. Instead use: FAnalyticsProviderGameAnalytics::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& Attributes)"));
}

void FAnalyticsProviderGameAnalytics::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount)
{
    UE_LOG(LogGameAnalyticsAnalytics, Display, TEXT("FAnalyticsProviderGameAnalytics::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount) - ignoring call. Instead use: FAnalyticsProviderGameAnalytics::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& Attributes)"));
}

void FAnalyticsProviderGameAnalytics::RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& Attributes)
{
    FString ItemType;
    FString ItemId;
    FString CartType;
    FString Receipt;
    FString Signature;
    bool AutoFetchReceipt = false;

    const int32 AttrCount = Attributes.Num();
    if (AttrCount > 0)
    {
        for (auto Attr : Attributes)
        {
            FString AttrString = Attr.AttrName;

            if (Attr.AttrName == TEXT("itemType"))
            {
                ItemType = Attr.ToString();
            }
            else if (Attr.AttrName == TEXT("itemId"))
            {
                ItemId = Attr.ToString();
            }
            else if (Attr.AttrName == TEXT("cartType"))
            {
                CartType = Attr.ToString();
            }
            else if (Attr.AttrName == TEXT("receipt"))
            {
                Receipt = Attr.ToString();
            }
            else if (Attr.AttrName == TEXT("autoFetchReceipt"))
            {
                if (Attr.ToString().ToBool())
                {
                    AutoFetchReceipt = true;
                }
            }
            else if (Attr.AttrName == TEXT("signature"))
            {
                Signature = Attr.ToString();
            }
        }

        if (!ItemType.IsEmpty() && !ItemId.IsEmpty() && !CartType.IsEmpty())
        {
#if PLATFORM_ANDROID
            if (!Receipt.IsEmpty() && !Signature.IsEmpty())
            {
                UGameAnalytics::addBusinessEvent(TCHAR_TO_ANSI(*GameCurrencyType), GameCurrencyAmount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType), TCHAR_TO_ANSI(*Receipt), TCHAR_TO_ANSI(*Signature));
            }
            else
            {
                UGameAnalytics::addBusinessEvent(TCHAR_TO_ANSI(*GameCurrencyType), GameCurrencyAmount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType));
            }
#elif PLATFORM_IOS
            if (!Receipt.IsEmpty())
            {
                UGameAnalytics::addBusinessEvent(TCHAR_TO_ANSI(*GameCurrencyType), GameCurrencyAmount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType), TCHAR_TO_ANSI(*Receipt));
            }
            else
            {
                if(AutoFetchReceipt)
                {
                    UGameAnalytics::addBusinessEventAndAutoFetchReceipt(TCHAR_TO_ANSI(*GameCurrencyType), GameCurrencyAmount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType));
                }
                else
                {
                    UGameAnalytics::addBusinessEvent(TCHAR_TO_ANSI(*GameCurrencyType), GameCurrencyAmount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType));
                }
            }
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_HTML5
            UGameAnalytics::addBusinessEvent(TCHAR_TO_ANSI(*GameCurrencyType), GameCurrencyAmount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType));
#endif
        }
        else
        {
            UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::RecordCurrencyPurchase wrong usage, for correct usage see: https://github.com/GameAnalytics/GA-SDK-UNREAL/wiki/Business%20Event"));
        }
    }
    else
    {
        UE_LOG(LogGameAnalyticsAnalytics, Warning, TEXT("FAnalyticsProviderGameAnalytics::RecordCurrencyPurchase wrong usage, for correct usage see: https://github.com/GameAnalytics/GA-SDK-UNREAL/wiki/Business%20Event"));
    }
}

void FAnalyticsProviderGameAnalytics::RecordCurrencyGiven(const FString& GameCurrencyType, int GameCurrencyAmount)
{
    UE_LOG(LogGameAnalyticsAnalytics, Display, TEXT("GameAnalytics::RecordCurrencyGiven - ignoring call"));
}

void FAnalyticsProviderGameAnalytics::RecordCurrencyGiven(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& Attributes)
{
    UE_LOG(LogGameAnalyticsAnalytics, Display, TEXT("GameAnalytics::RecordCurrencyGiven - ignoring call"));
}
