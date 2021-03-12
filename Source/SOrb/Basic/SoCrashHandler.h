// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "SoCrashHandler.generated.h"

class USoGameInstance;

// Handle crashing
// Also see:
// - https://docs.sentry.io/platforms/native/ue4/
// - https://forum.sentry.io/t/unreal-engine-crash-reporter-cant-get-it-to-work/7643
USTRUCT()
struct FSoCrashHandler
{
	GENERATED_USTRUCT_BODY()
	typedef FSoCrashHandler Self;
public:
	void Initialize(USoGameInstance* Instance);
	void Shutdown();

protected:
	// Adds the current player context to the crash error message
	void AddContextToCrashErrorMessage();

	// an ensure has occurred
	void HandleOnCrashFromEnsure();

	// error (crash) has occurred
	void HandleOnCrash();

	// After sent to server.
	void HandleOnShutdownAfterCrash(const bool bFromEnsure);

	// Opens up a google doc + the folder we need
	void OpenCrashFeedbackBrowserAndExplorer();

protected:
	// Used when we are crashing
	bool bSentCrashReport = false;
	FString CrashErrorMessage;

	// Backwards reference to the game instance, our owner
	USoGameInstance* GameInstance = nullptr;
};
