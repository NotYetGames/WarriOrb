// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Components/BillboardComponent.h"

#include "Settings/Input/SoInputNames.h"
#include "Settings/Input/SoInputSettingsTypes.h"

#include "SoCommandImageBilboardComponent.generated.h"

class UTexture2D;

UCLASS(ClassGroup = Rendering, collapsecategories, hidecategories = (Object, Activation, "Components|Activation", Physics, Collision, Lighting, Mesh, PhysicsVolume), editinlinenew, meta = (BlueprintSpawnableComponent))
class SORB_API USoCommandImageBilboardComponent : public UBillboardComponent
{
	GENERATED_BODY()
	typedef USoCommandImageBilboardComponent Self;
public:
	//
	// UActorComponent interface
	//
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//
	// UBillboardComponent Interface
	//
	void SetSprite(UTexture2D* NewSprite) override;

	//
	// Own Methods
	//

	UFUNCTION(BlueprintCallable)
	void SetActionNameType(const ESoInputActionNameType InType);

	ESoInputActionNameType GetActionNameType() const { return ActionNameType; }
	bool IsActionNameTypeValid() const { return ActionNameType != ESoInputActionNameType::IANT_None; }
	void ResetActionNameType()
	{
		ActionNameType = ESoInputActionNameType::IANT_None;
	}

protected:
	UFUNCTION()
	void HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType);

	void SubscribeToDeviceChanged();
	void UnSubscribeFromDeviceChanged();
	void UpdateImage();

protected:
	// Mutual exclusive with UICommand
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">UI")
	ESoInputActionNameType ActionNameType = ESoInputActionNameType::IANT_None;

	// Cached Current device Type
	ESoInputDeviceType DeviceType = ESoInputDeviceType::Keyboard;

	// Keep track if we are listening or not
	bool bIsListeningToDevicesChanges = false;
};
