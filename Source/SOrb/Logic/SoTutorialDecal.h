// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Settings/Input/SoInputNames.h"
#include "Logic/SoTriggerable.h"
#include "Basic/SoEventHandler.h"

#include "SoTutorialDecal.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UDecalComponent;
class USpotLightComponent;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct SORB_API FSoTutorialKeyParam
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D PressIntervals;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector RelativeLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESoInputActionNameType InputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUseSecondaryInputActionInsteadOfUnpressed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESoInputActionNameType SecondaryInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bDoublePress;
};


UCLASS()
class SORB_API ASoTutorialDecal : public AActor, public ISoEventHandler, public ISoTriggerable
{
	GENERATED_BODY()

public:
	ASoTutorialDecal();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;


	// SoEventHandler interface
	virtual void HandleSoPostLoad_Implementation() override;
	virtual void HandlePlayerRematerialize_Implementation() override {};
	virtual void HandlePlayerRespawn_Implementation() override {};
	virtual void HandleWhitelistedSplinesEntered_Implementation() override {};
	virtual void HandleWhitelistedSplinesLeft_Implementation() override {};
	// SoTriggerable interface
	void Trigger_Implementation(const FSoTriggerData& TriggerData) override;

protected:

	UFUNCTION(BlueprintCallable)
	void OnDeviceChanged(ESoInputDeviceType NewDeviceType);

	UFUNCTION(BlueprintCallable)
	void OnInputSettingsApplied();

	void Reinitialize();

	void SetScalarOnDecalSafe(UDecalComponent* Decal, FName Name, float Value);

	void Update();

	FVector GetDecalSize(bool bSquared) const;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UBillboardComponent* Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UArrowComponent* Arrow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UDecalComponent* DecalCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UDecalComponent* DecalKeyAnim0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UDecalComponent* DecalKeyAnim1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UDecalComponent* DecalKeyAnim2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USpotLightComponent* SpotLight;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalControl)
	bool bEnabledByDefault;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalControl)
	int32 TriggerBeforeFade;

	UPROPERTY()
	int32 TriggerCounter = 0;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalParam)
	float PeriodTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalParam)
	float KeyPeriodMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalParam)
	int32 RowNum = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalParam)
	int32 ColNum = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalParam)
	int32 UsedAtlasSlotCount = 64;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalParam)
	float Exponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalParam)
	UTexture2D* DecalTexture;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalParam)
	TArray<FSoTutorialKeyParam> KeyParamsKeyboard;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalParam)
	TArray<FSoTutorialKeyParam> KeyParamsController;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalDefaults)
	UMaterialInterface* BaseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalDefaults)
	UMaterialInterface* KeyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DecalDefaults)
	UMaterialInterface* KeyMaterialDoublePress;


	UPROPERTY(BlueprintReadOnly)
	ESoInputDeviceType CurrentDeviceType;

	float FadeTarget = 1.0f;
	float CurrentValue = 0.0f;
};
