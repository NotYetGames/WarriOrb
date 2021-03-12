// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "SoUICharacterPreview.generated.h"

/**
 *
 */
UCLASS()
class SORB_API USoUICharacterPreview : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetColorMultiplier(const FLinearColor& Color)
	{
		ColorMultiplier = Color;
		SoImage->GetDynamicMaterial()->SetVectorParameterValue(ColorMultiplierName, ColorMultiplier);
	}

	void SetActivePercent(float Value)
	{
		ActivePercent = Value;
		SoImage->GetDynamicMaterial()->SetScalarParameterValue(ActivePercentName, ActivePercent);
	}

	const FLinearColor& GetColorMultiplier() const { return ColorMultiplier; }

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = CharPreview)
	void UpdateFromPlayerCharacter(FName Emotion = NAME_None);

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* SoImage;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Options")
	FLinearColor ColorMultiplier = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	float ActivePercent = 1.0f;

	const FName ColorMultiplierName = FName("ColorMultiplier");
	const FName ActivePercentName = FName("ActivePercent");
};
