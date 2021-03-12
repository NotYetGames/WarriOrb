// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#if WARRIORB_WITH_ANALYTICS
#include "Interfaces/IAnalyticsProvider.h"
#endif // WARRIORB_WITH_ANALYTICS

/**
 * Mostly copied from AnalyticsBlueprintLibrary as we can't access it.
 * Not all methods are implemented:
 * 1. We do not need all
 * 2. GameAnalytics only supports some
 * 3. Safe way to record game analytics events
 */
class SORB_API FSoAnalyticsProvider
{
public:
#if WARRIORB_WITH_ANALYTICS
	// Gets the current analytics provider and outputs an error if nothing could be found
	static TSharedPtr<IAnalyticsProvider> GetDefaultConfiguredProvider(const FString& ContextErrorMessage);

	/** Starts an analytics session with custom attributes specified */
	static bool StartSessionWithAttributes(const TArray<FAnalyticsEventAttribute>& Attributes);

	/** Records an event has happened by name with a single attribute */
	static void RecordEventWithAttributes(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes);

	/** Records an in-game item was purchased with attributes */
	static void RecordItemPurchaseWithAttributes(const FString& ItemId, int32 ItemQuantity, const TArray<FAnalyticsEventAttribute>& Attributes);

	/** Records an error event has happened with attributes */
	static void RecordErrorWithAttributes(const FString& Error, const TArray<FAnalyticsEventAttribute>& Attributes);

	/** Records a user progress event has happened with a full list of progress hierarchy names and with attributes */
	static void RecordProgressWithFullHierarchyAndAttributes(const FString& ProgressType, const TArray<FString>& ProgressNames, const TArray<FAnalyticsEventAttribute>& Attributes);

	/** Records a user progress event has happened with attributes */
	static void RecordProgressWithAttributes(const FString& ProgressType, const FString& ProgressName, const TArray<FAnalyticsEventAttribute>& Attributes);
#endif // WARRIORB_WITH_ANALYTICS
	
	// Is there a default configured provider?
	FORCEINLINE static bool HasDefaultConfiguredProvider()
	{
#if WARRIORB_WITH_ANALYTICS
		return GetDefaultConfiguredProvider(TEXT("HasDefaultConfiguredProvider")).IsValid();
#else
		return false;
#endif
	}

	// Overrides the build version with our own
	static void InitializeBuildOverride();

	/** Starts an analytics session without any custom attributes specified */
	static bool StartSession();


	/** Ends an analytics session */
	static void EndSession();

	/** Flush all events. */
	static void FlushEvents();

	/** Sets all the custom dimensions values */
	static void SetCustomDimensions();

	/** Blocking. Waits for analytics to send */
	static void WaitForAnalyticsToSend();

	/** Sets the thread and event timers for the game analytics */
	static void SetPollTimers(double WaitSeconds, double ProcessEventsSeconds);

	/** Records an event has happened by name without any attributes (an event counter) */
	static void RecordEvent(const FString& EventName);

	/** Records an event has happened by name with a single attribute */
	static void RecordEventWithAttribute(const FString& EventName, const FString& AttributeName, const FString& AttributeValue);

	/** Gets the current session id from the analytics provider */
	static FString GetSessionId();

	/** Sets the session id (if supported) on the analytics provider */
	static void SetSessionId(const FString& SessionId);

	/** Gets the current user id from the analytics provider */
	static FString GetUserId();

	/** Sets the user id (if supported) on the analytics provider */
	static void SetUserId(const FString& UserId);

	/** Sets the game's build info (if supported) on the analytics provider */
	static void SetBuildInfo(const FString& BuildInfo);

	/** Records a user progress event has happened */
	static void RecordProgress(const FString& ProgressType, const FString& ProgressName);
};
