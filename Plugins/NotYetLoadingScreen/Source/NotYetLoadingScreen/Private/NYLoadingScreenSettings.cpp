// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "NYLoadingScreenSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Font.h"
#include "INYLoadingScreenModule.h"

#define LOCTEXT_NAMESPACE "LoadingScreen"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FNYLoadingScreenLoadingText::FNYLoadingScreenLoadingText()
{
	NormalText.Text = LOCTEXT("Loading", "LOADING");
	WaitForAnyKeyText.Text = LOCTEXT("PressAnyKey", "Press any key to continue...");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenBackgroundImage::AsyncLoad() const
{
	if (!bUseRandomBackgroundImages)
		return;

	auto& Module = INYLoadingScreenModule::Get();
	for (const FSoftObjectPath& Ref : BackgroundRandomImages)
	{
		Module.RequestAsyncLoad(Ref);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenImageInfo::AsyncLoad() const
{
	auto& Module = INYLoadingScreenModule::Get();
	Module.RequestAsyncLoad(Image.ToSoftObjectPath());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenLoadingImage::AsyncLoad() const
{
	auto& Module = INYLoadingScreenModule::Get();
	if (bUseSpinningImage)
	{
		Module.RequestAsyncLoad(SpinningImage.ToSoftObjectPath());
	}
	if (bUseThrobber)
	{
		Module.RequestAsyncLoad(ThrobberPieceImage.ToSoftObjectPath());
	}
	if (bUseMaterial)
	{
		Module.RequestAsyncLoad(Material);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedRef<STextBlock> FNYLoadingScreenText::NewTextBlock(const TAttribute<EVisibility>& Visibility) const
{
	return SNew(STextBlock)
		.Text(Text)
		.Visibility(Visibility)
		.Font(Font)
		.Justification(Justification)
		.AutoWrapText(bAutoWrap)
		.MinDesiredWidth(MinDesiredWidth)
		.WrapTextAt(WrapTextAt);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenLoading::AsyncLoad() const
{
	TitleContainer.AsyncLoad();
	TitleText.AsyncLoad();

	DescriptionContainer.AsyncLoad();
	DescriptionText.AsyncLoad();

	LoadingContainer.AsyncLoad();
	LoadingText.AsyncLoad();
	LoadingImage.AsyncLoad();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FNYLoadingScreenDescription::AsyncLoad() const
{
	for (const FNYLoadingScreenImageInfo& Img : Images)
	{
		Img.AsyncLoad();
	}

	Background.AsyncLoad();
	Loading.AsyncLoad();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UNYLoadingScreenSettings::UNYLoadingScreenSettings(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	if (!IsRunningDedicatedServer())
	{
		// static ConstructorHelpers::FObjectFinder<UFont> RobotoFontObj(TEXT("/Engine/EngineFonts/Roboto"));
		// Font = FSlateFontInfo(RobotoFontObj.Object, 20, FName("Normal"));
		// LoadingFont = FSlateFontInfo(RobotoFontObj.Object, 32, FName("Bold"));
		// AnyKeyFont = FSlateFontInfo(RobotoFontObj.Object, 32, FName("Bold"));
	}
}

void UNYLoadingScreenSettings::PostInitProperties()
{
	Super::PostInitProperties();

	// Override
	if (IsOverrideEventPeriod() && WorldTransitionScreen.Images.IsValidIndex(0))
	{
		WorldTransitionScreen.Images[0].Image = GetOverrideImage();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FNYLoadingScreenDescription& UNYLoadingScreenSettings::GetWorldTransitionScreen()
{
	// NOTE: this basically breaks saving at runtime
	// static bool bInitialized = false;
	// if (!bInitialized)
	// {
	// 	WorldTransitionScreen.Images = WorldTransitionImages;
	// 	WorldTransitionScreen.Loading = WorldTransitionLoading;
	// 	WorldTransitionScreen.Background = WorldTransitionBackground;
	// 	WorldTransitionScreen.Tips = WorldTransitionTips;
	//
	// 	bInitialized = true;
	// }

	return WorldTransitionScreen;
}


#undef LOCTEXT_NAMESPACE
