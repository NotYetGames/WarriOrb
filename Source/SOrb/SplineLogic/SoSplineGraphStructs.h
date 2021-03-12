// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreTypes.h"
#include "UObject/ObjectMacros.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoSplineGraphStructs.generated.h"


UENUM(meta=(ScriptName="SoSplineGraphEdgeType"))
enum class ESoSplineGraphEdge : uint8
{
	// basic edge, undirected
	ESGE_Walk	UMETA(DisplayName = "Walk"),

	// directional "walk off a cliff" edge
	ESGE_Fall	UMETA(DisplayName = "Fall")
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoSplineGraphEdge
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(VisibleAnywhere)
	int32 StartNodeIndex;

	UPROPERTY(VisibleAnywhere)
	int32 EndNodeIndex;

	UPROPERTY(VisibleAnywhere)
	float Length;

	UPROPERTY(VisibleAnywhere)
	float MaxHeight;

	UPROPERTY(VisibleAnywhere)
	ESoSplineGraphEdge Type;

public:

	FLinearColor GetEdgeColor() const;

	bool IsUndirected() const { return Type == ESoSplineGraphEdge::ESGE_Walk; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// int array wrapper so it can be placed inside a container
USTRUCT(BlueprintType)
struct FSoIntArray
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<int32> Elements;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoSplineGraphNode
{
	GENERATED_USTRUCT_BODY()

public:

	FSoSplineGraphNode() {};

	FSoSplineGraphNode(const FSoSplinePoint& InSplinePoint, float InZValue);

	FVector ToVector() const { return SplinePoint.ToVector(ZValue); }

public:

	UPROPERTY(VisibleAnywhere)
	FSoSplinePoint SplinePoint;

	UPROPERTY(VisibleAnywhere)
	float ZValue = 0.f;

	UPROPERTY(VisibleAnywhere)
	TArray<int32> EdgeIndices;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// used only inside the spline graph generation
// should not be stored as it references actors but it is not UStruct
// e.g. points are tested when edge is under construction
struct FSoSplineGraphPoint
{

public:

	FVector ToVector() const { return SplineLoc.ToVector(FloorZ); }

	FVector ToVector(float ZOffset) const { return SplineLoc.ToVector(FloorZ) + FVector(0.0f, 0.0f, ZOffset); }

public:

	// spline location of given point
	FSoSplinePoint SplineLoc;
	// Z coordinate of the floor in given point
	float FloorZ;
	// max height of walkable area at given point
	float Height;
};
