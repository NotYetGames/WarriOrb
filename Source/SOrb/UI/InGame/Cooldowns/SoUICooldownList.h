// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SoUICooldownList.generated.h"

class UPanelWidget;

 ////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoUICooldownList : public UUserWidget
{
	GENERATED_BODY()

protected:
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void OnCooldownStarted(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown);

	UFUNCTION()
	void OnCooldownBlocksEvent(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown);

	UFUNCTION()
	void OnCooldownEnded(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown);

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UPanelWidget* Container;

	int32 UsedEntryNum = 0;
};
