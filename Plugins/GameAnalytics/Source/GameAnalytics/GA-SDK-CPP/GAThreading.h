//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once


#include <functional>
#include <vector>
#include <chrono>
#include <memory>

namespace gameanalytics
{
    namespace threading
    {
        class GAThreading
        {
         public:
            typedef std::function<void()> Block;
            static void performTaskOnGAThread(const Block& taskBlock);

            // timers
            static void endThread();

            // Does this have any jobs left to process?
            static bool HasJobs();

            static bool IsThreadRunning();

            static double GetThreadWaitSeconds();
            static void SetThreadWaitSeconds(double NewInterval);
        };
    }
}
