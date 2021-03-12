// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

#include "SplineLogic/SoPlayerSpline.h"
#include "Levels/SoLevelTypes.h"

class FSoSplineConsoleCommand
{
public:

	FSoSplineConsoleCommand(TWeakObjectPtr<ASoPlayerSpline> InSpline) { Spline = InSpline; }

	void PerformTeleportToSpline(const TArray<FString>& Args);

	bool IsValid() const { return Spline.IsValid(); }
	const TWeakObjectPtr<ASoPlayerSpline> GetSpline() const { return Spline; }

private:
	TWeakObjectPtr<ASoPlayerSpline> Spline;
};


enum class ESoCharVariableCommand : uint8
{
	ECVC_SetInt = 0,
	ECVC_SetFloat,
	ECVC_SetBool,
	ECVC_SetName,

	ECVC_NumOf
};

class FSoConsoleCommandSetCharVariable
{
public:

	// used when the variable name is part of the console command (for auto complete)
	FSoConsoleCommandSetCharVariable(TWeakObjectPtr<AActor> Reference,
									 ESoCharVariableCommand Cmd,
									 FName Variable);

	// used when the variable name is console param too
	FSoConsoleCommandSetCharVariable(TWeakObjectPtr<AActor> Reference,
									 ESoCharVariableCommand Cmd);

	void ChangeCharVariable(const TArray<FString>& Args);

	const FString& GetCommandName() const { return CommandName; }
	const FString& GetCommandHelperText() const { return  HelperTexts[static_cast<int32>(Command)]; }

private:
	static const FString CommandNames[static_cast<int32>(ESoCharVariableCommand::ECVC_NumOf)];
	static const FString HelperTexts[static_cast<int32>(ESoCharVariableCommand::ECVC_NumOf)];

	TWeakObjectPtr<AActor> ReferenceActor;
	ESoCharVariableCommand Command = ESoCharVariableCommand::ECVC_SetInt;

	FName VariableName;
	FString CommandName;
	bool bVariableNameIsStringParam = false;
};

enum class ESoConsoleCommandAchievementType : uint8
{
	Unlock = 0,
	Reset,
	Show,
	SetProgress,

	NumOf
};


class FSoConsoleCommandAchievement
{
public:
	FSoConsoleCommandAchievement(TWeakObjectPtr<AActor> Reference, FName InAchievementName, ESoConsoleCommandAchievementType InCommand);

	const FString& GetCommandName() const { return CommandName; }
	const FString& GetCommandHelperText() const { return  HelperTexts[static_cast<int32>(Command)]; }

	void HandleCommand(const TArray<FString>& Args);

private:
	static const FString CommandNames[static_cast<int32>(ESoConsoleCommandAchievementType::NumOf)];
	static const FString HelperTexts[static_cast<int32>(ESoConsoleCommandAchievementType::NumOf)];

	TWeakObjectPtr<AActor> ReferenceActor;
	ESoConsoleCommandAchievementType Command = ESoConsoleCommandAchievementType::Unlock;
	FString CommandName;

	FName AchievementName;
};


class FSoConsoleCommands
{
public:
	// Register all the console commands
	void RegisterAllCommands(AActor* OwnerActor);

	// Unregister all the console commands and clears the cache.
	void UnRegisterAllCommands();

	bool AllowCheats() const;
	void ReloadCommands()
	{
		if (ReferenceActor.IsValid())
			RegisterAllCommands(ReferenceActor.Get());
	}

	static bool IsSpeedRunCompetitionMode();

private:
	void PrintBoolToConsole(bool bValue);

	void RegisterPrintCommands();
	void RegisterInputCommands();
	void RegisterSettingCommands();
	void RegisterAchievementCommands();
	void RegisterSteamCommands();
	void RegisterTeleportCommands();
	void RegisterDialogueCommands();
	void RegisterCharacterAndItemCommands();

	// Handle console commands
	void AddAllToCharacter(const TArray<FString>& Args);
	void AddAllKeyToCharacter(const TArray<FString>& Args);
	void AddAllQuestItemToCharacter(const TArray<FString>& Args);
	void AddAllUsableItemToCharacter(const TArray<FString>& Args);
	void AddAllWeaponToCharacter(const TArray<FString>& Args);
	void AddAllShardToCharacter(const TArray<FString>& Args);
	void AddAllJewelryToCharacter(const TArray<FString>& Args);
	void AddAllRuneStonesToCharacter(const TArray<FString>& Args);
	bool AddItems(UClass* ClassFilter);

	void InputForceDeviceType(const TArray<FString>& Args, const ESoInputDeviceType DeviceType);

	void TeleportToCheckpoint(const TArray<FString>& Args, const FName CheckPointName);
	void TeleportToEpisode(const TArray<FString>& Args, FName EpisodeName);
	void TeleportToChapter(const TArray<FString>& Args, FName ChapterName);
	void TeleportToMainMenu(const TArray<FString>& Args);

	void SetHp(const TArray<FString>& Args);
	void SetSpellCapacity(const TArray<FString>& Args);
	void EquipAllSpells(const TArray<FString>& Args);

	void Rest(const TArray<FString>& Args);
	void Resurrect(const TArray<FString>& Args);
	void KillAllEnemy(const TArray<FString>& Args);
	void StartOutro(const TArray<FString>& Args);

	void ToggleUI(const TArray<FString>& Args);
	void ToggleDisplayHitLines(const TArray<FString>& Args);
	void ToggleQuickSaveLoad(const TArray<FString>& Args);
	void ToggleDisplayEnemyCoverTraces(const TArray<FString>& Args);
	void ToggleDialogueDebugSkip(const TArray<FString>& Args);
	void ToggleAmbientLog(const TArray<FString>& Args);

	void ToggleLateWallJump(const TArray<FString>& Args);
	void SetLateWallJumpInterval(const TArray<FString>& Args);

	void ReloadAIConfig(const TArray<FString>& Args);

	void SetGameSpeedMultiplier(const TArray<FString>& Args);
	void SetListenerLerpValue(const TArray<FString>& Args);

	void TestVisibilityQuery(const TArray<FString>& Args);

	void SetDifficulty(const TArray<FString>& Args);

	void AddBonusHealth(const TArray<FString>& Args);
	void ClearBonusHealth(const TArray<FString>& Args);

	void ToggleLillian(const TArray<FString>& Args);

	void ToggleDmgCheat(const TArray<FString>& Args);

	void ToggleGodMode(const TArray<FString>& Args);
	void ToggleFlyMode(const TArray<FString>& Args);

private:
	// Used to get the UWorld.
	TWeakObjectPtr<AActor> ReferenceActor;

	TArray<IConsoleObject*> ConsoleObjects;
	TArray<FSoSplineConsoleCommand> SplineCommands;

	TArray<FSoConsoleCommandSetCharVariable> CharVariableCommands;

	TArray<FSoConsoleCommandAchievement> AchievementCommands;
};
