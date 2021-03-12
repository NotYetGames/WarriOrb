// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEPMarkerBased.h"

#include "Enemy/SoEnemy.h"
#include "Enemy/EActions/SoEAction.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "SplineLogic/SoMarker.h"
#include "SplineLogic/SoSplineHelper.h"
#include "CharacterBase/SoCharacterMovementComponent.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPSplineDistanceFromMarker::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const bool bResult = ASoMarker::IsMarkerInRangeFromSplineLocation(OwnerCharacter->GetTargetMarker(Index),
																	  ISoSplineWalker::Execute_GetSplineLocationI(OwnerCharacter),
																	  MinDistance,
																	  MaxDistance);
	return bResult == bShouldBeIn ? TrueValue : 0.0f;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPNotBetweenMarkers::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	ASoMarker* TargetMarker0 = OwnerCharacter->GetTargetMarker(Index0);
	ASoMarker* TargetMarker1 = OwnerCharacter->GetTargetMarker(Index1);
	if (TargetMarker0 == nullptr || TargetMarker1 == nullptr)
		return bFailsOnInvalidMarker ? 0.0f : TrueValue;

	const bool bBetween = USoSplineHelper::IsSplinepointBetweenPoints(OwnerCharacter->GetSoMovement()->GetSplineLocation(),
																	  TargetMarker0->GetSplineLocation(),
																	  TargetMarker1->GetSplineLocation());
	return (bBetween == bFailsIfNotBetween ? TrueValue : 0.0f);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPCharacterBetweenMarkers::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	return ((OwnerCharacter->IsPlayerBetweenMarkers(Index0, Index1, bResultOnInvalidMarkers)) != bInverse) ? TrueValue : 0.0f;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPCharacterSplineDistancefromMarker::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const bool bResult = ASoMarker::IsMarkerInRangeFromSplineLocation(OwnerCharacter->GetTargetMarker(Index),
																	  USoStaticHelper::GetPlayerSplineLocation(OwnerCharacter),
																	  MinDistance,
																	  MaxDistance);
	return bResult == bShouldBeIn ? TrueValue : 0.0f;
}
