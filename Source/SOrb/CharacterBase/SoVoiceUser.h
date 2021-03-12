// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "SoVoiceUser.generated.h"


class USceneComponent;

UENUM()
enum class ESoVoiceType : uint8
{
	SoVoiceTypeIdle = 0		UMETA(DisplayName = "Idle"),
	SoVoiceTypeHitReact		UMETA(DisplayName = "HitReact"),
	SoVoiceTypeDeath		UMETA(DisplayName = "Death"),
	SoVoiceTypeAttack		UMETA(DisplayName = "Attack"),
	SoVoiceTypeNoise		UMETA(DisplayName = "Noise")
};


/**
*
*/
UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoVoiceUser : public UInterface
{
	GENERATED_UINTERFACE_BODY()

};


/**
 * interface for damageable entities
 * both C++ and Blueprint classes can implement and use the interface
 */
class SORB_API ISoVoiceUser
{
	GENERATED_IINTERFACE_BODY()

	/** Used for voice and text location */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Voice)
	USceneComponent* GetComponent() const;

	/** Used for voice and text location */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Voice)
	FName GetSocketName() const;

	/** depends on random if vo should be spawned or not */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Voice)
	bool ShouldSpawnVO(ESoVoiceType VoiceType) const;
};
