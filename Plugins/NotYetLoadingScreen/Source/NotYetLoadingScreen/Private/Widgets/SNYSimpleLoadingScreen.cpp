// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SNYSimpleLoadingScreen.h"

#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Engine/Texture2D.h"
#include "Engine/UserInterfaceSettings.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Materials/MaterialInterface.h"
#include "Widgets/Images/SSpinningImage.h"

#define LOCTEXT_NAMESPACE "LoadingScreen"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SSimpleLoadingScreen
void SNYSimpleLoadingScreen::Construct(const FArguments& InArgs, const FNYLoadingScreenDescription& InScreenDescription)
{
	SAssignNew(WidgetRootOverlay, SOverlay);
	Redraw(InScreenDescription);
	ChildSlot
	[
		WidgetRootOverlay.ToSharedRef()
	];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SNYSimpleLoadingScreen::Redraw(const FNYLoadingScreenDescription& InScreenDescription)
{
	ScreenDescription = InScreenDescription;
	WidgetRootOverlay->ClearChildren();
	UpdateAttributes();

	// Add Game tips
	// TSharedRef<SWidget> TipWidget = SNullWidget::NullWidget;
	// FNYLoadingScreenTips TipsInfo = InScreenDescription.Tips;
	// if (TipsInfo.Tips.Num() > 0)
	// {
	// 	const int32 TipIndex = FMath::RandRange(0, TipsInfo.Tips.Num() - 1);
	//
	// 	TipWidget = SNew(STextBlock)
	// 		.WrapTextAt(TipsInfo.WrapAt)
	// 		.Font(TipsInfo.Font)
	// 		.Text(TipsInfo.Tips[TipIndex]);
	// }

	// Bottom default, TODO modify
	//WidgetRootOverlay->AddSlot()
	//.HAlign(HAlign_Fill)
	//.VAlign(VAlign_Bottom)
	//[
	//	SNew(SBorder)
	//	.HAlign(HAlign_Fill)
	//	.VAlign(VAlign_Fill)
	//	.BorderBackgroundColor(FLinearColor(0, 0, 0, 0.75))
	//	.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
	//	[
	//		SNew(SSafeZone)
	//		.HAlign(HAlign_Fill)
	//		.VAlign(VAlign_Bottom)
	//		.IsTitleSafe(true)
	//		[
	//			SNew(SDPIScaler)
	//			.DPIScale(this, &SNYSimpleLoadingScreen::GetDPIScale)
	//			[
	//				SNew(SHorizontalBox)

					// Circular throbber
					// TODO replace this
					//+SHorizontalBox::Slot()
					//.Padding(FMargin(25, 0.0f, 0, 0))
					//.VAlign(VAlign_Center)
					//.AutoWidth()
					//[
					//	SNew(SCircularThrobber)
					//	// Convert font size to pixels, pixel_size = point_size * resolution / 72, then half it to get radius
					//	.Radius((FontLoading.Size * 96.0f/72.0f) / 2.0f)
					//]

					//// Loading text
					//+SHorizontalBox::Slot()
					//.Padding(FMargin(40.0f, 0.0f, 0, 0))
					//.AutoWidth()
					//.VAlign(VAlign_Center)
					//[
					//	SNew(STextBlock)
					//	.Text(InScreenDescription.LoadingText)
					//	.Font(LoadingFont)
					//]

					//// Game tips
					//+SHorizontalBox::Slot()
					//.FillWidth(1)
					//.HAlign(HAlign_Fill)
					//[
					//	SNew(SSpacer)
					//	.Size(FVector2D(1.0f, 1.0f))
					//]

					//+SHorizontalBox::Slot()
					//.AutoWidth()
					//.HAlign(HAlign_Right)
					//.VAlign(VAlign_Center)
					//.Padding(FMargin(10.0f))
					//[
					//	TipWidget
					//]
	//			]
	//		]
	//	]
	//];


	// Draw Widgets
	AddBackgroundImageToRoot();

	SAssignNew(WidgetMainCanvasPanel, SConstraintCanvas);
	RedrawWidgetMainCanvasPanel();

	//
	// WidgetMainCanvasPanel
	//
	WidgetRootOverlay->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		//SNew(SBorder)
		//.HAlign(HAlign_Fill)
		//.VAlign(VAlign_Fill)
		//.Padding(0.f)
		//[
			//SNew(SSafeZone)
			//.HAlign(HAlign_Fill)
			//.VAlign(VAlign_Fill)
			//.IsTitleSafe(true)
			//[
				SNew(SDPIScaler)
				.DPIScale(this, &SNYSimpleLoadingScreen::GetDPIScale)
				[
					WidgetMainCanvasPanel.ToSharedRef()
				]
			//]
		//]
	];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SNYSimpleLoadingScreen::RedrawWidgetMainCanvasPanel()
{
	WidgetMainCanvasPanel->ClearChildren();

	// Add Images
	for (const FNYLoadingScreenImageInfo& Info : ScreenDescription.Images)
	{
		TSharedPtr<FNYLoadingScreenBrush> Brush = CreateBrushFromImageInfo(Info);
		AddImageWidgetToMainCanvasPanel(Brush, Info);
		BrushesImages.Add(Brush);
	}

	// Add loading text and image
	AddLoadingWidgets(ScreenDescription.Loading);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SNYSimpleLoadingScreen::AddBackgroundImageToRoot()
{
	const FNYLoadingScreenBackgroundImage& BackgroundInfo = ScreenDescription.Background;
	if (BackgroundInfo.bUseRandomBackgroundImages && BackgroundInfo.BackgroundRandomImages.Num() > 0)
	{
		// Add background image
		const int32 ImageIndex = FMath::RandRange(0, BackgroundInfo.BackgroundRandomImages.Num() - 1);
		BrushBackground = CreateBrushFromAssetReference(BackgroundInfo.BackgroundRandomImages[ImageIndex]);
		if (BrushBackground.IsValid())
		{
			WidgetRootOverlay->AddSlot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SScaleBox)
					.Stretch(BackgroundInfo.BackgroundRandomImagesStretch)
					[
						SNew(SImage)
						.Image(BrushBackground->GetSlateBrush())
					]
				];
		}
	}
	else
	{
		// FallBack
		WidgetRootOverlay->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SImage)
				.Image(nullptr)
				.ColorAndOpacity(BackgroundInfo.BackgroundFallbackColor)
			];
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SNYSimpleLoadingScreen::AddLoadingWidgets(const FNYLoadingScreenLoading& Info)
{
	// Set auto size if no size is set
	bool bLoadingContainerAutoSize = Info.LoadingContainer.AutoSize();
	FOptionalSize LoadingImageWidthOverride;
	FOptionalSize LoadingImageHeightOverride;
	if (Info.LoadingImage.Size.Equals(FVector2D::ZeroVector))
	{
		bLoadingContainerAutoSize = true;
	}
	else
	{
		LoadingImageWidthOverride = Info.LoadingImage.Size.X;
		LoadingImageHeightOverride = Info.LoadingImage.Size.Y;
	}

	// Handle different image types
	TSharedPtr<SWidget> WidgetImage = SNullWidget::NullWidget;

	// Set the correct image type
	if (Info.LoadingImage.bUseThrobber)
	{
		// Use Throbber
		BrushLoadingAnimation = CreateBrushFromAssetReference(Info.LoadingImage.ThrobberPieceImage.ToSoftObjectPath());
		if (BrushLoadingAnimation.IsValid())
		{
			if (Info.LoadingImage.bThrobberSpin)
			{
				WidgetImage = SNew(SCircularThrobber)
					.PieceImage(BrushLoadingAnimation->GetSlateBrush())
					.NumPieces(Info.LoadingImage.ThrobberNumImagePieces)
					.Period(Info.LoadingImage.ThrobberPeriod)
					.Radius(Info.LoadingText.NormalText.Font.Size * 96.0f / 72.0f);
			}
			else
			{
				WidgetImage = SNew(SThrobber)
					.PieceImage(BrushLoadingAnimation->GetSlateBrush())
					.NumPieces(Info.LoadingImage.ThrobberNumImagePieces)
					.Animate(SThrobber::All);
			}
		}
	}
	else if (Info.LoadingImage.bUseSpinningImage)
	{
		// Use Image
		BrushLoadingAnimation = CreateBrushFromAssetReference(Info.LoadingImage.SpinningImage.ToSoftObjectPath());
		if (BrushLoadingAnimation.IsValid())
		{
			WidgetImage = SNew(SSpinningImage)
				.Image(BrushLoadingAnimation->GetSlateBrush())
				.Period(1.f);
		}
	}
	else if (Info.LoadingImage.bUseMaterial)
	{
		// Use Material
		BrushLoadingAnimation = MakeBrushFromMaterial(Info.LoadingImage.Material, Info.LoadingImage.Size);
		if (BrushLoadingAnimation.IsValid())
		{
			WidgetImage = SNew(SImage)
				.Image(BrushLoadingAnimation->GetSlateBrush());
		}
	}

	// Default image, nothing else is good
	if (!BrushLoadingAnimation.IsValid())
	{
		WidgetImage = SNew(SCircularThrobber)
			.Radius(Info.LoadingText.NormalText.Font.Size * 96.0f / 72.0f);
	}

	WidgetMainCanvasPanel->AddSlot()
		.Anchors(Info.LoadingContainer.Anchors)
		.Alignment(Info.LoadingContainer.Alignment)
		.AutoSize(bLoadingContainerAutoSize)
		.Offset(Info.LoadingContainer.GetMargin())
		[
			SNew(SOverlay)

			+SOverlay::Slot()
			.HAlign(Info.LoadingContainer.SlotHorizontalAlignment)
			.VAlign(Info.LoadingContainer.SlotVerticalAlignment)
			[
				SNew(SHorizontalBox)

				// Loading Text
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(Info.LoadingText.NormalText.Padding)
				.AutoWidth()
				[
					SNew(SOverlay)

					// Normal loading text
					+SOverlay::Slot()
					[
						Info.LoadingText.NormalText.NewTextBlock(AttributeVisibleNormal)
					]

					// Wait for any key to continue text
					+SOverlay::Slot()
					[
						Info.LoadingText.WaitForAnyKeyText.NewTextBlock(AttributeVisibleWaitForAnyKey)
					]
				]

				// Image loading
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(Info.LoadingImage.Padding)
				[
					SAssignNew(WidgetLoadingImage, SBox)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Visibility_Lambda([this]()
					{
						return bWaitForAnyKeyInput ? EVisibility::Collapsed : EVisibility::Visible;
					})
					.WidthOverride(LoadingImageWidthOverride)
					.HeightOverride(LoadingImageHeightOverride)
					[
						WidgetImage.ToSharedRef()
					]
				]
			]
		];


	// Title Text
	WidgetMainCanvasPanel->AddSlot()
		.Anchors(Info.TitleContainer.Anchors)
		.Alignment(Info.TitleContainer.Alignment)
		.AutoSize(Info.TitleContainer.AutoSize())
		.Offset(Info.TitleContainer.GetMargin())
		[
			Info.TitleText.NewTextBlock(AttributeVisibleWaitForAnyKey)
		];


	// Description Text
	WidgetMainCanvasPanel->AddSlot()
		.Anchors(Info.DescriptionContainer.Anchors)
		.Alignment(Info.DescriptionContainer.Alignment)
		.AutoSize(Info.DescriptionContainer.AutoSize())
		.Offset(Info.DescriptionContainer.GetMargin())
		[
			Info.DescriptionText.NewTextBlock(AttributeVisibleWaitForAnyKey)
		];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SNYSimpleLoadingScreen::AddImageWidgetToMainCanvasPanel(TSharedPtr<FNYLoadingScreenBrush> Brush, const FNYLoadingScreenImageInfo& Info)
{
	if (!Brush.IsValid())
	{
		return;
	}

	// Set auto size if no size is set
	bool bAutoSize = false;
	if (Info.Size.Equals(FVector2D::ZeroVector))
	{
		bAutoSize = true;
	}

	// Set position
	FMargin Offset{ 0 };
	Offset.Left = Info.Position.X;
	Offset.Top = Info.Position.Y;

	// Set Size
	Offset.Right = Info.Size.X;
	Offset.Bottom = Info.Size.Y;

	WidgetMainCanvasPanel->AddSlot()
		.Anchors(Info.Anchors)
		.Alignment(Info.Alignment)
		.AutoSize(bAutoSize)
		.Offset(Offset)
		.ZOrder(Info.ZOrder)
		[
			SNew(SImage)
			.Image(Brush->GetSlateBrush())
		];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedPtr<FNYLoadingScreenBrush> SNYSimpleLoadingScreen::MakeBrushFromMaterial(const FSoftObjectPath& MaterialAsset, FVector2D ImageSize)
{
	UObject* MaterialObject = MaterialAsset.ResolveObject();
	if (UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(MaterialObject))
	{
		return FDeferredCleanupSlateBrush::CreateBrush(MaterialInterface, ImageSize);
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedPtr<FNYLoadingScreenBrush> SNYSimpleLoadingScreen::CreateBrushFromImageInfo(const FNYLoadingScreenImageInfo& Info)
{
	return CreateBrushFromAssetReference(Info.Image.ToSoftObjectPath());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedPtr<FNYLoadingScreenBrush> SNYSimpleLoadingScreen::CreateBrushFromAssetReference(const FSoftObjectPath& ImageAsset)
{
	UObject* ImageObject = ImageAsset.ResolveObject();
	if (ImageObject == nullptr)
	{
		ImageObject = ImageAsset.TryLoad();
	}
	if (UTexture2D* LoadingImage = Cast<UTexture2D>(ImageObject))
	{
		const auto Size = FVector2D(LoadingImage->GetSizeX(), LoadingImage->GetSizeY());
		return FDeferredCleanupSlateBrush::CreateBrush(LoadingImage, Size);
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SNYSimpleLoadingScreen::UpdateAttributes()
{
	AttributeVisibleNormal.Set(bWaitForAnyKeyInput ? EVisibility::Hidden : EVisibility::Visible);
	AttributeVisibleWaitForAnyKey.Set(bWaitForAnyKeyInput ? EVisibility::Visible : EVisibility::Hidden);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SNYSimpleLoadingScreen::SetWaitForAnyKeyInput(bool bInValue)
{
	bWaitForAnyKeyInput = bInValue;
	UpdateAttributes();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float SNYSimpleLoadingScreen::GetDPIScale() const
{
	const FVector2D& DrawSize = GetCachedGeometry().ToPaintGeometry().GetLocalSize();
	const FIntPoint Size((int32)DrawSize.X, (int32)DrawSize.Y);
	return GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(Size);
}

#undef LOCTEXT_NAMESPACE
