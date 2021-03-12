// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoOceanManager.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"

DEFINE_LOG_CATEGORY(LogSoWater);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoOceanManager::ASoOceanManager()
{
	// tick every frame
	PrimaryActorTick.bCanEverTick = false;

	// set default vaues
	EnableGerstnerWaves = true;
	GlobalWaveDirection = FVector2D(1.0, 0.0);
	GlobalWaveAmplitude = 250;
	GlobalWaveLength = 2000;
	GlobalSize = 10000;
	DistanceCheck = 2000;
	MaterialCollection = CreateDefaultSubobject<UMaterialParameterCollection>(TEXT("MaterialCollectionGerstner"));
	if (MaterialCollection == nullptr)
	{
		UE_LOG(LogSoWater, Error, TEXT("Failed to set MaterialCollection to be default MaterialCollectionGerstner"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoOceanManager::GetTimeSeconds(const UWorld* World) const
{
	if (World == nullptr) World = GetWorld();
	return World->GetTimeSeconds();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts or when spawned
void ASoOceanManager::BeginPlay()
{
	Super::BeginPlay();
	verifyf(MaterialCollection != nullptr, TEXT("MaterialCollection is empty in the Ocean Manager. Does it exist?"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called every frame
void ASoOceanManager::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// See http://http.developer.nvidia.com/GPUGems/gpugems_ch01.html, Equation 9 in particular for details
FVector ASoOceanManager::CalculateGerstnerWave(float RotationAngle,
											   float WaveLength,
											   float Amplitude,
											   float Steepness,
											   const FVector2D& Direction,
											   const FVector& Location,
											   float Time,
											   float TimePhase)
{
	// TimePhase or S (from the formulas), described as speed depending on the wavelength

	// W - from the formulas
	const float Frequency = (2 * PI) / WaveLength;

	// D - Direction: the horizontal vector perpendicular to the wave front along which the crest travels.
	// adjust direction, the rotation angle is normalized (values from 0 to 1)
	auto AdjustedDirection = FVector(Direction, /* Z */ 0);

	// rotate around the Z axis
	AdjustedDirection = AdjustedDirection.RotateAngleAxis(RotationAngle * 360, FVector::UpVector);
	AdjustedDirection = AdjustedDirection.GetSafeNormal();

	// WavePhase, the argument inside the sin/cos functions
	// W * dot(D, Position) + time * phase
	const float WavePhase = Frequency * FVector::DotProduct(AdjustedDirection, Location) + Time * TimePhase;

	// Q - Steepness
	// A - Amplitude: the height from the water plane to the wave crest.
	const float QA = Steepness * Amplitude;

	// calculate the trig functions
	const float CosValue = FMath::Cos(WavePhase);
	const float SinValue = FMath::Sin(WavePhase);

	return FVector(QA * AdjustedDirection.X * CosValue, QA * AdjustedDirection.Y * CosValue, Amplitude * SinValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoOceanManager::CalculateGerstnerWaveCluster(float MedianRotationAngle,
													  float MedianWaveLength,
													  float MedianAmplitude,
													  float MedianSteepness,
													  const FVector2D& MedianDirection,
													  const FVector& Location,
													  float Time,
													  float MedianTimePhase) const
{
	auto Sum = FVector(0, 0, 0);
	// first wave
	Sum += CalculateGerstnerWave(MedianRotationAngle, MedianWaveLength, MedianAmplitude, MedianSteepness, MedianDirection, Location, Time, MedianTimePhase);

	// the rest of the waves
	FString Param;
	for (uint8 i = 2; i <= 8; i++)
	{
		auto iString = FString::FromInt(i);

		// get parameters for current wave
		Param = "Wave" + iString + "_AngleModifier";
		const float rotation = GetMaterialCollectionFloat(Param);

		Param = "Wave" + iString + "_WavelengthModifier";
		const float wavelength = GetMaterialCollectionFloat(Param);

		Param = "Wave" + iString + "_AmplitudeModifier";
		const float amplitude = GetMaterialCollectionFloat(Param);

		Param = "Wave" + iString + "_PhaseModifier";
		const float phase = GetMaterialCollectionFloat(Param);

		Sum += CalculateGerstnerWave(rotation + MedianRotationAngle,
									 wavelength * MedianWaveLength,
									 amplitude * MedianAmplitude,
									 MedianSteepness,
									 MedianDirection,
									 Location,
									 Time,
									 phase + MedianTimePhase);
	}

	return Sum / 8;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoOceanManager::GetWaveHeight(const FVector& Location, const UWorld* World) const
{
	// offset by the Z component
	const float SeaLevel = RootComponent->GetComponentLocation().Z;

	// too far above or below, return base sea level
	if (!EnableGerstnerWaves || Location.Z - DistanceCheck > SeaLevel || Location.Z + DistanceCheck < SeaLevel)
		return FVector(0, 0, SeaLevel);

	// set defaults
	auto Sum = FVector(0, 0, 0);
	const float Time = GetTimeSeconds(World);

	// first cluster
	float Angle = GetMaterialCollectionFloat("Cluster1_Angle");
	float Steepness = GetMaterialCollectionFloat("Cluster1_Steepness");
	float Phase = GetMaterialCollectionFloat("Cluster1_Phase");

	Sum += CalculateGerstnerWaveCluster(Angle, GlobalWaveLength, GlobalWaveAmplitude, Steepness, GlobalWaveDirection, Location, Time, Phase);

	// second cluster
	const float wavelength = GetMaterialCollectionFloat("Cluster2_MultiplierWavelength");
	const float amplitude = GetMaterialCollectionFloat("Cluster2_MultiplierAmplitude");
	Angle = GetMaterialCollectionFloat("Cluster2_Angle");
	Steepness = GetMaterialCollectionFloat("Cluster2_Steepness");
	Phase = GetMaterialCollectionFloat("Cluster2_Phase");

	Sum += CalculateGerstnerWaveCluster(Angle, wavelength * GlobalWaveLength, amplitude * GlobalWaveAmplitude, Steepness, GlobalWaveDirection, Location, Time, Phase);

	return Sum / 2 + FVector(0, 0, SeaLevel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoOceanManager::GetMaterialCollectionFloat(FString Name) const
{
	check(Name.IsEmpty() == false);
	// lost too much time on this https://answers.unrealengine.com/questions/172334/umaterialparametercollectioninstance-and-minimalap.html
	return UKismetMaterialLibrary::GetScalarParameterValue(GetWorld(), MaterialCollection, *Name);
}
