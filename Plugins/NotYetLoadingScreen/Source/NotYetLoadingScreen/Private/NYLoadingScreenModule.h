// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "INYLoadingScreenModule.h"
#include "NYLoadingScreenSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/StreamableManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNYLoadingScreen, All, All);


class FNYLoadingScreenModule : public INYLoadingScreenModule
{
	typedef FNYLoadingScreenModule Self;
public:
	FNYLoadingScreenModule();

	/** IModuleInterface implementation */
	void StartupModule() override;
	void ShutdownModule() override;
	bool IsGameModule() const override
	{
		return true;
	}

	/** INYLoadingScreenModule implementation */
	TSharedPtr<INYLoadingScreenInstance> GetInstance() const override;

	void RegisterConsoleCommands(AActor* InReferenceActor = nullptr) override;
	void UnregisterConsoleCommands() override;
	void RequestAsyncLoad(const FSoftObjectPath& Reference) override;

protected:
	// Streaming stuff
	// https://wiki.unrealengine.com/TAssetPtr_and_Asynchronous_Asset_Loading
	FStreamableManager AssetLoader;
	TArray<TSharedPtr<FStreamableHandle>> AssetHandlesPool;

	/** Holds the console commands for this Module */
	TArray<IConsoleCommand*> ConsoleCommands;

	/** Reference Actor used to get the UWorld. */
	AActor* ReferenceActor = nullptr;
};
