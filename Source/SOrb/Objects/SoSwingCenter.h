// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Components/SphereComponent.h"
#include "SoSwingCenter.generated.h"


UCLASS()
class SORB_API ASoSwingCenter : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoSwingCenter(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent)
	FVector GetSwingCenter();

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintImplementableEvent)
	void CharEnteredArea();

	UFUNCTION(BlueprintImplementableEvent)
	void CharLeftArea();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USphereComponent* Collision;
};
