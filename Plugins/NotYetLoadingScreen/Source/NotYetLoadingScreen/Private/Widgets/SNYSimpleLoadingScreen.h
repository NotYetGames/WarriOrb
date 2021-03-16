// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "NYLoadingScreenSettings.h"

#include "Styling/SlateBrush.h"
#include "Slate/DeferredCleanupSlateBrush.h"


class SOverlay;
class SConstraintCanvas;
class SBox;
class STextBlock;


//
// Special brush to prevent garbage collection
// NOTE can't use FSlateDynamicImageBrush in loading screen because of bug
//
// struct FNYLoadingScreenBrushImplementation : public FSlateDynamicImageBrush, public FGCObject
// {
// 	FNYLoadingScreenBrushImplementation(class UTexture2D* InTexture, const FVector2D& InImageSize, const FName InImagePath)
// 		: FSlateDynamicImageBrush(InTexture, InImageSize, InImagePath)
// 	{
// 	}
//
// 	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
// 	{
// 		UObject* Object = GetResourceObject();
// 		if (Object)
// 		{
// 			Collector.AddReferencedObject(Object);
// 		}
// 	}
//
// 	// GetSlateBrush
// };

typedef FDeferredCleanupSlateBrush FNYLoadingScreenBrush;

class SNYSimpleLoadingScreen : public SCompoundWidget
{
	typedef SNYSimpleLoadingScreen Self;
public:
	SLATE_BEGIN_ARGS(SNYSimpleLoadingScreen) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const FNYLoadingScreenDescription& InScreenDescription);
	void Redraw(const FNYLoadingScreenDescription& InScreenDescription);
	void SetWaitForAnyKeyInput(bool bInValue);

private:
	void UpdateAttributes();
	FText GetLoadingText() const;
	float GetDPIScale() const;
	TSharedPtr<FNYLoadingScreenBrush> CreateBrushFromImageInfo(const FNYLoadingScreenImageInfo& Info);
	TSharedPtr<FNYLoadingScreenBrush> CreateBrushFromAssetReference(const FSoftObjectPath& ImageAsset);

	TSharedPtr<FNYLoadingScreenBrush> MakeBrushFromMaterial(const FSoftObjectPath& MaterialAsset, FVector2D ImageSize);

	void RedrawWidgetMainCanvasPanel();
	void AddBackgroundImageToRoot();
	void AddLoadingWidgets(const FNYLoadingScreenLoading& Info);
	void AddImageWidgetToMainCanvasPanel(TSharedPtr<FNYLoadingScreenBrush> Brush, const FNYLoadingScreenImageInfo& Info);

private:
	// Widgets
	TSharedPtr<FNYLoadingScreenBrush> BrushBackground = nullptr;
	TSharedPtr<FNYLoadingScreenBrush> BrushLoadingAnimation = nullptr;

	TArray<TSharedPtr<FNYLoadingScreenBrush>> BrushesImages;

	TSharedPtr<SConstraintCanvas> WidgetMainCanvasPanel = nullptr;
	TSharedPtr<SOverlay> WidgetRootOverlay = nullptr;
	TSharedPtr<SBox> WidgetLoadingImage;

	FNYLoadingScreenDescription ScreenDescription;

	TAttribute<EVisibility> AttributeVisibleNormal;
	TAttribute<EVisibility> AttributeVisibleWaitForAnyKey;

	bool bWaitForAnyKeyInput = false;
};
