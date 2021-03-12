// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoPlayerController.h"

#include "Engine/Engine.h"
#include "Camera/CameraComponent.h"

#include "Basic/SoGameInstance.h"
#include "Basic/SoCheatManager.h"
#include "Character/SoCharacter.h"
#include "Settings/SoGameSettings.h"
#include "Settings/Input/SoInputHelper.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "GameFramework/CheatManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/LocalPlayer.h"

#include "Online/SoOnlineHelper.h"

#include "SaveFiles/SoWorldState.h"


DEFINE_LOG_CATEGORY(LogSoPlayerController);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::InitInputSystem()
{
	Super::InitInputSystem();
	bShouldPerformFullTickWhenPaused = true;
	GameSettings = USoGameSettings::GetInstance();
	check(GameSettings);

	// Initialize the custom key mappings
	if (PlayerInput)
	{
		static const FString EmptyCommand;
#if WARRIORB_WITH_EDITOR
		// We need to reset it back
		PlayerInput->SetBind(TEXT("F1"), TEXT("viewmode wireframe"));
		PlayerInput->SetBind(TEXT("F9"), TEXT("shot showui"));
		PlayerInput->SetBind(TEXT("F5"), TEXT("viewmode ShaderComplexity"));
#else
		//PlayerInput->SetBind(TEXT("F1"), TEXT("viewmode CollisionPawn"));
		//PlayerInput->SetBind(TEXT("F9"), EmptyCommand);
		PlayerInput->SetBind(TEXT("F5"), EmptyCommand);
#endif // !WARRIORB_WITH_EDITOR

		GameSettings->TryAutoSetGamepadLayout();
		GameSettings->LoadSettings();
		GameSettings->RebuildInputKeymapsForPlayerInput(PlayerInput);

		// Is gamepad connect? prefer that over keyboard
		if (USoPlatformHelper::IsAnyGamepadConnected())
		{
			UpdateLastInputState(true);
		}

		// Set mouse
		SetInputModeForGamepad(false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::GetAudioListenerPosition(FVector& OutLocation, FVector& OutFrontDir, FVector& OutRightDir)
{
	FVector ViewLocation;
	FRotator ViewRotation;

	if (ASoCharacter* SoCharacter = Cast<ASoCharacter>(GetCharacter()))
	{
		if (GetViewTarget() == SoCharacter &&
			(PlayerCameraManager == nullptr || PlayerCameraManager->PendingViewTarget.Target == nullptr))
		{
			const FVector CharacterPosition = SoCharacter->GetActorLocation();
			const FTransform& CamTransform = SoCharacter->GetSideViewCameraComponent()->GetComponentTransform();

			OutLocation = FMath::Lerp(CharacterPosition, CamTransform.GetLocation(), ListenerLerpValue);
			OutFrontDir = CamTransform.GetUnitAxis(EAxis::X);
			OutRightDir = CamTransform.GetUnitAxis(EAxis::Y);

			return;
		}
	}

	GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FRotationTranslationMatrix ViewRotationMatrix(ViewRotation, ViewLocation);
	OutLocation = ViewLocation;
	OutFrontDir = ViewRotationMatrix.GetUnitAxis(EAxis::X);
	OutRightDir = ViewRotationMatrix.GetUnitAxis(EAxis::Y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoPlayerController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
#if PLATFORM_XBOXONE
	// check if we are in login screen where there are more players spawned so we can get the one which pressed the A button first
	// unfortunately otherwise we would not get input from controllers
	if (USoOnlineHelper::bWaitForFirstControllerInput)
	{
		if (Key != EKeys::Gamepad_FaceButton_Bottom)
			return true;

		USoGameInstance* SoGameInstance = USoGameInstance::GetInstance(this);
		if (!USoOnlineHelper::IsLocalPlayerSignedIn(GetLocalPlayer()))
		{
			// the controller belongs to a user who is not signed in (probably nobody is)
			// we just open the external UI
			USoOnlineHelper::ShowExternalLoginUI(SoGameInstance);
			return true;
		}

		// the controller belongs to this local player, we can destroy the other ones
		USoOnlineHelper::DestroyLocalPlayers(GetLocalPlayer());
		USoOnlineHelper::bWaitForFirstControllerInput = false;

		// just some debug log junk
		const auto OnlineSub = IOnlineSubsystem::Get();
		const auto IdentityInterface = OnlineSub->GetIdentityInterface();
		TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(GetLocalPlayer()->GetControllerId());
		UE_LOG(LogTemp, Warning, TEXT("SetCachedControllerNetID: %s"), *UserId->ToString());

		// we have to set the user index so we save/load from this user
		FSoWorldState::Get().SetUserIndex(GetLocalPlayer()->GetControllerId());

		// have to reload metadata for the new player because we need that in menu (to display continue and restart or only start play for prologue)
		FSoWorldState::Get().ReloadGameMetaData();

		// force reload menu, even if though we are already there, because everything might be messed up cause of the additional player spawn
		// optional solution would be to create a different playercontroller/map/gamemode for the selection screen, but meh
		SoGameInstance->TeleportToMainMenu(false, true);

		return true;
	}
#endif
	const bool bResult = Super::InputKey(Key, EventType, AmountDepressed, bGamepad);

	//const FName KeyName = Key.GetFName();
	//if (FSoInputKey::IsRightThumbStickDirection(KeyName))
	//{
	//	if (EventType == IE_Pressed)
	//		PressedRightStickKeys.Add(KeyName);
	//	else if (EventType == IE_Released)
	//		PressedRightStickKeys.Remove(KeyName);
	//}

	if (bResult)
	{
		ResetTimeSinceLastInput();
		UpdateLastInputState(Key.IsGamepadKey());
	}

	//UE_LOG(LogSoPlayerController, Warning, TEXT("ASoPlayerController::InputKey = %s"), *KeyName.ToString());
	return bResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoPlayerController::InputTouch(uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, float Force, FDateTime DeviceTimestamp, uint32 TouchpadIndex)
{
	const bool bResult = Super::InputTouch(Handle, Type, TouchLocation, Force, DeviceTimestamp, TouchpadIndex);
	// Meh, we don't handle touch devices
	// if (bResult)
	// {
	// 	ResetTimeSinceLastInput();
	// 	UpdateLastInputState(false);
	// }
	return bResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoPlayerController::InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	const bool bIsUsingAxis = Super::InputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad);

	// Actual motion, stops gamepad/mouse from pinging
	if (bIsUsingAxis)
	{
		ResetTimeSinceLastInput();
		UpdateLastInputState(Key.IsGamepadKey());
	}

	return bIsUsingAxis;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoPlayerController::InputMotion(const FVector& Tilt, const FVector& RotationRate, const FVector& Gravity, const FVector& Acceleration)
{
	const bool bResult = Super::InputMotion(Tilt, RotationRate, Gravity, Acceleration);
	if (bResult)
	{
		ResetTimeSinceLastInput();
		UpdateLastInputState(true);
	}
	return bResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::UpdateLastInputState(bool bFromGamepad)
{
	const bool bNotify = bIsLastInputFromGamepad != bFromGamepad;
	bIsLastInputFromGamepad = bFromGamepad;
	if (bNotify)
	{
		BroadcastDeviceChanged(GetCurrentDeviceType());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::BroadcastDeviceChanged(ESoInputDeviceType NewDevice)
{
	// Set the text labels properly for some gamepad key
	switch (NewDevice)
	{
	case ESoInputDeviceType::Gamepad_Xbox:
		EKeys::SetConsoleForGamepadLabels(EConsoleForGamepadLabels::XBoxOne);
		break;
	case ESoInputDeviceType::Gamepad_PlayStation:
		EKeys::SetConsoleForGamepadLabels(EConsoleForGamepadLabels::PS4);
		break;
	default:
		EKeys::SetConsoleForGamepadLabels(EConsoleForGamepadLabels::None);
		break;
	}

	LastDeviceTypeBroadcasted = NewDevice;
	DeviceTypeChanged.Broadcast(LastDeviceTypeBroadcasted);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoPlayerController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate)
{
	UE_LOG(LogSoPlayerController, Log, TEXT("SetPause = %d"), bPause);
	ResetTimeSinceLastInput();
	return Super::SetPause(bPause, CanUnpauseDelegate);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::AddCheats(bool bForce)
{
	Super::AddCheats(bForce);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::EnableCheats()
{
	// NOTE: don't use force
	AddCheats();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::PlayerTick(float DeltaTime)
{
	// if (!IsPaused())
	// {
	TimeSecondsSinceLastInput += DeltaTime;
	// }
	Super::PlayerTick(DeltaTime);

	// Get delta of mouse
	float MouseX = 0;
	float MouseY = 0;
	GetMousePosition(MouseX, MouseY);
	const float MouseDeltaX = LastMouseX - MouseX;
	const float MouseDeltaY = LastMouseY - MouseY;
	// GetInputMouseDelta(MouseDeltaX, MouseDeltaY);

	bWasMouseMovedThisFrame = !FMath::IsNearlyZero(MouseDeltaX) || !FMath::IsNearlyZero(MouseDeltaY);
	if (bIsLastInputFromGamepad)
	{
		// Gamepad: Hide mouse cursor
		SetInputModeForGamepad(true);
		SetIsMouseEnabled(false);
	}
	else if (bWasMouseMovedThisFrame)
	{
		// Keyboard: Sync state with allowed state
		SetInputModeForGamepad(false);
		if (bShowMouseCursor != bAllowMouse)
			SetIsMouseEnabled(bAllowMouse);
	}

	// UE_LOG(LogSoPlayerController, Verbose, TEXT("Mouse DeltaX = %f, DeltaY = %f"), MouseDeltaX, MouseDeltaY);
	LastMouseX = MouseX;
	LastMouseY = MouseY;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString ASoPlayerController::ConsoleCommand(const FString& Command, bool bWriteToLog)
{
	if (USoCheatManager::IsAllowCheatsPasswordValid(Command))
	{
		if (auto* GameInstance = USoGameInstance::GetInstance(this))
			GameInstance->SetAllowCheats(true);
	}

	return Super::ConsoleCommand(Command, bWriteToLog);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoPlayerController* ASoPlayerController::GetInstance(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
		return nullptr;

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		return Cast<ASoPlayerController>(World->GetFirstPlayerController());

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::SetIsMouseEnabled(bool bEnabled)
{
	if (bShowMouseCursor == bEnabled)
		return;

	bShowMouseCursor = bEnabled;
	bEnableClickEvents = bEnabled;
	bEnableTouchEvents = bEnabled;
	bEnableMouseOverEvents = bEnabled;
	bEnableTouchOverEvents = bEnabled;
	UE_LOG(LogSoPlayerController, Log, TEXT("SetIsMouseEnabled: bEnabled = %d"), bEnabled);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::SetInputModeForGamepad(bool bEnable)
{
	if (bInputModeForGamepad == bEnable)
		return;

	if (bEnable)
	{
		// Gamepad
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}
	else
	{
		// Keyboard
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}

	bInputModeForGamepad = bEnable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputDeviceType ASoPlayerController::GetCurrentDeviceType() const
{
#if PLATFORM_SWITCH || PLATFORM_XBOXONE
	// On these platforms only the gamepad type is supported
	check(USoPlatformHelper::HasHardcodedGamepad());
	return USoPlatformHelper::GetHardcodedGamepadType();
#else
	// Force
	if (bUseForcedDeviceType)
		return ForcedDeviceType;

	// Gamepad
	if (bIsLastInputFromGamepad)
	{
		// Desktop most likely return UI type from settings
		return GetGamepadDeviceTypeFromSettings();
	}

	// Default is keyboard
	return ESoInputDeviceType::Keyboard;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputDeviceType ASoPlayerController::GetGamepadDeviceTypeFromSettings() const
{
	// Get type from settings
	return USoInputHelper::ConvertInputGamepadUITypeToInputDeviceType(GameSettings->GetGamepadLayoutType());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::NotifyUserSettingsGamepadUITypeChanged(bool bForce)
{
	const ESoInputDeviceType NewDeviceType = GetCurrentDeviceType();
	if (bForce || NewDeviceType != LastDeviceTypeBroadcasted)
	{
		BroadcastDeviceChanged(NewDeviceType);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::EnableForcedDeviceType(ESoInputDeviceType InDeviceType)
{
	ForcedDeviceType = InDeviceType;
	bUseForcedDeviceType = true;
	BroadcastDeviceChanged(GetCurrentDeviceType());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::DisableForcedDeviceType()
{
	bUseForcedDeviceType = false;
	BroadcastDeviceChanged(GetCurrentDeviceType());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerController::HandleNewGamepadInput()
{
	if (GameSettings->IsAutoDetectGamepadLayout())
		GameSettings->AutoSetGamepadLayoutFromConnectedGamepad();
}
