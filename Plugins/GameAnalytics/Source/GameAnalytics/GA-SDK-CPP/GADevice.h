//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include <string>

namespace gameanalytics
{
    namespace device
    {
        class GADevice
        {
        public:
            static void setSdkGameEngineVersion(const std::string& sdkGameEngineVersion);
            static const std::string getGameEngineVersion();
            static void setGameEngineVersion(const std::string& gameEngineVersion);
            static void setConnectionType(const std::string& connectionType);
            static const std::string getConnectionType();
            static const std::string getRelevantSdkVersion();
            static const std::string getBuildPlatform();
            static const std::string getOSVersion();
            static void setDeviceModel(const std::string& deviceModel);
            static const std::string getDeviceModel();
            static void setDeviceManufacturer(const std::string& deviceManufacturer);
            static const std::string getDeviceManufacturer();
            static void setWritablePath(const std::string& writablePath);
            static const std::string getWritablePath();
            static void UpdateConnectionType();
        };
    }
}
