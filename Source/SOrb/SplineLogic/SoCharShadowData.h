// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoCharShadowData.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoCharShadowKeyNode
{
	GENERATED_USTRUCT_BODY()

public:	
	FSoCharShadowKeyNode() {};

	FSoCharShadowKeyNode(float Z) :Distance(Z), Elevation(Z), Azimuth(Z), Strength(Z) {};

	void CopyValuesFrom(const FSoCharShadowKeyNode& Ref);

	static const FSoCharShadowKeyNode& GetInvalidKey();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Distance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Elevation = -0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Azimuth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Strength = 1.0f;
}; 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoCharShadowNodeList
{
	GENERATED_USTRUCT_BODY()

public:

	FSoCharShadowKeyNode GetInterpolated(const float DistanceOnSpline, int32& InOutIndex) const;

	// rescale the CamKey positions based on the last key's position (it should be == with the SplineLength)
	void ReScale(float SplineLength);

	// ehh
	int32 CreateNewKey(float Distance, int32 KeyIndex);

	int32 GetClosestIndex(float Distance) const;
	int32 GetPrevIndex(float Distance) const;
	int32 GetNextIndex(float Distance) const;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (DlgWriteIndex))
	TArray<FSoCharShadowKeyNode> Keys;
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoWorldCharShadowData
{
	GENERATED_USTRUCT_BODY()

public:
	
	UPROPERTY()
	TMap<FString, FSoCharShadowNodeList> Data;
};
