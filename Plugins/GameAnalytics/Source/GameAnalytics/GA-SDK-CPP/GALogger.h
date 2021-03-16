//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "Foundation/GASingleton.h"
#include <string>
#include <memory>


namespace gameanalytics
{
    namespace logging
    {
        enum EGALoggerMessageType
        {
            Error = 0,
            Warning = 1,
            Info = 2,
            Debug = 3
        };

        class GALogger : public GASingleton<GALogger>
        {
         public:
            GALogger();

            // set debug enabled (client)
            static void setInfoLog(bool enabled);
            static void setVerboseInfoLog(bool enabled);

            // Debug (w/e always shows, d only shows during SDK development, i shows when client has set debugEnabled to YES)
            static void  w(const std::string& format);//const char* format, ...);
            static void  e(const std::string& format);//const char* format, ...);
            static void  d(const std::string& format);//const char* format, ...);
            static void  i(const std::string& format);//const char* format, ...);
            static void ii(const std::string& format);//const char* format, ...);

            void sendNotificationMessage(const std::string& message, EGALoggerMessageType type);

            static void customInitializeLog();
            static void addCustomLogStream(std::ostream& os);
        };
    }
}
