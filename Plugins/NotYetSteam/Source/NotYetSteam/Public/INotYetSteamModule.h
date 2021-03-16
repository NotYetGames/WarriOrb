// Copyright (c) Daniel Butum. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "INYSteamSubsystem.h"

const FName NOT_YET_STEAM_PLUGIN_NAME(TEXT("NotYetSteam"));


/**
 * The public interface to this module
 */
class NOTYETSTEAM_API INotYetSteamModule : public IModuleInterface
{
public:
	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static INotYetSteamModule& Get()
	{
		return FModuleManager::LoadModuleChecked<INotYetSteamModule>(NOT_YET_STEAM_PLUGIN_NAME);
	}

	/**
	 * Loads the module into memory if it can
	 */
	static bool LoadModule()
	{
		if (IsAvailable())
		{
			return true;
		}

		EModuleLoadResult FailureReason;
		IModuleInterface* Result = FModuleManager::Get().LoadModuleWithFailureReason(NOT_YET_STEAM_PLUGIN_NAME, FailureReason);
		if (Result == nullptr || !IsAvailable())
		{
			return false;
		}

		return true;
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded(NOT_YET_STEAM_PLUGIN_NAME);
	}

	static const FString& GetSteamVersion()
	{
#ifndef STEAM_SDK_VER
#error "STEAM_SDK_VER from SteamWorks plugin not defined :("
#endif // !NOT_YET_STEAM_SDK_VERSION

		static const FString Version(STEAM_SDK_VER);
		return Version;
	}

public:
	virtual ~INotYetSteamModule() {}

	virtual INYSteamSubsystemPtr GetSteamSubsystem() const = 0;
	virtual bool AreSteamDllsLoaded() const = 0;
	virtual void Initialize() = 0;
};
