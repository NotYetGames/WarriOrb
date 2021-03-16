// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/Layout/SScaleBox.h"
#include "MoviePlayer.h"
#include "Engine/DeveloperSettings.h"
#include "Widgets/Layout/Anchors.h"
#include "Framework/Text/TextLayout.h"
#include "Widgets/Text/STextBlock.h"

#include "NYLoadingScreenSettings.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenBackgroundImage
{
	GENERATED_USTRUCT_BODY()
public:
	void AsyncLoad() const;

public:
	// If we should use BackgroundRandomImages
	UPROPERTY(Config, EditAnywhere, Category = Background)
	bool bUseRandomBackgroundImages = false;

	/** The texture display while in the loading screen on top of the movie. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Background, meta = (AllowedClasses = "Texture2D"))
	TArray<FSoftObjectPath> BackgroundRandomImages;

	/** The scaling type to apply to images. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Background)
	TEnumAsByte<EStretch::Type> BackgroundRandomImagesStretch = EStretch::ScaleToFit;

	// Fallback color for the background if we don't use anything else
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Background)
	FLinearColor BackgroundFallbackColor = FLinearColor::Black;
};


USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenMovieInfo
{
	GENERATED_USTRUCT_BODY()
public:
	void AsyncLoad() const {}

public:
	/** If true, movies can be skipped by clicking the loading screen as long as loading is done. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category=Movies)
	bool bMoviesAreSkippable = false;

	/** If true, movie playback continues until Stop is called. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category=Movies)
	bool bWaitForManualStop = false;

	/** Should we just play back, loop, etc.  NOTE: if playback type is MT_LoadingLoop, then MoviePlayer will auto complete when in the last movie and load finishes regardless of bAutoCompleteWhenLoadingCompletes */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category= Movies)
	TEnumAsByte<EMoviePlaybackType> PlaybackType;

	/** The movie paths local to the game's Content/Movies/ directory without extension. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category=Movies)
	TArray<FString> MoviePaths;
};


USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenImageInfo
{
	GENERATED_USTRUCT_BODY()
public:
	void AsyncLoad() const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Image)
	TSoftObjectPtr<UTexture2D> Image;

	// By default it will be top left corner
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Image)
	FAnchors Anchors;

	// If zero it will size to content automatically
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Image)
	FVector2D Size = FVector2D::ZeroVector;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Image)
	FVector2D Position = FVector2D::ZeroVector;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Image)
	FVector2D Alignment{ 0.5f, 0.5f };

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Image)
	int32 ZOrder = 0;
};


USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenLoadingImage
{
	GENERATED_USTRUCT_BODY()
public:
	void AsyncLoad() const;

public:
	// If zero it will size to content automatically
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Image)
	FVector2D Size{180.f, 180.f};

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Image)
	FMargin Padding{0.f, -18.f, 0.f, 0.f};


	// If true will use SpinningImage
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = SpinningImage)
	bool bUseSpinningImage = true;

	// Only used if bUseSpinningImage = true
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = SpinningImage)
	TSoftObjectPtr<UTexture2D> SpinningImage;


	// If true will use ThrobberPieceImage
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Throbber)
	bool bUseThrobber = false;

	// To spin the image in a circular motion?
	// Only used if bUseThrobber = true
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Throbber)
	bool bThrobberSpin = true;

	// Only used if bUseThrobber = true
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Throbber)
	TSoftObjectPtr<UTexture2D> ThrobberPieceImage;

	// Number of pieces to use
	// Only used if bUseThrobber = true
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Throbber)
	int32 ThrobberNumImagePieces = 6;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Throbber)
	float ThrobberPeriod = 1.5f;


	// If True will use Material as the loading image
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Material)
	bool bUseMaterial = false;

	// Animated image or material
	// Only used if bUseMaterial = true
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Material, meta = (AllowedClasses = "Texture2D UMaterialInterface"))
	FSoftObjectPath Material;
};

// Info for a slot inside SConstraintCanvas
USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenContainer
{
	GENERATED_USTRUCT_BODY()
public:
	void AsyncLoad() const {}

public:
	bool AutoSize() const { return Size.Equals(FVector2D::ZeroVector);  }
	FMargin GetMargin() const
	{
		FMargin Margin{ 0 };
		Margin.Left = Position.X;
		Margin.Top = Position.Y;

		// Set Size
		Margin.Right = Size.X;
		Margin.Bottom = Size.Y;

		return Margin;
	}

	// By default extend to the bottom screen
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Container)
	FAnchors Anchors{0.f, 1.f, 1.f, 1.f};

	// If zero it will size to content automatically
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Container)
	FVector2D Size = FVector2D::ZeroVector;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Container)
	FVector2D Position{0.f, -200.f};

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Container)
	FVector2D Alignment = FVector2D::ZeroVector;

	// Only used in some cases
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Container)
	TEnumAsByte<EHorizontalAlignment> SlotHorizontalAlignment = EHorizontalAlignment::HAlign_Center;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Container)
	TEnumAsByte<EVerticalAlignment> SlotVerticalAlignment = EVerticalAlignment::VAlign_Center;
};


// Info for STextBlock
USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenText
{
	GENERATED_USTRUCT_BODY()
public:
	void AsyncLoad() const {}

	// From the properties of this struct
	TSharedRef<STextBlock> NewTextBlock(const TAttribute<EVisibility>& Visibility) const;

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FText Text;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FMargin Padding{0.f, 0.f, 30.f, 0.f};

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FSlateFontInfo Font;

	// Whether to wrap text automatically based on the widget's computed horizontal space.  IMPORTANT: Using automatic wrapping can result
	// in visual artifacts, as the the wrapped size will computed be at least one frame late!  Consider using WrapTextAt instead.  The initial
	// desired size will not be clamped.  This works best in cases where the text block's size is not affecting other widget's layout.
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	bool bAutoWrap = true;

	// Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs.
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	float WrapTextAt = 0;

	// How the text should be aligned with the margin.
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	TEnumAsByte<ETextJustify::Type> Justification = ETextJustify::Center;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	float MinDesiredWidth = 0;
};

USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenLoadingText
{
	GENERATED_USTRUCT_BODY()
public:
	FNYLoadingScreenLoadingText();
	void AsyncLoad() const {}

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FNYLoadingScreenText NormalText;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FNYLoadingScreenText WaitForAnyKeyText;
};

USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenLoading
{
	GENERATED_USTRUCT_BODY()
public:
	void AsyncLoad() const;

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FNYLoadingScreenContainer TitleContainer;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FNYLoadingScreenText TitleText;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FNYLoadingScreenContainer DescriptionContainer;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FNYLoadingScreenText DescriptionText;

	// Information about the container of the loading screen text + loading image
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Container)
	FNYLoadingScreenContainer LoadingContainer;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Text)
	FNYLoadingScreenLoadingText LoadingText;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Image)
	FNYLoadingScreenLoadingImage LoadingImage;
};


USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenTips
{
	GENERATED_USTRUCT_BODY()
public:
	// The font to display the tips in.
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Fonts)
	FSlateFontInfo Font;

	/** The size of the tip before it's wrapped to the next line. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Advice)
	float WrapAt = 1000.0f;

	/** The tips to display on the load screen. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Advice)
	TArray<FText> Tips;
};


USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenDescription
{
	GENERATED_USTRUCT_BODY()

public:
	FNYLoadingScreenDescription() {}
	void AsyncLoad() const;

	/** The minimum time that a loading screen should be opened for, -1 if there is no minimum time. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Loading)
	//float MinimumLoadingScreenDisplayTime = -1;

	/** If true, the loading screen will disappear as soon as loading is done. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Loading)
	//bool bAutoCompleteWhenLoadingCompletes = true;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movies)
	//bool bShowMovies = false;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movies)
	//FNYLoadingScreenMovieInfo MovieInfo;

	/**  Should we show the images/tips/loading text?  Generally you'll want to set this to false if you just want to show a movie. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Display)
	//bool bShowUIOverlay = true;

	// All the images to display
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Images)
	TArray<FNYLoadingScreenImageInfo> Images;

	// Text displayed beside the animated icon
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Loading)
	FNYLoadingScreenLoading Loading;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Background)
	FNYLoadingScreenBackgroundImage Background;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = Tips)
	FNYLoadingScreenTips Tips;
};

// Assume the date is in the same year, so we only care about the month, day, hour
USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenDateSameYear
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = LoadingScreenOverrideEvent, meta = (ClampMin = "1", ClampMax = "12", UIMin = "1", UIMax = "12"))
	int32 Month = 1;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = LoadingScreenOverrideEvent, meta = (ClampMin = "1", ClampMax = "31", UIMin = "1", UIMax = "31"))
	int32 Day = 1;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = LoadingScreenOverrideEvent, meta = (ClampMin = "0", ClampMax = "23", UIMin = "0", UIMax = "23"))
	int32 Hour = 0;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = LoadingScreenOverrideEvent, meta = (ClampMin = "0", ClampMax = "59", UIMin = "0", UIMax = "59"))
	int32 Minute = 0;
};

// Halloween, Christmas, etc events
USTRUCT(BlueprintType)
struct NOTYETLOADINGSCREEN_API FNYLoadingScreenOverrideEvent
{
	GENERATED_USTRUCT_BODY()

public:
	// This will replace the first image from FNYLoadingScreenDescription.Images[0].Image
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = LoadingScreenOverrideEvent)
	TSoftObjectPtr<UTexture2D> Image;

	// The date when this custom event starts, inclusive
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = LoadingScreenOverrideEvent)
	FNYLoadingScreenDateSameYear StartDate;

	// The date when this custom event starts, exclusive
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = LoadingScreenOverrideEvent)
	FNYLoadingScreenDateSameYear EndDate;

public:
	// Is the current time in the specified event period
	bool IsOverridePeriod() const
	{
		// We use the same year as the current time
		return IsDateTimeInOverridePeriod(FDateTime::Now());
	}

	bool IsDateTimeInOverridePeriod(const FDateTime& DateTime) const
	{
		const int32 CurrentYear = DateTime.GetYear();
		const FDateTime StartDateTime = FDateTime(CurrentYear, StartDate.Month, StartDate.Day, StartDate.Hour);
		const FDateTime EndDateTime = FDateTime(CurrentYear, EndDate.Month, EndDate.Day, EndDate.Hour);

		return DateTime >= StartDateTime && DateTime < EndDateTime;
	}
};


/**
 * Settings for the simple loading screen plugin.
 */
UCLASS(config = NYLoadingScreenSettings, DefaultConfig, meta = (DisplayName = "Loading Screen"))
class NOTYETLOADINGSCREEN_API UNYLoadingScreenSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNYLoadingScreenSettings(const FObjectInitializer& Initializer);

	void PostInitProperties() override;

	//
	// UDeveloperSettings interface
	//

	/** Gets the settings container name for the settings, either Project or Editor */
	FName GetContainerName() const override { return "Project"; }
	/** Gets the category for the settings, some high level grouping like, Editor, Engine, Game...etc. */
	FName GetCategoryName() const override { return "Game"; };
	/** The unique name for your section of settings, uses the class's FName. */
	// FName GetSectionName() const override { return "NY Loading Screen"; };

	//
	// Own Methods
	//

	FNYLoadingScreenDescription& GetWorldTransitionScreen();

	bool IsHalloweenEventPeriod() const
	{
		return HalloweenOverrideEvent.IsOverridePeriod();
	}
	bool IsDateTimeInHalloweenEventPeriod(const FDateTime& DateTime) const
	{
		return HalloweenOverrideEvent.IsDateTimeInOverridePeriod(DateTime);
	}
	TSoftObjectPtr<UTexture2D> GetOverrideImage() const
	{
		if (IsHalloweenEventPeriod())
		{
			return HalloweenOverrideEvent.Image;
		}

		return nullptr;
	}

	bool IsOverrideEventPeriod() const
	{
		return IsHalloweenEventPeriod();
	}


protected:
	// Use StartupScreen on startup
	//UPROPERTY(config, EditAnywhere, Category = Screens)
	//bool bHasStartupScreen = false;

	/** The startup screen for the project. */
	//UPROPERTY(config, EditAnywhere, Category = Screens)
	//FNYLoadingScreenDescription StartupScreen;

	/** The default load screen between maps. */
	UPROPERTY(Config, EditAnywhere, Category = Screen)
	FNYLoadingScreenDescription WorldTransitionScreen;

	UPROPERTY(Config, EditAnywhere, Category = OverrideEvents)
	FNYLoadingScreenOverrideEvent HalloweenOverrideEvent;

	//
	// NOTE: We moved the variables from FNYLoadingScreenDescription here so that it is easier to read in the config file
	//

	// UPROPERTY(Config, EditAnywhere, Category = Images, meta = (DisplayName = "Images"))
	// TArray<FNYLoadingScreenImageInfo> WorldTransitionImages;
	//
	// // Text displayed beside the animated icon
	// UPROPERTY(Config, EditAnywhere, Category = Loading, meta = (DisplayName = "Loading"))
	// FNYLoadingScreenLoading WorldTransitionLoading;
	//
	// UPROPERTY(Config, EditAnywhere, Category = Background, meta = (DisplayName = "Background"))
	// FNYLoadingScreenBackgroundImage WorldTransitionBackground;
	//
	// UPROPERTY(Config, EditAnywhere, Category = Tips, meta = (DisplayName = "Tips"))
	// FNYLoadingScreenTips WorldTransitionTips;
};
