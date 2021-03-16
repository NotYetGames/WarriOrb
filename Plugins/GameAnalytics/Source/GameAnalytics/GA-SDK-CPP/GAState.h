//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include <vector>
#include <string>
#include <map>

#include "Foundation/GASingleton.h"
#include "GameAnalytics.h"

namespace gameanalytics
{
    namespace state
    {
        // TODO(nikolaj): needed? remove.. if not
        // typedef void(*Callback) ();

        class GAState : public GASingleton<GAState>
        {
         public:
            static void setUserId(const std::string& id);
            static const std::string getIdentifier();
            static bool isInitialized();
            static int getSessionNum();
            static int getTransactionNum();
            static const std::string getSessionId();
            static const std::string getCurrentCustomDimension01();
            static const std::string getCurrentCustomDimension02();
            static const std::string getCurrentCustomDimension03();
            static const std::string getGameKey();
            static const std::string getGameSecret();
            static void setAvailableCustomDimensions01(const std::vector<std::string>& dimensions);
            static void setAvailableCustomDimensions02(const std::vector<std::string>& dimensions);
            static void setAvailableCustomDimensions03(const std::vector<std::string>& dimensions);
            static void setAvailableResourceCurrencies(const std::vector<std::string>& availableResourceCurrencies);
            static void setAvailableResourceItemTypes(const std::vector<std::string>& availableResourceItemTypes);
            static void setBuild(const std::string& build);
            static bool isEnabled();
            static void setCustomDimension01(const std::string& dimension);
            static void setCustomDimension02(const std::string& dimension);
            static void setCustomDimension03(const std::string& dimension);
            static void setFacebookId(const std::string& facebookId);
            static void incrementSessionNum();
            static void incrementTransactionNum();
            static void incrementProgressionTries(const std::string& progression);
            static int getProgressionTries(const std::string& progression);
            static void clearProgressionTries(const std::string& progression);
            static bool hasAvailableCustomDimensions01(const std::string& dimension1);
            static bool hasAvailableCustomDimensions02(const std::string& dimension2);
            static bool hasAvailableCustomDimensions03(const std::string& dimension3);
            static bool hasAvailableResourceCurrency(const std::string& currency);
            static bool hasAvailableResourceItemType(const std::string& itemType);
            static void endSessionAndStopQueue(bool endThread);
            static void resumeSessionAndStartQueue();
            static void internalInitialize();
            static void setManualSessionHandling(bool flag);
            static bool useManualSessionHandling();
            static bool sessionIsStarted();
        };
    }
}
