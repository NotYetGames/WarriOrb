// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoUIUserWidget.h"
#include "SoUIExternalLink.h"

#include "SoUISteamExternalLink.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;


UCLASS()
class SORB_API USoUISteamExternalLink : public USoUIExternalLink
{
	GENERATED_BODY()
public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// USoUIExternalLink interface
	//
	void OpenLink() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Steam")
	int32 ExternalAppId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Steam")
	bool bAddToCart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Steam")
	bool bShowCart = true;
};
