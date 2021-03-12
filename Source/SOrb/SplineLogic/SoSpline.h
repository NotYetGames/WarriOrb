// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"

#include "SoSpline.generated.h"

UCLASS()
class SORB_API ASoSpline : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoSpline(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	bool IsInPrevSplines(ASoSpline* SplineToCheck) const;
	bool IsInNextSplines(ASoSpline* SplineToCheck) const;

	ASoSpline* GetPrevSpline(const float ZCoordinate) const;
	ASoSpline* GetNextSpline(const float ZCoordinate) const;

	const TArray<ASoSpline*>& GetPrevSplines() const { return PrevSplines; }
	const TArray<ASoSpline*>& GetNextSplines() const { return NextSplines; }

	FVector GetWorldLocationAtDistance(const float Distance) const;
	FRotator GetWorldRotationAtDistance(const float Distance) const;

	FVector GetStartWorldLocation() const;
	FVector GetEndWorldLocation() const;

	const USplineComponent* GetSplineComponent() const { return Spline; }
	USplineComponent* GetSplineComponent() { return Spline; }
	int32 GetSplineDirection() const { return Direction; }
	float GetSplineLength() const { return Spline ? Spline->GetSplineLength() : 0.f; }
	const FString& GetSplineName() const { return SplineName; }
	const FName GetSplineFName() const { return FName(*SplineName); }

	UFUNCTION(BlueprintCallable, Category = Construction)
	static void MakeCircleSpline(ASoSpline* Spline, float Radius, float TangentModifier, const FVector Scale = FVector(1.0f, 1.0f, 1.0f));

	UFUNCTION(BlueprintCallable, Category = Construction)
	static void MakeSymmetry(ASoSpline* Spline);

protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = SoSpline)
	USplineComponent* Spline;

	// -1 or 1, defines the camera angle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|General")
	int32 Direction = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	FString SplineName = "DefaultSplineName";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	TArray<ASoSpline*> NextSplines;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	TArray<ASoSpline*> PrevSplines;

	// 3d widgets to visualize the borders
	UPROPERTY(EditAnywhere, EditFixedSize, BlueprintReadOnly, Category = "LevelDesign|General", Meta = (MakeEditWidget = true))
	TArray<FVector> PrevZLevels;

	UPROPERTY(EditAnywhere, EditFixedSize, BlueprintReadOnly, Category = "LevelDesign|General", Meta = (MakeEditWidget = true))
	TArray<FVector> NextZLevels;

	// spline distances the AI won't cross (e.g. to stop him from walking off a cliff)
	UPROPERTY(VisibleAnywhere, Category = AIData)
	TArray<float> AIStopPoints;

	UPROPERTY(EditAnywhere, Category = AIData)
	bool bShowAIStopPoints;

	// used to visualize and edit the points if bShowAIStopPoints is true
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AIData, Meta = (MakeEditWidget = true))
	TArray<FVector> AIStopPointsV;
};
