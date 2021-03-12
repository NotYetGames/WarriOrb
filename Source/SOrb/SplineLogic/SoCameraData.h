// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

// wth is wrong with u, unreal?
#include "SoSplinePoint.h"
#include "SoCameraData.generated.h"

UENUM()
enum class ESoCamInterpolationMethod : uint8
{
	// https://codeplea.com/simple-interpolation

	ECIM_Linear = 0		UMETA(DisplayName = "Linear"),
	ECIM_Cosine			UMETA(DisplayName = "Cosine"),
	ECIM_SmoothStep		UMETA(DisplayName = "SmoothStep"),
	ECIM_Acceleration	UMETA(DisplayName = "Acceleration"),
	ECIM_Deceleration	UMETA(DisplayName = "Deceleration"),

	ECIM_NumOf			UMETA(Hidden)
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoCamKeyNode
{
	GENERATED_USTRUCT_BODY()

public:
	FSoCamKeyNode();
	FSoCamKeyNode(const FSoSplinePoint& InSplinePoint);

	FSoCamKeyNode(float ArmLength, float DeltaYaw, float DeltaPitch, float MaxHorizontalOffset, FVector DeltaOffset);

	FRotator GetRotation() const;

	// angle of the camera modifies which button is forward and which is backward
	// it can be 0, then we should use the one we used last time
	int32 GetDirectionModifier() const;

	static FSoCamKeyNode GetInterpolatedCamData(const FSoCamKeyNode& First,
												const FSoCamKeyNode& Second,
												float FirstSplineDistance,
												float SecondSplineDistance,
												float DistanceOnSpline);

	// the location (spline position, world location, distance on spline, etc.) stays the same
	// the other parameters are copied from _Source
	void CopyParams(const FSoCamKeyNode& _Source, bool CopyDistance);

	/** compares the camera key related data, location and interpolation only data is ignored */
	bool CompareParams(const FSoCamKeyNode& _Source) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Distance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ArmLength = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DeltaYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DeltaPitch = -20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHorizontalOffset = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector DeltaOffset = FVector(0,0,0);

	// this data effects the edge after this point if there is any
	// if false, the spline curve's effect between the two key point will be ignored
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseSplineOnInterpolation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LockOffset = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESoCamInterpolationMethod InterpolationMethod = ESoCamInterpolationMethod::ECIM_SmoothStep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (DlgNoExport))
	FSoSplinePoint SplinePoint;

	// only in interpolated stuff, otherwise invalid
	FVector Position = FVector::ZeroVector;

	// only in interpolated stuff, otherwise invalid
	FRotator Rotation = FRotator::ZeroRotator;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoCamKeyNodeModifier
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DeltaYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bModifyYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 KeyIndex;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoCamNodeList
{
	GENERATED_USTRUCT_BODY()

public:
	FSoCamKeyNode GetInterpolatedCamData(float DistanceOnSpline, int32& InOutIndex, bool bForceSplineBased = false) const;

	// rescale the CamKey positions based on the last key's position (it should be == with the SplineLength)
	void ReScale(float SplineLength);

	void CreateNewCamKey(const FSoSplinePoint& SplinePoint, int32 CamKeyIndex);

	void Modify(const FSoCamKeyNodeModifier& Modifier);

private:

	FVector GetPosition(int32 CamKeyIndex) const;
	FRotator GetRotation(int32 CamKeyIndex) const;
	bool CalcAndCheckLockOffset(int32 CamKeyIndex, int32& OutIndex) const;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (DlgWriteIndex))
	TArray<FSoCamKeyNode> CamKeys;
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoWorldCamData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	TMap<FString, FSoCamNodeList> Data;
};
