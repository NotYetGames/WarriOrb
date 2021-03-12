// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoPreviewCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Character/SoCharacter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoPreviewCharacter::ASoPreviewCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SoCharMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SoMesh"));
	RootComponent = SoCharMesh;
	auto SkelMesh = ConstructorHelpers::FObjectFinder<USkeletalMesh>(TEXT("/Game/SO/Assets/CharacterMeshMat/SOrbMesh.SOrbMesh"));
	SoCharMesh->SetSkeletalMesh(SkelMesh.Object);

	SoLeftItem = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoLeftItem"));
	SoLeftItem->SetupAttachment(SoCharMesh, "Item_L");
	SoLeftItem->SetRelativeLocation(FVector(-1.071995, 13.090923, -0.000017));
	SoLeftItem->SetRelativeRotation({ -0.000031, 0.000073, -89.999939 });

	SoRightItem = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoRightItem"));
	SoRightItem->SetupAttachment(SoCharMesh, "ItemAttachPoint");
	SoRightItem->SetRelativeLocation(FVector(-1.071995, 13.090923, -0.000017));
	SoRightItem->SetRelativeRotation({ -0.000031, 0.000073, -89.999939 });
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if WITH_EDITOR
void ASoPreviewCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SoLeftItem->SetStaticMesh(LeftItemMesh);
	if (SoLeftItem->GetMaterials().Num() > 0 && LeftItemMaterial != nullptr)
		SoLeftItem->SetMaterial(0, LeftItemMaterial);

	SoRightItem->SetStaticMesh(RightItemMesh);
	if (SoRightItem->GetMaterials().Num() > 0 && RightItemMaterial != nullptr)
		SoRightItem->SetMaterial(0, RightItemMaterial);
}
#endif



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPreviewCharacter::SetMeshMaterialSafe(USkeletalMeshComponent* MeshComponent, USkeletalMesh* Mesh, UMaterialInterface* Material)
{
	if (MeshComponent == nullptr)
		return;

	MeshComponent->SetSkeletalMesh(Mesh);
	if (MeshComponent->GetMaterials().Num() > 0 && Material != nullptr)
		MeshComponent->SetMaterial(0, Material);
}
