// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoACameraEdit.h"

#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"

#include "SoADefault.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Character/SoCharacter.h"
#include "SplineLogic/SoEditorGameInterface.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Components/InputComponent.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoACameraEdit::USoACameraEdit() :
	USoActivity(EActivity::EA_CameraEdit)
{
	Presets.Add({ "Down12", { { 1200, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, 300.0f } },
							  { 1200, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, 300.0f } },
							  { 1200, 0.0f, -15.0f, 100.0f, FVector{ 0.0f, 0.0f, 250.0f } },
							  { 1200, 0.0f, -10.0f, 100.0f, FVector{ -500, 0.0f, 300.0f } },
							  { 1200, 0.0f, -10.0f, 100.0f, FVector{  500, 0.0f, 300.0f } },
	} });

	Presets.Add({ "Down13", { { 1300, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, 300.0f } },
							  { 1300, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, 300.0f } },
							  { 1300, 0.0f, -15.0f, 100.0f, FVector{ 0.0f, 0.0f, 250.0f } },
							  { 1300, 0.0f, -10.0f, 100.0f, FVector{ -500, 0.0f, 300.0f } },
							  { 1300, 0.0f, -10.0f, 100.0f, FVector{  500, 0.0f, 300.0f } },
	} });

	Presets.Add({ "Mid12", { { 1200, 0.0f, -20.0f, 100.0f, FVector{ 0.0f, 0.0f, 0.0f } },
							 { 1200, 0.0f, -15.0f, 100.0f, FVector{ 0.0f, 0.0f, 0.0f } },
							 { 1200, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, 0.0f } },
							 { 1200, 0.0f, -10.0f, 100.0f, FVector{ -500, 0.0f, 300.0f } },
							 { 1200, 0.0f, -10.0f, 100.0f, FVector{ 500, 0.0f, 300.0f } }
	} });

	Presets.Add({ "Mid15", { { 1500, 0.0f, -20.0f, 100.0f, FVector{ 0.0f, 0.0f, 0.0f } },
							 { 1500, 0.0f, -15.0f, 100.0f, FVector{ 0.0f, 0.0f, 0.0f } },
							 { 1500, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, 0.0f } },
							 { 1500, 0.0f, -10.0f, 100.0f, FVector{ -500, 0.0f, 300.0f } },
							 { 1500, 0.0f, -10.0f, 100.0f, FVector{ 500, 0.0f, 300.0f } }
	} });

	Presets.Add({ "Up12", { { 1200, 0.0f, -20.0f, 100.0f, FVector{ 0.0f, 0.0f, -250.0f } },
							{ 1200, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, -250.0f } },
							{ 1200, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, -300.0f } },
							{ 1200, 0.0f, -10.0f, 100.0f, FVector{ -500, 0.0f, -250.0f } },
							{ 1200, 0.0f, -10.0f, 100.0f, FVector{ 500, 0.0f, -250.0f } }
	} });

	Presets.Add({ "Up15", { { 1500, 0.0f, -20.0f, 100.0f, FVector{ 0.0f, 0.0f, -250.0f } },
							{ 1500, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, -250.0f } },
							{ 1500, 0.0f, -10.0f, 100.0f, FVector{ 0.0f, 0.0f, -300.0f } },
							{ 1500, 0.0f, -10.0f, 100.0f, FVector{ -500, 0.0f, -250.0f } },
							{ 1500, 0.0f, -10.0f, 100.0f, FVector{ 500, 0.0f, -250.0f } }
	} });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);
	Orb->SetEditorWidgetVisibility(ESoEditorUI::EEUI_Camera, true);

	if (bSuperMode)
		OnSuperModeChange(true);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);
	Orb->SetEditorWidgetVisibility(ESoEditorUI::EEUI_Camera, false);

	if (bSuperMode)
		OnSuperModeChange(false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tack
void USoACameraEdit::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);

	if (bSuperMode)
		SuperModeTick(DeltaSeconds);

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;
	auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;
	// Orb->GetClosestCamIndex() calls this->GetClosestCamIndex(), weird
	const int32 ClosestIndex = Orb->GetClosestCamIndex();
	for (int32 i = 0; i < CamKeys.Num(); ++i)
	{
		FVector Loc = PlayerSpline->GetSplineComponent()->GetWorldLocationAtDistanceAlongSpline(CamKeys[i].Distance);
		Loc.Z = Orb->GetActorLocation().Z + 100;
		DrawDebugPoint(GetWorld(), Loc, 10.0f, i == ClosestIndex ? FColor(125, 0, 0) : FColor(0, 125, 0));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::UpdateCamera(float DeltaSeconds)
{
	Super::UpdateCamera(DeltaSeconds);

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	if (PlayerSpline == nullptr)
		return;
	auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

	const FSoCamKeyNode InterpolatedNode = Orb->GetInterpolatedCamData();
	bPresetUsed = false;
	// update preset
	for (int32 i = 0; i < Presets.Num(); ++i)
		for (int32 j = 0; j < Presets[i].Variants.Num(); ++j)
			if (InterpolatedNode.CompareParams(Presets[i].Variants[j]))
			{
				bPresetUsed = true;
				ActivePreset = i;
				ActivePresetSubType = j;
			}

	if (bOnCamKeyPosition)
	{
		if (Orb->ActiveCamKey >= CamKeys.Num())
			bOnCamKeyPosition = false;
		else
		{
			// let's check if we moved away from key position
			const FVector Loc = CamKeys[Orb->ActiveCamKey].SplinePoint;

			if (FVector2D::Distance(FVector2D(Loc), FVector2D(Orb->GetActorLocation())) > 2 * KINDA_SMALL_NUMBER)
				bOnCamKeyPosition = false;
		}
	}

	if (!bOnCamKeyPosition)
		return;

	const float DeltaX = Orb->InputComponent->GetAxisValue("MouseX");
	const float DeltaY = Orb->InputComponent->GetAxisValue("MouseY");

	auto& CamKey = CamKeys[Orb->ActiveCamKey];

	// update camera edit
	if (Orb->bLeftMouseBtnPressed)
	{
		if (Orb->bCtrlPressed)
			CamKey.DeltaYaw += DeltaX * DeltaSeconds * 100;
		else
			CamKey.DeltaOffset += FVector(DeltaX * DeltaSeconds * 300, 0, 0);
	}

	if (Orb->bRightMouseBtnPressed)
	{
		if (Orb->bCtrlPressed)
			CamKey.DeltaPitch += DeltaY * DeltaSeconds * 100;
		else
			CamKey.DeltaOffset += FVector(0, 0, DeltaY * 300 * DeltaSeconds);
	}

	if (Orb->bMiddleMousePressed)
		CamKey.ArmLength -= DeltaY * DeltaSeconds * 400;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::InteractKeyPressed(bool bPrimary)
{
	if (bPrimary)
		bCamEditHintVisible = !bCamEditHintVisible;
	else
	{
		if (!bOnCamKeyPosition)
			return;

		FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
		ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
		auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

		if (!CamKeys.IsValidIndex(Orb->ActiveCamKey))
			return;

		if (bPresetUsed && Presets.Num() != 0)
			ActivePreset = (ActivePreset + 1) % Presets.Num();

		ActivePresetSubType = FMath::Clamp(ActivePresetSubType, 0, Presets[ActivePreset].Variants.Num() - 1);

		CamKeys[Orb->ActiveCamKey].CopyParams(Presets[ActivePreset].Variants[ActivePresetSubType], false);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::DebugFeature()
{
	if (!bOnCamKeyPosition)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

	if (!CamKeys.IsValidIndex(Orb->ActiveCamKey))
		return;

	if (bPresetUsed && Presets.IsValidIndex(ActivePreset))
		ActivePresetSubType = (ActivePresetSubType + 1) % Presets[ActivePreset].Variants.Num();

	CamKeys[Orb->ActiveCamKey].CopyParams(Presets[ActivePreset].Variants[ActivePresetSubType], false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoACameraEdit::GetPrevCamKeyNodeSplineDistance() const
{
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(ISoSplineWalker::Execute_GetSplineLocationI(Orb).GetSpline());
	if (PlayerSpline == nullptr)
		return 0.0f;

	const auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

	if (bOnCamKeyPosition)
	{
		if (Orb->ActiveCamKey > 0)
			return CamKeys[Orb->ActiveCamKey - 1].SplinePoint.GetDistance();
		return 0.0f;
	}
	return CamKeys[Orb->ActiveCamKey].SplinePoint.GetDistance();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoACameraEdit::GetNextCamKeyNodeSplineDistance() const
{
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(ISoSplineWalker::Execute_GetSplineLocationI(Orb).GetSpline());
	if (PlayerSpline == nullptr)
		return 0.0f;

	const auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

	if (Orb->ActiveCamKey + 1 < CamKeys.Num())
		return CamKeys[Orb->ActiveCamKey + 1].SplinePoint.GetDistance();
	return 0.0f;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoACameraEdit::GetClosestCamIndex() const
{
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(ISoSplineWalker::Execute_GetSplineLocationI(Orb).GetSpline());
	if (PlayerSpline == nullptr)
		return -1;

	const auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

	float PrevDist = BIG_NUMBER;
	float NextDist = BIG_NUMBER;
	int32 PrevIx = -1;
	int32 NextIx = -1;
	if (bOnCamKeyPosition)
		return Orb->ActiveCamKey;

	PrevIx = Orb->ActiveCamKey;
	PrevDist = CamKeys[FMath::Clamp(PrevIx, 0, CamKeys.Num() - 1)].SplinePoint.GetDistance();

	if (Orb->ActiveCamKey + 1 < CamKeys.Num())
	{
		NextIx = Orb->ActiveCamKey + 1;
		NextDist = CamKeys[NextIx].SplinePoint.GetDistance();
	}

	const float Dist = ISoSplineWalker::Execute_GetSplineLocationI(Orb).GetDistance();
	if (fabs(Dist - PrevDist) > fabs(Dist - NextDist))
		return NextIx;
	return PrevIx;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::SaveEditedData()
{
	FSoEditorGameInterface::SaveCameraData(Orb->GetWorld());

#if WITH_EDITOR
	// reload file in editor world to apply changes
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	FSoEditorGameInterface::LoadCameraData(EditorWorld);
	UPackage* Package = EditorWorld->GetOutermost();
	if (Package != nullptr && !Package->IsDirty())
		Package->SetDirtyFlag(true);
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::LoadEditedData()
{
	FSoEditorGameInterface::LoadCameraData(Orb->GetWorld());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::CreateKey()
{
	if (!bOnCamKeyPosition)
	{
		FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
		ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
		PlayerSpline->GetCamNodeList().CreateNewCamKey(SplineLocation, Orb->ActiveCamKey);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::MoveClosestKeyHere()
{
	if (bOnCamKeyPosition)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());

	if (PlayerSpline == nullptr)
		return;

	auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

	if (Orb->ActiveCamKey == CamKeys.Num() - 1)
		return;

	const float PrevDist = CamKeys[Orb->ActiveCamKey].SplinePoint.GetDistance();
	const float NextDist = CamKeys[Orb->ActiveCamKey + 1].SplinePoint.GetDistance();
	const float Dist = SplineLocation.GetDistance();

	if (fabs(Dist - PrevDist) < fabs(Dist - NextDist))
	{
		if (Orb->ActiveCamKey != 0)
		{
			CamKeys[Orb->ActiveCamKey].SplinePoint = SplineLocation;
			CamKeys[Orb->ActiveCamKey].Distance = SplineLocation.GetDistance();
			bOnCamKeyPosition = true;
		}
	}
	else
	{
		if (Orb->ActiveCamKey < CamKeys.Num() - 2)
		{
			CamKeys[Orb->ActiveCamKey + 1].SplinePoint = SplineLocation;
			CamKeys[Orb->ActiveCamKey + 1].Distance = SplineLocation.GetDistance();
			Orb->ActiveCamKey += 1;
			bOnCamKeyPosition = true;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// removes KeyNode (first and last is immune)
void USoACameraEdit::DeleteActiveKeyNode()
{
	if (!bOnCamKeyPosition || Orb->ActiveCamKey == 0)
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

	if (Orb->ActiveCamKey == CamKeys.Num() - 1)
		return;

	CamKeys.RemoveAt(Orb->ActiveCamKey, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::CopyActiveKeyData()
{
	if (!bOnCamKeyPosition)
		return;

	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	const auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

	NodeCopy.CopyParams(CamKeys[Orb->ActiveCamKey], true);
	bNodeCopyInitialized = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::PasteToActiveKeyData()
{
	if ((!bOnCamKeyPosition) || (!bNodeCopyInitialized))
		return;

	FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;

	CamKeys[Orb->ActiveCamKey].CopyParams(NodeCopy, false);
	bNodeCopyInitialized = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::JumpToNextKey()
{
	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();

	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	const auto& CamKeys = PlayerSpline->GetCamNodeList().CamKeys;
	Orb->ActiveCamKey = FMath::Min(Orb->ActiveCamKey + 1, CamKeys.Num() - 1);

	const FSoSplinePoint CamLocation = CamKeys[Orb->ActiveCamKey].SplinePoint;

	Orb->SetPositionOnSplineSP(CamLocation, Orb->GetActorLocation().Z, false, false);
	bOnCamKeyPosition = true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::JumpToPrevKey()
{
	if (bOnCamKeyPosition)
		Orb->ActiveCamKey = FMath::Max(Orb->ActiveCamKey - 1, 0);

	const FSoSplinePoint SplineLocation = Orb->SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	const FSoSplinePoint CamLocation = PlayerSpline->GetCamNodeList().CamKeys[Orb->ActiveCamKey].SplinePoint;

	Orb->SetPositionOnSplineSP(CamLocation, Orb->GetActorLocation().Z, false, false);
	bOnCamKeyPosition = true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::SpecialEditButtonPressed(int32 Index)
{
	auto SplineLocation = Orb->SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
	auto& CamKey = PlayerSpline->GetCamNodeList().CamKeys[Orb->ActiveCamKey];

	if (Index == 0)
		CamKey.bUseSplineOnInterpolation = !CamKey.bUseSplineOnInterpolation;
	else
	{
		int32 NewValue = (static_cast<int32>(CamKey.InterpolationMethod) + 1) % static_cast<int32>(ESoCamInterpolationMethod::ECIM_NumOf);
		CamKey.InterpolationMethod = static_cast<ESoCamInterpolationMethod>(NewValue);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::SuperEditModePressed()
{
	bSuperMode = !bSuperMode;
	if (Orb->SoActivity == this)
		OnSuperModeChange(bSuperMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACameraEdit::ToggleWeapons()
{
	// secret speed mode
	MovementSpeed = MovementSpeed > 2000 ? 500 : 4000;
}
