//
// GA-SDK-CPP
// Copyright 2015 CppWrapper. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

#if PLATFORM_LINUX
using STRING = const char*;
using RETURN_STRING = const char*;
#else
using STRING = const std::string&;
using RETURN_STRING = std::string;
#endif

namespace gameanalytics
{

    /*!
     @enum
     @discussion
     This enum is used to specify flow in resource events
     @constant GAResourceFlowTypeSource
     Used when adding to a resource currency
     @constant GAResourceFlowTypeSink
     Used when subtracting from a resource currency
     */
    enum EGAResourceFlowType
    {
        Source = 1,
        Sink = 2
    };

    /*!
     @enum
     @discussion
     his enum is used to specify status for progression event
     @constant GAProgressionStatusStart
     User started progression
     @constant GAProgressionStatusComplete
     User succesfully ended a progression
     @constant GAProgressionStatusFail
     User failed a progression
     */
    enum EGAProgressionStatus
    {
        Start = 1,
        Complete = 2,
        Fail = 3
    };

    /*!
     @enum
     @discussion
     his enum is used to specify severity of an error event
     @constant GAErrorSeverityDebug
     @constant GAErrorSeverityInfo
     @constant GAErrorSeverityWarning
     @constant GAErrorSeverityError
     @constant GAErrorSeverityCritical
     */
    enum EGAErrorSeverity
    {
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Critical = 5
    };

    enum EGAGender
    {
        Male = 1,
        Female = 2
    };

    class GameAnalytics
    {
     public:
        // configure calls should be used before initialize
        static void configureAvailableCustomDimensions01(const std::vector<std::string>& customDimensions);
        static void configureAvailableCustomDimensions01(STRING customDimensions);
        static void configureAvailableCustomDimensions02(const std::vector<std::string>& customDimensions);
        static void configureAvailableCustomDimensions02(STRING customDimensions);
        static void configureAvailableCustomDimensions03(const std::vector<std::string>& customDimensions);
        static void configureAvailableCustomDimensions03(STRING customDimensions);
        static void configureAvailableResourceCurrencies(const std::vector<std::string>& resourceCurrencies);
        static void configureAvailableResourceCurrencies(STRING resourceCurrencies);
        static void configureAvailableResourceItemTypes(const std::vector<std::string>& resourceItemTypes);
        static void configureAvailableResourceItemTypes(STRING resourceCurrencies);
        static void configureBuild(STRING build);
        static void configureWritablePath(STRING writablePath);
        static void configureDeviceModel(STRING deviceModel);
        static void configureDeviceManufacturer(STRING deviceManufacturer);

        // the version of SDK code used in an engine. Used for sdk_version field.
        // !! if set then it will override the SdkWrapperVersion.
        // example "unity 4.6.9"
        static void configureSdkGameEngineVersion(STRING sdkGameEngineVersion);
        // the version of the game engine (if used and version is available)
        static void configureGameEngineVersion(STRING engineVersion);

        static void configureUserId(STRING uId);

        // initialize - starting SDK (need configuration before starting)
        static void initialize(STRING gameKey, STRING gameSecret);

        // add events
        static void addBusinessEvent(STRING currency, int amount, STRING itemType, STRING itemId, STRING cartType);

        static void addResourceEvent(EGAResourceFlowType flowType, STRING currency, float amount, STRING itemType, STRING itemId);

        static void addProgressionEvent(EGAProgressionStatus progressionStatus, STRING progression01, STRING progression02, STRING progression03);

        static void addProgressionEvent(EGAProgressionStatus progressionStatus, STRING progression01, STRING progression02, STRING progression03, int score);

        static void addDesignEvent(STRING eventId);
        static void addDesignEvent(STRING eventId, double value);
        static void addErrorEvent(EGAErrorSeverity severity, STRING message);

        // set calls can be changed at any time (pre- and post-initialize)
        // some calls only work after a configure is called (setCustomDimension)
        static void setEnabledInfoLog(bool flag);
        static void setEnabledVerboseLog(bool flag);
        static void setEnabledManualSessionHandling(bool flag);
        static void setCustomDimension01(STRING dimension01);
        static void setCustomDimension02(STRING dimension02);
        static void setCustomDimension03(STRING dimension03);
        static void setFacebookId(STRING facebookId);
        static void setGender(EGAGender gender);
        static void setBirthYear(int birthYear);

        static void startSession();
        static void endSession();

        static RETURN_STRING getCommandCenterValueAsString(STRING key);
        static RETURN_STRING getCommandCenterValueAsString(STRING key, STRING defaultValue);
        static bool isCommandCenterReady();
        static RETURN_STRING getConfigurationsContentAsString();

        // game state changes
        // will affect how session is started / ended
        static void onResume();
        static void onSuspend();
        static void onQuit();

     private:
        static bool isSdkReady(bool needsInitialized);
        static bool isSdkReady(bool needsInitialized, bool warn);
        static bool isSdkReady(bool needsInitialized, bool warn, std::string message);
    };
} // namespace gameanalytics
