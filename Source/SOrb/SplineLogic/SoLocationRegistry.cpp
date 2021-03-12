// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoLocationRegistry.h"

#include "SplineLogic/SoMarker.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoLocationRegistry, All, All)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoLocationRegistry::ASoLocationRegistry()
{
	PrimaryActorTick.bCanEverTick = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint ASoLocationRegistry::GetSplineLocation(FName LocationName) const
{
	const ASoMarker* Marker = GetMarker(LocationName);
	if (Marker == nullptr)
		return FSoSplinePoint{};

	return Marker->GetSplineLocation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoLocationRegistry::GetZLocation(FName LocationName) const
{
	const ASoMarker* Marker = GetMarker(LocationName);
	if (Marker == nullptr)
		return 0.0f;

	return Marker->GetActorLocation().Z;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoLocationRegistry::GetForwardVector(FName LocationName) const
{
	const FSoSplinePoint SplinePoint = GetSplineLocation(LocationName);
	if (!SplinePoint.IsValid(true))
		return FVector(1.0f, 0.0f, 0.0f);

	return SplinePoint.GetDirection() * (ShouldMarkerLookBackwards(LocationName) ? -1.0 : 1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const ASoMarker* ASoLocationRegistry::GetMarker(FName LocationName) const
{
	ASoMarker* const* MarkerPtr = SplineLocations.Find(LocationName);
	return MarkerPtr == nullptr ? nullptr : *MarkerPtr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoLocationRegistry::RegisterSplineLocation(FName Name, ASoMarker* Location)
{
	if (Location == nullptr)
		return;

	if (Location->GetLevel() != GetLevel())
	{
		UE_LOG(LogSoLocationRegistry, Warning, TEXT("Failed to register spline location: registry and marker has to be in the same level!"));
		return;
	}

	if (SplineLocations.Contains(Name))
		SplineLocations[Name] = Location;
	else
		SplineLocations.Add(Name, Location);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoLocationRegistry* ASoLocationRegistry::GetInstance(UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		const TActorIterator<ASoLocationRegistry> LocationIter(World);
		if (!LocationIter || *LocationIter == nullptr)
		{
			UE_LOG(LogSoLocationRegistry, Error, TEXT("ASoLocationRegistry::GetInstance: Failed to get location registry actor. Does the level contain one?"));
			return nullptr;
		}

		// First one found
		return *LocationIter;
	}

	return nullptr;
}
