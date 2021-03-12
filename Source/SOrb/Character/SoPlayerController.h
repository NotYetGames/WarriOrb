// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "GameFramework/PlayerController.h"

#include "Settings/Input/SoInputSettingsTypes.h"

#include "SoPlayerController.generated.h"

class USoGameSettings;

DECLARE_LOG_CATEGORY_EXTERN(LogSoPlayerController, All, All);

// if false most likely it is a keyboard
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoOnDeviceTypeChanged, ESoInputDeviceType, DeviceType);

// Custom player controller class
UCLASS()
class SORB_API ASoPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	//
	// APlayerController interface
	//

	/**
	 * Spawn the appropriate class of PlayerInput.
	 * Only called for player controllers that belong to local players.
	 */
	void InitInputSystem() override;

	// get audio listener position and orientation
	void GetAudioListenerPosition(FVector& OutLocation, FVector& OutFrontDir, FVector& OutRightDir) override;

	// Handles a key press
	bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;
	bool InputTouch(uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, float Force, FDateTime DeviceTimestamp, uint32 TouchpadIndex) override;
	bool InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad) override;
	bool InputMotion(const FVector& Tilt, const FVector& RotationRate, const FVector& Gravity, const FVector& Acceleration) override;
	bool SetPause(bool bPause, FCanUnpause CanUnpauseDelegate = FCanUnpause()) override;

	// Cheats
	void AddCheats(bool bForce = false) override;
	void EnableCheats() override;

	/**
	 * Processes player input (immediately after PlayerInput gets ticked) and calls UpdateRotation().
	 * PlayerTick is only called if the PlayerController has a PlayerInput object. Therefore, it will only be called for locally controlled PlayerControllers.
	 */
	void PlayerTick(float DeltaTime) override;

	// Input console command
	FString ConsoleCommand(const FString& Command, bool bWriteToLog = true) override;

	//
	// Own methods
	//

	UFUNCTION(BlueprintPure, DisplayName = "Get So Player Controller", Category = PlayerController, meta = (WorldContext = "WorldContextObject"))
	static ASoPlayerController* GetInstance(const UObject* WorldContextObject);

	// Enable/Disable mouse support
	// NOTE: You should probably use the hint bAllowMouse instead of this
	void SetIsMouseEnabled(bool bEnabled);

	// Allows the mouse to show if it is moved
	void SetIsMouseAllowed(bool bAllow) { bAllowMouse = bAllow; }
	bool IsMouseAllowed() const { return bAllowMouse; }

	float GetTimeSecondsSinceLastInput() const { return TimeSecondsSinceLastInput; }

	// Is the last input from the gamepad?
	bool IsLastInputFromGamepad() const { return bIsLastInputFromGamepad; }

	// Gets the current user device type, also queries the user settings if input is from gamepad
	ESoInputDeviceType GetCurrentDeviceType() const;
	ESoInputDeviceType GetGamepadDeviceTypeFromSettings() const;

	// Notify us that the user settings UI type changed from the settings
	// NOTE: should only be called from settings
	void NotifyUserSettingsGamepadUITypeChanged(bool bForce = false);

	// Device was changed
	FSoOnDeviceTypeChanged& OnDeviceTypeChanged() { return DeviceTypeChanged; }

	FORCEINLINE void ResetTimeSinceLastInput() { TimeSecondsSinceLastInput = 0.f; }
	//FORCEINLINE bool IsUsingRightThumbStick() const { return PressedRightStickKeys.Num() > 0; }

	FORCEINLINE void SetListenerLerpValue(float NewValue) { ListenerLerpValue = FMath::Clamp(NewValue, 0.0f, 1.0f); }

	void HandleNewGamepadInput();

	int32 GetLastJoystickIndexUsed() const { return LastJoystickIndexUsed; }

	// Force device type, useful for debugging
	void EnableForcedDeviceType(ESoInputDeviceType InDeviceType);
	void DisableForcedDeviceType();

protected:
	void UpdateLastInputState(bool bFromGamepad);
	void BroadcastDeviceChanged(ESoInputDeviceType NewDevice);

	void SetInputModeForGamepad(bool bEnable);
	
protected:
	// Tells us where the last input came from
	UPROPERTY(BlueprintReadOnly, Category = ">Input")
	bool bIsLastInputFromGamepad = false;

	// Tells us the time passed since last input from the user keyboard/mouse/
	// NOTE: this only applies for when the game is not paused
	UPROPERTY(BlueprintReadOnly, Category = ">Input")
	float TimeSecondsSinceLastInput = 0.f;

	// Event that tells if the input device changed
	UPROPERTY(BlueprintAssignable, Category = ">Input")
	FSoOnDeviceTypeChanged DeviceTypeChanged;

	// Keeps track of the pressed right stick actions
	//TSet<FName> PressedRightStickKeys;

	// Contains the last device type broadcasted.
	UPROPERTY()
	ESoInputDeviceType LastDeviceTypeBroadcasted = ESoInputDeviceType::Keyboard;

	// The SDL JoystickIndex of the last game controller used.
	UPROPERTY()
	int32 LastJoystickIndexUsed = INDEX_NONE;

	/** 0.0f: character position, 1.0f: camera position */
	UPROPERTY()
	float ListenerLerpValue = 0.5f;

	// Cache of variable
	UPROPERTY()
	USoGameSettings* GameSettings = nullptr;

	// Used to force the debug type for debugging
	UPROPERTY()
	ESoInputDeviceType ForcedDeviceType = ESoInputDeviceType::Gamepad_Generic;
	UPROPERTY()
	bool bUseForcedDeviceType = false;

	// Last mouse  position
	float LastMouseX = 0.f;
	float LastMouseY = 0.f;

	// Mouse was moved this frame
	bool bWasMouseMovedThisFrame = false;

	// Allows the mouse to show once it is moved and last input is not from gamepad
	bool bAllowMouse = false;

	// Keep track of input mode
	bool bInputModeForGamepad = false;
};
