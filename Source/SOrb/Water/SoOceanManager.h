// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "Materials/MaterialParameterCollection.h"
#include "SoOceanManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSoWater, Log, All);

// TODO write comments
UCLASS(BlueprintType, Blueprintable)
class SORB_API ASoOceanManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoOceanManager();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Ocean Manager")
	FVector GetWaveHeight(const FVector& Location, const UWorld* World = nullptr) const;

	FORCEINLINE const FVector2D& GetGlobalWaveDirection() const { return GlobalWaveDirection; }
private:
	// Calculate the gertnser wave
	static FVector CalculateGerstnerWave(float RotationAngle,
										 float WaveLength,
										 float Amplitude,
										 float Steepness,
										 const FVector2D& Direction,
										 const FVector& Location,
										 float Time,
										 float TimePhase);

	// Calculate the cluster of 8
	FVector CalculateGerstnerWaveCluster(float MedianRotationAngle,
										 float MedianWaveLength,
										 float MedianAmplitude,
										 float MedianSteepness,
										 const FVector2D& MedianDirection,
										 const FVector& Location,
										 float Time,
										 float MedianTimePhase) const;

	// Gets the time from the argument if it's not null, otherwise use GetWorld()
	float GetTimeSeconds(const UWorld* World) const;

	// Get the float value from MaterialCollection
	float GetMaterialCollectionFloat(FString Name) const;

protected:
	// To have or not to have Gerstner waves
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean Manager")
	bool EnableGerstnerWaves;

	// The global direction the waves travel.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean Manager")
	FVector2D GlobalWaveDirection;

	// The global speed multiplier of the waves.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean Manager")
	float GlobalWaveLength;

	// The global amplitude multiplier of the waves.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean Manager")
	float GlobalWaveAmplitude;

	// The global size
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean Manager")
	float GlobalSize;

	// Get the wave values from the material collection with is shared with the Material
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean Manager")
	UMaterialParameterCollection* MaterialCollection;

	/**
	 * Optimization:
	 * If the distance of a point to base sea level exceeds DistanceCheck (too far below or above),
	 * skip the Gerstner calculations and return base sea level.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean Manager")
	float DistanceCheck;
};
