// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoASkyControlEdit.h"
#include "EngineUtils.h"

#include "SoADefault.h"
#include "Character/SoCharacter.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Basic/Helpers/SoMathHelper.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoASkyControlEdit::USoASkyControlEdit() :
	USoActivity(EActivity::EA_SkyEdit)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);
	Orb->SetEditorWidgetVisibility(ESoEditorUI::EEUI_Sky, true);

	if (bSuperMode)
		OnSuperModeChange(true);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);
	Orb->SetEditorWidgetVisibility(ESoEditorUI::EEUI_Sky, false);

	if (bSuperMode)
		OnSuperModeChange(false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tack
void USoASkyControlEdit::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);

	if (bSuperMode)
		SuperModeTick(DeltaSeconds);

	if (!bOnKeyPosition)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;

	const float Distance = SplineLocation.GetDistance();
	FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();
	const int32 Index = SkyPoints.GetClosestIndex(Distance);
	if (SkyPoints.ControlPoints.IsValidIndex(Index))
	{
		if (fabs(SkyPoints.ControlPoints[Index].Distance - Distance) > KINDA_SMALL_NUMBER)
			bOnKeyPosition = false;
	}
	else
		bOnKeyPosition = false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::SpecialEditButtonPressed(int32 Index)
{
	if (PresetNames.Num() == 0)
		return;

	// switch sky texture
	if (bOnKeyPosition)
	{
		FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
		ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
		if (PlayerSpline == nullptr)
			return;

		FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();
		const int32 ClosestIndex = SkyPoints.GetClosestIndex(SplineLocation.GetDistance());

		int32 PresetIndex = 0;
		for (int32 i = 0; i < PresetNames.Num(); ++i)
			if (PresetNames[i] == SkyPoints.ControlPoints[ClosestIndex].Preset)
			{
				PresetIndex = i;
				break;
			}
		PresetIndex = USoMathHelper::WrapIndexAround(PresetIndex + (Orb->bCtrlPressed ? -1 : 1), PresetNames.Num());
		SkyPoints.ControlPoints[ClosestIndex].Preset = PresetNames[PresetIndex];
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::InteractKeyPressed(bool bPrimaryKey)
{
	bEditHintVisible = !bEditHintVisible;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::SaveEditedData()
{
	// overrides the editor world data from the game world data

	TMap<FString, ASoPlayerSpline*> GameSplineMap;
	for (TActorIterator<ASoPlayerSpline> ActorItr(Orb->GetWorld()); ActorItr; ++ActorItr)
		GameSplineMap.Add((*ActorItr)->GetSplineName(), *ActorItr);

#if WITH_EDITOR
	// reload file in editor world to apply changes
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();

	for (TActorIterator<ASoPlayerSpline> ActorItr(EditorWorld); ActorItr; ++ActorItr)
	{
		ASoPlayerSpline*const* SplinePtrPtr = GameSplineMap.Find((*ActorItr)->GetSplineName());
		if (SplinePtrPtr == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Spline %s found in editor, does not exist in game - that shouldn't be possible!"), *(*ActorItr)->GetSplineName());
		}
		else
			(*ActorItr)->GetSkyControlPoints() = (*SplinePtrPtr)->GetSkyControlPoints();
	}

	UPackage* Package = EditorWorld->GetOutermost();
	if (Package != nullptr && !Package->IsDirty())
		Package->SetDirtyFlag(true);
#endif

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::LoadEditedData()
{
	// no reload here
	// user could just leave the game and reenter if he/she/it wants to cancel sky editing
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::CreateKey()
{
	if (!bOnKeyPosition)
	{
		FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
		ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
		PlayerSpline->GetSkyControlPoints().CreateNewPoint(SplineLocation.GetDistance());
		bOnKeyPosition = true;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::MoveClosestKeyHere()
{
	if (bOnKeyPosition)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	const float Distance = SplineLocation.GetDistance();

	if (PlayerSpline == nullptr)
		return;

	FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();

	if (SkyPoints.ControlPoints.Num() == 0)
		return;

	const int32 PreIndex = SkyPoints.GetPrevIndex(Distance);
	const int32 PostIndex = SkyPoints.GetNextIndex(Distance);

	int32 Index = PostIndex;
	if (fabs(Distance - SkyPoints.ControlPoints[PreIndex].Distance) < fabs(Distance - SkyPoints.ControlPoints[PostIndex].Distance))
		Index = PreIndex;

	if (Index == 0 || Index == SkyPoints.ControlPoints.Num() - 1)
		return;

	SkyPoints.ControlPoints[Index].Distance = Distance;
	bOnKeyPosition = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// removes KeyNode (first and last is immune)
void USoASkyControlEdit::DeleteActiveKeyNode()
{
	if (!bOnKeyPosition)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	const float Distance = SplineLocation.GetDistance();

	if (PlayerSpline == nullptr)
		return;

	FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();
	const int32 Index = SkyPoints.GetClosestIndex(Distance);

	// return type could be -1
	// first and last point can't be removed
	// if handles both cases
	if (Index > 0 && Index < SkyPoints.ControlPoints.Num() - 1)
		SkyPoints.ControlPoints.RemoveAt(Index);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::CopyActiveKeyData()
{
	if (!bOnKeyPosition)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;

	FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();
	const int32 Index = SkyPoints.GetClosestIndex(SplineLocation.GetDistance());
	if (SkyPoints.ControlPoints.IsValidIndex(Index))
	{
		NodeCopy = SkyPoints.ControlPoints[Index];
		bNodeCopyInitialized = true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::PasteToActiveKeyData()
{
	if (!bOnKeyPosition || (!bNodeCopyInitialized))
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;

	FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();
	const int32 Index = SkyPoints.GetClosestIndex(SplineLocation.GetDistance());
	if (SkyPoints.ControlPoints.IsValidIndex(Index))
	{
		const float Distance = SkyPoints.ControlPoints[Index].Distance;
		SkyPoints.ControlPoints[Index] = NodeCopy;
		SkyPoints.ControlPoints[Index].Distance = Distance;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::JumpToNextKey()
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;

	const FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();

	int32 Index = 0;
	if (bOnKeyPosition)
		Index = SkyPoints.GetClosestIndex(SplineLocation.GetDistance()) + 1;
	else
		Index = SkyPoints.GetNextIndex(SplineLocation.GetDistance());

	if (SkyPoints.ControlPoints.IsValidIndex(Index))
	{
		FSoSplinePoint KeyPoint = SplineLocation;
		KeyPoint.SetDistance(SkyPoints.ControlPoints[Index].Distance);
		Orb->SetPositionOnSplineSP(KeyPoint, Orb->GetActorLocation().Z, false, false);
		bOnKeyPosition = true;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::JumpToPrevKey()
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;

	const FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();

	int32 Index = 0;
	if (bOnKeyPosition)
		Index = SkyPoints.GetClosestIndex(SplineLocation.GetDistance()) - 1;
	else
		Index = SkyPoints.GetPrevIndex(SplineLocation.GetDistance());

	if (SkyPoints.ControlPoints.IsValidIndex(Index))
	{
		FSoSplinePoint KeyPoint = SplineLocation;
		KeyPoint.SetDistance(SkyPoints.ControlPoints[Index].Distance);
		Orb->SetPositionOnSplineSP(KeyPoint, Orb->GetActorLocation().Z, false, false);
		bOnKeyPosition = true;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::SuperEditModePressed()
{
	bSuperMode = !bSuperMode;
	if (Orb->SoActivity == this)
		OnSuperModeChange(bSuperMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::IsOnKey(bool& bOnKey, int32& NodeIndex, FName& Preset) const
{
	bOnKey = bOnKeyPosition;

	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;
	const FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();
	NodeIndex = SkyPoints.GetClosestIndex(SplineLocation.GetDistance());

	if (SkyPoints.ControlPoints.IsValidIndex(NodeIndex))
		Preset = SkyPoints.ControlPoints[NodeIndex].Preset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::GetPrevKeyNodeData(FName& Preset, float& Percent, float& Distance) const
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;
	const FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();
	const int32 NodeIndex = SkyPoints.GetPrevIndex(SplineLocation.GetDistance());

	if (SkyPoints.ControlPoints.IsValidIndex(NodeIndex))
	{
		Preset = SkyPoints.ControlPoints[NodeIndex].Preset;
		Distance = fabs(SkyPoints.ControlPoints[NodeIndex].Distance - SplineLocation.GetDistance());
		Percent = 1.0f - SkyPoints.GetInterpolated(SplineLocation).SecondWeight;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASkyControlEdit::GetNextKeyNodeData(FName& Preset, float& Percent, float& Distance) const
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;
	const FSoSkyControlPoints& SkyPoints = PlayerSpline->GetSkyControlPoints();
	const int32 NodeIndex = SkyPoints.GetNextIndex(SplineLocation.GetDistance());

	if (SkyPoints.ControlPoints.IsValidIndex(NodeIndex))
	{
		Preset = SkyPoints.ControlPoints[NodeIndex].Preset;
		Distance = fabs(SkyPoints.ControlPoints[NodeIndex].Distance - SplineLocation.GetDistance());
		Percent = SkyPoints.GetInterpolated(SplineLocation).SecondWeight;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoASkyControlEdit::GetNodeCount() const
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return 0;

	return PlayerSpline->GetSkyControlPoints().ControlPoints.Num();
}
