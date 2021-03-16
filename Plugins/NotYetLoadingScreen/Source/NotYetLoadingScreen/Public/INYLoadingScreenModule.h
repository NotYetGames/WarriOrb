// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "INYLoadingScreenInstance.h"
#include "Templates/SharedPointer.h"

class AActor;

/**
 * The public interface to this module
 */
class NOTYETLOADINGSCREEN_API INYLoadingScreenModule : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static INYLoadingScreenModule& Get()
	{
		return FModuleManager::LoadModuleChecked<INYLoadingScreenModule>("NotYetLoadingScreen");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("NotYetLoadingScreen");
	}

	virtual void RequestAsyncLoad(const FSoftObjectPath& Reference) = 0;

public:
	// Manually start/stop
	virtual TSharedPtr<INYLoadingScreenInstance> GetInstance() const = 0;

	virtual void RegisterConsoleCommands(AActor* InReferenceActor = nullptr) = 0;
	virtual void UnregisterConsoleCommands() = 0;
};
