// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoMathHelper.h"
#include <cmath>

#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"

#include "SplineLogic/SoSpline.h"
#include "SplineLogic/SoSplinePoint.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint64 USoMathHelper::TruncDoubleToUint64(const double Number)
{
	return static_cast<uint64>(std::trunc(Number));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoMathHelper::AreActorsClose(AActor* First, AActor* Second, float MaxDistance)
{
	return First != nullptr &&
		   Second != nullptr &&
		   ((First->GetActorLocation() - Second->GetActorLocation()).SizeSquared() < MaxDistance * MaxDistance);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FRotator USoMathHelper::InterpolateRotation(FRotator A, FRotator B, float T, ESoInterpolationMethod Method)
{
	for (int32 i = 0; i < 3; ++i)
	{
		if (fabs((&A.Yaw)[i] - (&B.Yaw)[i]) > 180.0f)
		{
			if ((&A.Yaw)[i] < (&B.Yaw)[i])
				(&A.Yaw)[i] += 360;
			else
				(&B.Yaw)[i] += 360;
		}
	}

	return Interpolate(A, B, T, Method);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoMathHelper::InterpolateVector(const FVector& A, const FVector& B, float T, ESoInterpolationMethod Method)
{
	return Interpolate(A, B, T, Method);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoMathHelper::GetProjectedNormal(const FVector& PlaneDirVector, const FVector& Normal)
{
	FVector PlaneNormal = PlaneDirVector.CrossProduct(PlaneDirVector, FVector(0.f, 0.f, 1.f));
	PlaneNormal.Normalize();
	return FVector::VectorPlaneProject(Normal, PlaneNormal).GetSafeNormal();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoMathHelper::GetSlideVector(const FVector& Delta, float Time, const FVector& Normal, const FHitResult& Hit)
{
	FVector PlaneNormal = Delta.CrossProduct(Delta, FVector(0.f, 0.f, 1.f));
	PlaneNormal.Normalize();
	const FVector ProjectedNormal = FVector::VectorPlaneProject(Normal, PlaneNormal).GetSafeNormal();

	return FVector::VectorPlaneProject(Delta, ProjectedNormal) * Time;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoMathHelper::CalculateAngle(const FSoSplinePoint& SplinePoint, const FVector& V1, const FVector& V2)
{
	const FVector PlaneNormal = SplinePoint.GetPlaneNormal();
	const FVector ProjectedV1 = FVector::VectorPlaneProject(V1, PlaneNormal).GetSafeNormal();
	const FVector ProjectedV2 = FVector::VectorPlaneProject(V2, PlaneNormal).GetSafeNormal();

	const float Angle = FMath::Acos(FVector::DotProduct(ProjectedV1, ProjectedV2));
	const int32 Sign = FMath::Sign(FVector::DotProduct(PlaneNormal, FVector::CrossProduct(ProjectedV1, ProjectedV2)));
	return Angle * Sign;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoMathHelper::CalculateAngle(const FVector2D& V1, const FVector2D& V2)
{
	const FVector2D V1N = V1.GetSafeNormal();
	const FVector2D V2N = V2.GetSafeNormal();
	return FMath::Atan2(V2N.Y, V2N.X) - FMath::Atan2(V1N.Y, V1N.X);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoMathHelper::CalcRangeAttackVelocity(
	const FVector& StartPos,
	const FSoSplinePoint& StartLocation,
	const FVector& TargetPos,
	const FSoSplinePoint& TargetLocation,
	float BulletVelocity,
	float GravityScale,
	int32 PreferredDiscriminantSign)
{
	FVector Velocity = FVector(0.0f, 0.0f, 0.0f);
	const bool bOk = CalcRangeAttackInternal(
		nullptr,
		StartPos,
		StartLocation,
		TargetPos,
		TargetLocation,
		{},
		BulletVelocity,
		GravityScale,
		0,
		PreferredDiscriminantSign,
		Velocity
	);
	if (bOk)
		return Velocity;

	return FVector(0.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoMathHelper::CheckRangeAttack(
	const UWorld* World,
	const FVector& StartPos,
	const FSoSplinePoint& StartLocation,
	const FVector& TargetPos,
	const FSoSplinePoint& TargetLocation,
	const TArray<const AActor*>& ActorsToIgnore,
	float BulletVelocity,
	float GravityScale,
	int32 Precision,
	int32 PreferredDiscriminantSign
)
{
	FVector Velocity = FVector(0.0f, 0.0f, 0.0f);
	return CalcRangeAttackInternal(
		World,
		StartPos,
		StartLocation,
		TargetPos,
		TargetLocation,
		ActorsToIgnore,
		BulletVelocity,
		GravityScale,
		Precision,
		PreferredDiscriminantSign,
		Velocity
	);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoMathHelper::CalcRangeAttackVelocity(
	const FVector& StartPos,
	const FVector& TargetPos,
	float BulletVelocity,
	float GravityScale,
	int32 PreferredDiscriminantSign
)
{
	FVector Velocity = FVector(0.0f, 0.0f, 0.0f);
	CalcRangeAttackInternal(nullptr, StartPos, TargetPos, {}, BulletVelocity, GravityScale, 0, PreferredDiscriminantSign, Velocity);
	return Velocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoMathHelper::CheckRangeAttack(
	const UWorld* World,
	const FVector& StartPos,
	const FVector& TargetPos,
	const TArray<const AActor*>& ActorsToIgnore,
	float BulletVelocity,
	float GravityScale,
	int32 Precision,
	int32 PreferredDiscriminantSign
)
{
	FVector Velocity = FVector(0.0f, 0.0f, 0.0f);
	return CalcRangeAttackInternal(World, StartPos, TargetPos, ActorsToIgnore, BulletVelocity, GravityScale, Precision, PreferredDiscriminantSign, Velocity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoMathHelper::CalcRangeAttackInternal(
	const UWorld* World,
	const FVector& StartPos,
	const FSoSplinePoint& StartLocation,
	const FVector& TargetPos,
	const FSoSplinePoint& TargetLocation,
	const TArray<const AActor*>& ActorsToIgnore,
	float BulletVelocity,
	float GravityScale,
	int32 Precision,
	int32 PreferredDiscrimintantSign,
	FVector& OutVelocity
)
{
	// http://gamedev.stackexchange.com/questions/53552/how-can-i-find-a-projectiles-launch-angle
	// https://en.wikipedia.org/wiki/Trajectory_of_a_projectile#Angle_required_to_hit_coordinate_.28x.2Cy.29

	const float v = BulletVelocity;
	const float g = 1000 * GravityScale;

	const float Difference = StartLocation.GetDistanceFromSplinePoint(TargetLocation);
	const int32 Dir = -FMath::Sign(Difference);
	// distance along spline
	const float x = FMath::Abs(Difference);
	// target height delta
	const float y = TargetPos.Z - StartPos.Z;

	FVector2D Velocity;
	if (fabs(GravityScale) < KINDA_SMALL_NUMBER)
	{
		// trivial case, but idk why the rest of the magic does not handle this properly
		Velocity = FVector2D(x, y).GetSafeNormal() * BulletVelocity;
	}
	else
	{
		if (x > BIG_NUMBER / 2.0f)
			return false;

		const float Discriminant = (v*v*v*v) - g * (g * x * x + 2 * y * v * v);
		if (Discriminant < -KINDA_SMALL_NUMBER)
			return false;

		if (Discriminant < KINDA_SMALL_NUMBER)
			Velocity = FVector2D(g * x, v * v).GetSafeNormal() * BulletVelocity;
		else
			Velocity = FVector2D(g * x, v * v + PreferredDiscrimintantSign * FMath::Sqrt(Discriminant)).GetSafeNormal() * BulletVelocity;
	}

	OutVelocity = StartLocation.GetDirection() * Velocity.X * Dir;
	OutVelocity.Z = Velocity.Y;

	FVector LastWorldPoint = StartPos;
	for (int32 i = 1; i <= Precision && World != nullptr; ++i)
	{
		const float xt = i * (x / static_cast<float>(Precision));
		const float t = xt / Velocity.X;
		const float yt = Velocity.Y * t - g / 2.0f * t * t;

		FVector ActualWorldPoint = (StartLocation + Dir * xt);
		ActualWorldPoint.Z = StartPos.Z + yt;

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
		FCollisionQueryParams QueryParams;
		for (auto* Actor : ActorsToIgnore)
				QueryParams.AddIgnoredActor(Actor);

		const bool bHit = World->LineTraceTestByObjectType(LastWorldPoint, ActualWorldPoint, ObjectQueryParams, QueryParams);

		// const FColor DebugColor = bHit ? FColor(255, 0, 0) : FColor(0, 255, 0);
		// DrawDebugLine(World, LastWorldPoint, ActualWorldPoint, DebugColor, false, 1.0f, 0, 12.333);

		if (bHit)
			return false;

		LastWorldPoint = ActualWorldPoint;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoMathHelper::CalcRangeAttackInternal(
	const UWorld* World,
	const FVector& StartPos,
	const FVector& TargetPos,
	const TArray<const AActor*>& ActorsToIgnore,
	float BulletVelocity,
	float GravityScale,
	int32 Precision,
	int32 PreferredDiscrimintantSign,
	FVector& OutVelocity
)
{
	// http://gamedev.stackexchange.com/questions/53552/how-can-i-find-a-projectiles-launch-angle
	// https://en.wikipedia.org/wiki/Trajectory_of_a_projectile#Angle_required_to_hit_coordinate_.28x.2Cy.29

	const float v = BulletVelocity;
	const float g = 1000 * GravityScale;

	const FVector SourceToTarget = (TargetPos - StartPos);

	// horizontal distance
	const float x = (TargetPos - StartPos).Size2D();
	// target height delta
	const float y = TargetPos.Z - StartPos.Z;

	const float Discriminant = (v*v*v*v) - g * (g * x * x + 2 * y * v * v);
	if (Discriminant < -KINDA_SMALL_NUMBER)
		return false;

	FVector2D Velocity;
	if (Discriminant < KINDA_SMALL_NUMBER)
		Velocity = FVector2D(g * x, v * v).GetSafeNormal() * BulletVelocity;
	else
		Velocity = FVector2D(g * x, v * v + PreferredDiscrimintantSign * FMath::Sqrt(Discriminant)).GetSafeNormal() * BulletVelocity;

	const FVector Dir = FVector(SourceToTarget.X, SourceToTarget.Y, 0.0f).GetSafeNormal();
	OutVelocity = Dir * Velocity.X;
	OutVelocity.Z = Velocity.Y;

	FVector LastWorldPoint = StartPos;
	for (int32 i = 1; i <= Precision && World != nullptr; ++i)
	{
		const float xt = i * (x / static_cast<float>(Precision));
		const float t = xt / Velocity.X;
		const float yt = Velocity.Y * t - g / 2.0f * t * t;

		FVector ActualWorldPoint = (StartPos + Dir * xt);
		ActualWorldPoint.Z = StartPos.Z + yt;

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
		FCollisionQueryParams QuaryParams;
		for (auto* Actor : ActorsToIgnore)
				QuaryParams.AddIgnoredActor(Actor);

		const bool bHit = World->LineTraceTestByObjectType(LastWorldPoint, ActualWorldPoint, ObjectQueryParams, QuaryParams);

		// const FColor DebugColor = bHit ? FColor(255, 0, 0) : FColor(0, 255, 0);
		// DrawDebugLine(World, LastWorldPoint, ActualWorldPoint, DebugColor, false, 1.0f, 0, 12.333);

		if (bHit)
			return false;

		LastWorldPoint = ActualWorldPoint;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoMathHelper::ClampYaw(float AngleCenter, float MaxDistance, float AngleToClamp)
{
	// force both angle to be in [0-360]
	AngleCenter = FMath::Fmod(AngleCenter + 360.0f, 360.0f);
	AngleToClamp = FMath::Fmod(AngleToClamp + 360.0f, 360.0f);

	const float MinAngle = AngleCenter - MaxDistance;
	const float MaxAngle = AngleCenter + MaxDistance;

	// nothing to done if it is already in the interval
	if (AngleToClamp >= MinAngle && AngleToClamp <= MaxAngle)
		return AngleToClamp;

	// clamp to closest border, both direction is checked
	const float PossibleTargets[4] = {
		MinAngle,
		MinAngle + 360 * (MinAngle > AngleToClamp ? -1 : 1),
		MaxAngle,
		MaxAngle + 360 * (MaxAngle > AngleToClamp ? -1 : 1)
	};

	float SelectedDistance = fabs(AngleToClamp - MinAngle);
	int32 SelectedIndex = 0;
	for (int32 i = 1; i < 4; ++i)
	{
		const float ActualDistance = fabs(AngleToClamp - PossibleTargets[i]);
		if (ActualDistance < SelectedDistance)
		{
			SelectedDistance = ActualDistance;
			SelectedIndex = i;
		}
	}

	return PossibleTargets[SelectedIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoMathHelper::CalculateCurrentPositionAndSteepnessOnParabol(
	float Percent,
	const FVector2D& SourceToTarget,
	float MiddlePointPercentX,
	float MiddleZ
)
{
	// https://www.desmos.com/calculator/lac2i0bgum
	// X1, Y1 = 0,0
	// X2, Y2 = SourceToTarget.X, SourceToTarget.Y
	// X3, Y3 = SourceToTarget.X / 2, MiddleZ

	// f(x) = aX^2 + bx + c

	// A1 = -X1^2 + X2^2
	// B1 = -X1 + X2
	// D1 = -Y1 + Y2
	// A2 = -X2^2 + X3^2
	// B2 = -X2 + X3
	// D2 = -Y2 + Y3
	// BMultiplier = - (B2/B1)
	// A3 = BMultiplier * A1 + A2
	// D3 = BMultiplier * D1 + D2

	// a = D3/A3
	// b = (D1 -A1*a/)B1
	// c = y1 - aX1^2 - bX1

	const float X3 = SourceToTarget.X * MiddlePointPercentX;

	const float A1 = SourceToTarget.X * SourceToTarget.X;
	const float B1 = SourceToTarget.X;
	const float D1 = SourceToTarget.Y;
	const float A2 = -SourceToTarget.X * SourceToTarget.X + X3 * X3;
	const float B2 = -SourceToTarget.X + X3;
	const float D2 = -SourceToTarget.Y + MiddleZ;
	const float BMultiplier = -(B2 / B1);
	const float A3 = BMultiplier * A1 + A2;
	const float D3 = BMultiplier * D1 + D2;

	const float a = D3 / A3;
	const float b = (D1 - A1 * a) / B1;
	// const float c = 0;

	const float x = SourceToTarget.X * Percent;
	const float y = a * x * x + b * x;

	// derivative: m = 2ax + b
	// y = mx + b
	const float m = 2 * a * x + b;
	// const FVector2D Dir0 = FVector2D(0.0f, b);
	// const FVector2D Dir1 = FVector2D(1.0f, m + b);
	// const FVector2D Direction = (Dir1 - Dir0).GetSafeNormal();
	return FVector(x, y, FVector2D(1.0f, m).GetSafeNormal().Y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copied from imageplatecomponent.cpp
FMatrix CalculateProjectionMatrix(const FMinimalViewInfo& MinimalView)
{
	FMatrix ProjectionMatrix;

	if (MinimalView.ProjectionMode == ECameraProjectionMode::Orthographic)
	{
		const float YScale = 1.0f / MinimalView.AspectRatio;

		const float HalfOrthoWidth = MinimalView.OrthoWidth / 2.0f;
		const float ScaledOrthoHeight = MinimalView.OrthoWidth / 2.0f * YScale;

		const float NearPlane = MinimalView.OrthoNearClipPlane;
		const float FarPlane = MinimalView.OrthoFarClipPlane;

		const float ZScale = 1.0f / (FarPlane - NearPlane);
		const float ZOffset = -NearPlane;

		ProjectionMatrix = FReversedZOrthoMatrix(
			HalfOrthoWidth,
			ScaledOrthoHeight,
			ZScale,
			ZOffset
		);
	}
	else
	{
		// Avoid divide by zero in the projection matrix calculation by clamping FOV
		ProjectionMatrix = FReversedZPerspectiveMatrix(
			FMath::Max(0.001f, MinimalView.FOV) * (float)PI / 360.0f,
			MinimalView.AspectRatio,
			1.0f,
			GNearClippingPlane
		);
	}

	if (!MinimalView.OffCenterProjectionOffset.IsZero())
	{
		const float Left = -1.0f + MinimalView.OffCenterProjectionOffset.X;
		const float Right = Left + 2.0f;
		const float Bottom = -1.0f + MinimalView.OffCenterProjectionOffset.Y;
		const float Top = Bottom + 2.0f;
		ProjectionMatrix.M[2][0] = (Left + Right) / (Left - Right);
		ProjectionMatrix.M[2][1] = (Bottom + Top) / (Bottom - Top);
	}

	return ProjectionMatrix;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoMathHelper::CalculateVectorsForShadowTransform(USceneCaptureComponent2D* SceneCaptureComponent, FLinearColor& OutRow0, FLinearColor& OutRow1)
{
	FMinimalViewInfo MinimalViewInfo;

	if (!SceneCaptureComponent)
		return;

	MinimalViewInfo.Location = SceneCaptureComponent->GetComponentLocation();
	MinimalViewInfo.Rotation = SceneCaptureComponent->GetComponentRotation();

	MinimalViewInfo.FOV = SceneCaptureComponent->FOVAngle;
	MinimalViewInfo.AspectRatio = SceneCaptureComponent->TextureTarget ? float(SceneCaptureComponent->TextureTarget->SizeX) / SceneCaptureComponent->TextureTarget->SizeY : 1.f;
	MinimalViewInfo.bConstrainAspectRatio = false;
	MinimalViewInfo.ProjectionMode = SceneCaptureComponent->ProjectionType;
	MinimalViewInfo.OrthoWidth = SceneCaptureComponent->OrthoWidth;

	const FMatrix ViewRotationMatrix = FInverseRotationMatrix(
		MinimalViewInfo.Rotation) * FMatrix(FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1)
	);

	FMatrix ProjectionMatrix;
	if (SceneCaptureComponent && SceneCaptureComponent->bUseCustomProjectionMatrix)
		ProjectionMatrix = AdjustProjectionMatrixForRHI(SceneCaptureComponent->CustomProjectionMatrix);
	else
		ProjectionMatrix = AdjustProjectionMatrixForRHI(CalculateProjectionMatrix(MinimalViewInfo));

	const FMatrix ViewMatrix = FTranslationMatrix(-MinimalViewInfo.Location) * ViewRotationMatrix;

	const FMatrix Matrix = (ViewMatrix * ProjectionMatrix).GetTransposed();
	OutRow0 = FLinearColor(Matrix.M[0][0], Matrix.M[0][1], Matrix.M[0][2], Matrix.M[0][3]);
	OutRow1 = FLinearColor(Matrix.M[1][0], Matrix.M[1][1], Matrix.M[1][2], Matrix.M[1][3]);
}
