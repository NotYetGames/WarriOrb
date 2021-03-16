// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystem.h"

#include "Interfaces/NYSteamPresence.h"
#include "Interfaces/NYSteamStatsAndAchievements.h"
#include "Interfaces/NYSteamExternalUI.h"



/**
 *	OnlineSubsystem - Copied from IOnlineSubsystem
 */
class NOTYETSTEAM_API INYSteamSubsystem
{
public:
	INYSteamSubsystem() {}
	virtual ~INYSteamSubsystem() {}

	/**
	 * Determine if the subsystem for a given interface is enabled by config and command line
	 * @return true if the subsystem is enabled by config
	 */
	static bool IsEnabledByConfigOrCLI();

	// Gets the steam subsystem from unreal
	static IOnlineSubsystem* GetUnrealSteamSubsystem();

	virtual bool Initialize() = 0;
	virtual bool Shutdown() = 0;
	virtual bool IsEnabled() const = 0;
	virtual FString GetAppIDAsString() const = 0;
	virtual uint32 GetAppIDAsUInt32() const = 0;
	virtual FString GetCurrentGameLanguage() const = 0;

	virtual FString GetDllAPI64Sha1Sum() const = 0;

	// Checks if your executable was launched through Steam and relaunches it through Steam if it wasn't.
	// https://partner.steamgames.com/doc/api/steam_api#SteamAPI_RestartAppIfNecessary
	virtual bool RestartAppIfNecessary(int32 AppId) const = 0;

	/**
	 * Get the interface for accessing online achievements
	 * @return Interface pointer for the appropriate online achievements service
	 */
	virtual INYSteamStatsAndAchievementsPtr GetStatsAndAchievements() const = 0;

	/**
	 * Get the interface for managing rich presence information
	 * @return Interface pointer for the appropriate online user service
	 */
	virtual INYSteamPresencePtr GetPresence() const = 0;

	/**
	 * Get the interface for accessing the external UIs of a service
	 * @return Interface pointer for the appropriate external UI service
	 */
	virtual INYSteamExternalUIPtr GetExternalUI() const = 0;

protected:
	static const FName SubsystemName;
};

typedef TSharedPtr<INYSteamSubsystem, ESPMode::ThreadSafe> INYSteamSubsystemPtr;
