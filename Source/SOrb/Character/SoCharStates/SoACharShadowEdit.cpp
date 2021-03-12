// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoACharShadowEdit.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Components/SceneCaptureComponent2D.h"

#include "SoADefault.h"
#include "Character/SoCharacter.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Components/InputComponent.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoACharShadowEdit::USoACharShadowEdit() :
	USoActivity(EActivity::EA_CharShadowEdit)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);
	Orb->SetEditorWidgetVisibility(ESoEditorUI::EEUI_CharShadow, true);

	if (bSuperMode)
		OnSuperModeChange(true);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);
	Orb->SetEditorWidgetVisibility(ESoEditorUI::EEUI_CharShadow, false);

	if (bSuperMode)
		OnSuperModeChange(false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::Tick(float DeltaSeconds)
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

	FSoCharShadowNodeList& CharShadowKeys = PlayerSpline->GetCharShadowControlPoints();

	const int32 Index = CharShadowKeys.GetClosestIndex(Distance);
	if (CharShadowKeys.Keys.IsValidIndex(Index))
	{
		if (fabs(CharShadowKeys.Keys[Index].Distance - Distance) > KINDA_SMALL_NUMBER)
			bOnKeyPosition = false;
	}
	else
		bOnKeyPosition = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::UpdateCharMaterials(float DeltaSeconds)
{
	Super::UpdateCharMaterials(DeltaSeconds);
#if WARRIORB_USE_CHARACTER_SHADOW
	DrawDebugLine(Orb->GetWorld(), Orb->GetActorLocation(), Orb->SoScreenCapture->GetComponentLocation(), FColor::Emerald, false, 0.2f);
#endif // WARRIORB_USE_CHARACTER_SHADOW

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;
	auto& Keys = PlayerSpline->GetCharShadowControlPoints().Keys;
	if (bOnKeyPosition)
		if (!Keys.IsValidIndex(Orb->ActiveShadowKey) || fabs(Keys[Orb->ActiveShadowKey].Distance - SplineLocation.GetDistance()) > KINDA_SMALL_NUMBER)
			bOnKeyPosition = false;

	if (!bOnKeyPosition)
		return;

	const float DeltaX = Orb->InputComponent->GetAxisValue("MouseX");
	const float DeltaY = Orb->InputComponent->GetAxisValue("MouseY");

	auto& Key = Keys[Orb->ActiveShadowKey];

	if (Orb->bLeftMouseBtnPressed)
		Key.Azimuth = FMath::Fmod(Key.Azimuth + DeltaX * DeltaSeconds + 2 * PI, 2 * PI);

	if (Orb->bRightMouseBtnPressed)
		Key.Elevation = FMath::Clamp(Key.Elevation + DeltaY * DeltaSeconds, -PI / 2.0f, PI / 2.0f);

	if (Orb->bMiddleMousePressed)
		Key.Strength = FMath::Clamp(Key.Strength + DeltaY * DeltaSeconds, 0.0f, 2.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::InteractKeyPressed(bool bPrimaryKey)
{
	bEditHintVisible = !bEditHintVisible;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::SaveEditedData()
{
#if WITH_EDITOR
	// overrides the editor world data from the game world data
	TMap<FString, ASoPlayerSpline*> GameSplineMap;
	for (TActorIterator<ASoPlayerSpline> ActorItr(Orb->GetWorld()); ActorItr; ++ActorItr)
		GameSplineMap.Add((*ActorItr)->GetSplineName(), *ActorItr);

	// reload file in editor world to apply changes
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();

	for (TActorIterator<ASoPlayerSpline> ActorItr(EditorWorld); ActorItr; ++ActorItr)
	{
		ASoPlayerSpline*const* SplinePtrPtr = GameSplineMap.Find((*ActorItr)->GetSplineName());
		if (SplinePtrPtr == nullptr)
			UE_LOG(LogTemp, Warning, TEXT("Spline %s found in editor, does not exist in game - that shouldn't be possible!"), *(*ActorItr)->GetSplineName())
		else
			(*ActorItr)->GetCharShadowControlPoints() = (*SplinePtrPtr)->GetCharShadowControlPoints();
	}

	UPackage* Package = EditorWorld->GetOutermost();
	if (Package != nullptr && !Package->IsDirty())
		Package->SetDirtyFlag(true);
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::LoadEditedData()
{
	// no reload here
	// user could just leave the game and reenter if he/she/it wants to cancel sky editing
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::CreateKey()
{
	if (!bOnKeyPosition)
	{
		FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
		ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
		Orb->ActiveShadowKey = PlayerSpline->GetCharShadowControlPoints().CreateNewKey(SplineLocation.GetDistance(), 0);
		bOnKeyPosition = true;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::MoveClosestKeyHere()
{
	if (bOnKeyPosition)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	const float Distance = SplineLocation.GetDistance();

	if (PlayerSpline == nullptr)
		return;

	FSoCharShadowNodeList& KeyNodes = PlayerSpline->GetCharShadowControlPoints();

	if (KeyNodes.Keys.Num() == 0)
		return;

	const int32 PreIndex = KeyNodes.GetPrevIndex(Distance);
	const int32 PostIndex = KeyNodes.GetNextIndex(Distance);

	int32 Index = PostIndex;
	if (fabs(Distance - KeyNodes.Keys[PreIndex].Distance) < fabs(Distance - KeyNodes.Keys[PostIndex].Distance))
		Index = PreIndex;

	KeyNodes.Keys[Index].Distance = Distance;
	bOnKeyPosition = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// removes KeyNode
void USoACharShadowEdit::DeleteActiveKeyNode()
{
	if (!bOnKeyPosition)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	const float Distance = SplineLocation.GetDistance();

	if (PlayerSpline == nullptr)
		return;

	FSoCharShadowNodeList& ShadowKeys = PlayerSpline->GetCharShadowControlPoints();
	const int32 Index = ShadowKeys.GetClosestIndex(Distance);

	if (ShadowKeys.Keys.IsValidIndex(Index))
		ShadowKeys.Keys.RemoveAt(Index);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::CopyActiveKeyData()
{
	if (!bOnKeyPosition)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;

	FSoCharShadowNodeList& ShadowKeys = PlayerSpline->GetCharShadowControlPoints();
	const int32 Index = ShadowKeys.GetClosestIndex(SplineLocation.GetDistance());
	if (ShadowKeys.Keys.IsValidIndex(Index))
	{
		NodeCopy = ShadowKeys.Keys[Index];
		bNodeCopyInitialized = true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::PasteToActiveKeyData()
{
	if (!bOnKeyPosition || !bNodeCopyInitialized)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;

	FSoCharShadowNodeList& ShadowKeys = PlayerSpline->GetCharShadowControlPoints();
	const int32 Index = ShadowKeys.GetClosestIndex(SplineLocation.GetDistance());
	if (ShadowKeys.Keys.IsValidIndex(Index))
	{
		const float Distance = ShadowKeys.Keys[Index].Distance;
		ShadowKeys.Keys[Index] = NodeCopy;
		ShadowKeys.Keys[Index].Distance = Distance;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::JumpToNextKey()
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;

	const FSoCharShadowNodeList& ShadowKeys = PlayerSpline->GetCharShadowControlPoints();

	int32 Index = 0;
	if (bOnKeyPosition)
		Index = ShadowKeys.GetClosestIndex(SplineLocation.GetDistance()) + 1;
	else
		Index = ShadowKeys.GetNextIndex(SplineLocation.GetDistance());

	if (ShadowKeys.Keys.IsValidIndex(Index))
	{
		FSoSplinePoint KeyPoint = SplineLocation;
		KeyPoint.SetDistance(ShadowKeys.Keys[Index].Distance);
		Orb->SetPositionOnSplineSP(KeyPoint, Orb->GetActorLocation().Z, false, false);
		bOnKeyPosition = true;
	}

	Orb->ActiveShadowKey = Index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::JumpToPrevKey()
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;

	const FSoCharShadowNodeList& ShadowKeys = PlayerSpline->GetCharShadowControlPoints();

	int32 Index = 0;
	if (bOnKeyPosition)
		Index = ShadowKeys.GetClosestIndex(SplineLocation.GetDistance()) - 1;
	else
		Index = ShadowKeys.GetPrevIndex(SplineLocation.GetDistance());

	if (ShadowKeys.Keys.IsValidIndex(Index))
	{
		FSoSplinePoint KeyPoint = SplineLocation;
		KeyPoint.SetDistance(ShadowKeys.Keys[Index].Distance);
		Orb->SetPositionOnSplineSP(KeyPoint, Orb->GetActorLocation().Z, false, false);
		bOnKeyPosition = true;
	}

	Orb->ActiveShadowKey = Index;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACharShadowEdit::SuperEditModePressed()
{
	bSuperMode = !bSuperMode;
	if (Orb->SoActivity == this)
		OnSuperModeChange(bSuperMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoCharShadowKeyNode USoACharShadowEdit::GetInterpolatedKey() const
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return FSoCharShadowKeyNode::GetInvalidKey();

	const FSoCharShadowNodeList& ShadowKeys = PlayerSpline->GetCharShadowControlPoints();

	if (bOnKeyPosition && ShadowKeys.Keys.IsValidIndex(Orb->ActiveShadowKey))
		return ShadowKeys.Keys[Orb->ActiveShadowKey];

	int32 Key = Orb->ActiveShadowKey;
	return ShadowKeys.GetInterpolated(SplineLocation.GetDistance(), Key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoCharShadowKeyNode& USoACharShadowEdit::GetPrevKeyNodeData() const
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return FSoCharShadowKeyNode::GetInvalidKey();

	const FSoCharShadowNodeList& ShadowKeys = PlayerSpline->GetCharShadowControlPoints();
	const int32 NodeIndex = ShadowKeys.GetPrevIndex(SplineLocation.GetDistance());

	if (ShadowKeys.Keys.IsValidIndex(NodeIndex))
		return ShadowKeys.Keys[NodeIndex];

	return FSoCharShadowKeyNode::GetInvalidKey();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoCharShadowKeyNode& USoACharShadowEdit::GetNextKeyNodeData() const
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return FSoCharShadowKeyNode::GetInvalidKey();

	const FSoCharShadowNodeList& ShadowKeys = PlayerSpline->GetCharShadowControlPoints();
	const int32 NodeIndex = ShadowKeys.GetNextIndex(SplineLocation.GetDistance());

	if (ShadowKeys.Keys.IsValidIndex(NodeIndex))
		return ShadowKeys.Keys[NodeIndex];

	return FSoCharShadowKeyNode::GetInvalidKey();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoACharShadowEdit::GetNodeCount() const
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return 0;

	return PlayerSpline->GetCharShadowControlPoints().Keys.Num();
}
