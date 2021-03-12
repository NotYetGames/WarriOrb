// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "SoDisplaySettingsTypes.generated.h"


// All the aspect options supported
UENUM(BlueprintType)
enum class ESoDisplayAspectRatio : uint8
{
	// Unknown or not supported
	Unknown = 0,

	Ratio_4_3,
	Ratio_16_9,
	Ratio_16_10,
};


/**
 * Struct to also handle the actual resolution Width/Height as an int
 * Also has comparison functions + aspect ratio
 */
USTRUCT(BlueprintType)
struct SORB_API FSoDisplayResolution
{
	GENERATED_USTRUCT_BODY()
private:
	typedef FSoDisplayResolution Self;

public:
	FSoDisplayResolution() : Resolution(FIntPoint::NoneValue) {}

	// Performs sanity checks
	FSoDisplayResolution(uint32 InWidth, uint32 InHeight, ESoDisplayAspectRatio InAspectRatio) :
		Resolution(InWidth, InHeight), AspectRatio(InAspectRatio)
	{
		// Sanity check if aspect ratios match
#if DO_CHECK
		const float Ratio_Input = GetAspectRatioFloat();
		switch (AspectRatio)
		{
			case ESoDisplayAspectRatio::Ratio_4_3:
				checkf(AreAspectRatiosEqual(Ratio_Input, Ratio_4_3),
					TEXT("Resolution Input is not aspect ratio 4:3. Width = %d, Height = %d"), Resolution.X, Resolution.Y);
				break;
			case ESoDisplayAspectRatio::Ratio_16_9:
				checkf(AreAspectRatiosEqual(Ratio_Input, Ratio_16_9),
					TEXT("Resolution Input is not aspect ratio 16:9. Width = %d, Height = %d"), Resolution.X, Resolution.Y);
				break;
			case ESoDisplayAspectRatio::Ratio_16_10:
				checkf(AreAspectRatiosEqual(Ratio_Input, Ratio_16_10),
					TEXT("Resolution Input is not aspect ratio 16:10. Width = %d, Height = %d"), Resolution.X, Resolution.Y);
				break;
			default:
				// Unknown :(
				break;
		}
#endif // DO_CHECK
	}

	// Automatically set AspectRatio enum
	FSoDisplayResolution(uint32 InWidth, uint32 InHeight) : Resolution(InWidth, InHeight)
	{
		const float Ratio_Input = GetAspectRatioFloat();
		if (AreAspectRatiosEqual(Ratio_Input, Ratio_4_3))
			AspectRatio = ESoDisplayAspectRatio::Ratio_4_3;
		else if (AreAspectRatiosEqual(Ratio_Input, Ratio_16_9))
			AspectRatio = ESoDisplayAspectRatio::Ratio_16_9;
		else if (AreAspectRatiosEqual(Ratio_Input, Ratio_16_10))
			AspectRatio = ESoDisplayAspectRatio::Ratio_16_10;
		else
			AspectRatio = ESoDisplayAspectRatio::Unknown;
	}
	FSoDisplayResolution(const FIntPoint& Point) : FSoDisplayResolution(Point.X, Point.Y) {}

	// Are the two aspect rations equal, different variants
	static bool AreAspectRatiosEqual(const float A, const float B) { return FMath::IsNearlyEqual(A, B, ErrorTolerance); }
	static bool AreAspectRatiosEqual(const ESoDisplayAspectRatio Enum, const float AspectRatio) { return AreAspectRatiosEqual(AspectRatio, Enum); }
	static bool AreAspectRatiosEqual(const float AspectRatio, const ESoDisplayAspectRatio Enum)
	{
		switch (Enum)
		{
			case ESoDisplayAspectRatio::Ratio_4_3:
				return AreAspectRatiosEqual(AspectRatio, Ratio_4_3);
			case ESoDisplayAspectRatio::Ratio_16_9:
				return AreAspectRatiosEqual(AspectRatio, Ratio_16_9);
			case ESoDisplayAspectRatio::Ratio_16_10:
				return AreAspectRatiosEqual(AspectRatio, Ratio_16_10);
			default:
				return false;
		}
	}

	// Sort Predicate for ascending resolutions
	bool operator==(const Self& Other) const { return Resolution == Other.Resolution; }
	bool operator<(const Self& Other) const  { return Resolution.X < Other.Resolution.X && Resolution.Y < Other.Resolution.Y; }
	bool operator<=(const Self& Other) const { return Resolution.X <= Other.Resolution.X && Resolution.Y <= Other.Resolution.Y; }
	bool operator>(const Self& Other) const  { return Resolution.X > Other.Resolution.X && Resolution.Y > Other.Resolution.Y; }
	bool operator>=(const Self& Other) const { return Resolution.X >= Other.Resolution.X && Resolution.Y >= Other.Resolution.Y; }

	// Gets the aspect ratio as a float
	constexpr float GetAspectRatioFloat() const { return static_cast<float>(Resolution.X) / static_cast<float>(Resolution.Y); }

	// TMap key hash
	friend uint32 GetTypeHash(const FSoDisplayResolution& This)
	{
		return GetTypeHash(This.Resolution);
	}

public:
	// X - Width, Y - Height
	UPROPERTY(BlueprintReadOnly)
	FIntPoint Resolution;

	// Not needed, this is just cached
	UPROPERTY(BlueprintReadOnly)
	ESoDisplayAspectRatio AspectRatio = ESoDisplayAspectRatio::Unknown;

private:
	static constexpr float Ratio_4_3 = 4.f / 3.f;
	static constexpr float Ratio_16_9 = 16.f / 9.f;
	static constexpr float Ratio_16_10 = 16.f / 10.f;
	static constexpr float ErrorTolerance = 1.e-2f; // only the first two decimals must match
};
