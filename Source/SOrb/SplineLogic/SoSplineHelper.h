// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "SplineLogic/SoSplinePointPtr.h"
#include "SoSplineHelper.generated.h"


struct FHitResult;


UCLASS()
class SORB_API USoSplineHelper : public UObject
{
	GENERATED_BODY()

public:

	USoSplineHelper(const FObjectInitializer& ObjectInitializer) {};
	~USoSplineHelper() {};

	UFUNCTION(BlueprintPure, Category = "SoCharacter")
	static FSoSplinePoint ExtractSplinePointPtr(const FSoSplinePointPtr& SplinePointPtr);

	UFUNCTION(BlueprintCallable, Category = "SoCharacter")
	static void SetSplinePointFromPtr(UPARAM(Ref)FSoSplinePoint& SplineLocation, const FSoSplinePointPtr& SplinePointPtr);

	UFUNCTION(BlueprintCallable, Category = SplinePoint)
	static FSoSplinePointPtr ConstructSplinePointPtr(const FSoSplinePoint& SplineLocation);

	UFUNCTION(BlueprintCallable, Category = "SoCharacter")
	static void SetSplineLocationReferenceZ(UPARAM(Ref)FSoSplinePoint& SplineLocation, float ReferenceZ);

	UFUNCTION(BlueprintPure, Category = "SoCharacter")
	static FVector GetRandomLocationNearSplineLocation(UPARAM(Ref)FSoSplinePoint& SplineLocation, float Range, float LocationZ);

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	static int32 GetDirectionModifierFromVector(const FSoSplinePoint& SplineLocation, const FVector& Vector)
	{
		return SplineLocation.GetDirectionModifierFromVector(Vector);
	}

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	static FVector GetDirectionFromVector(const FSoSplinePoint& SplineLocation, const FVector& Vector)
	{
		return SplineLocation.GetDirectionFromVector(Vector);
	}

	/**
	 *  Function used to place/translate spline based actors in the EDITOR, should not be used in-game
	 *  Works only if bSplinePosCorrectionEnabled is true (there is an editor button to toggle it)
	 *
	 *  Press and hold E: actor can move to another spline
	 *  Press and hold Q: actor spline location won't be updated (actor can move freely)
	 *
	 *  Actor: the actor we would like to place
	 *  SplineLocationPtr: safe way to store the spline ptr
	 *  bPlayerSplineOnly: if it has to consider ASoPlayerSplines only (objects can use other type of splines, e.g. spline mesh - rail)
	 *  bIgnoreZ: if true the z coordinates won't take into account when the closest spline is selected
	 *  bForcedCanModifySpline: spline can be modified even if E isn't pressed
	 */
	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static void UpdateSplineLocationRef(AActor* Actor,
										UPARAM(Ref)FSoSplinePointPtr& SplineLocationPtr,
										bool bPlayerSplineOnly = true,
										bool bIgnoreZ = true,
										bool bForcedCanModifySpline = false);

	// clamps spline distance based on spline length
	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static void ClampSplinePoint(UPARAM(Ref)FSoSplinePoint& SplinePoint);

	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static void AddToSplinePoint(UPARAM(Ref)FSoSplinePoint& SplinePoint, float Delta) { SplinePoint += Delta; }

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	static FSoSplinePoint GetOffsetedSplinePoint(const FSoSplinePoint& SplinePoint, float Delta);

	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static FVector GetWorldLocationFromSplinePoint(const FSoSplinePoint& SplinePoint);

	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static FVector GetWorldLocationFromSplinePointZ(const FSoSplinePoint& SplinePoint, float ZValue);

	/** Use this rotation in source to look at target in spline space (along spline), spline distance is limited to 10000 in search */
	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static FRotator GetSplineLookAtRotation(const FSoSplinePoint& Source, const FSoSplinePoint& Target);

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	static int32 GetSplineLookAtSign(const FSoSplinePoint& Source, const FSoSplinePoint& Target);

	// calculates the 3d starter velocity from the components
	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static FVector GetSplineVelocity(const FSoSplinePoint& SplinePoint, float SplineDirVelocity, float ZVelocity);

	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static FVector2D GetDirVec2D(const FSoSplinePoint& StartSP, float StartZ, const FSoSplinePoint& EndSP, float EndZ);

	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static bool CheckSplinePoint(const FSoSplinePoint& SplinePoint, bool bPrintWarnings);

	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static float GetDistanceBetweenSplinePoints(const FSoSplinePoint& First, const FSoSplinePoint& Second);

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	static bool IsSplinepointBetweenPoints(const FSoSplinePoint& Point, const FSoSplinePoint& Start, const FSoSplinePoint& End);

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	static float ClampOffsetBetweenSplinePoints(const FSoSplinePoint& Source,
												const FSoSplinePoint& Border0,
												const FSoSplinePoint& Border1,
												float Offset);

	/** bForward is based on ReferenceDirection */
	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	static void IsSplinePointTargetInRange(const FSoSplinePoint& Source,
										   const FSoSplinePoint& Target,
										   const FVector& ReferenceDirection,
										   float RangeDistance,
										   bool& OutForward,
										   bool& OutInRange);

	/**
	* ActorToPush is expected to be a ISplineWalker
	* he is moved with DeltaMovement in real life, and he tries to follow it along spline
	* if HitResult == nullptr then there is no sweep
	* @return: false: actor has no valid spline location
	*/
	static bool PushSplineWalker(AActor* ActorToPush,
								 const FVector& DeltaMovement,
								 float DeltaSeconds,
								 FHitResult* HitResult,
								 AActor* RematerializeLocation,
								 int32 DamageAmountIfStuck,
								 bool bForceDamage);

	static bool IsSplinePosCorrectionEnabled() { return bSplinePosCorrectionEnabled; }
	static void SetSplinePosCorrection(bool bNewValue) { bSplinePosCorrectionEnabled = bNewValue; }

private:
	static bool bSplinePosCorrectionEnabled;

};
