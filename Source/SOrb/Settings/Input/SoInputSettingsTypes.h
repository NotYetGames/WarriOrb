// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerInput.h"

#include "SoInputSettingsTypes.generated.h"

class UTexture2D;

// Used to check the integrity of FSoInputActionKeyMapping and FSoInputAxisKeyMapping
UENUM()
enum class ESoInputIntegrity : uint8
{
	Ok = 0,

	// The Default is not actually a default
	NotDefault,

	// Default.Name and Modified.Name mismatch
	NamesMismatch,

	// Default.Key and Modified.Key are not of the same type
	KeyCategoryMismatch
};


// Used internally only
UENUM(BlueprintType)
enum class ESoInputDeviceType : uint8
{
	Keyboard = 0			UMETA(DisplayName = "Keyboard"),
	Gamepad_Generic			UMETA(DisplayName = "Gamepad Generic"),
	Gamepad_Xbox			UMETA(DisplayName = "Xbox"),
	Gamepad_PlayStation		UMETA(DisplayName = "PlayStation"),
	Gamepad_Switch			UMETA(DisplayName = "Nintendo Switch"),

	Num						UMETA(Hidden)
};


USTRUCT(BlueprintType)
struct SORB_API FSoInputActionKeyMapping
{
	GENERATED_USTRUCT_BODY()
public:
	// The original key mapping, must keep track so that we know what exactly we modified, gamepad, keyboard or
	// if we just overwrote just one shortcut for an action name, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FInputActionKeyMapping Default;

	// Our own version, modified
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FInputActionKeyMapping Modified;

public:
	FName GetActionName() const { return Default.ActionName; }
	bool IsUnbinding() const { return !Modified.Key.IsValid(); }
	bool IsKeyboard() const;
	bool IsGamepad() const;

	bool operator==(const FSoInputActionKeyMapping& Other) const;
	bool operator<(const FSoInputActionKeyMapping& Other) const;
	FString ToString() const;
	ESoInputIntegrity ValidateIntegrity() const;
};

FORCEINLINE uint32 GetTypeHash(const FInputActionKeyMapping& Key)
{
	uint32 Hash = HashCombine(GetTypeHash(Key.ActionName), GetTypeHash(Key.Key));
	Hash = HashCombine(Hash, GetTypeHash(Key.bAlt + Key.bCmd + Key.bShift + Key.bCtrl));
	return Hash;
}


USTRUCT(BlueprintType)
struct SORB_API FSoInputAxisKeyMapping
{
	GENERATED_USTRUCT_BODY()
public:
	// The original axis mapping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FInputAxisKeyMapping Default;

	// Our own version, modified
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FInputAxisKeyMapping Modified;

public:
	bool IsKeyboard() const;
	bool IsGamepad() const;

	bool operator==(const FSoInputAxisKeyMapping& Other) const;
	bool operator<(const FSoInputAxisKeyMapping& Other) const;
	FString ToString() const;
	ESoInputIntegrity ValidateIntegrity() const;
};

FORCEINLINE uint32 GetTypeHash(const FInputAxisKeyMapping& Key)
{
	// NOTE we ignore scale
	return HashCombine(GetTypeHash(Key.AxisName), GetTypeHash(Key.Key));
}


// Keeps all the textures for the key name / UI command
USTRUCT()
struct FSoInputUITextures
{
	GENERATED_USTRUCT_BODY()

public:
	UTexture2D* GetTextureForDeviceType(const ESoInputDeviceType DeviceType) const;

public:
	UPROPERTY(EditAnywhere, Category = Input)
	UTexture2D* Keyboard = nullptr;

	UPROPERTY(EditAnywhere, Category = Input)
	UTexture2D* Xbox = nullptr;

	UPROPERTY(EditAnywhere, Category = Input)
	UTexture2D* PlayStation = nullptr;

	UPROPERTY(EditAnywhere, Category = Input)
	UTexture2D* Switch = nullptr;
};


// Defines the input key to display if for an ESoUICommmand
// For example EUC_MainMenuEnter can have enter and space, but maybe we want the second one which is index 1
// By default it uses the first index
USTRUCT(BlueprintType)
struct FSoInputUICommandPriorities
{
	GENERATED_USTRUCT_BODY()

public:
	FORCEINLINE bool IsValidKeyboard() const { return KeyboardPriority > INDEX_NONE; }
	FORCEINLINE bool IsValidGamepad() const { return GamepadPriority > INDEX_NONE; }

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Input)
	int32 KeyboardPriority = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Input)
	int32 GamepadPriority = 0;
};


// All the textures of a keyboard key
USTRUCT(BlueprintType)
struct FSoInputKeyboardKeyTextures
{
	GENERATED_USTRUCT_BODY()
public:
	UTexture2D* GetTexture(bool bPressed = true) const;

public:
	// Default state
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Input)
	UTexture2D* Pressed = nullptr;

	// Only in some case used
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Input)
	UTexture2D* Unpressed = nullptr;
};


// All the textures of a gamepad key
USTRUCT(BlueprintType)
struct FSoInputGamepadKeyTextures
{
	GENERATED_USTRUCT_BODY()
public:
	UTexture2D* GetTextureForDeviceType(const ESoInputDeviceType DeviceType) const;

	FORCEINLINE bool IsValid() const
	{
		return Xbox != nullptr && PlayStation != nullptr && Switch != nullptr;
	}

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Input)
	UTexture2D* Xbox = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Input)
	UTexture2D* PlayStation = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Input)
	UTexture2D* Switch = nullptr;
};
