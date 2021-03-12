// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SoUICooldownEntry.generated.h"

class UImage;

 ////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoUICooldownEntry : public UUserWidget
{
	GENERATED_BODY()

protected:
	void NativeConstruct() override;

public:
	void Setup(int32 InIndex, const UObject* InObjectWithCooldown, float InRemainingTime);
	void Setup(const USoUICooldownEntry* Reference);
	void ClearAndHide();

	// return value true if it should be displayed
	bool Update();

	int32 GetIndex() const { return Index; }
	float GetRemainingTime() const { return RemainingTime; }
	const UObject* GetObjectWithCooldown() const { return ObjectWithCooldown; }

	UFUNCTION(BlueprintImplementableEvent)
	void OnBlockEvent();

protected:
	UPROPERTY()
	const UObject* ObjectWithCooldown;

	float RemainingTime;

	int32 Index;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* Line;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* Icon;

	const FName PercentName = FName("Percent");
};
