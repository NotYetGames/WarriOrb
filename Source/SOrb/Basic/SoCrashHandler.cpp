// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoCrashHandler.h"

#include "Misc/CoreDelegates.h"
#include "HAL/ExceptionHandling.h"
#include "HAL/ThreadHeartBeat.h"
#include "GenericPlatform/GenericPlatformStackWalk.h"
#include "HAL/PlatformStackWalk.h"

#include "SoGameInstance.h"
#include "Character/SoCharacter.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "SplineLogic/SoSplineWalker.h"
#include "Helpers/SoPlatformHelper.h"
#include "Levels/SoLevelHelper.h"
#include "Online/Analytics/SoAnalytics.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "SoGameSingleton.h"


DEFINE_LOG_CATEGORY_STATIC(LogSoCrashHandler, All, All)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCrashHandler::Initialize(USoGameInstance* Instance)
{
	UE_LOG(LogSoCrashHandler, Log, TEXT("Initialize"));

	check(Instance);
	GameInstance = Instance;

	// Listen for crashes
	// TODO: Maybe use FGenericPlatformMisc:::SetCrashHandler
	FCoreDelegates::OnHandleSystemError.AddRaw(this, &Self::HandleOnCrash);
	FCoreDelegates::OnHandleSystemEnsure.AddRaw(this, &Self::HandleOnCrashFromEnsure);
	FCoreDelegates::OnShutdownAfterError.AddRaw(this, &Self::HandleOnShutdownAfterCrash, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCrashHandler::Shutdown()
{
	FCoreDelegates::OnHandleSystemError.RemoveAll(this);
	FCoreDelegates::OnHandleSystemEnsure.RemoveAll(this);
	FCoreDelegates::OnShutdownAfterError.RemoveAll(this);
	UE_LOG(LogSoCrashHandler, Log, TEXT("Shutdown"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCrashHandler::AddContextToCrashErrorMessage()
{
	if (GameInstance == nullptr)
		return;

	// Add Info about the system
	CrashErrorMessage += USoPlatformHelper::ToStringPlatformContext();

	// Game Crash context contains information about the location of the player
	FString Message;
	const FName ChapterName = USoLevelHelper::GetChapterNameFromObject(GameInstance);
	FVector CharacterWorldLocation(INDEX_NONE);
	FName LevelName;
	FString SplineName;
	int32 SplineDistance = INDEX_NONE;

	// TODO: Crash in crash? :Sweat:
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(GameInstance))
	{
		CharacterWorldLocation = Character->GetActorLocation();
		LevelName = USoLevelHelper::GetLevelNameFromActor(Character);
		const FSoSplinePoint SplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Character);
		SplineDistance = SplineLocation.GetDistance();

		if (SplineLocation.IsValid(false))
			Message = TEXT("SplineLocation is valid.");
		else
			Message = TEXT("SplineLocation is not valid.");

		const ASoPlayerSpline* Spline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
		if (Spline)
			SplineName = Spline->GetSplineName();
		else
			SplineName = TEXT("Spline is NOT valid");
	}
	else
	{
		Message = TEXT("Could NOT get Character");
	}

	CrashErrorMessage += FString::Printf(
		TEXT("Game CONTEXT:\n")
		TEXT("\tMessage = `%s`\n")
		TEXT("\tChapterName = `%s`\n")
		TEXT("\tLevelName = `%s`\n")
		TEXT("\tSplineName = `%s`\n")
		TEXT("\tSplineDistance = %d\n")
		TEXT("\tCharacterWorldLocation: %s\n")
		TEXT("\n"),
		*Message, *ChapterName.ToString(), *LevelName.ToString(), *SplineName, SplineDistance, *CharacterWorldLocation.ToString());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCrashHandler::HandleOnCrashFromEnsure()
{
	// DISABLED, as we don't care that much about ensures/checks
	return;

	if (GameInstance == nullptr)
		return;

	// A crash happened because an ensure() failed
	CrashErrorMessage = TEXT("- Crash happened from an ensure() -\n");
	AddContextToCrashErrorMessage();

	// Do the stack trace manually. Append to GErrorHist
	if (!FPlatformMisc::IsDebuggerPresent())
	{
		// Adapted from FDebug::EnsureFailed
		// No debugger attached, so generate a call stack and submit a crash report
		// Walk the stack and dump it to the allocated memory.
		const SIZE_T StackTraceSize = 65535;
		ANSICHAR* StackTrace = (ANSICHAR*)FMemory::SystemMalloc(StackTraceSize);
		if (StackTrace != nullptr)
		{
			// Stop checking heartbeat for this thread. Ensure can take a lot of time (when stackwalking)
			// Thread heartbeat will be resumed the next time this thread calls FThreadHeartBeat::Get().HeartBeat();
			// The reason why we don't call HeartBeat() at the end of this function is that maybe this thread
			// Never had a heartbeat checked and may not be sending heartbeats at all which would later lead to a false positives when detecting hangs.
			FThreadHeartBeat::Get().KillHeartBeat();
			FGameThreadHitchHeartBeat::Get().FrameStart(true);

			StackTrace[0] = 0;
			FPlatformStackWalk::StackWalkAndDumpEx(StackTrace, StackTraceSize, 0, FGenericPlatformStackWalk::EStackWalkFlags::FlagsUsedWhenHandlingEnsure);

			// Append the stack trace
			FCString::Strncat(GErrorHist, ANSI_TO_TCHAR(StackTrace), ARRAY_COUNT(GErrorHist) - 1);
			FMemory::SystemFree(StackTrace);
		}
	}

	// Manually call it because otherwise the ensure assertion does not crash/usually
	HandleOnShutdownAfterCrash(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCrashHandler::HandleOnCrash()
{
	if (GameInstance == nullptr)
		return;

	// We know a crash happened, unexpected
	CrashErrorMessage = TEXT("- Crash happened from unknown -\n");
	AddContextToCrashErrorMessage();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCrashHandler::HandleOnShutdownAfterCrash(const bool bFromEnsure)
{
	if (GameInstance == nullptr)
		return;
	if (bSentCrashReport)
		return;

	// Some of these should be filled with some usable info.
	// GErrorHist
	// GErrorExceptionDescription
	// GErrorMessage
	CrashErrorMessage += TEXT("Crash CONTEXT:\n");
	CrashErrorMessage += FString::Printf(TEXT("GCrashType = %d\n"), GetCrashType());
	CrashErrorMessage += FString::Printf(TEXT("GErrorHist = \n%s\n"), GErrorHist);
	CrashErrorMessage += FString::Printf(TEXT("GErrorExceptionDescription = \n%s\n"), GErrorExceptionDescription);
	//CrashErrorMessage += FString::Printf(TEXT("GErrorMessage = \n%s"), GErrorMessage);

	// Only register game thread crashes
	const bool bIsInGameThread = IsInGameThread();
	if (USoAnalytics* Analytics = GameInstance->GetAnalytics())
	{
		if (bIsInGameThread)
		{
			Analytics->RecordErrorEvent(ESoAnalyticsErrorType::EAE_Critical, CrashErrorMessage);
			Analytics->FlushEventsAndWait(true);
		}

		// Also log to the file
		UE_LOG(LogSoCrashHandler, Error, LINE_TERMINATOR LINE_TERMINATOR TEXT("=== USoGameInstance CRASH HANDLE ===") LINE_TERMINATOR TEXT("%s"), *CrashErrorMessage);
		if (GLog)
			GLog->PanicFlushThreadedLogs();
	}

	if (!bFromEnsure)
	{
		bSentCrashReport = true;
	}
	CrashErrorMessage.Empty();

	if (bIsInGameThread)
		OpenCrashFeedbackBrowserAndExplorer();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCrashHandler::OpenCrashFeedbackBrowserAndExplorer()
{
	// DISABLED as we just send the data to a third party
	return;

	// Don't open anything if we are at conference running a video demo
#if !WARRIORB_WITH_EDITOR && !WARRIORB_WITH_VIDEO_DEMO

	FString FeedbackURL;
	if (USoPlatformHelper::IsDemo())
	{
		if (WARRIORB_WITH_STEAM)
			FeedbackURL = USoGameSingleton::Get().FeedbackURLDemoSteam;
		else
			FeedbackURL = USoGameSingleton::Get().FeedbackURLDemo;
	}
	else
	{
		// Main Game
		if (WARRIORB_WITH_STEAM)
			FeedbackURL = USoGameSingleton::Get().FeedbackURLDemoSteam;
		else
			FeedbackURL = USoGameSingleton::Get().FeedbackURL;
	}

	USoPlatformHelper::LaunchURL(FeedbackURL, true);

	// Open Warriorb/ Folder
	static const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	USoPlatformHelper::ExploreFolder(ProjectDir);
#endif
}
