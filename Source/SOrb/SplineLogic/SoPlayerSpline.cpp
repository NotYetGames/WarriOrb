// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoPlayerSpline.h"

#include "Components/SplineComponent.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Character/SoCharacter.h"
#include "Levels/SoLevelManager.h"
#include "SplineLogic/SoMarker.h"
#include "SplineLogic/SoEditorGameInterface.h"
#include "Basic/SoGameMode.h"
#include "Levels/SoLevelHelper.h"

bool ASoPlayerSpline::bHideCameraKeys = true;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoLevelClaim::DoesClaim(const float SplineDistance) const
{
	return (LowerBorder == nullptr || LowerBorder->GetSplineLocation().GetDistance() <= SplineDistance)
		&& (UpperBorder == nullptr || UpperBorder->GetSplineLocation().GetDistance() >= SplineDistance);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoPlayerSpline::ASoPlayerSpline(const FObjectInitializer& ObjectInitializer) :
	ASoSpline(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerSpline::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SkyControlPoints.ValidateStartAndEnd(this);

	// naming scheme:
	// - first two characters determines a level (e.g. LLShore -> LuckyLake
	// - first two characters determines a level + To + next two characters determines another level (e.g. PCToDO -> PortalCave, DwarvenOutpost)
	const FName PrimaryLevelFromName = USoLevelHelper::GetLevelNameFromPreTag(SplineName.Left(2));
	bool bHasPrimaryLevelFromName = false;
	FName SecondaryLevelFromName = NAME_None;
	bool bHasSecondaryLevelFromName = false;

	if (SplineName.Len() > 6 && SplineName.Mid(2, 2).Equals("To"))
		SecondaryLevelFromName = USoLevelHelper::GetLevelNameFromPreTag(SplineName.Mid(4, 2));

	for (auto& LevelClaim : LevelClaims)
	{
		LevelClaim.LevelName = FName(*LevelClaim.Level.GetAssetName());

		if (LevelClaim.LevelName == PrimaryLevelFromName || LevelClaim.LevelName == SecondaryLevelFromName)
		{
			// level based on name, whole spline must be involved
			LevelClaim.LowerBorder = nullptr;
			LevelClaim.UpperBorder = nullptr;
			if (LevelClaim.LevelName == PrimaryLevelFromName)
				bHasPrimaryLevelFromName = true;
			else
				bHasSecondaryLevelFromName = true;
		}
		else
		{
			bool bPrintError = false;
			if (LevelClaim.LowerBorder != nullptr && LevelClaim.LowerBorder->GetLevel() != GetLevel())
			{
				LevelClaim.LowerBorder = nullptr;
				bPrintError = true;
			}
			if (LevelClaim.UpperBorder != nullptr && LevelClaim.UpperBorder->GetLevel() != GetLevel())
			{
				LevelClaim.UpperBorder = nullptr;
				bPrintError = true;
			}
			if (bPrintError)
				UE_LOG(LogSoLevelManager, Warning, TEXT("LevelClaim marker has to be located in the same level as the spline it is located at!"));

			// ToThink: maybe we don't need this, if we ever need to define to opened spline segments for the same streaming level, it should be removed :/
			if (LevelClaim.LowerBorder != nullptr && LevelClaim.UpperBorder != nullptr &&
				LevelClaim.LowerBorder->GetSplineLocation().GetDistance() > LevelClaim.UpperBorder->GetSplineLocation().GetDistance())
			{
				auto* Temp = LevelClaim.LowerBorder;
				LevelClaim.LowerBorder = LevelClaim.UpperBorder;
				LevelClaim.UpperBorder = Temp;

				UE_LOG(LogSoLevelManager, Warning, TEXT("LevelClaim markers swapped: LowerBorder has to have the smaller spline distance!"));
			}
		}
	}

	if (!bHasPrimaryLevelFromName && PrimaryLevelFromName != NAME_None)
		LevelClaims.Add( FSoLevelClaim{ nullptr, nullptr, *USoLevelHelper::GetLevelFromPreTag(SplineName.Left(2)), PrimaryLevelFromName });

	if (!bHasSecondaryLevelFromName && SecondaryLevelFromName != NAME_None)
		LevelClaims.Add(FSoLevelClaim{ nullptr, nullptr, *USoLevelHelper::GetLevelFromPreTag(SplineName.Mid(4, 2)), SecondaryLevelFromName });

	for (auto& Segment : SoulKeeperFreeSegments)
		if (Segment.LowerBorder != nullptr && Segment.UpperBorder != nullptr && Segment.LowerBorder->GetDistanceOnSpline() > Segment.UpperBorder->GetDistanceOnSpline())
			Swap(Segment.LowerBorder, Segment.UpperBorder);

	UpdateCameraKeyData();

	UpdateAmbientControlKeys();
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts or when spawned
void ASoPlayerSpline::BeginPlay()
{
	Super::BeginPlay();

	// init cam data - maybe there is no entry atm
	CamNodes.CamKeys.Empty();
	CamNodes.CamKeys.Add(FSoCamKeyNode(FSoSplinePoint(this, 0)));
	CamNodes.CamKeys.Add(FSoCamKeyNode(FSoSplinePoint(this, Spline->GetSplineLength())));

	AmbientControlPointsWorldSpace = AmbientControlPoints;
	for (FSoAmbientControlPoint& ControlPoint : AmbientControlPointsWorldSpace)
		ControlPoint.Location = GetTransform().TransformPosition(ControlPoint.Location);

	AmbientControlPointsWorldSpace.Sort([](const FSoAmbientControlPoint& A, const FSoAmbientControlPoint& B)
	{
		return A.DistanceOnSpline < B.DistanceOnSpline;
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoPlayerSpline::IsSoulkeeperUsageAllowed(float Distance, float ZDistance) const
{
	for (const auto& Segment : SoulKeeperFreeSegments)
		if ((Segment.LowerBorder == nullptr || Segment.LowerBorder->GetDistanceOnSpline() < Distance) &&
			(Segment.UpperBorder == nullptr || Segment.UpperBorder->GetDistanceOnSpline() > Distance))
			return false;

	static const float SoulkeeperBanHalfWidth = 200.0f;
	static const float SoulkeeperBanHalfHeight = 400.0f;

	for (const auto& SpawnLoc : EnemySpawnLocations)
		if ((fabs(Distance - SpawnLoc.Distance) < SoulkeeperBanHalfWidth) && (fabs(ZDistance - SpawnLoc.ZDistance) < SoulkeeperBanHalfHeight))
		{
			if (SpawnLoc.GroupName == NAME_None ||
				ASoGameMode::Get(this).IsEnemyGroupStillActive(SpawnLoc.GroupName))
				return false;
		}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerSpline::RegisterEnemySpawnLocation(float Distance, float ZDistance, FName GroupName)
{
	// one location should be only registered once
	for (const FSoEnemySpawnLocation& SpawnLoc : EnemySpawnLocations)
		if (fabs(SpawnLoc.Distance - Distance) < 1.0f &&
			fabs(SpawnLoc.ZDistance - ZDistance) < 1.0f &&
			SpawnLoc.GroupName == GroupName)
			return;

	EnemySpawnLocations.Add(FSoEnemySpawnLocation{ Distance, ZDistance, GroupName });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerSpline::SetupCameraData(const FSoCamNodeList& CamNodeList)
{
	if (CamNodeList.CamKeys.Num() < 2)
	{
		UE_LOG(LogSoCameraDataIO, Error, TEXT("ASoSpline::SetupCameraData failed - input must contain at least 2 nodes"));
		return;
	}

	CameraKeyPoints.Empty();

	CamNodes = CamNodeList;
	// CamNodes.ReScale(Spline->GetSplineLength());
	CamNodes.CamKeys[CamNodes.CamKeys.Num() - 1].Distance = Spline->GetSplineLength();

	for (int i = 0; i < CamNodes.CamKeys.Num(); ++i)
	{
		CamNodes.CamKeys[i].SplinePoint = FSoSplinePoint(this, CamNodes.CamKeys[i].Distance);
		CamNodes.CamKeys[i].Position = CamNodes.CamKeys[i].SplinePoint;

#if WITH_EDITOR
		CameraKeyPoints.Add(GetTransform().InverseTransformPosition(CamNodes.CamKeys[i].Position + FVector(0, 0, CamKeyVisZOffset)));

		if (!bHideCameraKeys)
			CameraKeyPointsV = CameraKeyPoints;
#endif
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerSpline::UpdateCameraKeyData()
{
	if (CameraKeyPoints.Num() == CamNodes.CamKeys.Num() && CameraKeyPoints.Num() > 1)
	{
		// update cam data based on visualized stuff
		if (!bHideCameraKeys && CameraKeyPointsV.Num() == CamNodes.CamKeys.Num())
				CameraKeyPoints = CameraKeyPointsV;

		// fix first key & visualization point
		CamNodes.CamKeys[0].Distance = 0.0f;
		CamNodes.CamKeys[0].SplinePoint = FSoSplinePoint(this, 0.0f);
		CamNodes.CamKeys[0].Position = CamNodes.CamKeys[0].SplinePoint;
		CameraKeyPoints[0] = GetTransform().InverseTransformPosition(CamNodes.CamKeys[0].Position + FVector(0, 0, CamKeyVisZOffset));

		// fix last key & visualization point
		const int32 LastIndex = CamNodes.CamKeys.Num() - 1;
		CamNodes.CamKeys[LastIndex].Distance = Spline->GetSplineLength();
		CamNodes.CamKeys[LastIndex].SplinePoint = FSoSplinePoint(this, CamNodes.CamKeys[LastIndex].Distance);
		CamNodes.CamKeys[LastIndex].Position = CamNodes.CamKeys[LastIndex].SplinePoint;
		CameraKeyPoints[LastIndex] = GetTransform().InverseTransformPosition(CamNodes.CamKeys[LastIndex].Position + FVector(0, 0, CamKeyVisZOffset));

		// modify cam data based on the rest
		for (int32 i = 1; i < LastIndex; ++i)
		{
			// calc spline point based on the visualizer point
			FSoSplinePoint& Point = CamNodes.CamKeys[i].SplinePoint;
			FVector WorldPoint = GetTransform().TransformPosition(CameraKeyPoints[i]);
			WorldPoint.Z = GetActorLocation().Z;
			Point.SetDistanceFromWorldLocation(WorldPoint);
			// refresh data based on the calculated spline point
			CamNodes.CamKeys[i].Distance = Point.GetDistance();
			CamNodes.CamKeys[i].Position = Point;
		}

		// sort based on distance to keep the data valid
		CamNodes.CamKeys.Sort([](const FSoCamKeyNode& First, const FSoCamKeyNode& Second) { return First.Distance < Second.Distance; });

		// refresh visualizer locations to force them to the spline
		for (int32 i = 1; i < LastIndex; ++i)
			CameraKeyPoints[i] = GetTransform().InverseTransformPosition(CamNodes.CamKeys[i].Position + FVector(0, 0, CamKeyVisZOffset));
	}
	else
	{
		CameraKeyPoints.Empty();
		for (int i = 0; i < CamNodes.CamKeys.Num(); ++i)
			CameraKeyPoints.Add(GetTransform().InverseTransformPosition(CamNodes.CamKeys[i].Position + FVector(0, 0, CamKeyVisZOffset)));
	}

	// modify the visualizers based on the real data
	if (bHideCameraKeys)
		CameraKeyPointsV.Empty();
	else
		CameraKeyPointsV = CameraKeyPoints;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoPlayerSpline::UpdateAmbientControlKeys()
{
	for (FSoAmbientControlPoint& ControlPoint : AmbientControlPoints)
	{
		// calc spline point based on the visualizer point
		FSoSplinePoint Point;
		Point.SetSpline(this);
		const FVector WorldPoint = FVector(FVector2D(GetTransform().TransformPosition(ControlPoint.Location)), GetActorLocation().Z);
		Point.SetDistanceFromWorldLocation(WorldPoint);
		ControlPoint.DistanceOnSpline = Point.GetDistance();

		const FVector NewLocation = GetTransform().InverseTransformPosition(FVector(Point));
		ControlPoint.Location.X = NewLocation.X;
		ControlPoint.Location.Y = NewLocation.Y;
	}
}
