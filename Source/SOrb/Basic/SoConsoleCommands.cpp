// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoConsoleCommands.h"
#include "Components/SplineComponent.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "RenderCore/Public/RendererInterface.h"
#include "EngineModule.h"
#include "Components/SkeletalMeshComponent.h"

#include "DlgManager.h"

#include "CharacterBase/SoMortal.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameInstance.h"
#include "Basic/SoGameMode.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Character/SoCharacter.h"
#include "Character/SoCharStates/SoADead.h"
#include "Character/SoCharStates/SoAStrike.h"
#include "Character/SoCharStates/SoADefault.h"
#include "Character/SoCharStates/SoATeleport.h"
#include "Character/SoCharStates/SoALillian.h"
#include "Character/SoCharStates/SoAInUI.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "CharacterBase/SoCombatComponent.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Items/ItemTemplates/SoQuestItemTemplate.h"
#include "Items/ItemTemplates/SoItemTemplateQuestBook.h"
#include "Items/ItemTemplates/SoItemTemplateJewelry.h"
#include "Items/ItemTemplates/SoUsableItemTemplate.h"
#include "Items/ItemTemplates/SoItemTemplateShard.h"
#include "Items/ItemTemplates/SoWeaponTemplate.h"
#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"
#include "Enemy/SoEnemy.h"
#include "SplineLogic/SoLocationRegistry.h"
#include "SplineLogic/SoPrecomputedVisibility.h"
#include "Levels/SoLevelHelper.h"
#include "Items/SoInventoryComponent.h"
#include "Items/SoItem.h"
#include "Settings/SoGameSettings.h"
#include "Levels/SoLevelManager.h"
#include "Online/Achievements/SoAchievementManager.h"
#include "Basic/SoAudioManager.h"
#include "Character/SoPlayerController.h"
#include "Basic/SoGameSingleton.h"
#include "SaveFiles/SoWorldState.h"

#if WARRIORB_WITH_STEAM
#include "NYSteamHelper.h"
#include "INotYetSteamModule.h"
#endif // WARRIORB_WITH_STEAM

#include "Online/SoOnlineHelper.h"
#include "Helpers/SoPlatformHelper.h"
#include "Items/SoItemHelper.h"
#include "Settings/Input/SoInputHelper.h"
#include "HAL/PlatformOutputDevices.h"
#include "Levels/SoEpisodeLevelTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoConsoleCommands, All, All)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplineConsoleCommand::PerformTeleportToSpline(const TArray<FString>& Args)
{
	if (!IsValid())
		return;

	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(Spline->GetWorld());
	if (Character == nullptr)
		return;

	float Distance = 0.0f;
	if (Args.Num() > 0)
		Distance = FMath::Clamp(FCString::Atof(*Args[0]), 0.0f, Spline->GetSplineComponent()->GetSplineLength());

	float ZValue = Spline->GetActorLocation().Z;
	if (Args.Num() > 1)
		ZValue += FMath::Clamp(FCString::Atof(*Args[1]), -100000.0f, 100000.0f);

	const FSoSplinePoint Point{ Spline.Get(), Distance };

	// Wait for all levels to load.
	// SoLevelManager::Get().ClaimSplineLocation(Point);
	// while (SoLevelManager::IsAnyLevelLoading(Spline->GetWorld()));

	Character->SoATeleport->SetupTeleport(Point, ZValue, false, false);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoConsoleCommandSetCharVariable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FString FSoConsoleCommandSetCharVariable::CommandNames[static_cast<int32>(ESoCharVariableCommand::ECVC_NumOf)] =
{
	"SO.SetInt",		// ECVC_SetInt,
	"SO.SetFloat",		// ECVC_SetFloat,
	"SO.SetBool",		// ECVC_SetBool,
	"SO.SetName",		// ECVC_SetName,
};

const FString FSoConsoleCommandSetCharVariable::HelperTexts[static_cast<int32>(ESoCharVariableCommand::ECVC_NumOf)] =
{
	"Sets an int variable (SoPlayerCharacterSheet)",
	"Sets a float variable (SoPlayerCharacterSheet)",
	"Sets a bool variable (SoPlayerCharacterSheet)",
	"Sets a name variable (SoPlayerCharacterSheet)",
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoConsoleCommandSetCharVariable::FSoConsoleCommandSetCharVariable(TWeakObjectPtr<AActor> Reference,
																   ESoCharVariableCommand Cmd,
																   FName Variable) :
	ReferenceActor(Reference),
	Command(Cmd),
	VariableName(Variable)
{
	CommandName = CommandNames[static_cast<int32>(Cmd)] + "." + Variable.ToString();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoConsoleCommandSetCharVariable::FSoConsoleCommandSetCharVariable(TWeakObjectPtr<AActor> Reference,
																   ESoCharVariableCommand Cmd) :
	ReferenceActor(Reference),
	Command(Cmd),
	bVariableNameIsStringParam(true)
{
	CommandName = CommandNames[static_cast<int32>(Cmd)];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommandSetCharVariable::ChangeCharVariable(const TArray<FString>& Args)
{
	if (!ReferenceActor.IsValid())
		return;

	ASoCharacter* Char = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor.Get());
	if (Char == nullptr)
		return;

	USoPlayerCharacterSheet* SoSheet = Char->GetPlayerCharacterSheet();
	if (SoSheet == nullptr)
		return;

	const int32 ValueIndex = bVariableNameIsStringParam ? 1 : 0;
	if (bVariableNameIsStringParam)
	{
		if (Args.Num() == 0)
		{
			UE_LOG(LogSoConsoleCommands, Warning, TEXT("Failed to execute command: missing argument(s)!"));
			return;
		}
		VariableName = FName(*Args[0]);
	}

	switch (Command)
	{
	case ESoCharVariableCommand::ECVC_SetInt:
			SoSheet->SetIntValue(VariableName, Args.IsValidIndex(ValueIndex) ? FCString::Atoi(*Args[ValueIndex]) : 0);
			break;

	case ESoCharVariableCommand::ECVC_SetFloat:
			SoSheet->SetFloatValue(VariableName, Args.IsValidIndex(ValueIndex) ? FCString::Atof(*Args[ValueIndex]) : 0.0f);
			break;

	case ESoCharVariableCommand::ECVC_SetName:
			SoSheet->SetNameValue(VariableName, Args.IsValidIndex(ValueIndex) ? FName(*Args[ValueIndex]) : NAME_None);
			break;

	case ESoCharVariableCommand::ECVC_SetBool:
			SoSheet->SetBoolValue(VariableName, Args.IsValidIndex(ValueIndex) ? FCString::ToBool(*Args[ValueIndex]) : true);
			break;

		default:
			break;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoConsoleCommandAchievement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FString FSoConsoleCommandAchievement::CommandNames[static_cast<int32>(ESoConsoleCommandAchievementType::NumOf)] =
{
	"SO.Achievements.Unlock",
	"SO.Achievements.Reset",
	"SO.Achievements.Show",
	"SO.Achievements.SetProgress"
};

const FString FSoConsoleCommandAchievement::HelperTexts[static_cast<int32>(ESoConsoleCommandAchievementType::NumOf)] =
{
	"Mark the achievement as achieved",
	"Reset the achievement aka lock it (Only works in non shipping)",
	"Shows the user a pop-up notification with the current progress of an achievement.",
	"Sets the progress of this achievement (NOTE: only works for achievemnts that have progress)"
};

FSoConsoleCommandAchievement::FSoConsoleCommandAchievement(TWeakObjectPtr<AActor> Reference, FName InAchievementName, ESoConsoleCommandAchievementType InCommand)
	: ReferenceActor(Reference),
	Command(InCommand),
	AchievementName(InAchievementName)
{
	CommandName = CommandNames[static_cast<int32>(Command)] + "." + AchievementName.ToString();
}


void FSoConsoleCommandAchievement::HandleCommand(const TArray<FString>& Args)
{
	if (!ReferenceActor.IsValid())
		return;

	const auto& GameInstance = USoGameInstance::Get(ReferenceActor.Get());
	USoAchievementManager* AchievementManager = GameInstance.GetAchievementManager();
	if (!AchievementManager)
		return;

	if (!AchievementManager->IsUserIDValid())
		return;

	float ArgProgress = INDEX_NONE;
	if (Args.Num() > 0)
		ArgProgress = FMath::Abs(FCString::Atof(*Args[0]));

	switch (Command)
	{
	case ESoConsoleCommandAchievementType::Unlock:
		AchievementManager->UnlockAchievement(ReferenceActor.Get(), AchievementName);
		break;
	case ESoConsoleCommandAchievementType::Reset:
		AchievementManager->ResetAchievement(ReferenceActor.Get(), AchievementName);
		break;
	case ESoConsoleCommandAchievementType::Show:
		AchievementManager->ShowAchievementProgress(AchievementName);
		break;
	case ESoConsoleCommandAchievementType::SetProgress:
		if (ArgProgress > INDEX_NONE)
		{
			checkNoEntry();
			// if (AchievementManager->IsProgressAchievement(AchievementName))
			// {
			// 	AchievementManager->SetAchievementProgressForName(AchievementName, ArgProgress);
			// }
			// else
			// {
			// 	const FString Message = FString::Printf(TEXT("Achievement = %s is not a progress achievement. Use Unlock instead"), *AchievementName.ToString());
			// 	USoPlatformHelper::PrintToConsole(ReferenceActor.Get(), Message);
			// 	UE_LOG(LogSoConsoleCommands, Warning, TEXT("%s"), *Message);
			// }
		}
		else
		{
			const FString Message = TEXT("ArgProgress is not set");
			USoPlatformHelper::PrintToConsole(ReferenceActor.Get(), Message);
			UE_LOG(LogSoConsoleCommands, Warning, TEXT("%s"), *Message);
		}
		break;
	default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoConsoleCommands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::RegisterAllCommands(AActor* OwnerActor)
{
	// Clear previous state
	UnRegisterAllCommands();

	ReferenceActor = OwnerActor;
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	// Load all dialogues/items into memory
	UDlgManager::LoadAllDialoguesIntoMemory();
	USoItemHelper::LoadAllItemsIntoMemory();

	RegisterPrintCommands();
	RegisterInputCommands();
	RegisterSettingCommands();
	RegisterAchievementCommands();
	RegisterSteamCommands();
	RegisterTeleportCommands();
	RegisterDialogueCommands();
	RegisterCharacterAndItemCommands();

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Crash.Check"),
		TEXT("Claims and loads all Levels"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			checkf(false, TEXT("Crashing because of console command"));
		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ClaimAndLoadAllLevels"),
		TEXT("Claims and loads all Levels"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			FSoLevelManager::Get().ClaimAndLoadAllLevels(ReferenceActor.Get()->GetWorld());
		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.StartOutro"),
		TEXT("Guess What"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::StartOutro), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleUI"),
		TEXT("Enables/Disables ingame UI"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleUI), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleHitLines"),
		TEXT("Enables/Disables debug visualizations of hit lines"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleDisplayHitLines), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleQuickSaveLoad"),
		TEXT("Enables/Disables the quick save/;pad"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleQuickSaveLoad), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleEnemyCoverTraces"),
		TEXT("Enables/Disables debug visualizations of cover traces"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleDisplayEnemyCoverTraces), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleSkipDialogues"),
		TEXT("Enables/Disables dialogue fast forward mode"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleDialogueDebugSkip), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleAmbientLog"),
		TEXT("Enables/Disables ambient log"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleAmbientLog), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleLateWallJump"),
		TEXT("Enables/Disables easier WallJumps"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleLateWallJump), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.SetLateWallJumpInterval"),
		TEXT("Sets the time interval player can still walljump after hitting the wall"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::SetLateWallJumpInterval), ECVF_Default));

	IConsoleManager::Get().CallAllConsoleVariableSinks();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoConsoleCommands::AllowCheats() const
{
	if (const auto* GameInstance = USoGameInstance::GetInstance(ReferenceActor.Get()))
		return GameInstance->AllowCheats();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::PrintBoolToConsole(bool bValue)
{
	if (!ReferenceActor.IsValid())
		return;

	USoPlatformHelper::PrintToConsole(ReferenceActor.Get(), FString::Printf(TEXT("%s"), bValue ? TEXT("True") : TEXT("False")));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::RegisterPrintCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();
	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Print.GameInstanceState"),
		TEXT("Prints USoGameInstance::State"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			const FString Message = FString::Printf(TEXT("USoGameInstance::State = %s"), *USoGameInstance::Get(ReferenceActor.Get()).GetCurrentStateAsString());
			USoPlatformHelper::PrintToAll(ReferenceActor.Get(), Message);
		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Print.LogPath"),
		TEXT("Prints the log path"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			const FString Path = FPaths::ConvertRelativePathToFull(FPlatformOutputDevices::GetAbsoluteLogFilename());
			const FString Message = FString::Printf(TEXT("LogPath = %s"), *Path);
			USoPlatformHelper::PrintToAll(ReferenceActor.Get(), Message);
		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Print.EngineVersion"),
		TEXT("Prints engine version"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			const FString Message = FString::Printf(
				TEXT("GPackageFileUE4Version = %d, GPackageFileLicenseeUE4Version = %d, Engine = %s"),
				GPackageFileUE4Version, GPackageFileLicenseeUE4Version, *FEngineVersion::Current().ToString()
			);
			USoPlatformHelper::PrintToAll(ReferenceActor.Get(), Message);
		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Print.LoadingLevels"),
		TEXT("Prints All Loading Levels"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			TArray<FName> LoadingLevelNames;
			if (USoLevelHelper::GetAllLevelsThatAreLoading(ReferenceActor.Get(), LoadingLevelNames))
			{
				USoPlatformHelper::PrintToAll(ReferenceActor.Get(), TEXT("Levels Loading:"));
				for (const FName LevelName : LoadingLevelNames)
				{
					USoPlatformHelper::PrintToAll(ReferenceActor.Get(), FString::Printf(TEXT("\t%s"), *LevelName.ToString()));
				}
			}
			else
			{
				USoPlatformHelper::PrintToAll(ReferenceActor.Get(), TEXT("No Levels are loading"));
			}

		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Print.VisibleLevels"),
		TEXT("Prints All Visible Levels"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			TArray<FName> VisibleLevelNames;
			if (USoLevelHelper::GetAllVisibleLevels(ReferenceActor.Get(), VisibleLevelNames))
			{
				USoPlatformHelper::PrintToAll(ReferenceActor.Get(), TEXT("Levels Visible:"));
				for (const FName LevelName : VisibleLevelNames)
				{
					USoPlatformHelper::PrintToAll(ReferenceActor.Get(), FString::Printf(TEXT("\t%s"), *LevelName.ToString()));
				}
			}
			else
			{
				USoPlatformHelper::PrintToAll(ReferenceActor.Get(), TEXT("No Levels are visible"));
			}

		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Print.PlatformContext"),
		TEXT("Prints the info about the current platform"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			const FString PlatformInfo = USoPlatformHelper::ToStringPlatformContext();
			USoPlatformHelper::PrintToAll(ReferenceActor.Get(), PlatformInfo);
		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Print.IsGameStarted"),
		TEXT("Prints GameInstance.IsGameStarted"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			const FString Output = FString::Printf(TEXT("IsGameStarted=%d"), USoGameInstance::Get(ReferenceActor.Get()).IsGameStarted());
			USoPlatformHelper::PrintToAll(ReferenceActor.Get(), Output);
		}), ECVF_Default)
	);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::RegisterInputCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	// EnableForcedDeviceType
	for (int32 DeviceTypeIndex = static_cast<int32>(ESoInputDeviceType::Keyboard);
		DeviceTypeIndex < static_cast<int32>(ESoInputDeviceType::Num); DeviceTypeIndex++)
	{
		const ESoInputDeviceType DeviceType = static_cast<ESoInputDeviceType>(DeviceTypeIndex);

		const FString CommandName = TEXT("SO.Input.EnableForcedDeviceType.") + USoInputHelper::DeviceTypeToFriendlyString(DeviceType);
		ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
			*CommandName,
			TEXT("Forces the device type to be displayed"),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::InputForceDeviceType, DeviceType), ECVF_Default));
	}

	// DisableForcedDeviceType
	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Input.DisableForcedDeviceType"),
		TEXT("Disables the forced device type"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			if (ASoPlayerController * SoController = ASoPlayerController::GetInstance(ReferenceActor.Get()))
				SoController->DisableForcedDeviceType();

		}), ECVF_Default)
	);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::RegisterSettingCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();
	// Demo commands
	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Demo.Video.ToggleDisableMainMenu"),
		TEXT("Disables/Enables the main menu"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			auto& Settings = USoGameSettings::Get();
			Settings.SetIsVideoDemoDisableMenu(!Settings.CanVideoDemoDisableMenu());
			Settings.ApplySettings(false);
			PrintBoolToConsole(Settings.CanVideoDemoDisableMenu());
		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Settings.TogglePauseGameGameOnIdle"),
		TEXT("Enables/Disables Pause on Idle "),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			auto& Settings = USoGameSettings::Get();
			Settings.SetPauseGameOnIdle(!Settings.CanPauseGameOnIdle());
			Settings.ApplySettings(false);
			PrintBoolToConsole(Settings.CanPauseGameOnIdle());
		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Settings.TogglePauseGameWhenUnfocused"),
		TEXT("Enables/Disables Pause when game is unfocused "),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			auto& Settings = USoGameSettings::Get();
			Settings.SetPauseGameWhenUnfocused(!Settings.CanPauseGameWhenUnfocused());
			Settings.ApplySettings(false);
			PrintBoolToConsole(Settings.CanPauseGameWhenUnfocused());
		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Settings.ToggleCustomMapsEnabled"),
		TEXT("Enables/Disables custom maps support"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			auto& Settings = USoGameSettings::Get();
			Settings.SetAreCustomMapsEnabled(!Settings.AreCustomMapsEnabled());
			Settings.ApplySettings(false);
			PrintBoolToConsole(Settings.AreCustomMapsEnabled());
		}), ECVF_Default)
	);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::RegisterAchievementCommands()
{
	if (!AllowCheats())
		return;

	IConsoleManager& ConsoleManager = IConsoleManager::Get();
#if !UE_BUILD_SHIPPING
	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Achievements.ResetAll"),
		TEXT("Resets all achievements (NOTE: Only works in non shipping mode)"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			if (USoAchievementManager* Manager = USoGameInstance::Get(ReferenceActor.Get()).GetAchievementManager())
			{
				Manager->ResetAchievements();
				USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor.Get())->RemoveBoolsWithPrefix("A_");
			}
		}), ECVF_Default)
	);
#endif // !UE_BUILD_SHIPPING

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Achievements.PrintAll"),
		TEXT("Print All Achievements"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			USoPlatformHelper::PrintToConsole(ReferenceActor.Get(), TEXT("Achievements"));
			for (const FName AchievementName : FSoAchievement::GetAllAchievementNames())
			{
				USoPlatformHelper::PrintToConsole(ReferenceActor.Get(), AchievementName.ToString());
			}
		}), ECVF_Default)
	);


	for (const FName AchievementName : FSoAchievement::GetAllAchievementNames())
	{
		AchievementCommands.Add({ ReferenceActor, AchievementName, ESoConsoleCommandAchievementType::Unlock });
		AchievementCommands.Add({ ReferenceActor, AchievementName, ESoConsoleCommandAchievementType::Show });
		// AchievementCommands.Add({ ReferenceActor, AchievementName, ESoConsoleCommandAchievementType::SetProgress });
#if !UE_BUILD_SHIPPING
		AchievementCommands.Add({ ReferenceActor, AchievementName, ESoConsoleCommandAchievementType::Reset });
#endif // !UE_BUILD_SHIPPING
	}

	for (const FSoConsoleCommandAchievement& Cmd : AchievementCommands)
	{
		ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
			*Cmd.GetCommandName(),
			*Cmd.GetCommandHelperText(),
			FConsoleCommandWithArgsDelegate::CreateRaw(&Cmd, &FSoConsoleCommandAchievement::HandleCommand), ECVF_Default));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::RegisterSteamCommands()
{
#if WARRIORB_WITH_STEAM
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Steam.ShowFriendsUI"),
		TEXT("Shows the friends UI"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			INYSteamSubsystemPtr Steam = INotYetSteamModule::Get().GetSteamSubsystem();
			if (Steam.IsValid())
			{
				Steam->GetExternalUI()->ShowFriendsUI();
			}

		}), ECVF_Default)
	);
	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Steam.ShowAchievementsUI"),
		TEXT("Shows the Achievements UI"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			INYSteamSubsystemPtr Steam = INotYetSteamModule::Get().GetSteamSubsystem();
			if (Steam.IsValid())
			{
				Steam->GetExternalUI()->ShowAchievementsUI();
			}

		}), ECVF_Default)
	);

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Steam.SetRichPresenceStatus"),
		TEXT("Sets the rich presence with a custom <message>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			if (Args.Num() == 0)
			{
				USoPlatformHelper::PrintToConsole(ReferenceActor.Get(), TEXT("No arguments given"));
				return;
			}

			TSharedPtr<const FUniqueNetId> UserID = USoOnlineHelper::GetUniqueNetIDFromObject(ReferenceActor.Get());
			const FString& Message = Args[0];

			INYSteamSubsystemPtr Steam = INotYetSteamModule::Get().GetSteamSubsystem();
			if (Steam.IsValid())
			{
				Steam->GetPresence()->SetRichPresenceStatus(*UserID, Message);
			}
		}), ECVF_Default)
	);

#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::RegisterTeleportCommands()
{
	if (!AllowCheats())
		return;

	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	// Spline Teleport Commands
	for (TActorIterator<ASoPlayerSpline> ActorItr(ReferenceActor.Get()->GetWorld()); ActorItr; ++ActorItr)
		SplineCommands.Add({ *ActorItr });

	// Teleport to spline
	for (FSoSplineConsoleCommand& Command : SplineCommands)
	{
		const FString CommandName = FString("SO.T.") + Command.GetSpline()->GetName();
		const FString HelpText = TEXT("Player character jumps to selected spline. Optional parameters: Distance, ZValue");
		ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
			*CommandName,
			*HelpText,
			FConsoleCommandWithArgsDelegate::CreateRaw(&Command, &FSoSplineConsoleCommand::PerformTeleportToSpline), ECVF_Default));
	}

	// Teleport to Chapter
	if (ReferenceActor.IsValid())
	{
		const FName CurrentChapterName = USoLevelHelper::GetChapterNameFromObject(ReferenceActor.Get());
		const TArray<FSoChapterMapParams>& AllChapters = USoLevelHelper::GetAllChaptersData();


		for (const auto& Chapter : AllChapters)
		{
			const FName ChapterName = Chapter.GetMapName();

			// Ignore current
			if (CurrentChapterName == ChapterName)
				continue;

			const FString CommandName = TEXT("SO.TeleportToChapter.") + ChapterName.ToString();
			ConsoleObjects.Add(
				ConsoleManager.RegisterConsoleCommand(
					*CommandName,
					TEXT("Teleports to the selected chapter on the default checkpoint"),
					FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::TeleportToChapter, ChapterName),
					ECVF_Default
				)
			);
		}
	}

	// Teleport to Episode
	if (ReferenceActor.IsValid())
	{
		const FName CurrentEpisodeName = USoLevelHelper::GetEpisodeNameFromObject(ReferenceActor.Get());
		const TArray<FSoEpisodeMapParams>& AllEpisodes = USoLevelHelper::GetAllEpisodesData();

		for (const auto& Episode : AllEpisodes)
		{
			const FName EpisodeName = Episode.GetMapName();

			// Ignore current
			if (CurrentEpisodeName == EpisodeName)
				continue;

			const FString CommandName = TEXT("SO.TeleportToEpisode.") + EpisodeName.ToString();
			ConsoleObjects.Add(
				ConsoleManager.RegisterConsoleCommand(
					*CommandName,
					TEXT("Teleports to the selected episode"),
					FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::TeleportToEpisode, EpisodeName),
					ECVF_Default
				)
			);
		}
	}

	// Teleport to checkpoint
	if (const ASoLocationRegistry* LocationRegistry = ASoLocationRegistry::GetInstance(ReferenceActor.Get()))
	{
		for (const auto& Elem : LocationRegistry->GetAllCheckpoints())
		{
			const FName CheckpointFName = Elem.Key;
			const FString CommandName = TEXT("SO.TeleportToCheckpoint.") + (CheckpointFName == NAME_None ? FString("DefaultStart") : CheckpointFName.ToString());
			ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
				*CommandName,
				TEXT("Teleport the selected checkpoint"),
				FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::TeleportToCheckpoint, CheckpointFName), ECVF_Default));
		}
	}

	// Teleport to main menu
	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.TeleportToMainMenu"),
		TEXT("Teleports to the main menu level"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::TeleportToMainMenu), ECVF_Default));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::RegisterDialogueCommands()
{
	if (!AllowCheats())
		return;

	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	// Character story data commands
	ASoCharacter* Char = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor.Get());
	if (Char == nullptr)
		return;

	TArray<FName> Names;
	const FName CharName = IDlgDialogueParticipant::Execute_GetParticipantName(Char);

	// Integers
	UDlgManager::GetAllDialoguesIntNames(CharName, Names);
	for (const auto Name : Names)
		CharVariableCommands.Add({ ReferenceActor, ESoCharVariableCommand::ECVC_SetInt, Name });

	// Floats
	Names.Empty();
	UDlgManager::GetAllDialoguesFloatNames(CharName, Names);

	for (const auto Name : Names)
		CharVariableCommands.Add({ ReferenceActor, ESoCharVariableCommand::ECVC_SetFloat, Name });

	// bool
	Names.Empty();
	UDlgManager::GetAllDialoguesBoolNames(CharName, Names);

	for (const auto Name : Names)
		CharVariableCommands.Add({ ReferenceActor, ESoCharVariableCommand::ECVC_SetBool, Name });

	// default ones - they expect the variable name as parameter
	CharVariableCommands.Add({ ReferenceActor, ESoCharVariableCommand::ECVC_SetInt });
	CharVariableCommands.Add({ ReferenceActor, ESoCharVariableCommand::ECVC_SetFloat });
	CharVariableCommands.Add({ ReferenceActor, ESoCharVariableCommand::ECVC_SetBool });

	for (const FSoConsoleCommandSetCharVariable& Cmd : CharVariableCommands)
	{
		ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
			*Cmd.GetCommandName(),
			*Cmd.GetCommandHelperText(),
			FConsoleCommandWithArgsDelegate::CreateRaw(&Cmd, &FSoConsoleCommandSetCharVariable::ChangeCharVariable), ECVF_Default));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoConsoleCommands::IsSpeedRunCompetitionMode()
{
	static bool bValue = FParse::Param(FCommandLine::Get(), TEXT("SpeedRunCompetition"));
	return bValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::RegisterCharacterAndItemCommands()
{
	if (!AllowCheats())
		return;

	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.SetHP"),
		TEXT("Modifies both the max and the current HP of the character. Params: amount (int)"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::SetHp), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.SetSpellsCapacity"),
		TEXT("Set the spells capacity of the player. Params: amount (int)"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::SetSpellCapacity), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Resurrect"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::Resurrect), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Rest"),
		TEXT("Opens rest panel, restores character values"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::Rest), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.Kill"),
		TEXT("All ASoEnemy instance gets a kill signal"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::KillAllEnemy), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.AddAll"),
		TEXT("Shortcut for all add commands. SO.Add*"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::AddAllToCharacter), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.AddKeys"),
		TEXT("Adds all key to inventory"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::AddAllKeyToCharacter), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.AddQuestItems"),
		TEXT("Adds all quest items to inventory"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::AddAllQuestItemToCharacter), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.AddWeapons"),
		TEXT("Adds all weapons to inventory"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::AddAllWeaponToCharacter), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.AddShards"),
		TEXT("Adds all clothes to inventory"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::AddAllShardToCharacter), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.AddJewel"),
		TEXT("Adds all Jewelry to inventory"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::AddAllJewelryToCharacter), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.AddUsables"),
		TEXT("Adds all usable items to inventory"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::AddAllUsableItemToCharacter), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.AddRuneStones"),
		TEXT("Adds all RuneStones to inventory"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::AddAllRuneStonesToCharacter), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.EquipAllSpells"),
		TEXT("Equips all spells it can"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::EquipAllSpells), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ReloadAIConfig"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ReloadAIConfig), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.SetGameSpeedMultiplier"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::SetGameSpeedMultiplier), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.SetAudioListenerLerpValue"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::SetListenerLerpValue), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleSoVisibilityQuery"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::TestVisibilityQuery), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.SetDifficulty"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::SetDifficulty), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.AddBonusHealth"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::AddBonusHealth), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ClearBonusHealth"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ClearBonusHealth), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleLillian"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleLillian), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleDmgCheat"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleDmgCheat), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleGodMode"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleGodMode), ECVF_Default));

	ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("SO.ToggleFlyMode"),
		TEXT("Guess what"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSoConsoleCommands::ToggleFlyMode), ECVF_Default));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::UnRegisterAllCommands()
{
	// remove objects - they are released in UnregisterConsoleObject
	for (IConsoleObject* Object : ConsoleObjects)
		IConsoleManager::Get().UnregisterConsoleObject(Object);

	ConsoleObjects.Empty();
	SplineCommands.Empty();
	CharVariableCommands.Empty();
	AchievementCommands.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::AddAllToCharacter(const TArray<FString>& Args)
{
	AddAllKeyToCharacter(Args);
	AddAllQuestItemToCharacter(Args);
	AddAllUsableItemToCharacter(Args);
	AddAllWeaponToCharacter(Args);
	AddAllShardToCharacter(Args);
	AddAllJewelryToCharacter(Args);
	AddAllRuneStonesToCharacter(Args);
	EquipAllSpells(Args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::AddAllKeyToCharacter(const TArray<FString>& Args)
{
	if (!AllowCheats())
		return;

	if (!ReferenceActor.IsValid())
		return;

	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld());

	if (Character == nullptr)
		return;

	for (TObjectIterator<USoQuestItemTemplate> Itr; Itr; ++Itr)
		if ((*Itr) != nullptr && (*Itr)->QuestItemType == ESoQuestItemType::EQIT_Key)
		{
			FSoItem Item;
			Item.Template = *Itr;
			Character->AddItem(Item, false);
		}
	UE_LOG(LogSoConsoleCommands, Display, TEXT("All keys added to inventory!"));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoConsoleCommands::AddItems(UClass* ClassFilter)
{
	if (!ReferenceActor.IsValid())
		return false;

	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld());

	if (Character == nullptr)
		return false;

	for (TObjectIterator<USoItemTemplate> Itr; Itr; ++Itr)
		if ((*Itr)->GetClass() == ClassFilter)
		{
			FSoItem Item;
			Item.Template = *Itr;
			if (Item.Template->IsStackable())
				Item.Amount = Item.Template->GetMaxStackNum();
			Character->AddItem(Item, false);
		}

	UE_LOG(LogSoConsoleCommands, Display, TEXT("Items from class %s added to inventory!"), *ClassFilter->GetName());
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::TeleportToCheckpoint(const TArray<FString>& Args, const FName CheckPointName)
{
	if (!ReferenceActor.IsValid())
		return;

	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor.Get()))
		Character->TeleportToCheckpointName(CheckPointName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::InputForceDeviceType(const TArray<FString>& Args, const ESoInputDeviceType DeviceType)
{
	if (!ReferenceActor.IsValid())
		return;

	if (ASoPlayerController * SoController = ASoPlayerController::GetInstance(ReferenceActor.Get()))
		SoController->EnableForcedDeviceType(DeviceType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::TeleportToChapter(const TArray<FString>& Args, FName ChapterName)
{
	if (!ReferenceActor.IsValid())
		return;

	if (!USoLevelHelper::IsValidChapterName(ChapterName))
		return;

	USoGameInstance::Get(ReferenceActor.Get()).TeleportToChapter(ChapterName, NAME_None, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::TeleportToEpisode(const TArray<FString>& Args, FName EpisodeName)
{
	if (!ReferenceActor.IsValid())
		return;

	if (!USoLevelHelper::IsValidEpisodeName(EpisodeName))
		return;

	USoGameInstance::Get(ReferenceActor.Get()).TeleportToEpisode(EpisodeName, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::TeleportToMainMenu(const TArray<FString>& Args)
{
	if (!ReferenceActor.IsValid())
		return;

	USoGameInstance::Get(ReferenceActor.Get()).TeleportToMainMenu(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::AddAllQuestItemToCharacter(const TArray<FString>& Args)
{
	AddItems(USoQuestItemTemplate::StaticClass());
	AddItems(USoItemTemplateQuestBook::StaticClass());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::AddAllJewelryToCharacter(const TArray<FString>& Args)
{
	AddItems(USoItemTemplateJewelry::StaticClass());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::AddAllRuneStonesToCharacter(const TArray<FString>& Args)
{
	AddItems(USoItemTemplateRuneStone::StaticClass());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::AddAllUsableItemToCharacter(const TArray<FString>& Args)
{
	AddItems(USoUsableItemTemplate::StaticClass());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::AddAllWeaponToCharacter(const TArray<FString>& Args)
{
	AddItems(USoWeaponTemplate::StaticClass());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::AddAllShardToCharacter(const TArray<FString>& Args)
{
	AddItems(USoItemTemplateShard::StaticClass());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::SetHp(const TArray<FString>& Args)
{
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld());
	if (Character == nullptr)
		return;

	const int32 Value = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 100;
	Character->GetPlayerCharacterSheet()->SetMaxHealth(Value);
	Character->GetPlayerCharacterSheet()->SetHealth(Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::SetSpellCapacity(const TArray<FString>& Args)
{
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld());
	if (Character == nullptr)
		return;

	const int32 Value = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 100;
	Character->SetSpellsCapacity(Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::EquipAllSpells(const TArray<FString>& Args)
{
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld());
	if (Character == nullptr)
		return;

	Character->SetSpellsCapacity(100);

	// Iterate over items list
	// TODO do bounds checking on spells amount
	TArray<FSoItem> EquippedRuneStones = Character->GetPlayerCharacterSheet()->GetEquippedSpells();
	const TArray<FSoItem>& ItemList = Character->GetInventory()->GetItemList();
	for (const FSoItem& Item : ItemList)
	{
		if (EquippedRuneStones.Num() >= 7)
			break;

		if (USoItemTemplateRuneStone* RuneStone = Cast<USoItemTemplateRuneStone>(Item.Template))
		{
			bool bAdded = false;
			for (FSoItem& EquippedRuneStone : EquippedRuneStones)
			{
				if (EquippedRuneStone.Template == RuneStone)
				{
					bAdded = true;
					EquippedRuneStone.Amount += 1;
				}
			}

			if (!bAdded)
			{
				FSoItem Stone;
				Stone.Amount = 1;
				Stone.Template = RuneStone;
				EquippedRuneStones.Add(Stone);
			}
		}
	}
	Character->GetPlayerCharacterSheet()->SetEquippedSpells(EquippedRuneStones);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::Resurrect(const TArray<FString>& Args)
{
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld());
	Character->SoActivity->SwitchActivity(Character->SoADead);
	Character->Revive(false);
	Character->SoActivity->SwitchActivity(Character->SoADefault);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::Rest(const TArray<FString>& Args)
{
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld());
	Character->SoAInUI->OpenRestPanel();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::KillAllEnemy(const TArray<FString>& Args)
{
	if (ReferenceActor != nullptr && ReferenceActor->GetWorld() != nullptr)
	{
		for (TActorIterator<ASoEnemy> ActorItr(ReferenceActor->GetWorld()); ActorItr; ++ActorItr)
			ISoMortal::Execute_Kill(*ActorItr, true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::StartOutro(const TArray<FString>& Args)
{
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld());
	Character->SoAInUI->StartEndGamePanel();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleUI(const TArray<FString>& Args)
{
	if (!ReferenceActor.IsValid())
		return;

	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld());

	if (Character == nullptr)
		return;

	Character->OnToggleUI().Broadcast();

	// TODO: hide UI
	// Character->HandleUICommand(ESoUICommand::EUC_ToggleUI, Character);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleDisplayHitLines(const TArray<FString>& Args)
{
	USoCombatComponent::bDisplayDebugHitLines = !USoCombatComponent::bDisplayDebugHitLines;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleQuickSaveLoad(const TArray<FString>& Args)
{
	ASoCharacter::bEnableQuickSaveLoad = !ASoCharacter::bEnableQuickSaveLoad;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleDisplayEnemyCoverTraces(const TArray<FString>& Args)
{
	ASoEnemy::bDisplayMeleeHitTraceLines = !ASoEnemy::bDisplayMeleeHitTraceLines;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleDialogueDebugSkip(const TArray<FString>& Args)
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
		Character->bDebugSkipDialogues = !Character->bDebugSkipDialogues;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleAmbientLog(const TArray<FString>& Args)
{
	USoAudioManager::Get(ReferenceActor.Get()).TogglePrintAmbientLog();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleLateWallJump(const TArray<FString>& Args)
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
		Character->GetSoMovement()->ToggleAllowLateWallJump();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::SetLateWallJumpInterval(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
		return;

	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
		Character->GetSoMovement()->SetLateWallJumpInterval(FCString::Atof(*Args[0]));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ReloadAIConfig(const TArray<FString>& Args)
{
	USoGameInstance::Get(ReferenceActor.Get()).ClearConfigData();

	for (TActorIterator<ASoEnemy> ActorItr(ReferenceActor->GetWorld()); ActorItr; ++ActorItr)
		(*ActorItr)->ReloadScripts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::SetGameSpeedMultiplier(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
		return;

	USoGameSettings::Get().SetGameSpeed(FCString::Atof(*Args[0]));
	USoGameSettings::Get().ApplyGameSettings(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::SetListenerLerpValue(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
		return;

	const float Value = FCString::Atof(*Args[0]);
	if (ASoPlayerController* PlayerController = Cast<ASoPlayerController>(USoStaticHelper::GetPlayerController(ReferenceActor.Get())))
		PlayerController->SetListenerLerpValue(Value);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::TestVisibilityQuery(const TArray<FString>& Args)
{
	static bool bSet = false;
	static SoCustomCulling CustomCulling;

	if (bSet)
	{
		GetRendererModule().UnregisterCustomCullingImpl(&CustomCulling);
		bSet = false;
	}
	else
	{
		GetRendererModule().RegisterCustomCullingImpl(&CustomCulling);
		bSet = true;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::SetDifficulty(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
		return;

	const int32 Value = FCString::Atoi(*Args[0]);

	if (Value >= 0 && Value < static_cast<int32>(ESoDifficulty::NumOf))
	{
		const ESoDifficulty Difficulty = static_cast<ESoDifficulty>(Value);
		FSoWorldState::Get().SetGameDifficulty(Difficulty);

		if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
		{
			Character->GetPlayerCharacterSheet()->OnDifficultyChanged(Difficulty);
			Character->Revive(false);
		}

		switch (Difficulty)
		{
			case ESoDifficulty::Sane:
				UE_LOG(LogTemp, Warning, TEXT("Game Difficulty Changed To: Sane"));
				break;

			case ESoDifficulty::Intended:
				UE_LOG(LogTemp, Warning, TEXT("Game Difficulty Changed To: Intended"));
				break;

			case ESoDifficulty::Insane:
				UE_LOG(LogTemp, Warning, TEXT("Game Difficulty Changed To: Insane"));
				break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::AddBonusHealth(const TArray<FString>& Args)
{
	FSoDmg BonusHealth;
	BonusHealth.Physical = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.0f;
	BonusHealth.Magical = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.0f;
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
	{
		Character->GetPlayerCharacterSheet()->AddBonusHealth(BonusHealth);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ClearBonusHealth(const TArray<FString>& Args)
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
	{
		Character->GetPlayerCharacterSheet()->ClearBonusHealth();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleLillian(const TArray<FString>& Args)
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
	{
		if (Cast<USoALillian>(Character->SoActivity) != nullptr)
		{
			Character->SoActivity->SwitchActivity(Character->SoADefault);
			Character->GetPlayerCharacterSheet()->SetBoolValue(USoStaticHelper::GetLillianFormName(), false);
			Character->ChangeUIVisibility.Broadcast(true);
		}
		else
		{
			Character->SoActivity->SwitchActivity(Character->SoALillian);
			Character->GetPlayerCharacterSheet()->SetBoolValue(USoStaticHelper::GetLillianFormName(), true);
			Character->ChangeUIVisibility.Broadcast(true);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleDmgCheat(const TArray<FString>& Args)
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
		Character->SoAStrike->ToggleDmgCheat();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleGodMode(const TArray<FString>& Args)
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
		Character->bGodMode = !Character->bGodMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoConsoleCommands::ToggleFlyMode(const TArray<FString>& Args)
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(ReferenceActor->GetWorld()))
	{
		Character->bFlyCheatOn = !Character->bFlyCheatOn;
		Character->SoActivity->OnSuperModeChange(Character->bFlyCheatOn);
	}
}
