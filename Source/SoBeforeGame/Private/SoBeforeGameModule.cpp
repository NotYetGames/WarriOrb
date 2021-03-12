// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoBeforeGameModule.h"

#include "Modules/ModuleManager.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/Paths.h"

#include "SoLocalization.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoBeforeGameModule, All, All)

// NOTE: the name (third param) seems to be deprecated
IMPLEMENT_GAME_MODULE(FSoBeforeGameModule, SoBeforeGame);

// https://stackoverflow.com/questions/27490858/how-can-you-compare-two-character-strings-statically-at-compile-time/27491087#27491087
constexpr bool strings_equal(char const * a, char const * b) {
	return *a == *b && (*a == '\0' || strings_equal(a + 1, b + 1));
}
#define COMPILE_CHECK_STRING_TABLE_ID(MacroValue, TruthValue) \
	static_assert(strings_equal(MacroValue, TruthValue),  "Invalid String Table ID. Check the line number")


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoBeforeGameModule::StartupModule()
{
	UE_LOG(LogSoBeforeGameModule, Log, TEXT("StartupModule"));

	FModuleManager::Get().OnModulesChanged().AddLambda([this](FName ModuleName, EModuleChangeReason Reason)
	{
		if (ModuleName == TEXT("AssetRegistry") && Reason == EModuleChangeReason::ModuleLoaded)
		{
			InitStringTables();
		}
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoBeforeGameModule::ShutdownModule()
{
	UE_LOG(LogSoBeforeGameModule, Log, TEXT("ShutdownModule"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoBeforeGameModule::InitStringTables()
{
	// Problems depending on loading phase:
	// - PostConfigInit - live string  reload does not work in editor mode
	// - PreEarlyLoadingScreen - We get warnings in packaged game, but everything seems to work
	//
	// --- DOES NOT WORK ---
	// So because we are in PostConfigInit and the directory watcher does not fire in the constructor of FStringTableRegistry
	// because GIsEditor is still false at this point.
	//
	// So we temporarily set GIsEditor to true if compiled with WITH_EDITOR
	//
	// Because: InitEngineTextLocalization needs to fire first
	// --- DOES NOT WORK ---
	//
	//
	// What works if we init our tables right before ProcessNewlyLoadedUObjects() from LaunchEngineLoop.cpp
	// Is what causes errors because of the default objects.
	// The module right before is AssetRegistry
	//

	// Most likely will fail
// #if WITH_EDITOR
// 	if (!GIsEditor)
// 	{
// 		UE_LOG(LogSoBeforeGameModule, Error, TEXT("InitStringTables: GIsEditor = false but WITH_EDITOR = true"));
// 	}
// #endif

	// Init the constructor and directory watcher
	static FStringTableRegistry& TableRegistry = FStringTableRegistry::Get();

	// Restore
// #if WITH_EDITOR
// 	GIsEditor = bIsEditorPreviousValue;
// #endif

	// Register String tables that are not already registered
	UE_LOG(LogSoBeforeGameModule, Log, TEXT("InitStringTables"));
	if (!TableRegistry.FindStringTable(STRING_TABLE_UI_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_UI, "UI");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_UI, "UI", "Localization/StringTables/UI.csv");
	}

	if (!TableRegistry.FindStringTable(STRING_TABLE_INTERACTION_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_INTERACTION, "Interaction");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_INTERACTION, "Interaction", "Localization/StringTables/Interaction.csv");
	}

	if (!TableRegistry.FindStringTable(STRING_TABLE_DIALOGUE_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_DIALOGUE, "Dialogue");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_DIALOGUE, "Dialogue", "Localization/StringTables/Dialogue.csv");
	}

	if (!TableRegistry.FindStringTable(STRING_TABLE_ITEMS_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_ITEMS, "Items");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_ITEMS, "Items", "Localization/StringTables/Items.csv");
	}

	if (!TableRegistry.FindStringTable(STRING_TABLE_SPELLS_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_SPELLS, "Spells");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_SPELLS, "Spells", "Localization/StringTables/Spells.csv");
	}

	if (!TableRegistry.FindStringTable(STRING_TABLE_AREAS_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_AREAS, "Areas");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_AREAS, "Areas", "Localization/StringTables/Areas.csv");
	}

	if (!TableRegistry.FindStringTable(STRING_TABLE_ENEMIES_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_ENEMIES, "Enemies");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_ENEMIES, "Enemies", "Localization/StringTables/Enemies.csv");
	}

	if (!TableRegistry.FindStringTable(STRING_TABLE_EPISODES_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_EPISODES, "Episodes");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_EPISODES, "Episodes", "Localization/StringTables/Episodes.csv");
	}

	if (!TableRegistry.FindStringTable(STRING_TABLE_FLOATING_ENEMY_AND_NPC_LINES_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_FLOATING_ENEMY_AND_NPC_LINES, "FloatingEnemyAndNPCLines");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_FLOATING_ENEMY_AND_NPC_LINES, "FloatingEnemyAndNPCLines", "Localization/StringTables/FloatingEnemyAndNPCLines.csv");
	}

	if (!TableRegistry.FindStringTable(STRING_TABLE_CREDITS_TEXT).IsValid())
	{
		COMPILE_CHECK_STRING_TABLE_ID(STRING_TABLE_CREDITS, "Credits");
		LOCTABLE_FROMFILE_GAME(STRING_TABLE_CREDITS, "Credits", "Localization/StringTables/Credits.csv");
	}
}

#undef COMPILE_CHECK_STRING_TABLE_ID
