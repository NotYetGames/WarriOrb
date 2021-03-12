// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Components/BoxComponent.h"
#include "SoInteractableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoInteractableNotify);

class ASoCharacter;

/**
 *  Can be used by actors implementing the ISoInteractable interface
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SORB_API USoInteractableComponent : public UBoxComponent
{
	GENERATED_BODY()

public:

	USoInteractableComponent();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = Interaction)
	void ActivateInteractable();

	UFUNCTION(BlueprintCallable, Category = Interaction)
	void DeactivateInteractable();

	//UFUNCTION(BlueprintCallable, Category = Interaction)
	bool IsActive() const { return bActive; }

	// overlap callbacks
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:

	/** called when the character enteres the interactable area, but only if it is active */
	UPROPERTY(BlueprintAssignable)
	FSoInteractableNotify OnCharOverlapBegin;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bActive;

	/** pointer to character, only valid if he is inside the area */
	UPROPERTY(BlueprintReadOnly)
	ASoCharacter* SoCharacter;
};
