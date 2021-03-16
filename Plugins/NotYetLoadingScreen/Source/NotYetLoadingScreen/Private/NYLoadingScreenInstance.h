// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NYLoadingScreenSettings.h"
#include "INYLoadingScreenInstance.h"
#include "Containers/Ticker.h"

class UGameViewportClient;
class SVirtualWindow;
class SWidget;
class SDPIScaler;
class SBorder;
class SNYSimpleLoadingScreen;
class IGameMoviePlayer;

// Class created out of frustration because all loading screen tutorials online are retarded
// And FDefaultGameMoviePlayer is not much better
// TODO maybe add one day movie support
class NOTYETLOADINGSCREEN_API FNYLoadingScreenInstance : public INYLoadingScreenInstance, public TSharedFromThis<FNYLoadingScreenInstance>, public FTickerObjectBase
{
	typedef FNYLoadingScreenInstance Self;
public:
	static TSharedPtr<FNYLoadingScreenInstance> Get();
	virtual ~FNYLoadingScreenInstance();

	// INYLoadingScreenInstance interface
	void SetWaitForAnyKeyInput(bool bInValue, bool bFocus = true) override;
	bool IsWaitForAnyKeyInput() const override { return bWaitForAnyKeyInput; }
	void SetWaitForAnyKeyInputData(TSoftObjectPtr<UTexture2D> Image, FText Title, FText Description) override;

	void UpdateLoadingScreenTip(FText Tip) override;

	bool CanHideLoadingScreen() const override;
	void SetAutoShowLoadingScreen(bool bInValue) override;
	bool ShowLoadingScreen(bool bInPlayUntilStopped = true, float InPlayTime = -1.f) override;
	bool HideLoadingScreen() override;
	bool IsLoadingScreenVisible() const override  { return IsSlateLoadingScreenVisible() || IsMovieLoadingScreenVisible(); }
	void RedrawLoadingScreenWidget() override;

	void TestShowWidget() override;
	void TestHideWidget() override;

	void Initialize();

protected:
	static void Create()
	{
		check(IsInGameThread() && !IsInSlateThread());
		check(!Instance.IsValid());
		Instance = MakeShareable(new Self);
	}
	static void Destroy()
	{
		check(IsInGameThread() && !IsInSlateThread());
		Instance.Reset();
	}
	FNYLoadingScreenInstance();

	bool Tick(float DeltaTime) override;

	bool IsThisFocused() const;
	void FocusThis();
	void FocusReturnToUser();

	void Shutdown();
	UGameViewportClient* GetGameViewportClient() const;
	float GetViewportDPIScale() const { return 1.f; }

	// FCoreUObjectDelegates
	void OnPreLoadMap(const FString& LevelName);
	void OnPostLoadMap(UWorld* LoadedWorld);
	void OnPreExit();

	/** Callback for clicking on the viewport */
	FReply OnLoadingScreenKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);

	// Slate methods
	bool IsSlateLoadingScreenVisible() const;
	bool ShowSlateLoadingScreen();
	bool AddSlateWidgetToGameViewport();
	bool HideSlateLoadingScreen();

	// IGameMoviePlayer methods
	bool ShowMovieLoadingScreen();
	bool HideMovieLoadingScreen();
	bool IsMovieLoadingScreenVisible() const;
	void SetupMovieLoadingScreen();
	IGameMoviePlayer* GetSafeMoviePlayer() const;
	bool HasMoviePlayer() const;

	/** Called before playing a movie if the loading screen has not been prepared. */
	void HandleMoviePrepareLoadingScreen();

	/* Callback for when the LoadingScreen setup above in WidgetLoadingScreen is displayed **/
	void HandleMoviePlaybackStarted();
	void HandleMoviePlaybackFinished();
	void FinishMoviePlaying();
	void HandleMovieClipFinished(const FString& MovieName);

protected:
	// Was Initialized called
	bool bInitialized = false;

	//
	// User provided values
	//

	// Automatically show loading screen on world transition
	bool bAutoShowLoadingScreen = true;

	// Wait for movie playing screen
	bool bPlayUntilStopped = true;

	// How much to play?
	float PlayTime = -1.f;

	// Are we waiting for the user to press an input button before we can hide loading screen
	bool bWaitForAnyKeyInput = false;

	// User has initiated the show/hide
	bool bUserCalledShow = false;

	// Is the movie playing?
	bool bIsMoviePlaying = false;

	// Are we moving between worlds?
	bool bIsTransitioningWorlds = false;

	// Got something
	bool bGotAnyKeyInput = false;

	// Remember what the user had focused before
	TSharedPtr<SWidget> PreviousFocusedWidget = nullptr;

	// Delegates
	FDelegateHandle DelegatePrepareLoadingScreen;

	// The widget which includes all contents of the loading screen, widgets and movie player and all
	TSharedPtr<SWidget> WidgetLoadingScreenRootContainer = nullptr;

	// Keep a reference just to remove it
	TSharedPtr<SWidget> WidgetPtrGameViewport = nullptr;

	// DPIScaler parented to the UserWidgetHolder to ensure correct scaling
	TSharedPtr<SDPIScaler> WidgetDPIScaler = nullptr;

	// Container around WidgetLoadingScreen
	TSharedPtr<SBorder> WidgetLoadingScreenContainer = nullptr;

	// Hold our widget
	TSharedPtr<SNYSimpleLoadingScreen> WidgetLoadingScreen = nullptr;

	// Singleton handle
	static TSharedPtr<FNYLoadingScreenInstance> Instance;
};
