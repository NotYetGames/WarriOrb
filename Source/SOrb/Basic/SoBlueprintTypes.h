// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoBlueprintTypes.generated.h"

// Because I don't trust in BP structs
class UMaterialInstance;


/**
 * used to overwrite materials inside spline meshes
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoMaterialSlotOverride
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MeshIndex;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaterialSlotIndex;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UMaterialInstance* Material;
};



USTRUCT(BlueprintType, Blueprintable)
struct FSoMovementOverrideEntry
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MovementSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Duration;
};
