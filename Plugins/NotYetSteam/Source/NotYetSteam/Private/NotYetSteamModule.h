// Copyright 2019 Daniel Butum

#pragma once

#include "INotYetSteamModule.h"
#include "Containers/Ticker.h"

class FNotYetSteamModule : public INotYetSteamModule
{
	typedef FNotYetSteamModule Self;
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void Initialize() override;
	bool AreSteamDllsLoaded() const override;
	INYSteamSubsystemPtr GetSteamSubsystem() const override { return SteamSubsystem; }

private:
    void LoadSteamModules();
    FString GetDllPath();

private:
	// Make this false and call Initialize yourself
	// TODO remove this name and Initialize as this is confusing
	static constexpr bool bInitializeAtStartup = true;

	/** Handle to the client steam dll */
	void* SteamClientLibraryHandle = nullptr;

	// Our own Steam subsystem
	INYSteamSubsystemPtr SteamSubsystem = nullptr;
};
