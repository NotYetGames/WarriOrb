// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoUIUserWidget.h"

#include "SoUIExternalLink.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;


DECLARE_DELEGATE(FSoOpenLinkEventCPP);


UCLASS()
class SORB_API USoUIExternalLink : public USoUIUserWidget
{
	GENERATED_BODY()
public:
	USoUIExternalLink(const FObjectInitializer& ObjectInitializer);

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// Own methods
	//

	UFUNCTION(BlueprintCallable, Category = ">State")
	virtual void UpdateFromCurrentState();

	UFUNCTION(BlueprintCallable, Category = ">ExternalLink")
	virtual void OpenLink();

	//
	// USoUIUserWidget interface
	//
	bool NavigateOnPressed(bool bPlaySound = true) override;
	void OnStateChanged() override;

	// NOTE: left empty on purpose as we are not using the active state
	void SetIsActive(bool bActive, bool bPlaySound = false) override {}

	// Is enabled and visible
	bool IsValidWidget() const override { return GetIsEnabled() && IsVisible(); }

	FSoOpenLinkEventCPP& OnOpenLinkEvent() { return OpenLinkEventCPP; }

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* ImageElement = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TextElement = nullptr;

	// Image to display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">ExternalLink")
	UTexture2D* Image;

	// Text to display under image
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">ExternalLink")
	FText Text;

	// Text should always show?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">ExternalLink")
	bool bAlwaysShowText = false;

	// Default, when this is selected
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">ExternalLink", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OpacityHighlighted = 1.f;

	// When not selected
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">ExternalLink", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OpacityUnHighlighted = 0.5f;

	// Where does this go
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">ExternalLink")
	FString ExternalLink;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">ExternalLink")
	bool bCopyExternalLinkIntoClipboard = false;

	// If you override this the default thing won't happen
	FSoOpenLinkEventCPP OpenLinkEventCPP;
};
