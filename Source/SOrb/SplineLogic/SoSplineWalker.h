// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "UObject/Interface.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoSplineWalker.generated.h"

/**
*
*/
UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoSplineWalker : public UInterface
{
	GENERATED_UINTERFACE_BODY()

};

/**
*  Interface for everything moving on the spline system
*  both C++ and Blueprint classes can implement and use the interface
*/
class SORB_API ISoSplineWalker
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = SplineMovement)
	FSoSplinePoint GetSplineLocationI() const;

	// does not modify actor location, only the registered spline location
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = SplineMovement)
	void SetSplineLocation(const FSoSplinePoint& SplinePoint, bool bUpdateOrientation);

	// called after a kinematic process modified the location of the actor
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = SplineMovement)
	void OnPushed(const FVector& DeltaMovement, float DeltaSeconds, bool bStuck, AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage);
};
