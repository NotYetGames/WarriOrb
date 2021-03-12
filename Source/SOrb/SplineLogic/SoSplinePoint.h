// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

#include "SoSplinePoint.generated.h"

class ASoSpline;
class USplineComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogSoSplineSys, Log, All);

/**
* A SplinePoint is a {point, direction} pair
* the point is identified by a Spline and a Distance value
* IMPORTANT UPDATE: the struct has a reference actor member variable
* the owner has to make sure that it is set and reserved correctly
* check ReferenceActor for more information
*/
USTRUCT(BlueprintType)
struct FSoSplinePoint
{
	GENERATED_USTRUCT_BODY()
public:

	FSoSplinePoint();
	FSoSplinePoint(ASoSpline* InSpline, float InDistance = 0.f);
	FSoSplinePoint(const FSoSplinePoint& TheOther);

	FSoSplinePoint operator+(float DeltaDistance) const;
	FSoSplinePoint operator-(float DeltaDistance) const { return *this + (-DeltaDistance); }

	FSoSplinePoint GetScaledSplinePoint(const float ScaleFactor) const { return FSoSplinePoint(Spline, Distance * ScaleFactor); }

	void ClampSplineData();

	void SetReferenceActor(AActor* Actor) { ReferenceActor = Actor; }
	void SetReferenceZ(float Z) { ReferenceZ = Z; }

	float GetReferenceZ() const;

	void operator=(const FSoSplinePoint& OtherPoint);

	// sets distance and spline based on other, but the reference actor is kept
	void CopySplineLocation(const FSoSplinePoint& OtherPoint);

	/**
	* Distance between two SplinePoints along the spline
	* The sign indicates the direction we have to go from the given spline point B to reach A if (A-B)
	* Same as GetDistanceFromSplinePoint()
	*/
	float operator-(const FSoSplinePoint& OtherPoint) const;

	// (A-B) implementation, because there was a time when the two function was different, and now I am too lazy to remove one of them
	// if it isn't in MaxAcceptedDistance, return value is inf(BIG_NUMBER)
	// The sign indicates the direction we have to go from the given spline point B to reach A if (A - B)
	float GetDistanceFromSplinePoint(const FSoSplinePoint& OtherPoint, float MaxAcceptedDistance = BIG_NUMBER) const;

	float GetDistanceFromSplinePointWithZSquered(const FSoSplinePoint& OtherPoint, float MaxAcceptedDistance = BIG_NUMBER) const;


	/**
	* Translates the point with an offset
	* the offset has to be small enough, it can not cause more than one spline switch
	* IMPORTANT: DeltaDistance will be "projected" to xy plane
	*/
	void operator+=(float DeltaDistance);

	/**
	* Translates the point with an offset
	* the offset has to be small enough, it can not cause more than one spline switch
	* if the end of the spline reached bOutSplineEnd is set to true, and OutRestDistance is set to the not applied part of the distance
	*/
	void ProjectAndAddToDistance(float DeltaDistance, bool& bOutSplineEnd, float& OutRestDistanceTime);
	void AddToDistance(const float Offset, bool& bOutSplineEnd, float& OutRestDistanceTime);

	void SetDistanceFromWorldLocation(const FVector& WorldLocation);

	void SetDistance(float InDistance);
	void SetDistanceFromInputKey(float InputKey);
	void SetSpline(ASoSpline* Spline);
	ASoSpline* GetSpline() { return Spline; }
	const ASoSpline* GetSpline() const { return Spline; }

	operator FVector() const;
	operator FVector2D() const;
	// has nothing to do with the point's direction, it just returns the rotation of the Spline at the distance defined by *this
	operator FRotator() const;

	FVector GetWorldLocation(float ZValue) const;

	FVector2D ToVector2D() const;
	FVector ToVector(float ZValue) const;

	float GetDistance() const { return Distance; }
	
	bool IsValid(bool bPrintWarning = true) const;
	float GetActiveSplineLength() const;
	
	/** Sets the spline to nullptr and resets all other values as well */
	void Invalidate();

	// normal of the tangential plane in the actual spline location
	FVector GetPlaneNormal() const;

	FVector GetDirection() const;
	// spline direction vector at point, maybe multiplied by -1 (to have the same orientation as V)
	FVector GetDirectionFromVector(const FVector V) const;
	// angle(Direction,V) < 180 ? 1 : -1
	int32 GetDirectionModifierFromVector(const FVector V) const;

	// search for closest spline point, starting from *this
	// sign(MaxDistance) determines the search direction
	FSoSplinePoint GetEstimatedSplineLocation(const FVector WorldLocation, const float MaxDistance) const;

protected:

	// distance from the startpoint of the spline the point located at
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Distance = 0.f;

	// the spline the point is located at
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ASoSpline* Spline = nullptr;

	// used to get a z location which helps to decide which neighbor spline comes next
	// maybe it would be nicer passing the z value as a parameter to the AddDistance() functions, but...
	// since operator+ and operator+= is already used all over the code...
	UPROPERTY()
	AActor* ReferenceActor = nullptr;

	/** used instead of ReferenceActor if the SplinePoint is not attached to an actor; value is never copied between spline points*/
	float ReferenceZ = 0.f;
};
