#include "GameAnalytics.h"
#include "UEGameAnalytics.h"

#if PLATFORM_IOS
#include "../GA-SDK-IOS/GameAnalyticsCpp.h"
#elif PLATFORM_ANDROID
#include "../GA-SDK-ANDROID/GameAnalyticsJNI.h"
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
#if PLATFORM_LINUX
#include "Json.h"
#endif
#include "../GA-SDK-CPP/GameAnalytics.h"
#include "../GA-SDK-CPP/GAState.h"
#include "../GA-SDK-CPP/GAThreading.h"
#include "../GA-SDK-CPP/GALogger.h"
#include "../GA-SDK-CPP/GADevice.h"
#include "../GA-SDK-CPP/GAEvents.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#elif PLATFORM_HTML5
#include "Json.h"
#include "../GA-SDK-HTML5/GameAnalytics.h"
#endif

UGameAnalytics::UGameAnalytics(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UGameAnalytics::configureAvailableCustomDimensions01(const std::vector<std::string>& list)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::configureAvailableCustomDimensions01(list);
#elif PLATFORM_ANDROID
    gameanalytics::jni_configureAvailableCustomDimensions01(list);
#elif PLATFORM_LINUX
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions01(TCHAR_TO_ANSI(*arrayString));
#elif PLATFORM_MAC || PLATFORM_WINDOWS
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions01(list);
#elif PLATFORM_HTML5
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    js_configureAvailableCustomDimensions01(TCHAR_TO_ANSI(*arrayString));
#endif
}

void UGameAnalytics::configureAvailableCustomDimensions02(const std::vector<std::string>& list)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::configureAvailableCustomDimensions02(list);
#elif PLATFORM_ANDROID
    gameanalytics::jni_configureAvailableCustomDimensions02(list);
#elif PLATFORM_LINUX
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions02(TCHAR_TO_ANSI(*arrayString));
#elif PLATFORM_MAC || PLATFORM_WINDOWS
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions02(list);
#elif PLATFORM_HTML5
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    js_configureAvailableCustomDimensions02(TCHAR_TO_ANSI(*arrayString));
#endif
}

void UGameAnalytics::configureAvailableCustomDimensions03(const std::vector<std::string>& list)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::configureAvailableCustomDimensions03(list);
#elif PLATFORM_ANDROID
    gameanalytics::jni_configureAvailableCustomDimensions03(list);
#elif PLATFORM_LINUX
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions03(TCHAR_TO_ANSI(*arrayString));
#elif PLATFORM_MAC || PLATFORM_WINDOWS
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions03(list);
#elif PLATFORM_HTML5
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    js_configureAvailableCustomDimensions03(TCHAR_TO_ANSI(*arrayString));
#endif
}

void UGameAnalytics::configureAvailableResourceCurrencies(const std::vector<std::string>& list)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::configureAvailableResourceCurrencies(list);
#elif PLATFORM_ANDROID
    gameanalytics::jni_configureAvailableResourceCurrencies(list);
#elif PLATFORM_LINUX
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    gameanalytics::GameAnalytics::configureAvailableResourceCurrencies(TCHAR_TO_ANSI(*arrayString));
#elif PLATFORM_MAC || PLATFORM_WINDOWS
    gameanalytics::GameAnalytics::configureAvailableResourceCurrencies(list);
#elif PLATFORM_HTML5
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    js_configureAvailableResourceCurrencies(TCHAR_TO_ANSI(*arrayString));
#endif
}

void UGameAnalytics::configureAvailableResourceItemTypes(const std::vector<std::string>& list)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::configureAvailableResourceItemTypes(list);
#elif PLATFORM_ANDROID
    gameanalytics::jni_configureAvailableResourceItemTypes(list);
#elif PLATFORM_LINUX
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    gameanalytics::GameAnalytics::configureAvailableResourceItemTypes(TCHAR_TO_ANSI(*arrayString));
#elif PLATFORM_MAC || PLATFORM_WINDOWS
    gameanalytics::GameAnalytics::configureAvailableResourceItemTypes(list);
#elif PLATFORM_HTML5
    TArray<TSharedPtr<FJsonValue>> array;
    for (std::string s : list)
    {
        TSharedRef<FJsonValueString> JsonValueString = MakeShareable(new FJsonValueString(ANSI_TO_TCHAR(s.c_str())));
        array.Add(JsonValueString);
    }
    FString arrayString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&arrayString);
    FJsonSerializer::Serialize(array, Writer);
    js_configureAvailableResourceItemTypes(TCHAR_TO_ANSI(*arrayString));
#endif
}

void UGameAnalytics::configureBuild(const char *build)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::configureBuild(build);
#elif PLATFORM_ANDROID
    gameanalytics::jni_configureBuild(build);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::configureBuild(build);
#elif PLATFORM_HTML5
    js_configureBuild(build);
#endif
}

void UGameAnalytics::configureUserId(const char *userId)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::configureUserId(userId);
#elif PLATFORM_ANDROID
    gameanalytics::jni_configureUserId(userId);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::configureUserId(userId);
#elif PLATFORM_HTML5
    js_configureUserId(userId);
#endif
}

void UGameAnalytics::configureSdkGameEngineVersion(const char *gameEngineSdkVersion)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::configureSdkGameEngineVersion(gameEngineSdkVersion);
#elif PLATFORM_ANDROID
    gameanalytics::jni_configureSdkGameEngineVersion(gameEngineSdkVersion);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::configureSdkGameEngineVersion(gameEngineSdkVersion);
#elif PLATFORM_HTML5
    js_configureSdkGameEngineVersion(gameEngineSdkVersion);
#endif
}

void UGameAnalytics::configureGameEngineVersion(const char *gameEngineVersion)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::configureGameEngineVersion(gameEngineVersion);
#elif PLATFORM_ANDROID
    gameanalytics::jni_configureGameEngineVersion(gameEngineVersion);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::configureGameEngineVersion(gameEngineVersion);
#elif PLATFORM_HTML5
    js_configureGameEngineVersion(gameEngineVersion);
#endif
}

void UGameAnalytics::configureWritablePath(const char *writablePath)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#else
	gameanalytics::GameAnalytics::configureWritablePath(writablePath);
#endif
}

void UGameAnalytics::initialize(const char *gameKey, const char *gameSecret)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::initialize(gameKey, gameSecret);
#elif PLATFORM_ANDROID
    gameanalytics::jni_initialize(gameKey, gameSecret);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::initialize(gameKey, gameSecret);
#elif PLATFORM_HTML5
    js_initialize(gameKey, gameSecret);
#endif
}

#if PLATFORM_IOS
void UGameAnalytics::addBusinessEvent(const char *currency, int amount, const char *itemType, const char *itemId, const char *cartType, const char *receipt)
{
#if !WITH_EDITOR
    GameAnalyticsCpp::addBusinessEvent(currency, amount, itemType, itemId, cartType, receipt, "");
#endif
}

void UGameAnalytics::addBusinessEventAndAutoFetchReceipt(const char *currency, int amount, const char *itemType, const char *itemId, const char *cartType)
{
#if !WITH_EDITOR || TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
    GameAnalyticsCpp::addBusinessEventAndAutoFetchReceipt(currency, amount, itemType, itemId, cartType, "");
#endif
}
#elif PLATFORM_ANDROID
void UGameAnalytics::addBusinessEvent(const char *currency, int amount, const char *itemType, const char *itemId, const char *cartType, const char *receipt, const char *signature)
{
#if !WITH_EDITOR || TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
    gameanalytics::jni_addBusinessEventWithReceipt(currency, amount, itemType, itemId, cartType, receipt, "google_play", signature, "");
#endif
}
#endif

void UGameAnalytics::addBusinessEvent(const char *currency, int amount, const char *itemType, const char *itemId, const char *cartType)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::addBusinessEvent(currency, amount, itemType, itemId, cartType, NULL, "");
#elif PLATFORM_ANDROID
    gameanalytics::jni_addBusinessEvent(currency, amount, itemType, itemId, cartType, "");
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::addBusinessEvent(currency, amount, itemType, itemId, cartType/*, ""*/);
#elif PLATFORM_HTML5
    js_addBusinessEvent(currency, amount, itemType, itemId, cartType, "");
#endif
}

void UGameAnalytics::addResourceEvent(EGAResourceFlowType flowType, const char *currency, float amount, const char *itemType, const char *itemId)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::addResourceEvent((int)flowType, currency, amount, itemType, itemId, "");
#elif PLATFORM_ANDROID
    gameanalytics::jni_addResourceEvent((int)flowType, currency, amount, itemType, itemId, "");
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::addResourceEvent((gameanalytics::EGAResourceFlowType)((int)flowType), currency, amount, itemType, itemId/*, ""*/);
#elif PLATFORM_HTML5
    js_addResourceEvent((int)flowType, currency, amount, itemType, itemId, "");
#endif
}

void UGameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01)
{
#if PLATFORM_IOS
    addProgressionEvent(progressionStatus, progression01, (const char *)NULL, (const char *)NULL);
#else
    addProgressionEvent(progressionStatus, progression01, "", "");
#endif
}

void UGameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, int score)
{
#if PLATFORM_IOS
    addProgressionEvent(progressionStatus, progression01, (const char *)NULL, (const char *)NULL, score);
#else
    addProgressionEvent(progressionStatus, progression01, "", "", score);
#endif
}

void UGameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, const char *progression02)
{
#if PLATFORM_IOS
    addProgressionEvent(progressionStatus, progression01, progression02, (const char *)NULL);
#else
    addProgressionEvent(progressionStatus, progression01, progression02, "");
#endif
}

void UGameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, const char *progression02, int score)
{
#if PLATFORM_IOS
    addProgressionEvent(progressionStatus, progression01, progression02, (const char *)NULL, score);
#else
    addProgressionEvent(progressionStatus, progression01, progression02, "", score);
#endif
}

void UGameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, const char *progression02, const char *progression03)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::addProgressionEvent((int)progressionStatus, progression01, progression02, progression03, "");
#elif PLATFORM_ANDROID
    gameanalytics::jni_addProgressionEvent((int)progressionStatus, progression01, progression02, progression03, "");
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)((int)progressionStatus), progression01, progression02, progression03/*, ""*/);
#elif PLATFORM_HTML5
    js_addProgressionEvent((int)progressionStatus, progression01, progression02, progression03, "");
#endif
}

void UGameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, const char *progression02, const char *progression03, int score)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::addProgressionEventWithScore((int)progressionStatus, progression01, progression02, progression03, score, "");
#elif PLATFORM_ANDROID
    gameanalytics::jni_addProgressionEventWithScore((int)progressionStatus, progression01, progression02, progression03, score, "");
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)((int)progressionStatus), progression01, progression02, progression03, score/*, ""*/);
#elif PLATFORM_HTML5
    js_addProgressionEventWithScore((int)progressionStatus, progression01, progression02, progression03, score, "");
#endif
}

void UGameAnalytics::addDesignEvent(const char *eventId)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::addDesignEvent(eventId, "");
#elif PLATFORM_ANDROID
    gameanalytics::jni_addDesignEvent(eventId, "");
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::addDesignEvent(eventId/*, ""*/);
#elif PLATFORM_HTML5
    js_addDesignEvent(eventId, "");
#endif
}

void UGameAnalytics::addDesignEvent(const char *eventId, float value)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::addDesignEventWithValue(eventId, value, "");
#elif PLATFORM_ANDROID
    gameanalytics::jni_addDesignEventWithValue(eventId, value, "");
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::addDesignEvent(eventId, value/*, ""*/);
#elif PLATFORM_HTML5
    js_addDesignEventWithValue(eventId, value, "");
#endif
}

void UGameAnalytics::addErrorEvent(EGAErrorSeverity severity, const char *message)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::addErrorEvent((int)severity, message, "");
#elif PLATFORM_ANDROID
    gameanalytics::jni_addErrorEvent((int)severity, message, "");
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::addErrorEvent((gameanalytics::EGAErrorSeverity)((int)severity), message/*, ""*/);
#elif PLATFORM_HTML5
    js_addErrorEvent((int)severity, message, "");
#endif
}

void UGameAnalytics::setEnabledInfoLog(bool flag)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::setEnabledInfoLog(flag);
#elif PLATFORM_ANDROID
    gameanalytics::jni_setEnabledInfoLog(flag);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::setEnabledInfoLog(flag);
#elif PLATFORM_HTML5
    js_setEnabledInfoLog(flag);
#endif
}

void UGameAnalytics::setEnabledVerboseLog(bool flag)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::setEnabledVerboseLog(flag);
#elif PLATFORM_ANDROID
    gameanalytics::jni_setEnabledVerboseLog(flag);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::setEnabledVerboseLog(flag);
#elif PLATFORM_HTML5
    js_setEnabledVerboseLog(flag);
#endif
}

void UGameAnalytics::setEnabledManualSessionHandling(bool flag)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::setEnabledManualSessionHandling(flag);
#elif PLATFORM_ANDROID
    gameanalytics::jni_setEnabledManualSessionHandling(flag);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::setEnabledManualSessionHandling(flag);
#elif PLATFORM_HTML5
    js_setManualSessionHandling(flag);
#endif
}

void UGameAnalytics::setCustomDimension01(const char *customDimension)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::setCustomDimension01(customDimension);
#elif PLATFORM_ANDROID
    gameanalytics::jni_setCustomDimension01(customDimension);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::setCustomDimension01(customDimension);
#elif PLATFORM_HTML5
    js_setCustomDimension01(customDimension);
#endif
}

void UGameAnalytics::setCustomDimension02(const char *customDimension)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::setCustomDimension02(customDimension);
#elif PLATFORM_ANDROID
    gameanalytics::jni_setCustomDimension02(customDimension);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::setCustomDimension02(customDimension);
#elif PLATFORM_HTML5
    js_setCustomDimension02(customDimension);
#endif
}

void UGameAnalytics::setCustomDimension03(const char *customDimension)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::setCustomDimension03(customDimension);
#elif PLATFORM_ANDROID
    gameanalytics::jni_setCustomDimension03(customDimension);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::setCustomDimension03(customDimension);
#elif PLATFORM_HTML5
    js_setCustomDimension03(customDimension);
#endif
}

void UGameAnalytics::setFacebookId(const char *facebookId)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::setFacebookId(facebookId);
#elif PLATFORM_ANDROID
    gameanalytics::jni_setFacebookId(facebookId);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::setFacebookId(facebookId);
#elif PLATFORM_HTML5
    js_setFacebookId(facebookId);
#endif
}

void UGameAnalytics::setGender(EGAGender gender)
{
    switch(gender)
    {
        case EGAGender::male:
        {
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
			// Empty
#elif PLATFORM_IOS
        	GameAnalyticsCpp::setGender("male");
#elif PLATFORM_ANDROID
        	gameanalytics::jni_setGender((int)gender);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
            gameanalytics::GameAnalytics::setGender((gameanalytics::EGAGender)((int)gender));
#elif PLATFORM_HTML5
            js_setGender((int)gender);
#endif
        }
        break;

        case EGAGender::female:
        {
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
			// Empty
#elif PLATFORM_IOS
        	GameAnalyticsCpp::setGender("female");
#elif PLATFORM_ANDROID
        	gameanalytics::jni_setGender((int)gender);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
            gameanalytics::GameAnalytics::setGender((gameanalytics::EGAGender)((int)gender));
#elif PLATFORM_HTML5
            js_setGender((int)gender);
#endif
        }
        break;
    }
}

void UGameAnalytics::setBirthYear(int birthYear)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::setBirthYear(birthYear);
#elif PLATFORM_ANDROID
    gameanalytics::jni_setBirthYear(birthYear);
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::setBirthYear(birthYear);
#elif PLATFORM_HTML5
    js_setBirthYear(birthYear);
#endif
}

void UGameAnalytics::startSession()
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::startSession();
#elif PLATFORM_ANDROID
    gameanalytics::jni_startSession();
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::startSession();
#elif PLATFORM_HTML5
    js_startSession();
#endif
}

void UGameAnalytics::endSession()
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#elif PLATFORM_IOS
    GameAnalyticsCpp::endSession();
#elif PLATFORM_ANDROID
    gameanalytics::jni_endSession();
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    gameanalytics::GameAnalytics::endSession();
#elif PLATFORM_HTML5
    js_endSession();
#endif
}

void UGameAnalytics::logGAStateInfo(const TCHAR* Context)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#else
	// Must be on the same thread so that we can log in the proper order
	UE_LOG(LogGameAnalyticsAnalytics, Verbose, TEXT("Called UGameAnalytics::logGAStateInfo (outside of thread) with Context = `%s`"), Context);
	gameanalytics::threading::GAThreading::performTaskOnGAThread([Context]()
	{
		const bool bIsInitialized = gameanalytics::state::GAState::isInitialized();
		const bool bIsEnabled = gameanalytics::state::GAState::isEnabled();
		const bool bIsSessionStarted = gameanalytics::state::GAState::sessionIsStarted();
		const double MainThreadWaitInSeconds = gameanalytics::threading::GAThreading::GetThreadWaitSeconds();
		const double ProcessEventsIntervalInSeconds = gameanalytics::events::GAEvents::GetEventsPollIntervalSeconds();
		UE_LOG(LogGameAnalyticsAnalytics, Verbose, TEXT("%s"), Context);
		UE_LOG(LogGameAnalyticsAnalytics,
			Verbose,
			TEXT("UGameAnalytics::logGAStateInfo: GAState::isInitialized = %d, GAState::isEnabled = %d, GAState::sessionIsStarted = %d, MainThreadWaitInSeconds = %f, ProcessEventsIntervalInSeconds = %f "),
			bIsInitialized, bIsEnabled, bIsSessionStarted, MainThreadWaitInSeconds, ProcessEventsIntervalInSeconds);
		UE_LOG(LogGameAnalyticsAnalytics, Verbose, TEXT(""));
	});
#endif
}

void UGameAnalytics::setThreadAndEventTimers(double ThreadWaitSeconds, double ThreadProcessEventsSeconds)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#else
	gameanalytics::threading::GAThreading::SetThreadWaitSeconds(ThreadWaitSeconds);
	gameanalytics::events::GAEvents::SetEventsPollIntervalSeconds(ThreadProcessEventsSeconds);
#endif
}

void UGameAnalytics::waitUntilJobsAreDone()
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
	// Empty
#else
	while (gameanalytics::threading::GAThreading::HasJobs())
	{
		if (!gameanalytics::threading::GAThreading::IsThreadRunning())
		{
			// Thread isn't even running anymore, what are you waiting for? get outta here kid
			break;
		}
		FPlatformProcess::Sleep(0.25f);
	}
#endif
}

const char* UGameAnalytics::getCommandCenterValueAsString(const char *key)
{
#if WITH_EDITOR
    return "";
#elif PLATFORM_IOS
    return GameAnalyticsCpp::getCommandCenterValueAsString(key);
#elif PLATFORM_ANDROID
    return gameanalytics::jni_getCommandCenterValueAsString(key);
#elif PLATFORM_LINUX
    return gameanalytics::GameAnalytics::getCommandCenterValueAsString(key);
#elif PLATFORM_MAC || PLATFORM_WINDOWS
    return gameanalytics::GameAnalytics::getCommandCenterValueAsString(key).c_str();
#elif PLATFORM_HTML5
    return js_getCommandCenterValueAsString(key);
#endif
}

const char* UGameAnalytics::getCommandCenterValueAsString(const char *key, const char *defaultValue)
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
    return "";
#elif PLATFORM_IOS
    return GameAnalyticsCpp::getCommandCenterValueAsString(key, defaultValue);
#elif PLATFORM_ANDROID
    return gameanalytics::jni_getCommandCenterValueAsStringWithDefaultValue(key, defaultValue);
#elif PLATFORM_LINUX
    return gameanalytics::GameAnalytics::getCommandCenterValueAsString(key, defaultValue);
#elif PLATFORM_MAC || PLATFORM_WINDOWS
    return gameanalytics::GameAnalytics::getCommandCenterValueAsString(key, defaultValue).c_str();
#elif PLATFORM_HTML5
    return js_getCommandCenterValueAsStringWithDefaultValue(key, defaultValue);
#endif
}

bool UGameAnalytics::isCommandCenterReady()
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
    return false;
#elif PLATFORM_IOS
    return GameAnalyticsCpp::isCommandCenterReady();
#elif PLATFORM_ANDROID
    return gameanalytics::jni_isCommandCenterReady();
#elif PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_LINUX
    return gameanalytics::GameAnalytics::isCommandCenterReady();
#elif PLATFORM_HTML5
    return js_isCommandCenterReady();
#endif
}

const char* UGameAnalytics::getConfigurationsContentAsString()
{
#if WITH_EDITOR && !TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE
    return "";
#elif PLATFORM_IOS
    return GameAnalyticsCpp::getConfigurationsContentAsString();
#elif PLATFORM_ANDROID
    return gameanalytics::jni_getConfigurationsContentAsString();
#elif PLATFORM_LINUX
    return gameanalytics::GameAnalytics::getConfigurationsContentAsString();
#elif PLATFORM_MAC || PLATFORM_WINDOWS
    return gameanalytics::GameAnalytics::getConfigurationsContentAsString().c_str();
#elif PLATFORM_HTML5
    return js_getConfigurationsContentAsString();
#endif
}

// Blueprint functions

void UGameAnalytics::AddBusinessEventIOS(const FString& Currency, int Amount, const FString& ItemType, const FString& ItemId, const FString& CartType, const FString& Receipt/*, const char *fields*/)
{
#if PLATFORM_IOS
    addBusinessEvent(TCHAR_TO_ANSI(*Currency), Amount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType), TCHAR_TO_ANSI(*Receipt));
#endif
}

void UGameAnalytics::AddBusinessEventAndAutoFetchReceipt(const FString& Currency, int Amount, const FString& ItemType, const FString& ItemId, const FString& CartType/*, const char *fields*/)
{
#if PLATFORM_IOS
    addBusinessEventAndAutoFetchReceipt(TCHAR_TO_ANSI(*Currency), Amount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType));
#endif
}

void UGameAnalytics::AddBusinessEventAndroid(const FString& Currency, int Amount, const FString& ItemType, const FString& ItemId, const FString& CartType, const FString& Receipt, const FString& Signature/*, const char *fields*/)
{
#if PLATFORM_ANDROID
    addBusinessEvent(TCHAR_TO_ANSI(*Currency), Amount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType), TCHAR_TO_ANSI(*Receipt), TCHAR_TO_ANSI(*Signature));
#endif
}

void UGameAnalytics::AddBusinessEvent(const FString& Currency, int Amount, const FString& ItemType, const FString& ItemId, const FString& CartType/*, const char *fields*/)
{
    addBusinessEvent(TCHAR_TO_ANSI(*Currency), Amount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId), TCHAR_TO_ANSI(*CartType));
}

void UGameAnalytics::AddResourceEvent(EGAResourceFlowType FlowType, const FString& Currency, float Amount, const FString& ItemType, const FString& ItemId/*, const char *fields*/)
{
    addResourceEvent(FlowType, TCHAR_TO_ANSI(*Currency), Amount, TCHAR_TO_ANSI(*ItemType), TCHAR_TO_ANSI(*ItemId));
}

void UGameAnalytics::AddProgressionEventWithOne(EGAProgressionStatus ProgressionStatus, const FString& Progression01/*, const char *fields*/)
{
    addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*Progression01));
}

void UGameAnalytics::AddProgressionEventWithOneAndScore(EGAProgressionStatus ProgressionStatus, const FString& Progression01, int Score/*, const char *fields*/)
{
    addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*Progression01), Score);
}

void UGameAnalytics::AddProgressionEventWithOneAndTwo(EGAProgressionStatus ProgressionStatus, const FString& Progression01, const FString& Progression02/*, const char *fields*/)
{
    addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*Progression01), TCHAR_TO_ANSI(*Progression02));
}

void UGameAnalytics::AddProgressionEventWithOneTwoAndScore(EGAProgressionStatus ProgressionStatus, const FString& Progression01, const FString& Progression02, int Score/*, const char *fields*/)
{
    addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*Progression01), TCHAR_TO_ANSI(*Progression02), Score);
}

void UGameAnalytics::AddProgressionEventWithOneTwoAndThree(EGAProgressionStatus ProgressionStatus, const FString& Progression01, const FString& Progression02, const FString& Progression03/*, const char *fields*/)
{
    addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*Progression01), TCHAR_TO_ANSI(*Progression02), TCHAR_TO_ANSI(*Progression03));
}

void UGameAnalytics::AddProgressionEvenWithOneTwoThreeAndScore(EGAProgressionStatus ProgressionStatus, const FString& Progression01, const FString& Progression02, const FString& Progression03, int Score/*, const char *fields*/)
{
    addProgressionEvent(ProgressionStatus, TCHAR_TO_ANSI(*Progression01), TCHAR_TO_ANSI(*Progression02), TCHAR_TO_ANSI(*Progression03), Score);
}

void UGameAnalytics::AddDesignEvent(const FString& EventId/*, const char *fields*/)
{
    addDesignEvent(TCHAR_TO_ANSI(*EventId));
}

void UGameAnalytics::AddDesignEventWithValue(const FString& EventId, float Value/*, const char *fields*/)
{
    addDesignEvent(TCHAR_TO_ANSI(*EventId), Value);
}

void UGameAnalytics::AddErrorEvent(EGAErrorSeverity Severity, const FString& Message/*, const char *fields*/)
{
    addErrorEvent(Severity, TCHAR_TO_ANSI(*Message));
}

void UGameAnalytics::SetCustomDimension01(const FString& CustomDimension)
{
    setCustomDimension01(TCHAR_TO_ANSI(*CustomDimension));
}

void UGameAnalytics::SetCustomDimension02(const FString& CustomDimension)
{
    setCustomDimension02(TCHAR_TO_ANSI(*CustomDimension));
}

void UGameAnalytics::SetCustomDimension03(const FString& CustomDimension)
{
    setCustomDimension03(TCHAR_TO_ANSI(*CustomDimension));
}

void UGameAnalytics::SetFacebookId(const FString& FacebookId)
{
    setFacebookId(TCHAR_TO_ANSI(*FacebookId));
}

void UGameAnalytics::SetGender(EGAGender Gender)
{
    setGender(Gender);
}

void UGameAnalytics::SetBirthYear(int BirthYear)
{
    setBirthYear(BirthYear);
}

FString UGameAnalytics::GetCommandCenterValueAsString(const FString& Key)
{
	return FString(getCommandCenterValueAsString(TCHAR_TO_ANSI(*Key)));
}

FString UGameAnalytics::GetCommandCenterValueAsStringWithDefaultValue(const FString& Key, const FString& DefaultValue)
{
	return FString(getCommandCenterValueAsString(TCHAR_TO_ANSI(*Key), TCHAR_TO_ANSI(*DefaultValue)));
}

bool UGameAnalytics::IsCommandCenterReady()
{
	return isCommandCenterReady();
}

FString UGameAnalytics::GetConfigurationsContentAsString()
{
	return FString(getConfigurationsContentAsString());
}
