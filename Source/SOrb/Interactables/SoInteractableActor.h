// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "SoInteractable.h"
#include "SoInteractableActor.generated.h"

UCLASS()
class SORB_API ASoInteractableActor : public AActor, public ISoInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoInteractableActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Interact_Implementation(ASoCharacter* Character) override { OnInteract(Character); }
	virtual FText GetInteractionText_Implementation() const override { return DisplayText; };
	virtual FSoInteractableMessage GetInteractionMessage_Implementation() const override { return Message; }
	virtual bool IsExclusive_Implementation() const override { return false; }
	virtual bool IsSecondKeyPrefered_Implementation() const override { return bSecondKeyPreferred; }
	virtual bool CanBeInteractedWithFromAir_Implementation() const override { return bCanBeInteractedWithFromAir; }

	UFUNCTION(BlueprintImplementableEvent, Category = Interaction)
	void OnInteract(ASoCharacter* pCharacter);

	UFUNCTION(BlueprintCallable, Category = Interaction)
	void Activate();

	UFUNCTION(BlueprintCallable, Category = Interaction)
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = Interaction)
	bool IsActive() const { return bActive; }

	// overlap callbacks
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:

	UFUNCTION(BlueprintNativeEvent, Category = Interaction)
	void OnReload();

	UFUNCTION(BlueprintNativeEvent, Category = Interaction)
	bool ShouldBeRegistredAsInteractable();

	UFUNCTION(BlueprintImplementableEvent, Category = Interaction)
	void OnCharEntered();

	UFUNCTION(BlueprintImplementableEvent, Category = Interaction)
	void OnCharLeft();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBoxComponent* Collision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bActive;

	/** stored for serialization (changed state is serialized) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bActiveByDefault;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSerializeState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoInteractableMessage Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSecondKeyPreferred;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanBeInteractedWithFromAir;

private:
	TArray<ASoCharacter*> PlayersInArea;
};
