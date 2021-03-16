//
//  GameAnalytics.h
//  GA-SDK-IOS
//
//  Copyright (c) 2015 GameAnalytics. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

#include "UObject/Object.h"
#include "GameAnalytics.generated.h"


UENUM()
enum class EGAResourceFlowType : uint8
{
    undefined = 0,
    source = 1,
    sink = 2
};

UENUM()
enum class EGAProgressionStatus : uint8
{
    undefined = 0,
    start = 1,
    complete = 2,
    fail = 3
};

UENUM()
enum class EGAErrorSeverity : uint8
{
    undefined = 0,
    debug = 1,
    info = 2,
    warning = 3,
    error = 4,
    critical = 5
};

UENUM()
enum class EGAGender : uint8
{
    undefined = 0,
    male = 1,
    female = 2
};


UCLASS()
class GAMEANALYTICS_API UGameAnalytics : public UObject
{
    GENERATED_UCLASS_BODY()

public:

    static void configureAvailableCustomDimensions01(const std::vector<std::string>& list);
    static void configureAvailableCustomDimensions02(const std::vector<std::string>& list);
    static void configureAvailableCustomDimensions03(const std::vector<std::string>& list);

    static void configureAvailableResourceCurrencies(const std::vector<std::string>& list);
    static void configureAvailableResourceItemTypes(const std::vector<std::string>& list);

    static void configureBuild(const char *build);
    static void configureUserId(const char *userId);
    static void configureSdkGameEngineVersion(const char *gameEngineSdkVersion);
    static void configureGameEngineVersion(const char *gameEngineVersion);
    static void configureWritablePath(const char *writablePath);
    static void initialize(const char *gameKey, const char *gameSecret);

#if PLATFORM_IOS
    static void addBusinessEvent(const char *currency, int amount, const char *itemType, const char *itemId, const char *cartType, const char *receipt/*, const char *fields*/);
    static void addBusinessEventAndAutoFetchReceipt(const char *currency, int amount, const char *itemType, const char *itemId, const char *cartType/*, const char *fields*/);
#elif PLATFORM_ANDROID
    static void addBusinessEvent(const char *currency, int amount, const char *itemType, const char *itemId, const char *cartType, const char *receipt, const char *signature/*, const char *fields*/);
#endif

    static void addBusinessEvent(const char *currency, int amount, const char *itemType, const char *itemId, const char *cartType/*, const char *fields*/);
    static void addResourceEvent(EGAResourceFlowType flowType, const char *currency, float amount, const char *itemType, const char *itemId/*, const char *fields*/);
    static void addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01/*, const char *fields*/);
    static void addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, int score/*, const char *fields*/);
    static void addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, const char *progression02/*, const char *fields*/);
    static void addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, const char *progression02, int score/*, const char *fields*/);
    static void addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, const char *progression02, const char *progression03/*, const char *fields*/);
    static void addProgressionEvent(EGAProgressionStatus progressionStatus, const char *progression01, const char *progression02, const char *progression03, int score/*, const char *fields*/);
    static void addDesignEvent(const char *eventId/*, const char *fields*/);
    static void addDesignEvent(const char *eventId, float value/*, const char *fields*/);
    static void addErrorEvent(EGAErrorSeverity severity, const char *message/*, const char *fields*/);

    static void setEnabledInfoLog(bool flag);
    static void setEnabledVerboseLog(bool flag);
    static void setEnabledManualSessionHandling(bool flag);
    static void setCustomDimension01(const char *customDimension);
    static void setCustomDimension02(const char *customDimension);
    static void setCustomDimension03(const char *customDimension);
    static void setFacebookId(const char *facebookId);
    static void setGender(EGAGender gender);
    static void setBirthYear(int birthYear);

    static void startSession();
    static void endSession();

    static const char* getCommandCenterValueAsString(const char *key);
    static const char* getCommandCenterValueAsString(const char *key, const char *defaultValue);
    static bool isCommandCenterReady();
    static const char* getConfigurationsContentAsString();

    // Own methods
    static void logGAStateInfo(const TCHAR* Context);

    static void setThreadAndEventTimers(double ThreadWaitSeconds, double ThreadProcessEventsSeconds);
    static void waitUntilJobsAreDone(); // obviously blocking


    // Bluprint functions

    // ONLY FOR IOS
    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddBusinessEventIOS(const FString& Currency, int Amount, const FString& ItemType, const FString& ItemId, const FString& CartType, const FString& Receipt/*, const char *fields*/);

    // ONLY FOR IOS
    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddBusinessEventAndAutoFetchReceipt(const FString& Currency, int Amount, const FString& ItemType, const FString& ItemId, const FString& CartType/*, const char *fields*/);

    // ONLY FOR ANDROID
    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddBusinessEventAndroid(const FString& Currency, int Amount, const FString& ItemType, const FString& ItemId, const FString& CartType, const FString& Receipt, const FString& Signature/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddBusinessEvent(const FString& Currency, int Amount, const FString& ItemType, const FString& ItemId, const FString& CartType/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddResourceEvent(EGAResourceFlowType FlowType, const FString& Currency, float Amount, const FString& ItemType, const FString& ItemId/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddProgressionEventWithOne(EGAProgressionStatus ProgressionStatus, const FString& Progression01/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddProgressionEventWithOneAndScore(EGAProgressionStatus ProgressionStatus, const FString& Progression01, int Score/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddProgressionEventWithOneAndTwo(EGAProgressionStatus ProgressionStatus, const FString& Progression01, const FString& Progression02/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddProgressionEventWithOneTwoAndScore(EGAProgressionStatus ProgressionStatus, const FString& Progression01, const FString& Progression02, int Score/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddProgressionEventWithOneTwoAndThree(EGAProgressionStatus ProgressionStatus, const FString& Progression01, const FString& Progression02, const FString& Progression03/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddProgressionEvenWithOneTwoThreeAndScore(EGAProgressionStatus ProgressionStatus, const FString& Progression01, const FString& Progression02, const FString& Progression03, int Score/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddDesignEvent(const FString& EventId/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddDesignEventWithValue(const FString& EventId, float Value/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void AddErrorEvent(EGAErrorSeverity Severity, const FString& Message/*, const char *fields*/);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void SetCustomDimension01(const FString& CustomDimension);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void SetCustomDimension02(const FString& CustomDimension);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void SetCustomDimension03(const FString& CustomDimension);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void SetFacebookId(const FString& FacebookId);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void SetGender(EGAGender Gender);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static void SetBirthYear(int BirthYear);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static FString GetCommandCenterValueAsString(const FString& Key);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static FString GetCommandCenterValueAsStringWithDefaultValue(const FString& Key, const FString& DefaultValue);

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static bool IsCommandCenterReady();

    UFUNCTION(BlueprintCallable, Category = "GameAnalytics")
    static FString GetConfigurationsContentAsString();
};
