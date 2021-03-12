// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "SoPreviewCharacter.generated.h"


class USkeletalMesh;
class USkeletalMeshComponent;
class UMaterialInterface;
class USkeletalMeshComponent;


/** class used to setup any character pose with clothes/weapons */
UCLASS()
class SORB_API ASoPreviewCharacter : public AActor
{
	GENERATED_BODY()
	
public:	
	ASoPreviewCharacter();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:

	void SetMeshMaterialSafe(USkeletalMeshComponent* MeshComponent, USkeletalMesh* Mesh, UMaterialInterface* Material);

protected:

	UPROPERTY(EditAnywhere, Category = Preview)
	UStaticMesh* LeftItemMesh;

	UPROPERTY(EditAnywhere, Category = Preview)
	UMaterialInterface* LeftItemMaterial;

	UPROPERTY(EditAnywhere, Category = Preview)
	UStaticMesh* RightItemMesh;

	UPROPERTY(EditAnywhere, Category = Preview)
	UMaterialInterface* RightItemMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview)
	USkeletalMeshComponent* SoCharMesh;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview)
	UStaticMeshComponent* SoLeftItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview)
	UStaticMeshComponent* SoRightItem;
};
