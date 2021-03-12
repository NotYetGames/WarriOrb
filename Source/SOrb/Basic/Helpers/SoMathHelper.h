// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoMathHelper.generated.h"

struct FHitResult;
struct FSoSplinePoint;
class USceneCaptureComponent2D;

UENUM()
enum class ESoInterpolationMethod : uint8
{
	// https://codeplea.com/simple-interpolation

	EIM_Linear			UMETA(DisplayName = "Linear"),
	EIM_Cosine			UMETA(DisplayName = "Cosine"),
	EIM_SmoothStep		UMETA(DisplayName = "SmoothStep"),
	EIM_Acceleration	UMETA(DisplayName = "Acceleration"),
	EIM_Deceleration	UMETA(DisplayName = "Deceleration"),

	EIM_NumOf			UMETA(Hidden)
};

UCLASS()
class SORB_API USoMathHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = Math)
	static bool AreActorsClose(AActor* First, AActor* Second, float MaxDistance);


	/**
	 * Make Number wrap around MaxNumber aka be in the range [0, MaxNumber)
	 * This function is basically the same as Number % MaxNumber but it also
	 * works with negative numbers.
	 *
	 * @param Number
	 * @param MaxNumber		The length  we are trying to wrap around
	 * @return the valid index with values in range [0, MaxNumber)
	 */
	UFUNCTION(BlueprintPure, Category = Math)
	static FORCEINLINE int32 WrapIndexAround(int32 Number, int32 MaxNumber)
	{
		return (MaxNumber + (Number % MaxNumber)) % MaxNumber;
	}

	// Is Number in [Start, End]
	static FORCEINLINE bool IsInClosedInterval(float Number, float Start, float End)
	{
		// First check if near the bounds then check if in range
		return FMath::IsNearlyEqual(Number, Start, KINDA_SMALL_NUMBER)
			|| FMath::IsNearlyEqual(Number, End, KINDA_SMALL_NUMBER)
			|| (Number > Start && Number < End);
	}

	static uint64 TruncDoubleToUint64(double Number);

	// Converts from bytes to megabytes (base 2)
	template <typename Type>
	static FORCEINLINE float BytesToMegaBytes(Type Bytes)
	{
		// 2^20
		static const float BytesInAMegaByte = FGenericPlatformMath::Pow(2.f, 20.f);
		return static_cast<float>(Bytes) / BytesInAMegaByte;
	}

	// 64 bits specialization
	template <uint64>
	static FORCEINLINE double BytesToMegaBytes(uint64 Bytes)
	{
		static const double BytesInAMegaByte = FGenericPlatformMath::Pow(2.f, 20.f);
		return static_cast<double>(Bytes) / BytesInAMegaByte;
	}
	template <int64>
	static FORCEINLINE double BytesToMegaBytes(int64 Bytes)
	{
		static const double BytesInAMegaByte = FGenericPlatformMath::Pow(2.f, 20.f);
		return static_cast<double>(Bytes) / BytesInAMegaByte;
	}

	// Converts from KiloByte to megabytes (base 2)
	template <typename Type>
	static FORCEINLINE float KiloBytesToMegaBytes(Type KiloBytes)
	{
		// 2^10
		static const float KiloBytesInAMegaByte = FGenericPlatformMath::Pow(2.f, 10.f);
		return static_cast<float>(KiloBytes) / KiloBytesInAMegaByte;
	}

	/** Vector.Rotation() for blueprint */
	UFUNCTION(BlueprintPure, Category = Math)
	static FORCEINLINE FRotator VectorToRotation(const FVector& Vector)
	{
		return Vector.Rotation();
	}

	UFUNCTION(BlueprintPure, Category = Math)
	static FORCEINLINE float InterpolateSmoothStep(float A, float B, float T)
	{
		const float Param = T * T * (3 - 2 * T);
		return A + Param * (B - A);
	}

	UFUNCTION(BlueprintPure, Category = Math)
	static FORCEINLINE float InterpolateAcceleration(float A, float B, float T)
	{
		const float Param = T * T;
		return A + Param * (B - A);
	}

	UFUNCTION(BlueprintPure, Category = Math)
	static FORCEINLINE float InterpolateDeceleration(float A, float B, float T)
	{
		const float Param = 1 - (1 - T) * (1 - T);
		return A + Param * (B - A);
	}

	template <typename Type>
	static Type InterpolateSmoothStep(Type A, Type B, float T)
	{
		const float Param = T * T * (3 - 2 * T);
		return A + Param * (B - A);
	}

	template <typename Type>
	static Type InterpolateCosine(Type A, Type B, float T)
	{
		const float Param = -FMath::Cos(PI * T) / 2.0f + 0.5f;
		return A + Param * (B - A);
	}

	template <typename Type>
	static Type InterpolateAcceleration(Type A, Type B, float T)
	{
		const float Param = T * T;
		return A + Param * (B - A);
	}

	template <typename Type>
	static Type InterpolateDeceleration(Type A, Type B, float T)
	{
		const float Param = 1 - (1 - T) * (1 - T);
		return A + Param * (B - A);
	}

	template <typename Type>
	static Type Interpolate(const Type& A, const Type& B, float T, ESoInterpolationMethod Method)
	{
		switch (Method)
		{
			case ESoInterpolationMethod::EIM_Cosine:
				return InterpolateCosine(A, B, T);

			case ESoInterpolationMethod::EIM_SmoothStep:
				return InterpolateSmoothStep(A, B, T);

			case ESoInterpolationMethod::EIM_Acceleration:
				return InterpolateAcceleration(A, B, T);

			case ESoInterpolationMethod::EIM_Deceleration:
				return InterpolateDeceleration(A, B, T);

			case ESoInterpolationMethod::EIM_NumOf:
			case ESoInterpolationMethod::EIM_Linear:
			default:
				return B * T + A * (1.0f - T);
		}
	}

	UFUNCTION(BlueprintPure)
	static FRotator InterpolateRotation(FRotator A, FRotator B, float T, ESoInterpolationMethod Method);

	UFUNCTION(BlueprintPure)
	static FVector InterpolateVector(const FVector& A, const FVector& B, float T, ESoInterpolationMethod Method);

	/** Works in radian, returns shortest distance between angles */
	FORCEINLINE static float ShortAngleDist(float Angle0, float Angle1)
	{
		static constexpr float Max = PI * 2;
		const float Delta = FMath::Fmod(Angle1 - Angle0, Max);
		return FMath::Fmod(2 * Delta, Max) - Delta;
	}

	/** Remaps Number from CurrentInputValue from range [InputMin, InputMax) to [OutputMin, OutputMax) */
	FORCEINLINE static int32 RemapRange(int32 CurrentInputValue, int32 InputMin, int32 InputMax, int32 OutputMin, int32 OutputMax)
	{
		return OutputMin + (CurrentInputValue - InputMin) * (OutputMax - OutputMin) / (InputMax - InputMin);
	}

	// originally they were only in charmovement but I guess they were required elsewhere too
	static FVector GetProjectedNormal(const FVector& PlaneDirVector, const FVector& Normal);
	static FVector GetSlideVector(const FVector& Delta, float Time, const FVector& Normal, const FHitResult& Hit);

	// calculate the angle between the projected V1 and V2, the projection plane is defined by the spline point
	static float CalculateAngle(const FSoSplinePoint& SplinePoint, const FVector& V1, const FVector& V2);

	static float CalculateAngle(const FVector2D& V1, const FVector2D& V2);

	/**
	*  Calculates the velocity with size "BulletVelocity" to throw something along spline from source to target
	*  Works along spline
	*  Returns nullvector if the target is unreachable with the given speed & gravity
	*  Precision num: amount of raycasts
	*/
	static FVector CalcRangeAttackVelocity(
		const FVector& StartPos,
		const FSoSplinePoint& StartLocation,
		const FVector& TargetPos,
		const FSoSplinePoint& TargetLocation,
		float BulletVelocity,
		float GravityScale,
		int32 PreferredDiscriminantSign
	);

	/**
	*  Checks if a projectile could hit target from source moving along the spline
	*  Works along spline
	*  Precision num: amount of raycasts
	*  Return value: true if it would hit the target
	*/
	static bool CheckRangeAttack(
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
	);

	/**
	*  Calculates the velocity with size "BulletVelocity" to throw something along spline from source to target
	*  Returns nullvector if the target is unreachable with the given speed & gravity
	*  Precision num: amount of raycasts
	*/
	UFUNCTION(BlueprintPure)
	static FVector CalcRangeAttackVelocity(
		const FVector& StartPos,
		const FVector& TargetPos,
		float BulletVelocity,
		float GravityScale,
		int32 PreferredDiscriminantSign
	);

	/**
	*  Checks if a projectile could hit target from source moving along the spline
	*  Precision num: amount of raycasts
	*  Return value: true if it would hit the target
	*/
	static bool CheckRangeAttack(
		const UWorld* World,
		const FVector& StartPos,
		const FVector& TargetPos,
		const TArray<const AActor*>& ActorsToIgnore,
		float BulletVelocity,
		float GravityScale,
		int32 Precision,
		int32 PreferredDiscriminantSign
	);

	/**
	 *  Clamps AngleCenter to interval [ AngleCenter - MaxDistance, AngleCenter + MaxDistance ]
	 */
	UFUNCTION(BlueprintPure, Category = Math)
	static float ClampYaw(float AngleCenter, float MaxDistance, float AngleToClamp);

	// X,Y: positon, Z: Steepness
	UFUNCTION(BlueprintPure, Category = Math)
	static FVector CalculateCurrentPositionAndSteepnessOnParabol(float Percent, const FVector2D& SourceToTarget, float MiddlePointPercentX, float MiddleZ);


	static void CalculateVectorsForShadowTransform(USceneCaptureComponent2D* CaptureComponent, FLinearColor& OutRow0, FLinearColor& OutRow1);

private:
	/**
	*  Calculates velocity for range attack + checks hit if Precision > 0
	*  Works along spline
	*  Precision: amount of raycasts - 0: no hit check
	*  Return value: true if it would hit the target
	*/
	static bool CalcRangeAttackInternal(
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
	);

	/**
	*  Calculates velocity for range attack + checks hit if Precision > 0
	*  Precision: amount of raycasts - 0: no hit check
	*  Return value: true if it would hit the target
	*/
	static bool CalcRangeAttackInternal(
		const UWorld* World,
		const FVector& StartPos,
		const FVector& TargetPos,
		const TArray<const AActor*>& ActorsToIgnore,
		float BulletVelocity,
		float GravityScale,
		int32 Precision,
		int32 PreferredDiscrimintantSign,
		FVector& OutVelocity
	);

};
