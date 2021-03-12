// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

#include "SoSpline.h"
#include "SoSkyControlData.h"
#include "SoCameraData.h"
#include "SoCharShadowData.h"
#include "SaveFiles/Stats/SoPlayerProgressSplineStats.h"
#include "Basic/Helpers/SoPlatformHelper.h"

#include "SoPlayerSpline.generated.h"

class ASoMarker;
class UFMODEvent;
class ASoPlayerSpline;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LowerBorder and UpperBorder have to be on the same spline, they define a spline segment
// The streaming level with the name LevelName has to be active if the player is in the defined segment
// if a Border Marker is nullptr the interval is opened in the given direction
USTRUCT(BlueprintType)
struct FSoLevelClaim
{
	GENERATED_USTRUCT_BODY()

public:
	// Does this level claim cover the spline distance?
	bool DoesClaim(const float SplineDistance) const;

public:
	// border with smaller distance from spline value (nullptr means 0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ASoMarker* LowerBorder;

	// border with bigger distance from spline value (nullptr means endless)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ASoMarker* UpperBorder;

	// streaming level bounded by this segment
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "World"))
	FSoftObjectPath Level;

	// name of the streaming level bounded by this segment
	UPROPERTY(BlueprintReadOnly)
	FName LevelName;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// used by the editor to setup the level claims
USTRUCT(BlueprintType)
struct FSoLevelClaimEntry
{
	GENERATED_USTRUCT_BODY()

public:
	// border with smaller distance from spline value (nullptr means 0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ASoMarker* LowerBorder = nullptr;

	// border with bigger distance from spline value (nullptr means endless)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ASoMarker* UpperBorder = nullptr;

	// the associated spline
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ASoPlayerSpline* Spline = nullptr;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoSoulKeeperFreeSegment
{
	GENERATED_USTRUCT_BODY()

public:
	// border with smaller distance from spline value (nullptr means 0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ASoMarker* LowerBorder = nullptr;

	// border with bigger distance from spline value (nullptr means endless)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ASoMarker* UpperBorder = nullptr;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Used to stop player from placing Soulkeeper to placed enemy spawn positions
 *  Registered in ASoEnemy Post Load
 *  Only counts if the enemy group is still alive (or none)
 */
USTRUCT(BlueprintType)
struct FSoEnemySpawnLocation
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Distance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ZDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName GroupName;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoAmbientParameter
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ParameterValue;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoAmbientControlPoint
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (MakeEditWidget = true))
	FVector Location;

	UPROPERTY(VisibleAnywhere)
	float DistanceOnSpline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSoAmbientParameter> Parameters;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API ASoPlayerSpline : public ASoSpline
{
	GENERATED_BODY()

public:

	ASoPlayerSpline(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	void RegisterEnemySpawnLocation(float Distance, float ZDistance, FName GroupName);

	void SetupCameraData(const FSoCamNodeList& CamNodeList);
	const FSoCamNodeList& GetCamNodeList() const { return CamNodes; }
	FSoCamNodeList& GetCamNodeList() { return CamNodes; }

	UFUNCTION(BlueprintCallable, Category = Camera)
	int32 GetCamKeyNum() const { return CamNodes.CamKeys.Num(); }

	const TArray<FSoLevelClaim>& GetLevelClaims() const { return LevelClaims; }

	const TArray<FSoAmbientControlPoint>& GetAmbientControlPoints() const { return AmbientControlPointsWorldSpace; }

	FSoSkyControlPoints& GetSkyControlPoints() { return SkyControlPoints; }
	const FSoSkyControlPoints& GetSkyControlPoints() const { return SkyControlPoints; }

	FSoCharShadowNodeList& GetCharShadowControlPoints() { return CharShadowControlPoints; }
	const FSoCharShadowNodeList& GetCharShadowControlPoints() const { return CharShadowControlPoints; }

	bool IsSoulkeeperUsageAllowed(float Distance, float ZDistance) const;

	bool ShouldSpawnSKBetweenCharacterAndCamera() const { return bSpawnSKBetweenCharacterAndCamera; }

	void UpdateCameraKeyData();
	void UpdateAmbientControlKeys();

	UFMODEvent* GetMusic() const { return Music; }

	float GetCharacterEmissiveStrength() const { return CharacterEmissiveStrength; }
	bool ShouldUseCharacterEmissiveStrength() const { return bUseCharacterEmissiveStrength; }

	UFUNCTION(BlueprintPure, Category = LevelDesgin)
	ASoMarker* GetResPointOverride(bool bExit) const { return bExit ? ResPointOverrideExit : ResPointOverride; }

	UFUNCTION(BlueprintCallable, Category = LevelDesgin)
	void SetRotateCameraWith180Yaw(bool bEnable) { bRotateCameraWith180Yaw = bEnable; }

	bool GetRotateCameraWith180Yaw() const { return bRotateCameraWith180Yaw; }

	//
	// Performance stats this player spline.
	// NOTE: Kept in the Spline so that it is easier to track and calculate
	//

	FORCEINLINE void TickPerformanceStats(float DeltaSeconds)
	{
		FrameTickForAverageNum++;
		FrameTickForCriticalNum++;
		TimeSinceAverageReset += DeltaSeconds;
		TimeSinceCriticalReset += DeltaSeconds;
	}

	// Updates the Critical FPS
	void UpdateCriticalPerformanceStats(float PlayerDistance)
	{
		if (TimeSinceCriticalReset < 1.f)
			return;

		if (USoPlatformHelper::IsGameInBackground(this))
		{
			// Prevent counting when game is in background
			ResetCriticalPerformanceStats();
			return;
		}

		const float AverageCriticalFPS = FrameTickForCriticalNum / TimeSinceCriticalReset;
		if (IsCriticalFPS(AverageCriticalFPS))
		{
			// Add new critical FPS entry
			FSoSplineCriticalFPSLocation CriticalFPS;
			CriticalFPS.UpdateCriticalAverageFPS(AverageCriticalFPS);
			CriticalFPS.UpdateAverageDistance(PlayerDistance);
			CriticalFPS.TimeSeconds = TimeSinceCriticalReset;

			CriticalFPSAreas.AddCriticalFPS(CriticalFPS);
		}

		ResetCriticalPerformanceStats();
	}

	FORCEINLINE float GetAverageFPS() const
	{
		return FrameTickForAverageNum / TimeSinceAverageReset;
	}

	FORCEINLINE bool HasAveragePerformanceStats(float AccumulateThreshold = 1.0f) const
	{
		return FrameTickForAverageNum > 0 && TimeSinceAverageReset > AccumulateThreshold;
	}

	FORCEINLINE bool HasCriticalPerformanceStats() const
	{
		return CriticalFPSAreas.HasAreas();
	}

	FORCEINLINE const FSoSplineCriticalFPSAreas& GetCriticalFPSAreas() const
	{
		return CriticalFPSAreas;
	}

	FORCEINLINE void ResetCriticalFPSAreas()
	{
		CriticalFPSAreas = {};
	}

	FORCEINLINE void ResetAveragePerformanceStats()
	{
		FrameTickForAverageNum = 0;
		TimeSinceAverageReset = 0.f;
	}

	FORCEINLINE void ResetCriticalPerformanceStats()
	{
		FrameTickForCriticalNum = 0;
		TimeSinceCriticalReset = 0.f;
	}

	static FORCEINLINE bool IsCriticalFPS(float FPS)
	{
		return FPS < CriticalFPSThreshold;
	}

public:
	static bool bHideCameraKeys;

	// Under this FPS everything gets reported
	static constexpr float CriticalFPSThreshold = 27.f;

protected:

	// list of streaming levels and associated spline segments
	// markers have to be in the same level as the spline
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	TArray<FSoLevelClaim> LevelClaims;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	TArray<FSoSoulKeeperFreeSegment> SoulKeeperFreeSegments;

	/** runtime constructed array of placed enemy locations */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	TArray<FSoEnemySpawnLocation> EnemySpawnLocations;

	/** Character Respawns here instead of arcade if he dies on this Spline if set */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	ASoMarker* ResPointOverride;

	/** Character can respawn here instead of ResPointOverride if he choses to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	ASoMarker* ResPointOverrideExit;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	FText AreaName;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	UFMODEvent* Music;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	float CharacterEmissiveStrength = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	bool bUseCharacterEmissiveStrength = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	bool bSpawnSKBetweenCharacterAndCamera = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelDesign|General")
	bool bRotateCameraWith180Yaw = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|CamData")
	FSoCamNodeList CamNodes;

	// camera point locations in local space - they will try to keep it as much as possible when the spline is modified
	UPROPERTY(EditFixedSize)
	TArray<FVector> CameraKeyPoints;

	// used to visualize (and replace) camera keys
	UPROPERTY(EditAnywhere, EditFixedSize, BlueprintReadWrite, Category = "LevelDesign|CamData", Meta = (MakeEditWidget = true))
	TArray<FVector> CameraKeyPointsV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|CamData")
	float CamKeyVisZOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|Ambient")
	TArray<FSoAmbientControlPoint> AmbientControlPoints;

	TArray<FSoAmbientControlPoint> AmbientControlPointsWorldSpace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|SkyControl")
	FSoSkyControlPoints SkyControlPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|CharShadowControl")
	FSoCharShadowNodeList CharShadowControlPoints;

	// Performance variables
	int32 FrameTickForAverageNum = 0;
	int32 FrameTickForCriticalNum = 0;

	float TimeSinceAverageReset = 0.f;
	float TimeSinceCriticalReset = 0.f;

	// Holds the instances of the critical fps entries
	FSoSplineCriticalFPSAreas CriticalFPSAreas;
};
