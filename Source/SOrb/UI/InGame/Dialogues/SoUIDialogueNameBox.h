// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SoUIDialogueNameBox.generated.h"

class UTexture2D;
class UTextBlock;
class UImage;

 ////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoUIDialogueNameBox : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetBackgroundImage(UTexture2D* Image);
	void SetDisplayText(const FText& TextToDisplay);

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	UTextBlock* NameText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = Widgets)
	UImage* Background;
};
