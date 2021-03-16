// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "NYLoadingScreenModule.h"
#include "NYLoadingScreenInstance.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "LoadingScreen"

IMPLEMENT_MODULE(FNYLoadingScreenModule, NotYetLoadingScreen)

DEFINE_LOG_CATEGORY(LogNYLoadingScreen)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FNYLoadingScreenModule::FNYLoadingScreenModule()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenModule::StartupModule()
{
	UE_LOG(LogNYLoadingScreen, Log, TEXT("StartupModule"));
	if (IsRunningDedicatedServer() || !FSlateApplication::IsInitialized() || IsRunningCommandlet())
	{
		return;
	}

	// Load for cooker reference
	// const UNYLoadingScreenSettings* Settings = GetDefault<UNYLoadingScreenSettings>();
	//TryToLoadImages(Settings->StartupScreen);

	FNYLoadingScreenInstance::Get()->Initialize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenModule::ShutdownModule()
{
	UE_LOG(LogNYLoadingScreen, Log, TEXT("ShutdownModule"));
	UnregisterConsoleCommands();

	for (auto& Handle : AssetHandlesPool)
	{
		if (Handle.IsValid())
		{
			Handle->ReleaseHandle();
		}

	}
	AssetHandlesPool.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedPtr<INYLoadingScreenInstance> FNYLoadingScreenModule::GetInstance() const
{
	return FNYLoadingScreenInstance::Get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenModule::RegisterConsoleCommands(AActor* InReferenceActor)
{
	// Unregister first to prevent double register of commands
	UnregisterConsoleCommands();

	ReferenceActor = InReferenceActor;
	IConsoleManager& ConsoleManager = IConsoleManager::Get();


	ConsoleCommands.Add(
		ConsoleManager.RegisterConsoleCommand(
			TEXT("NYLoadingScreen.TestShow"),
			TEXT("Test show the loading widget"),
			FConsoleCommandDelegate::CreateLambda([this]()
			{
				GetInstance()->TestShowWidget();
			}),
			ECVF_Default
		)
	);

	ConsoleCommands.Add(
		ConsoleManager.RegisterConsoleCommand(
			TEXT("NYLoadingScreen.TestHide"),
			TEXT("Test hide the loading widget"),
			FConsoleCommandDelegate::CreateLambda([this]()
			{
				GetInstance()->TestHideWidget();
			}),
			ECVF_Default
		)
	);

	ConsoleCommands.Add(
		ConsoleManager.RegisterConsoleCommand(
			TEXT("NYLoadingScreen.ToggleWaitForAnyKeyInput"),
			TEXT("Test hide the loading widget"),
			FConsoleCommandDelegate::CreateLambda([this]()
			{
				TSharedPtr<INYLoadingScreenInstance> Instance = GetInstance();
				Instance->SetWaitForAnyKeyInput(!Instance->IsWaitForAnyKeyInput());
			}),
			ECVF_Default
		)
	);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenModule::UnregisterConsoleCommands()
{
	ReferenceActor = nullptr;
	for (IConsoleCommand* Comand : ConsoleCommands)
	{
		IConsoleManager::Get().UnregisterConsoleObject(Comand);
	}
	ConsoleCommands.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenModule::RequestAsyncLoad(const FSoftObjectPath& Reference)
{
	static constexpr bool bManageActiveHandle = true;
	if (Reference.IsValid())
	{
		AssetHandlesPool.Add(
			AssetLoader.RequestAsyncLoad(Reference, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority, bManageActiveHandle)
		);
	}
}

#undef LOCTEXT_NAMESPACE
