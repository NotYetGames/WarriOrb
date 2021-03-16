// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

/**
* The public interface to this module
*/
class FGameAnalyticsEditor : public IModuleInterface
{
/*public:

    static inline FGameAnalyticsEditor& Get()
    {
        return FModuleManager::LoadModuleChecked< FGameAnalyticsEditor >("GameAnalyticsEditor");
    }*/

public:
    /** IModuleInterface implementation */
    void StartupModule();
    void ShutdownModule();
};
