// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

const FName SO_BEFORE_GAME_MODULE_NAME(TEXT("SoBeforeGame"));

class SOBEFOREGAME_API FSoBeforeGameModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;
	bool IsGameModule() const override
	{
		return true;
	}

	static FSoBeforeGameModule& Get() { return FModuleManager::LoadModuleChecked<FSoBeforeGameModule>(SO_BEFORE_GAME_MODULE_NAME); }
	static bool IsAvailable() { return FModuleManager::Get().IsModuleLoaded(SO_BEFORE_GAME_MODULE_NAME); }

	void InitStringTables();

protected:
	bool bInitStringTables = false;
};
