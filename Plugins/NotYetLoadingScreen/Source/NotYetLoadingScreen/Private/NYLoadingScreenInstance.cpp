// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NYLoadingScreenInstance.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout//SDPIScaler.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SVirtualWindow.h"
#include "MoviePlayer.h"

#include "Widgets/SNYSimpleLoadingScreen.h"
#include "Widgets/SNYLoadingScreenDefaultBorder.h"
#include "Widgets/SWeakWidget.h"
#include "Containers/Ticker.h"

#if WITH_EDITOR
#include "Editor.h"
#endif // WITH_EDITOR

DEFINE_LOG_CATEGORY_STATIC(LogNYLoadingScreenInstance, All, All);


// Only enable movie player for some platform
//#define NY_USE_UNREAL_LOADING_SCREEN (PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_SWITCH || PLATFORM_XBOX_ONE)
#define NY_USE_UNREAL_LOADING_SCREEN 1

TSharedPtr<FNYLoadingScreenInstance> FNYLoadingScreenInstance::Instance = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedPtr<FNYLoadingScreenInstance> FNYLoadingScreenInstance::Get()
{
	if (!Instance.IsValid())
		Create();

	return Instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FNYLoadingScreenInstance::FNYLoadingScreenInstance()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FNYLoadingScreenInstance::~FNYLoadingScreenInstance()
{
	if (bInitialized)
	{
		// This should not happen if initialize was called correctly.  This is a fallback to ensure that the movie player rendering tickable gets unregistered on the rendering thread correctly
		//Shutdown();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::Initialize()
{
	if (bInitialized)
	{
		UE_LOG(LogNYLoadingScreenInstance, Warning, TEXT("Initialize: already initializeed"));
		return;
	}

	if (IsRunningDedicatedServer() || IsRunningCommandlet())
	{
		UE_LOG(LogNYLoadingScreenInstance, Warning, TEXT("Initialize: Can't, running in wrong context"));
		return;
	}

	FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer();
	if (!Renderer)
	{
		UE_LOG(LogNYLoadingScreenInstance, Warning, TEXT("Initialize: Can't get FSlateRenderer"));
		return;
	}

	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("Initialize"));
	GetMutableDefault<UNYLoadingScreenSettings>()->GetWorldTransitionScreen().AsyncLoad();
	bInitialized = true;

	// Listen to core events
	FCoreUObjectDelegates::PreLoadMap.AddRaw(this, &Self::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &Self::OnPostLoadMap);
	FCoreDelegates::OnPreExit.AddRaw(this, &Self::OnPreExit);

#if WITH_EDITOR
	FEditorDelegates::EndPIE.AddLambda([this](bool bIsSimulating)
	{
		// Reset the viewport because it will be invalid anyways
		WidgetPtrGameViewport.Reset();
	});
#endif // WITH_EDITOR

	// Listen to Movie player events
#if NY_USE_UNREAL_LOADING_SCREEN
	if (IsMoviePlayerEnabled())
	{
		IGameMoviePlayer* MoviePlayer = GetSafeMoviePlayer();

		if (bAutoShowLoadingScreen)
		{
			DelegatePrepareLoadingScreen = MoviePlayer->OnPrepareLoadingScreen().AddRaw(this, &Self::HandleMoviePrepareLoadingScreen);
		}

		MoviePlayer->OnMoviePlaybackStarted().AddRaw(this, &Self::HandleMoviePlaybackStarted);
		MoviePlayer->OnMoviePlaybackFinished().AddRaw(this, &Self::HandleMoviePlaybackFinished);
		MoviePlayer->OnMovieClipFinished().AddRaw(this, &Self::HandleMovieClipFinished);
	}
	else
	{
#if  WITH_EDITOR
		UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("Initialize: Movie player is not enabled!"));
#else
		UE_LOG(LogNYLoadingScreenInstance, Warning, TEXT("Initialize: Movie player is not enabled!"));
#endif
	}
#endif // NY_USE_UNREAL_LOADING_SCREEN

	// Prepare the startup screen, the PrepareLoadingScreen callback won't be called
	// if we've already explicitly setup the loading screen.
	// TODO: this seems to be useless if we load directly in the MainMenuLevel
	//if (Settings->bHasStartupScreen)
	//{
		//BeginLoadingScreen(Settings->StartupScreen);
	//}

	//
	// Create UI
	//

	// Create widgets
	WidgetLoadingScreenRootContainer =
		SNew(SNYLoadingScreenDefaultBorder)
		.OnKeyDown(this, &Self::OnLoadingScreenKeyDown)
		[
			//SNew(SOverlay)
			//+SOverlay::Slot()
			//[
				//SAssignNew(UserWidgetDPIScaler, SDPIScaler)
				//[
					SAssignNew(WidgetLoadingScreenContainer, SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("NoBorder")))
					.Padding(0)
				//]
			//]
		];

	RedrawLoadingScreenWidget();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::RedrawLoadingScreenWidget()
{
	// Reset and start again
	// WidgetLoadingScreen.Reset();
	// SAssignNew(WidgetLoadingScreen, SNYSimpleLoadingScreen, ScreenDescription);

	const FNYLoadingScreenDescription& ScreenDescription = GetMutableDefault<UNYLoadingScreenSettings>()->GetWorldTransitionScreen();
	if (WidgetLoadingScreen.IsValid())
	{
		WidgetLoadingScreen->SetWaitForAnyKeyInput(bWaitForAnyKeyInput);
		WidgetLoadingScreen->Redraw(ScreenDescription);
	}
	else
	{
		SAssignNew(WidgetLoadingScreen, SNYSimpleLoadingScreen, ScreenDescription);
		WidgetLoadingScreen->SetWaitForAnyKeyInput(bWaitForAnyKeyInput);
	}

	WidgetLoadingScreenContainer->SetContent(WidgetLoadingScreen.ToSharedRef());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::TestShowWidget()
{
	RedrawLoadingScreenWidget();
	AddSlateWidgetToGameViewport();
	WidgetLoadingScreenRootContainer->SetVisibility(EVisibility::Visible);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::TestHideWidget()
{
	WidgetLoadingScreenRootContainer->SetVisibility(EVisibility::Hidden);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::Shutdown()
{
	UE_LOG(LogNYLoadingScreenInstance, Verbose, TEXT("Shutdown"));

	//HideLoadingScreen();
	bInitialized = false;
	bIsMoviePlaying = false;
	bIsTransitioningWorlds = false;
	bWaitForAnyKeyInput = false;
	bGotAnyKeyInput = false;

	WidgetLoadingScreenRootContainer.Reset();
	WidgetLoadingScreenContainer.Reset();
	WidgetDPIScaler.Reset();
	WidgetLoadingScreen.Reset();
	WidgetPtrGameViewport.Reset();

	FCoreDelegates::OnPreExit.RemoveAll(this);
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	// Clear movie listeners
#if NY_USE_UNREAL_LOADING_SCREEN
	IGameMoviePlayer* MoviePlayer = GetSafeMoviePlayer();
	MoviePlayer->OnPrepareLoadingScreen().RemoveAll(this);
	MoviePlayer->OnMoviePlaybackStarted().RemoveAll(this);
	MoviePlayer->OnMoviePlaybackFinished().RemoveAll(this);
	MoviePlayer->OnMovieClipFinished().RemoveAll(this);
#endif

	DelegatePrepareLoadingScreen.Reset();

	Destroy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UGameViewportClient* FNYLoadingScreenInstance::GetGameViewportClient() const
{
	if (!GEngine)
		return nullptr;

	return GEngine->GameViewport;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::Tick(float DeltaTime)
{
	// Focus this widget to get input
#if !WITH_EDITOR
	if (bWaitForAnyKeyInput)
	{
		if (!IsThisFocused())
			FocusThis();
	}
#endif // !WITH_EDITOR

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::IsThisFocused() const
{
	if (!WidgetLoadingScreenRootContainer.IsValid())
		return false;

	TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
	return FocusedWidget == WidgetLoadingScreenRootContainer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::FocusThis()
{
	if (!bWaitForAnyKeyInput)
		return;

	if (!WidgetLoadingScreenRootContainer.IsValid())
		return;

	// NOTE: this is only for singleplayer games with one player
	if (IsThisFocused())
	{
		// Similar
		UE_LOG(LogNYLoadingScreenInstance, Warning, TEXT("FocusThis: We already have this widget focused. Ignoring."));
		return;
	}
	// TODO animation

	PreviousFocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(0);

	UE_LOG(
		LogNYLoadingScreenInstance,
		Verbose,
		TEXT("FocusThis: PreviousFocusedWidget = %s"),
		PreviousFocusedWidget.IsValid() ? *PreviousFocusedWidget->ToString() : TEXT("nullptr")
	);
	FSlateApplication::Get().SetAllUserFocus(WidgetLoadingScreenRootContainer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::FocusReturnToUser()
{
	//UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	//if (GameEngine && !GIsRequestingExit)
	//{
	//	GameEngine->SwitchGameWindowToUseGameViewport();
	//}

	if (PreviousFocusedWidget.IsValid())
	{
		FSlateApplication::Get().SetAllUserFocus(PreviousFocusedWidget);
	}
	else
	{
		UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("FocusReturnToUser: PreviousFocusedWidget is invalid, nothing to return to"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::ShowLoadingScreen(bool bInPlayUntilStopped, float InPlayTime)
{
	// TOOD use this bPlayUntilStopped
	bPlayUntilStopped = bInPlayUntilStopped;
	PlayTime = InPlayTime;

	verify(bInitialized);
	UE_LOG(LogNYLoadingScreenInstance, Verbose, TEXT("ShowLoadingScreen"));
	if (WidgetDPIScaler.IsValid())
	{
		WidgetDPIScaler->SetDPIScale(GetViewportDPIScale());
	}

	// Start movie if not already
	//ShowMovieLoadingScreen();
	ShowSlateLoadingScreen();

	bGotAnyKeyInput = false;
	SetWaitForAnyKeyInput(false, false);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::HideLoadingScreen()
{
	UE_LOG(LogNYLoadingScreenInstance, Verbose, TEXT("HideLoadingScreen (bUserCalledStart = %d)"), bUserCalledShow);
	if (!CanHideLoadingScreen())
	{
		UE_LOG(LogNYLoadingScreenInstance, Warning, TEXT("HideLoadingScreen: Can't get hide loading screen because we are waiting for user input. See SetWaitForAnyKeyInput."));
		return false;
	}

	// Hide Movie and slate
#if NY_USE_UNREAL_LOADING_SCREEN
	HideMovieLoadingScreen();
#endif
	HideSlateLoadingScreen();

	// Focus Return back to viewport
	FocusReturnToUser();

	bUserCalledShow = false;
	bGotAnyKeyInput = false;
	SetWaitForAnyKeyInput(false, false);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::OnPreLoadMap(const FString& LevelName)
{
	UE_LOG(LogNYLoadingScreenInstance, Verbose, TEXT("OnPreLoadMap: Map = %s"), *LevelName);
	bIsTransitioningWorlds = true;

	// NOTE: not needed here if we don't use our own slate implementation
	if (bAutoShowLoadingScreen)
	{
		UE_LOG(LogNYLoadingScreenInstance, Verbose, TEXT("OnPreLoadMap: ShowLoadingScreen()"), *LevelName);
		ShowLoadingScreen();
		bUserCalledShow = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::OnPostLoadMap(UWorld* LoadedWorld)
{
	UE_LOG(LogNYLoadingScreenInstance, Verbose, TEXT("OnPostLoadMap: Map = %s"), *LoadedWorld->GetMapName());
	bIsTransitioningWorlds = false;

	// Most likely we are in editor, anyways finish loading
#if NY_USE_UNREAL_LOADING_SCREEN
	if (!HasMoviePlayer())
	{
		FinishMoviePlaying();
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::OnPreExit()
{
	Shutdown();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply FNYLoadingScreenInstance::OnLoadingScreenKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	//UE_LOG(LogNYLoadingScreenInstance, Verbose, TEXT("OnLoadingScreenKeyDown: Key = %s"), *KeyEvent.GetKey().GetFName().ToString());
	// Stop alt tab bug
	const bool bIsBlacklistedKey = KeyEvent.GetKey() == EKeys::LeftAlt;
	if (bIsBlacklistedKey || bIsMoviePlaying || bIsTransitioningWorlds || !IsLoadingScreenVisible() || !IsThisFocused())
		return FReply::Handled();

	if (bWaitForAnyKeyInput)
	{
		bGotAnyKeyInput = true;
	}

	return FReply::Handled();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::SetWaitForAnyKeyInput(bool bInValue, bool bFocus)
{
	if (bWaitForAnyKeyInput == bInValue)
		return;

	bWaitForAnyKeyInput = bInValue;
	WidgetLoadingScreen->SetWaitForAnyKeyInput(bWaitForAnyKeyInput);

	// Focus our widget so what we accept input
	if (bFocus)
		FocusThis();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::SetWaitForAnyKeyInputData(TSoftObjectPtr<UTexture2D> Image, FText Title, FText Description)
{
	UNYLoadingScreenSettings* Settings = GetMutableDefault<UNYLoadingScreenSettings>();
	// TODO: if it is a reference here we must switch back the image
	FNYLoadingScreenDescription ScreenDescription = Settings->GetWorldTransitionScreen();

	// Replace first image
	if (ScreenDescription.Images.Num()/* && Image.IsValid()*/) // no is valid check cause maybe it is not loaded yet
		ScreenDescription.Images[0].Image = Image;

	ScreenDescription.Loading.TitleText.Text = Title;
	ScreenDescription.Loading.DescriptionText.Text = Description;
	ScreenDescription.AsyncLoad();
	// Settings->SaveConfig();
	WidgetLoadingScreen->Redraw(ScreenDescription);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::UpdateLoadingScreenTip(FText Tip)
{
	UNYLoadingScreenSettings* Settings = GetMutableDefault<UNYLoadingScreenSettings>();
	FNYLoadingScreenDescription ScreenDescription = Settings->GetWorldTransitionScreen();

	ScreenDescription.Loading.LoadingText.NormalText.Text = Tip;
	
	ScreenDescription.AsyncLoad();
	WidgetLoadingScreen->Redraw(ScreenDescription);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::CanHideLoadingScreen() const
{
	return bWaitForAnyKeyInput ? bGotAnyKeyInput : true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::SetAutoShowLoadingScreen(bool bInValue)
{
#if NY_USE_UNREAL_LOADING_SCREEN
	// TODO do we need this?
	checkNoEntry();
	bAutoShowLoadingScreen = bInValue;
	if (!HasMoviePlayer())
		return;

	if (bAutoShowLoadingScreen)
	{
		// Register
		if (!DelegatePrepareLoadingScreen.IsValid())
		{
			DelegatePrepareLoadingScreen = GetSafeMoviePlayer()->OnPrepareLoadingScreen().AddRaw(this, &Self::HandleMoviePrepareLoadingScreen);
		}
	}
	else
	{
		// Unregister
		GetSafeMoviePlayer()->OnPrepareLoadingScreen().Remove(DelegatePrepareLoadingScreen);
		DelegatePrepareLoadingScreen.Reset();
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::IsSlateLoadingScreenVisible() const
{
	return WidgetLoadingScreenRootContainer.IsValid() ? WidgetLoadingScreenRootContainer->GetVisibility() == EVisibility::Visible : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::ShowSlateLoadingScreen()
{
	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("ShowSlateLoadingScreen"));
	if (IsSlateLoadingScreenVisible())
	{
		UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("ShowSlateLoadingScreen: Loading screen already showing. Ignoring"));
		return true;
	}

	AddSlateWidgetToGameViewport();
	WidgetLoadingScreenRootContainer->SetVisibility(EVisibility::Visible);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::AddSlateWidgetToGameViewport()
{
	UGameViewportClient* GameViewport = GetGameViewportClient();
	if (!GameViewport)
	{
		UE_LOG(LogNYLoadingScreenInstance, Warning, TEXT("AddSlateWidgetToGameViewport: Can't get GameViewportClient"));
		return false;
	}

	// Add it for the first time
	// NOTE: in editor builds this also gets reset on PIE end
	// NOTE: if we don't reset it the focus won't work anymore
	// TODO: find out why the fucking gameviewport changes? This Reset is here so that we fix the focus issues
	RedrawLoadingScreenWidget();
	WidgetPtrGameViewport.Reset();
	// if (!WidgetPtrGameViewport.IsValid())
	{
		// NOTE: Don't ever remove the widget from the game viewport otherwise we can't focus it
		WidgetPtrGameViewport = SNew(SWeakWidget)
			.PossiblyNullContent(WidgetLoadingScreenRootContainer.ToSharedRef());
		GameViewport->AddViewportWidgetContent(WidgetPtrGameViewport.ToSharedRef(), 9999999);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::HideSlateLoadingScreen()
{
	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("HideSlateLoadingScreen"));
	if (!IsSlateLoadingScreenVisible())
	{
		UE_LOG(LogNYLoadingScreenInstance, Warning, TEXT("HideSlateLoadingScreen: Loading screen is not showing"));
		return false;
	}

	// Own loading screen
	WidgetLoadingScreenRootContainer->SetVisibility(EVisibility::Hidden);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::ShowMovieLoadingScreen()
{
#if NY_USE_UNREAL_LOADING_SCREEN
	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("ShowMovieLoadingScreen"));
	if (!HasMoviePlayer() || bIsMoviePlaying || bIsTransitioningWorlds)
		return false;

	// Setup loading screen first
	SetupMovieLoadingScreen();
	return GetSafeMoviePlayer()->PlayMovie();
#else
	return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::HideMovieLoadingScreen()
{
#if NY_USE_UNREAL_LOADING_SCREEN
	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("HideMovieLoadingScreen"));
	if (!HasMoviePlayer() || !bIsMoviePlaying || bIsTransitioningWorlds)
		return false;

	GetSafeMoviePlayer()->StopMovie();
	GetSafeMoviePlayer()->SetupLoadingScreen(FLoadingScreenAttributes{});
	return true;
#else
	return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::IsMovieLoadingScreenVisible() const
{
	if (!HasMoviePlayer())
		return false;

	return bIsMoviePlaying;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::SetupMovieLoadingScreen()
{
#if NY_USE_UNREAL_LOADING_SCREEN
	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("SetupMovieLoadingScreen"));
	FLoadingScreenAttributes LoadingScreen;
	if (bUserCalledShow)
	{
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = true; // !bPlayUntilStopped;
		LoadingScreen.bMoviesAreSkippable = false;
		LoadingScreen.bWaitForManualStop = false; // bPlayUntilStopped;
		LoadingScreen.MinimumLoadingScreenDisplayTime = PlayTime;

		// This may causes a crash if is set to true
		LoadingScreen.bAllowEngineTick = false; // bPlayUntilStopped;
	}
	else
	{
		// Automatic
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
		LoadingScreen.MinimumLoadingScreenDisplayTime = 1.f;
	}
	LoadingScreen.bAllowInEarlyStartup = false;
	LoadingScreen.WidgetLoadingScreen = WidgetLoadingScreenRootContainer; // SNullWidget::NullWidget;

	if (!LoadingScreen.IsValid())
	{
		UE_LOG(LogNYLoadingScreenInstance, Warning, TEXT("SetupMovieLoadingScreen: We don't have any valid FLoadingScreenAttributes attributes, configure it properly!"));
	}

	GetSafeMoviePlayer()->SetupLoadingScreen(LoadingScreen);
#endif // NY_USE_UNREAL_LOADING_SCREEN
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IGameMoviePlayer* FNYLoadingScreenInstance::GetSafeMoviePlayer() const
{
	IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
	verify(MoviePlayer);
	return MoviePlayer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FNYLoadingScreenInstance::HasMoviePlayer() const
{
	return GetMoviePlayer() != nullptr && IsMoviePlayerEnabled();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::HandleMoviePrepareLoadingScreen()
{
	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("HandleMoviePrepareLoadingScreen"));
	SetupMovieLoadingScreen();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::HandleMoviePlaybackStarted()
{
	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("HandleMoviePlaybackStarted"));
	bIsMoviePlaying = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::HandleMoviePlaybackFinished()
{
	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("HandleMoviePlaybackFinished"));
	bIsMoviePlaying = false;
	FinishMoviePlaying();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::FinishMoviePlaying()
{
	// Show loading screen after map transition
	if (IsSlateLoadingScreenVisible())
	{
		if (bUserCalledShow)
		{
			UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("FinishMoviePlaying: Showing loading screen again because user did not finish the loading."));
			ShowLoadingScreen();
		}
		else
		{
			// Hide our own implementation because it start was called automatically
			UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("FinishMoviePlaying: Hiding loading screen again because the movie loading screen finished"));
			HideLoadingScreen();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenInstance::HandleMovieClipFinished(const FString& MovieName)
{
	UE_LOG(LogNYLoadingScreenInstance, Log, TEXT("HandleMovieClipFinished: MovieName = %s"), *MovieName);
}

#undef NY_USE_UNREAL_LOADING_SCREEN
