// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoLevelTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSoLevelManager, All, All);

struct FSoSplinePoint;
class UObject;
class UWorld;

/**
 *   Singleton class handling the level streaming based on the player character's spline location
 *
 *  There are two kind of levels:
 *
 *		Claimed Level: one or more level are claimed every frame based on the player's spline location
 *					   the claimed level is loaded
 *					   if it isn't claimed anymore it is unloaded after a few sec (HideTime)
 *
 *		Dependent Level: Claimed levels can have dependent level
 *						 A dependent level is loaded when any claimed level references it
 *						 Can be used for areas between two levels - when certain objects are visible from
 *						 two claimed levels a part of them can be moved to a dependent level which is always loaded
 *						 if any of the two claimed level is loaded
 *						 DEPENDENT LEVELS CAN NOT HAVE DEPENDENT LEVELS ATM
 */
class FSoLevelManager
{
public:
	static FSoLevelManager& Get();
	FSoLevelManager(const FSoLevelManager& Other) = delete;
	void operator=(const FSoLevelManager& Other) = delete;

	// void RegisterDependentLevel(FName DependentLevelName, const TArray<FName>& ClaimedLevels);

	/** has to be called each Tick */
	void Update(const FSoSplinePoint& PlayerLocation, float DeltaSeconds);

	/**
	 * Show all the levels that are claimed by the spline point.
	 * return value false: there is at least one not yet loaded claimed level
	 */
	bool ClaimSplineLocation(const FSoSplinePoint& ClaimedLocation, bool& bOutAllLoaded);

	/**
	 * Clears the active levels list, should be called in begin play / end play
	 * otherwise an old entry can stop the level being loaded (play in editor)
	 */
	void Empty();

	/** can be used to force load everything (for editor simulation) */
	void ClaimAndLoadAllLevels(UWorld* WorldPtr);

	/**
	 * Move/teleport character to the new chapter map
	 * NOTE: SHOULD NOT CALL THIS METHOD DIRECTLY, call the GameInstance one
	 * NOTE: by moving to the new level everything in the actors is reset
	 * And a new World is set. Only USoGameInstance has the variables still set
	 */
	bool TeleportToChapter(const UObject* ContextObject, FName ChapterName);

	/**
	 * Move/teleport character to the a new episode
	 * NOTE: SHOULD NOT CALL THIS METHOD DIRECTLY, call the GameInstance one
	 */
	bool TeleportToEpisode(const UObject* ContextObject, FName EpisodeName);

	// Teleports to t he main menu level
	bool TeleportToMainMenuLevel(const UObject* ContextObject);

	/** Hides the active LevelName  */
	void HideActiveLevel(const UObject* ContextObject, FName LevelName);

private:
	// Controls DependentLevelCounters
	void OnClaimedLevelUnloaded(const UObject* ContextObj, FName Level);

	static bool OpenLevel(const UObject* ContextObj, FName LevelName);
	bool ShowLevel(const UObject* ContextObj, FName LevelName);
	static bool HideLevel(const UObject* ContextObj, FName LevelName);
	void UnloadLevelIfNotVisible(const UObject* ContextObj, FName LevelName);


private:
	static constexpr float HideTime = 5.0f;

	FSoLevelManager() {};

	struct FSoLevelEntry
	{
		// Level Name (actually it is the PackageName)
		FName Name = NAME_None;

		/** time since it was last claimed */
		float Time = 0.f;

		FSoLevelEntry(FName InName, float InTime) : Name(InName), Time(InTime) {};
	};

	// Keep track of the active levels
	TArray<FSoLevelEntry> ActiveLevels;

	// NOTE: Not used, seems to be the same as USoGameSingleton::DependentLevels
	/**
	 *  Counter for dependent levels
	 *  Claimed level referencing the dependent level Loaded: counter increased
	 *												  Unloaded: counter decreased
	 *  Counter becomes 0: level unloaded
	 *  Counter becomes greater one from 0: level loaded
	 */
	TMap<FName, int32> DependentLevelCounters;

	// Levels marked to be hidden
	TSet<FName> LoggedToHideLevels;
};

