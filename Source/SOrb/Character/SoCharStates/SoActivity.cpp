// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoActivity.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "TimerManager.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/ObjectMacros.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"

// #include "DrawDebugHelpers.h"

#include "Character/SoCharacter.h"
#include "Character/SoSpringArmComponent.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Character/SoPlayerProgress.h"

#include "Character/SoCharStates/SoADefault.h"
#include "Character/SoCharStates/SoAWait.h"
#include "Character/SoCharStates/SoASwing.h"
#include "Character/SoCharStates/SoASlide.h"
#include "Character/SoCharStates/SoADead.h"
#include "Character/SoCharStates/SoAHitReact.h"
#include "Character/SoCharStates/SoAInUI.h"
#include "Character/SoCharStates/SoActivity.h"
#include "Character/SoCharStates/SoAStrike.h"
#include "Character/SoCharStates/SoAItemUsage.h"
#include "Character/SoCharStates/SoAAiming.h"
#include "Character/SoCharStates/SoAWeaponInArm.h"
#include "Character/SoCharStates/SoARoll.h"
#include "Character/SoCharStates/SoALeverPush.h"
#include "Character/SoCharStates/SoASkyControlEdit.h"
#include "Character/SoCharStates/SoACameraEdit.h"
#include "Character/SoCharStates/SoARoll.h"
#include "Character/SoCharStates/SoACharShadowEdit.h"
#include "Character/SoCharStates/SoALillian.h"
#include "Character/SoCharStates/SoATeleport.h"

#include "Items/ItemTemplates/SoUsableItemTemplate.h"
#include "Items/ItemTemplates/SoWeaponTemplate.h"

#include "Interactables/SoInteractable.h"
#include "Interactables/SoInteractableActor.h"

#include "Enemy/SoEnemyHelper.h"

#include "UI/SoUISystem.h"
#include "UI/General/SoUITypes.h"
#include "UI/Menu/SoUIMenuMain.h"

#include "Levels/SoLevelHelper.h"
#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Settings/SoGameSettings.h"

#include "Logic/SoTriggerable.h"

#include "SaveFiles/SoWorldState.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Public/UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogSoCharActivity, Log, All);


const FName USoActivity::TrampolineTag = FName("Trampoline");
const FName USoActivity::SlipperyPosTag = FName("SlipperyPos");
const FName USoActivity::SlipperyNegTag = FName("SlipperyNeg");
const FName USoActivity::RollOnlySurface = FName("RollOnly");
const FName USoActivity::NoWallJumpSurface = FName("NoWallJump");
const FName USoActivity::NoBounceSurface = FName("NoBounce");
const FName USoActivity::TriggerOnStand = FName("TriggerOnStand");

const FName USoActivity::EnabledCollisionProfileName = FName("SoPlayerCharacter");
const FName USoActivity::DisabledCollisionProfileName = FName("SoPlayerCharacterDisabled");

const float JumpPressedRecentlyTimeOffset = 0.3f;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		Orb = Cast<ASoCharacter>(GetOuter());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called on old activity
void USoActivity::SwitchActivity(USoActivity* NewActivity)
{
	if (NewActivity == this)
		return;

	// if it is nullptr the point is to leave the current state most likely
	if (NewActivity == nullptr)
		NewActivity = Orb->SoADefault;

#if WITH_EDITOR
	if (NewActivity->Orb == nullptr)
	{
		UE_LOG(LogSoCharActivity, Error, TEXT("USoActivity::SwitchActivity failed: NewActivity->Orb is nullptr. It should be initialized in SoCharacter::PreInitializeComponents!"));
		return;
	}
#endif

	USoActivity* OldActivity = Orb->SoActivity;
	bool bCouldInteract = false;
	if (OldActivity != nullptr)
	{
		bCouldInteract = OldActivity->CanInteract();
		OldActivity->OnExit(NewActivity);
	}
	Orb->SoActivity = NewActivity;
	NewActivity->OnEnter(this);
	NewActivity->LastActivity = this;

	if (bCouldInteract != Orb->SoActivity->CanInteract())
		Orb->OnInteractionLabelChanged.Broadcast();

	if (OldActivity != nullptr)
		OldActivity->OnPostExit(NewActivity);

	if (OldActivity == nullptr ||
		OldActivity->bCanHaveDefaultKeyLights != NewActivity->bCanHaveDefaultKeyLights)
		Orb->RefreshDefaultKeyLights();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// check some common problem based on flags
void USoActivity::OnEnter(USoActivity* OldActivity)
{
//#if WITH_EDITOR
//	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EActivity"), true);
//	const FString EnumName = EnumPtr ? EnumPtr->GetNameStringByIndex(static_cast<int32>(ID)) : "Invalid ID";
//	UE_LOG(LogSoCharActivity, Display, TEXT("Activity entered: %s"), *EnumName);
//#endif

	switch (LookDirection)
	{
		case ESoLookDirection::ELD_Movement:
			Orb->SoMovement->bOrientRotationToMovement = !Orb->bLockForwardVecPressed;
			break;
		case ESoLookDirection::ELD_Frozen:
			Orb->StoredForwardVector = Orb->GetActorForwardVector();
		default:
			break;
	}

	Orb->SoMovement->JumpZVelocity = JumpZVelocity;
	Orb->SoMovement->MaxWalkSpeed = MovementSpeed;

	HandleCollision();

	// Leg jump could influence the ActiveRollJumpHeight
	if (JumpType == ESoJumpType::EJT_Bounce || (JumpType == ESoJumpType::EJT_BounceIfPressed && Orb->bRollPressed))
		Orb->SoMovement->JumpZVelocity = Orb->GetActiveRollJumpHeight();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// we shall meet again
void USoActivity::OnExit(USoActivity* NewActivity)
{
	if (IsArmedState() &&
		!(NewActivity->IsArmedState() || NewActivity->CanBeArmedState()))
		Orb->OnArmedStateLeft();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called every frame
void USoActivity::Tick(float DeltaSeconds)
{
	// 1. handle collision size
	HandleCollision();

	// 2. clear jump&roll assist if necessary
	if (FMath::Abs(Orb->StoredMovementInputDirOnJump - Orb->GetMovementXAxisValue()) > KINDA_SMALL_NUMBER)
	{
		Orb->StoredMovementInputDirOnJump = -1.0f;
		Orb->StoredTimeOnJump = -1.0f;
	}

	// 3. fix orientation
	UpdateOrientation();

	// 4. some input based state change
	UpdateFloating();

	if (Orb->bRollPressed/* && !Orb->bFloatingActive*/)
		RollPressed();


	// 5. check for idle
	const FVector ActualPosition = Orb->GetActorLocation();
	if (FVector::DistSquared(ActualPosition, Orb->SavedPosition) > KINDA_SMALL_NUMBER)
	{
		Orb->IdleTime = 0.f;
		Orb->SavedPosition = ActualPosition;
	}
	else
	{
		Orb->IdleTime += DeltaSeconds;
		//if (Orb->IdleTime > Orb->GetMaxIdleTime())
		//{
		//	Orb->IdleTime = 0.0f;
		//	Orb->PauseGame();
		//}
	}


	// 6. update interaction if necessary
	if (Orb->SwingCenters.Num() > 0)
		Orb->OnInteractionLabelChanged.Broadcast();


	if (Orb->SoMovement->MovementMode == EMovementMode::MOVE_Falling)
	{
		// check if we have to raise our hands
		const FVector Start = Orb->GetActorLocation();
		const FVector End = Start - FVector(0.0f, 0.0f, 100.0f);
		FCollisionQueryParams QuaryParams;
		QuaryParams.AddIgnoredActor(Orb);
		QuaryParams.bIgnoreTouches = true;
		FCollisionResponseParams ResponseParams;
		Orb->bAlmostLanded = Orb->GetWorld()->LineTraceTestByChannel(Start, End, ECollisionChannel::ECC_WorldDynamic, QuaryParams);

		// if (Orb->bAlmostLanded)
		// 	DrawDebugLine(GetWorld(), Start, End, FColor::Red);
		// else
		// 	DrawDebugLine(GetWorld(), Start, End, FColor::Blue);
	}
	else
		Orb->bAlmostLanded = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::ShouldDecreaseCollision() const
{
	return !Orb->SoMovement->IsMovingOnGround();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::HandleCollision()
{
	if (ShouldDecreaseCollision())
		Orb->DecreaseCollisionSize();
	else
	{
		if (Orb->bCollisionAlreadyDecreased)
		{
			if (Orb->CanIncreaseCollision())
				Orb->IncreaseCollisionSize();
			else
				if (Orb->SoActivity != Orb->SoAHitReact &&
					Orb->SoActivity != Orb->SoADead &&
					Orb->SoActivity != Orb->SoATeleport)
				{
					SwitchActivity(Orb->SoARoll);
				}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::CanEscapeToMenu(FKey Key, ESoUICommand Command) const
{
	if (Command != ESoUICommand::EUC_MainMenuBack)
		return true;

	// Don't allow escape from face button
	if (!Orb->IsMainMenuOpened())
	{
		const FName KeyName = Key.GetFName();
		if (FSoInputKey::IsGamepadFaceButton(KeyName))
			return false;

		// Always disable main menu?
		if (WARRIORB_WITH_VIDEO_DEMO)
			if (USoGameSettings::Get().CanVideoDemoDisableMenu())
				return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::IsInMultipleGamepadCommandsWhitelist(ESoUICommand Command) const
{
	static const TSet<ESoUICommand> WhitelistGamepadCommands = {
		// Back conflict
		ESoUICommand::EUC_MainMenuBack, ESoUICommand::EUC_ActionBack,

		// Back conflict
		ESoUICommand::EUC_MainMenuEnter,

		// Can have both buttons pressed
		ESoUICommand::EUC_SpellSelect, ESoUICommand::EUC_SpellSelectAndCast
	};
	return WhitelistGamepadCommands.Contains(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::CanHandleUICommand(ESoUICommand Command) const
{
	// Ignore duplicate UI command, can happen because some UI commands can be duplicate
	if (Orb && Orb->UIInputPressedCommands.Num() > 0)
	{
		if (Orb->DeviceType != ESoInputDeviceType::Keyboard)
		{
			// Gamepad, Ignore multiple UI commands (except whitelist)
			if (Orb->UIInputPressedCommands.Num() >= 2 && !IsInMultipleGamepadCommandsWhitelist(Command))
				return false;
		}
		else
		{
			// Keyboard, allow multiple button presses that conflict
			const TSet<ESoUICommand>& ConflictingCommands = FSoInputActionName::GetUICommandsThatConflictWith(Command);
			if (ConflictingCommands.Num() > 0)
				return Orb->UIInputPressedCommands.Intersect(ConflictingCommands).Num() == 0;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::ForwardCommandToMainMenu(ESoUICommand Command)
{
	// default implementation: pass command to main menu
	if (USoUIMenuMain* MenuMain = Orb->GetUIMainMenu())
	{
		ISoUIEventHandler::Execute_OnUICommand(MenuMain, Command);
		return;
	}

	UE_LOG(LogSoCharActivity, Error, TEXT("Failed to handle UI command: improper UI setup!"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::HandleUICommand(FKey Key, ESoUICommand Command)
{
	UE_LOG(LogSoCharActivity, Verbose, TEXT("HandleUICommand Key = %s, Command = %s"), *Key.GetFName().ToString(), *FSoInputActionName::UICommandToActionName(Command).ToString());

	if (!CanHandleUICommand(Command))
		return;

	if (!CanEscapeToMenu(Key, Command))
		return;

	ForwardCommandToMainMenu(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::UpdateCamera(float DeltaSeconds)
{
	// Data from Camera Key Positions
	const FSoSplinePoint& SplineLocation = Orb->SoMovement->GetSplineLocation();

	if (!SplineLocation.IsValid()) return;

	const float Distance = SplineLocation.GetDistance();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());
#if WITH_EDITOR
	if (PlayerSpline == nullptr)
	{
		UE_LOG(LogSoCharActivity, Error, TEXT("Invalid spline at ASoCharacter::UpdateCamera! Is the player on a PlayerSpline?"));
		return;
	}
#endif

	// update camera z override
	if (Orb->bUseSavedCameraBoomZ && fabs(Orb->SavedCameraBoomZ - Orb->SavedCameraBoomZTarget) > KINDA_SMALL_NUMBER)
	{
		Orb->SavedCameraBoomZCounter = FMath::Max(Orb->SavedCameraBoomZCounter - DeltaSeconds, 0.0f);
		Orb->SavedCameraBoomZ = USoMathHelper::InterpolateSmoothStep(Orb->SavedCameraBoomZSource,
																	 Orb->SavedCameraBoomZTarget,
																	 1.0f - Orb->SavedCameraBoomZCounter / Orb->SavedCameraBoomZBlendTime);
	}

	// update applied camera x delta
	Orb->CameraXDeltaCounter = FMath::Max(Orb->CameraXDeltaCounter - DeltaSeconds, 0.0f);
	Orb->CameraXDeltaCurrent = USoMathHelper::InterpolateSmoothStep(Orb->CameraXDeltaSource,
																	Orb->CameraXDeltaTarget,
																	1.0f - Orb->CameraXDeltaCounter / Orb->CameraXDeltaBlendTime);



	Orb->SkyControlValue = PlayerSpline->GetSkyControlPoints().GetInterpolated(SplineLocation);
	const FSoCamNodeList& SplineCamData = PlayerSpline->GetCamNodeList();
	FSoCamKeyNode PointCamData = SplineCamData.GetInterpolatedCamData(Distance, Orb->ActiveCamKey);
	Orb->CameraBoom->TargetArmLength = PointCamData.ArmLength;

	if (PlayerSpline->GetRotateCameraWith180Yaw())
	{
		PointCamData.DeltaYaw += 180.0f;
		PointCamData.Rotation.Yaw += 180.0f;
	}

	Orb->bRotatedCamera = PointCamData.DeltaYaw > 90 || PointCamData.DeltaYaw < -90;

	// Z offset from movement
	if (Orb->SoMovement->MovementMode == EMovementMode::MOVE_Walking || Orb->SoMovement->IsFallVelocityOverriden())
		Orb->LastLandZ = Orb->GetActorLocation().Z - Orb->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// horizontal offset from movement
	// if (GetVelocity().Size2D() > KINDA_SMALL_NUMBER)
	{
		// int NewDir = (FVector2D(SplineLocation.GetDirection() /** SplineLocation.GetOrientation()*/) | FVector2D(GetVelocity()).GetSafeNormal()) > 0 ? 1 : -1;

		const int NewDir = (FVector2D(SplineLocation.GetDirection() /** SplineLocation.GetOrientation()*/) | FVector2D(Orb->GetActorForwardVector())) > 0 ? 1 : -1;

		// TODO: to decide if we need instant swap or slow
		Orb->HorizontalOffset = (NewDir != Orb->MovDir) ? -Orb->HorizontalOffset
														: FMath::FInterpTo(Orb->HorizontalOffset, PointCamData.MaxHorizontalOffset, DeltaSeconds, 1.0f);

		Orb->MovDir = NewDir;
	}

	const float NewDist = Orb->GetActorLocation().Z - Orb->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const float MaxZDistanceUpp = 10;
	const float MaxZDistanceDown = -300;

	float NewRelativeZ = NewDist + FMath::Clamp(Orb->LastLandZ - NewDist, MaxZDistanceDown, MaxZDistanceUpp);
	if (Orb->bUseCamMinZValue)
		NewRelativeZ = FMath::Max(NewRelativeZ, Orb->MinCamZValue - PointCamData.Position.Z);
	if (Orb->bUseCamMaxZValue)
		NewRelativeZ = FMath::Min(NewRelativeZ, Orb->MaxCamZValue - PointCamData.Position.Z);

	/*if (fabs(NewRelativeZ - SavedCamRelative) > 5.f)*/
	Orb->SavedCamRelative = NewRelativeZ;

	const FVector RelativeComponent = FVector(Orb->HorizontalOffset + Orb->CameraXDeltaCurrent * Orb->MovDir, 0.f, Orb->SavedCamRelative) +
									  PointCamData.DeltaOffset * FVector(Orb->MovDir, Orb->MovDir, 1.0f);

	Orb->CameraBoom->SetWorldLocation(PointCamData.Position);
	Orb->CameraBoom->SetWorldRotation(PointCamData.Rotation);

	if (Orb->bUseSavedCameraBoomZ || Orb->CameraUnfreezeCounter > 0.0f)
	{
		const float LerpValue = Orb->CameraUnfreezeCounter / Orb->CameraUnfreezeTime;
		Orb->CameraBoom->AddRelativeLocation(FVector(RelativeComponent.X,
													 RelativeComponent.Y,
													 FMath::Lerp(RelativeComponent.Z, Orb->SavedCameraBoomZ, LerpValue)));
	}
	else
	{
		Orb->CameraBoom->AddRelativeLocation(RelativeComponent);
		Orb->SavedCameraBoomZ = RelativeComponent.Z;
	}

	if (Orb->SavedCamMovModifier == 0)
		Orb->SavedCamMovModifier = ((FVector2D(Orb->SideViewCameraComponent->GetRightVector()) | FVector2D(SplineLocation.GetDirection())) < 0) ? 1 : -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::UpdateCharMaterials(float DeltaSeconds)
{
	bool bEnabled = false;

	const FSoSplinePoint& SplineLocation = Orb->SoMovement->GetSplineLocation();
	if (const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline()))
	{
		static const FName EmissiveName = FName("EmissiveStrength");

		// 1. Emissive Strength
		const float EmissiveValue = PlayerSpline->ShouldUseCharacterEmissiveStrength() ? PlayerSpline->GetCharacterEmissiveStrength() : 0.3f;
		Orb->GetMesh()->SetScalarParameterValueOnMaterials(EmissiveName, EmissiveValue);

		// 2. shadow post process
#if WARRIORB_USE_CHARACTER_SHADOW
		const FSoCharShadowNodeList& NodeList = PlayerSpline->GetCharShadowControlPoints();
		const FSoCharShadowKeyNode ShadowNode = NodeList.GetInterpolated(SplineLocation.GetDistance(), Orb->ActiveShadowKey);

		if (ShadowNode.Strength > KINDA_SMALL_NUMBER)
		{
			bEnabled = true;

			if (!Orb->bShadowWasEnabled)
			{
				Orb->SoScreenCapture->Activate();
				Orb->bShadowWasEnabled = true;
				if (Orb->LevelPostProcessVolume != nullptr)
					Orb->LevelPostProcessVolume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, Orb->ShadowMaterialPostProcessDynamic));
			}

			static const FName Transform0Name = FName("ShadowViewProjMatrix0");
			static const FName Transform1Name = FName("ShadowViewProjMatrix1");
			static const FName LightPosName = FName("LightPos");
			static const FName CharPosName = FName("CharPos");
			static const FName ShadowStrengthName = FName("Strength");

			// https://math.stackexchange.com/questions/1150232/finding-the-unit-direction-vector-given-azimuth-and-elevation

			const FVector DirVector = FVector(FMath::Sin(ShadowNode.Azimuth) * FMath::Cos(ShadowNode.Elevation),
											  FMath::Cos(ShadowNode.Azimuth) * FMath::Cos(ShadowNode.Elevation),
											  FMath::Sin(ShadowNode.Elevation)).GetSafeNormal();
			Orb->SoScreenCapture->SetWorldLocation(Orb->GetActorLocation() - DirVector * 10000);
			Orb->SoScreenCapture->SetWorldRotation(DirVector.Rotation());

			FLinearColor TransformRow0;
			FLinearColor TransformRow1;
			USoMathHelper::CalculateVectorsForShadowTransform(Orb->SoScreenCapture, TransformRow0, TransformRow1);

			Orb->ShadowMaterialPostProcessDynamic->SetVectorParameterValue(Transform0Name, TransformRow0);
			Orb->ShadowMaterialPostProcessDynamic->SetVectorParameterValue(Transform1Name, TransformRow1);

			Orb->ShadowMaterialPostProcessDynamic->SetVectorParameterValue(LightPosName, Orb->SoScreenCapture->GetComponentLocation());
			Orb->ShadowMaterialPostProcessDynamic->SetVectorParameterValue(CharPosName, Orb->GetActorLocation());

			Orb->ShadowMaterialPostProcessDynamic->SetScalarParameterValue(ShadowStrengthName, ShadowNode.Strength);
		}
#endif // WARRIORB_USE_CHARACTER_SHADOW
	}

#if WARRIORB_USE_CHARACTER_SHADOW
	if (!bEnabled && Orb->bShadowWasEnabled)
	{
		Orb->bShadowWasEnabled = false;
		if (Orb->LevelPostProcessVolume != nullptr)
			Orb->LevelPostProcessVolume->Settings.WeightedBlendables.Array.Empty();
		Orb->SoScreenCapture->Deactivate();
	}
#endif // WARRIORB_USE_CHARACTER_SHADOW
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::UpdateFloating()
{
	const bool bOnGround = Orb->SoMovement->IsMovingOnGround();
	if (Orb->bUmbrellaPressed &&
		!bOnGround &&
		!Orb->bFloatingActive &&
		Orb->GetPlayerCharacterSheet()->GetBoolValue(Orb->UmbrellaName))
	{
		StartFloating();
	}
	else if (Orb->bFloatingActive && (!Orb->bUmbrellaPressed || bOnGround))
	{
		StopFloating();
	}

	if (Orb->bFloatingActive)
		UpdateRollAnimDynamicValue(0.5f);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::OnAnimEvent(EAnimEvents Event)
{
	switch (Event)
	{
		case EAnimEvents::EAE_ClearRootMotion:
			Orb->SoMovement->ClearRootMotionDesc();
			// UE_LOG(LogTemp, Warning, TEXT("EAE_ClearRootMotion"));
			break;

		case EAnimEvents::EAE_ShowItemMesh:
			Orb->SoItemMesh->SetVisibility(true);
			break;

		default:
		{
			const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAnimEvents"), true);
			const FString EnumName = EnumPtr ? EnumPtr->GetNameStringByIndex(static_cast<int32>(Event)) : "Invalid EAnimEvents Value";
			UE_LOG(LogSoCharActivity, Warning, TEXT("Unhandeld animation event: %s"), *EnumName);
		}
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::UpdateOrientation(bool bPushedOnSpline)
{
	switch (LookDirection)
	{
		case ESoLookDirection::ELD_Frozen:
			// update forward vector
			Orb->StoredForwardVector = Orb->SoMovement->GetSplineLocation().GetDirectionFromVector(Orb->StoredForwardVector);
			Orb->SetActorRotation(Orb->StoredForwardVector.Rotation());
			break;

		case ESoLookDirection::ELD_Input:
			if (Orb->LastInputDirection.SizeSquared() > KINDA_SMALL_NUMBER)
				Orb->SetActorRotation(Orb->LastInputDirection.Rotation());
			break;

		case ESoLookDirection::ELD_Movement:

			if (Orb->bFreezeOrientationUntilLand)
			{
				Orb->StoredForwardVector = Orb->SoMovement->GetSplineLocation().GetDirectionFromVector(Orb->StoredForwardVector);
				Orb->SetActorRotation(Orb->StoredForwardVector.Rotation());

				if (Orb->SoMovement->IsMovingOnGround())
					Orb->bFreezeOrientationUntilLand = false;
			}
			else
			{
				if (Orb->bLockForwardVecPressed)
					Orb->SetActorRotation(Orb->SoMovement->GetSplineLocation().GetDirectionFromVector(Orb->GetActorForwardVector()).Rotation());
				else
				{
					if (bPushedOnSpline)
					{
						Orb->SetActorRotation(Orb->SoMovement->GetSplineLocation().GetDirectionFromVector(Orb->GetActorForwardVector()).Rotation());
						// two because worst case we need two, depends on the update order
						bWasPushedThisFrame = true;
						Orb->SoMovement->bOrientRotationToMovement = false;
					}
					else
					{
						// normal frame update
						if (bWasPushedThisFrame)
						{
							bWasPushedThisFrame = false;
							Orb->SoMovement->bOrientRotationToMovement = true;
						}
					}
				}
			}
			break;

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::IncreaseCollisionSize()
{
	if (!Orb->bCollisionAlreadyDecreased)
		return;

	Orb->bCollisionAlreadyDecreased = false;

	const FVector MeshOffset = FVector(0, 0, Orb->DecreasedHeight - Orb->NormalHeight);

	// collision is disabled temporary so we don't trigger traps we don't actually collide when the capsule is modified
	// note to self: this was a terrible solution, the character ended up leaving/entering areas when the collision size was changed
	// I just tried to swap the AddLocalTransform and the SetCapsuleHalfHeight calls now and removed this crap, let's see if it prevents the issues
	// Orb->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Orb->GetMesh()->AddLocalTransform(FTransform(MeshOffset));
	Orb->SoNewLights->AddLocalTransform(FTransform(MeshOffset));

	Orb->GetCapsuleComponent()->AddLocalTransform(FTransform(-MeshOffset));
	Orb->GetCapsuleComponent()->SetCapsuleHalfHeight(Orb->NormalHeight);
	Orb->GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, Orb->Mass);

	Orb->CapsuleBottomVFX->SetRelativeLocation(FVector(0.0f, 0.0f, -Orb->NormalHeight));
	// Orb->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::OnPreLanded(const FHitResult& Hit)
{
	Orb->StoredTimeOnJump = -1.0f;

	if (Orb->bFloatingActive)
	{
		StopFloating();
	}

	if (JumpType == ESoJumpType::EJT_Bounce || (JumpType == ESoJumpType::EJT_BounceIfPressed && Orb->bRollPressed))
	{
		ClearCooldownInAirFlags();
		return OnPreLandedBounce(Hit);
	}

	Orb->SetRollJumpLevel(0);

	AActor* HitActor = Hit.GetActor();

	if (HitActor != nullptr && (HitActor->ActorHasTag(SlipperyNegTag) || HitActor->ActorHasTag(SlipperyPosTag)))
		return true;

	// only allow jump from land if there is enough place to increase collision, otherwise we might stuck because the character falls back
	// before the jump time recently variable is reset
	if ((Orb->bJumpPressedRecently && !Orb->bBanJumpCauseSpamming && (Orb->CanIncreaseCollision() || !Orb->bCollisionAlreadyDecreased)) ||
		(HitActor != nullptr && HitActor->ActorHasTag(TrampolineTag)))
	{
		Orb->SoMovement->MovementMode = EMovementMode::MOVE_Walking; // walk so we can jump
		Orb->gTimerManager->ClearTimer(Orb->JumpPressedRecentlyTimer);
		Orb->JumpPressedRecentlyTimeOver();

		ClearCooldownInAirFlags();
		JumpPressed();
		// USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXFLegJump, Orb->GetActorTransform());
		Orb->JumpSFX->SetEvent(Orb->SFXFLegJump);
		Orb->JumpSFX->Play();
		Orb->OnLandJump.Broadcast();
		if (HitActor == nullptr || !HitActor->ActorHasTag(TrampolineTag))
			UGameplayStatics::SpawnEmitterAtLocation(Orb->GetWorld(), Orb->VFXBounceLarge, Hit.ImpactPoint, FRotator::ZeroRotator, true);
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::OnPreLandedBounce(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	float BaseActorVelocity = 0.0f;
	if (HitActor != nullptr)
	{
		if (HitActor->ActorHasTag(SlipperyNegTag) || HitActor->ActorHasTag(SlipperyPosTag))
			return true;

		BaseActorVelocity = HitActor->GetVelocity().Z;
	}

	// super isn't called for a reason: SetRollJumpLevel(0); shouldn't happen here
	Orb->OnRollHitBP(Hit.ImpactPoint, Hit.ImpactNormal, false);

	const float DefaultJumpBack = -Orb->SoMovement->Velocity.Z * DefaultBounceDampening;

	// maybe we jumped up with the biggest bounce jump, but then moved to a platform and didn't fall that much
	// big jump after that would look stupid
	// it is relative to base cause otherwise character could not bounce on a platform which moves up with const speed
	while (Orb->ActiveRollJumpLevel > 0 && BaseActorVelocity + 10.f - Orb->SoMovement->Velocity.Z < Orb->RollJumpLevels[Orb->ActiveRollJumpLevel])
		Orb->ActiveRollJumpLevel -= 1;

	// same but in the other direction: if we just walked down a platform but then did a huge fall we should bounce up to infinity and beyond
	while (Orb->ActiveRollJumpLevel < Orb->RollJumpLevels.Num() - 1 && BaseActorVelocity - 100.0f - Orb->SoMovement->Velocity.Z > Orb->RollJumpLevels[Orb->ActiveRollJumpLevel])
		Orb->ActiveRollJumpLevel += 1;

	const bool bIncreaseJumpHight = Orb->bJumpPressedRecently && !Orb->bBanJumpCauseSpamming;
	Orb->bJumpPressedRecently = false;

	// UE_LOG(LogTemp, Warning, TEXT("Orb->ActiveRollJumpLevel before: %d"), Orb->ActiveRollJumpLevel);

	if (!bIncreaseJumpHight)
	{
		Orb->LastMissedBounceTime = Orb->GetWorld()->GetTimeSeconds();
		Orb->MissedBounceLevel = FMath::Clamp(Orb->ActiveRollJumpLevel + 1, 0, Orb->RollJumpLevels.Num() - 1);
		Orb->MissedBounceLocationZ = Orb->GetActorLocation().Z;
	}

	float NewJumpVelocity = DefaultJumpBack;
	if (Orb->ActiveRollJumpLevel != 0 || bIncreaseJumpHight)
	{
		NewJumpVelocity = Orb->ModifyRollJumpLevel(bIncreaseJumpHight ? 1 : -1);

		// Achievement in main game: walljump and max bounce done before max bounce is explained with the tutorial decal
		if (USoPlatformHelper::IsNormalGame())
		{
			if (bIncreaseJumpHight && Orb->ActiveRollJumpLevel == Orb->RollJumpLevels.Num() - 1 && !Orb->SoPlayerCharacterSheet->GetBoolValue(Orb->MaxBounceDoneName))
			{
				Orb->SoPlayerCharacterSheet->SetBoolValue(Orb->MaxBounceDoneName, true);
				Orb->CheckSelfTrainedAchievement();
			}
		}
	}

	Orb->JumpPressedRecentlyTimeOver();

	if (DefaultJumpBack > NewJumpVelocity)
	{
		float NextLevel = Orb->ModifyRollJumpLevel(1);
		while (NextLevel < DefaultJumpBack)
		{
			NextLevel = Orb->ModifyRollJumpLevel(1);
			if (Orb->RollJumpLevels.Num() == Orb->ActiveRollJumpLevel + 1)
				break;
		}
		NewJumpVelocity = DefaultJumpBack;
	}

	// UE_LOG(LogTemp, Warning, TEXT("Orb->ActiveRollJumpLevel after: %d"), Orb->ActiveRollJumpLevel);

	const bool bForceJumpSurface = HitActor != nullptr && HitActor->ActorHasTag(TrampolineTag);

	// check forced bounce
	if (bForceJumpSurface)
		NewJumpVelocity = FMath::Max(NewJumpVelocity, Orb->GetRollJumpHeight(0));

	// apply base velocity
	NewJumpVelocity += BaseActorVelocity;

	// UE_LOG(LogTemp, Warning, TEXT("NewJumpVelocity: %f"), NewJumpVelocity);
	Orb->SoMovement->JumpZVelocity = NewJumpVelocity;

 	if (Orb->SoMovement->JumpZVelocity - 10.f > Orb->GetRollJumpHeight(0) / 2.f)
	{
		const FVector GroundLocation = Orb->GetActorLocation() - FVector(0.0f, 0.0f, Orb->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		if (fabs(Orb->LastLandZ - GroundLocation.Z) > 10.f)
			Orb->LastLandZ = GroundLocation.Z;

		Orb->bBanJumpCauseSpamming = false;
		Orb->SoMovement->Velocity.Z = Orb->SoMovement->JumpZVelocity;
		// UE_LOG(LogTemp, Warning, TEXT("Orb land seconds: %f"), Orb->GetWorld()->GetRealTimeSeconds());

		// Play bounce sound
		const int32 Index = FMath::Clamp(Orb->ActiveRollJumpLevel + 1, 0, Orb->RollJumpPitches.Num() - 1);
		USoAudioManager::PlaySoundAtLocation(Orb, Orb->GetBounceSFX(Index), Orb->GetActorTransform());

		if (!bForceJumpSurface)
			UGameplayStatics::SpawnEmitterAtLocation(Orb->GetWorld(), Orb->ActiveRollJumpLevel > 0 ? Orb->VFXBounceLarge : Orb->VFXBounceSmall,
													 GroundLocation,
													 FRotator::ZeroRotator,
													 true);

		return false;
	}

	// Play sound but also prevents spam
	// Delays for GetSoundTimeDelayStopSpam()
	const float CurrentTime = Orb->GetWorld()->GetTimeSeconds();
	if (CurrentTime - PreLandedBounceGroundTimeSinceLastSoundPlayed > Orb->GetSoundTimeDelayStopSpam())
	{
		if (Orb->SFXBounceVariants[0] != nullptr)
			USoAudioManager::PlaySoundAtLocation(Orb, Orb->GetBounceSFX(0), Orb->GetActorTransform());
		PreLandedBounceGroundTimeSinceLastSoundPlayed = CurrentTime;
	}

	Orb->SoMovement->JumpZVelocity = Orb->SetRollJumpLevel(0);
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::OnBaseChanged(AActor* ActualMovementBase)
{
	if (ActualMovementBase != nullptr)
	{
		const bool bPosSlide = ActualMovementBase->ActorHasTag(SlipperyPosTag);
		const bool bNegSlide = ActualMovementBase->ActorHasTag(SlipperyNegTag);
		if (bPosSlide || bNegSlide)
		{
			Orb->SoASlide->DirModifier = bPosSlide ? 1 : -1;
			SwitchActivity(Orb->SoASlide);
		}
		//else
		//{
		//  SEEMS like we don't need this
		//	if (ActualMovementBase->ActorHasTag(RollOnlySurface))
		//		SwitchActivity(Orb->SoARoll);
		//}

		if (ActualMovementBase->ActorHasTag(TriggerOnStand))
		{
			USoTriggerHelper::TriggerActor(ActualMovementBase, 1);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::DecreaseHealth(const FSoDmg& Damage)
{
	if (Damage.Physical < KINDA_SMALL_NUMBER && Damage.Magical < KINDA_SMALL_NUMBER)
		return true;

	const FSoDmg AppliedDmg = Orb->SoCharacterSheet->GetReducedDmg(Damage);
	USoEnemyHelper::DisplayDamageText(Orb, AppliedDmg, Orb->GetActorLocation(), Orb->GetActorForwardVector(), false, false, true);

	Orb->SoPlayerProgress->OnHpLost(AppliedDmg);
	return Orb->SoCharacterSheet->ApplyDmg(Damage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::OnDeath()
{
	const bool bSoulKeeperActive = Orb->ResRuneVisualActor != nullptr && Orb->ResRuneVisualActor->IsActive();
	Orb->GetPlayerProgress()->OnDeath(bSoulKeeperActive);
	Orb->SoADead->SetProperties(bSoulKeeperActive);
	SwitchActivity(Orb->SoADead);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called every frame
void USoActivity::Move(float Value)
{
	if (Orb->bForceForwardRun)
	{
		Value = -1.0f;
	}
	else
	{
		if (Orb->MovementStopMultiplier == 0 && FMath::IsNearlyZero(Value, KINDA_SMALL_NUMBER))
			Orb->MovementStopMultiplier = 1;

		if (Orb->SavedCamMovModifier != 0)
			Value *= Orb->SavedCamMovModifier;

		// forced movement is camera independent, thus it is "applied" after the camera thing
		if (Orb->ForcedMovementCounter > KINDA_SMALL_NUMBER)
			Value = Orb->ForcedMovementValue;

		Value *= Orb->MovementStopMultiplier;
	}

	if (!FMath::IsNearlyZero(Value, KINDA_SMALL_NUMBER))
	{
		const FVector MovDir = Orb->SoMovement->GetDirection();
		Orb->LastInputDirection = MovDir * Value;
		Orb->AddMovementInput(MovDir, Value);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::StrikePressed()
{
	if (Orb->SoAStrike->CanStrike(Orb->bSpecialStrikeWasRequestedLast) && CanBeInterrupted(EActivity::EA_Striking))
		SwitchActivity(Orb->SoAStrike);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::RightBtnPressed()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Player wanna fly away - he can't
void USoActivity::JumpPressed()
{
	// check if swing is possible
 	if (Orb->CouldSwing())
	{
		SwitchActivity(Orb->SoASwing);
		return;
	}

	// Double Jump Power
	if (Orb->SoMovement->MovementMode == EMovementMode::MOVE_Falling &&
		!Orb->bAirJumpedSinceLastLand &&
		CanJumpFromAir() &&
		Orb->SoPlayerCharacterSheet->GetBoolValue(Orb->DoubleJumpName))
	{
		Orb->bAirJumpedSinceLastLand = true;

		float Value = Orb->GetMovementXAxisValue();
		if (Orb->SavedCamMovModifier != 0)
			Value *= Orb->SavedCamMovModifier;
		Orb->SoMovement->Velocity = Orb->SoMovement->GetDirection() * Value * MovementSpeed * 1.5;
		Orb->SoMovement->Velocity.Z = JumpZVelocity * ((fabs(Value) > KINDA_SMALL_NUMBER) ? 1.0f : 1.1f);
		return;
	}

	if (Orb->bJumpPressedRecently == true)
		Orb->bBanJumpCauseSpamming = true;


	// Late bounce
	// Pressed jump right after landing
	const float CurrentTime = Orb->GetWorld()->GetTimeSeconds();
	if (Orb->SoMovement->Velocity.Z > KINDA_SMALL_NUMBER &&
		Orb->SoMovement->IsFalling() &&
		CurrentTime - Orb->LastMissedBounceTime < Orb->GetLateBounceThreshold())
	{
		// only if enabled
		const auto& GameSettings = USoGameSettings::Get();
		if ((GameSettings.GetBounceModeType() == ESoBounceModeType::Always) ||
			(GameSettings.GetBounceModeType() == ESoBounceModeType::OnEasy && FSoWorldState::Get().GetGameDifficulty() == ESoDifficulty::Sane))
		{
			Orb->SetRollJumpLevel(Orb->MissedBounceLevel);
			const float DesiredOriginalVelocity = Orb->GetActiveRollJumpHeight();

			const float TargetZ = Orb->MissedBounceLocationZ + (DesiredOriginalVelocity * DesiredOriginalVelocity) / (-2 * Orb->SoMovement->GetGravityZ());
			const float DeltaZ = TargetZ - Orb->GetActorLocation().Z;
			const float DesiredVelocityZ = FMath::Sqrt((-2 * Orb->SoMovement->GetGravityZ()) * DeltaZ);
			Orb->SoMovement->Velocity.Z = DesiredVelocityZ;

			// UE_LOG(LogTemp, Error, TEXT("Late BOUNCE: %f"), DesiredVelocityZ);

			Orb->LastMissedBounceTime = -1.0f;
			return;
		}
	}

	if (!Orb->bBanJumpCauseSpamming)
	{
		const bool bShouldOffsetCharOnJump = !Orb->bCollisionAlreadyDecreased;
		if (bShouldOffsetCharOnJump)
			Orb->GetCharacterMovement()->JumpZVelocity = JumpZVelocity - 117.82;

		bool bCoyoteJump = false;
		const float ThresholdSeconds = USoDateTimeHelper::NormalizeTime(0.15f);
		if (Orb->SoMovement->MovementMode == EMovementMode::MOVE_Falling &&
			Orb->SoMovement->Velocity.Z < 10.0f &&
			CurrentTime - Orb->LastSwitchToAirTime <= ThresholdSeconds)
		{
			bCoyoteJump = true;
			Orb->SoMovement->Velocity.Z = Orb->SoMovement->JumpZVelocity;
			Orb->SoMovement->SetMovementMode(MOVE_Falling);
		}

		if (bCoyoteJump || Orb->GetCharacterMovement()->DoJump(false)) // may or may not jump depending on the state
		{
			Orb->OnLandJump.Broadcast();

			if (bShouldOffsetCharOnJump)
			{
				Orb->DecreaseCollisionSize();
				Orb->AddActorWorldOffset(FVector(0, 0, Orb->NormalHeight - Orb->DecreasedHeight));
			}

			Orb->LastJumpTime = Orb->GetWorld()->GetTimeSeconds();

			const float MovementX = Orb->GetMovementXAxisValue();
			if (ID != EActivity::EA_Rolling && FMath::Abs(MovementX) > KINDA_SMALL_NUMBER)
			{
				Orb->StoredMovementInputDirOnJump = MovementX;
				Orb->StoredTimeOnJump = Orb->LastJumpTime;
			}

			OnJumped();
			// USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXFLegJump, Orb->GetActorTransform());
			Orb->JumpSFX->SetEvent(Orb->SFXFLegJump);
			Orb->JumpSFX->Play();
			const FVector GroundLocation = Orb->GetActorLocation() - FVector(0.0f, 0.0f, Orb->NormalHeight);
			UGameplayStatics::SpawnEmitterAtLocation(Orb->GetWorld(), Orb->VFXBounceLarge, GroundLocation, FRotator::ZeroRotator, true);
		}
		else
		{
			Orb->bJumpPressedRecently = true;
			Orb->gTimerManager->SetTimer(
				Orb->JumpPressedRecentlyTimer,
				Orb,
				&ASoCharacter::JumpPressedRecentlyTimeOver,
				USoDateTimeHelper::NormalizeTime(JumpPressedRecentlyTimeOffset),
				false
			);
		}

		// restore jump velocity
		if (bShouldOffsetCharOnJump)
			Orb->GetCharacterMovement()->JumpZVelocity = JumpZVelocity;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::TakeWeaponAway()
{
	if (Cast<const USoWeaponTemplate>((Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Weapon0)).Template) == nullptr)
		return;

	if (CanBeInterrupted(EActivity::EA_WeaponInArm))
	{
		SwitchActivity(Orb->SoAWeaponInArm);
		USoAudioManager::PlaySoundAtLocation(this, Orb->SFXArmedStart, Orb->GetActorTransform(), true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// play wanna use epic ball skill
void USoActivity::RollPressed()
{
	if (CanBeInterrupted(EActivity::EA_Rolling))
		SwitchActivity(Orb->SoARoll);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::ToggleWeapons()
{
	Orb->GetPlayerCharacterSheet()->ToggleWeapons();

	const FSoItem& Item = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
	const USoWeaponTemplate* WeaponTemplate = Cast<const USoWeaponTemplate>(Item.Template);
	if (WeaponTemplate == nullptr)
		return;

	Orb->SelectWeapon(Item);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::ToggleItems()
{
	Orb->GetPlayerCharacterSheet()->ToggleItemSlots();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::ToggleSpells(bool bQuickSelectionMode)
{
	// Implemented in USoAInUI
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::UseItemFromSlot0()
{
	// TODO: crossbow, is it needed?
	switch (Orb->GetPlayerCharacterSheet()->GetPrimaryItemType())
	{
		case ESoUsableItemType::EUIT_Crossbow:
			if (CanBeInterrupted(EActivity::EA_Aiming))
				SwitchActivity(Orb->SoAAiming);
			break;

		case ESoUsableItemType::EUIT_Potion:
			if (CanBeInterrupted(EActivity::EA_ItemUsage))
			{
				const FSoItem& Item = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Item0);
				USoUsableItemTemplate* UsableTemplate = Cast<USoUsableItemTemplate>(Item.Template);
				if (UsableTemplate == nullptr)
					return;

				USoAudioManager::PlaySoundAtLocation(Orb, UsableTemplate->SFXOnUse, Orb->GetActorTransform());
				Orb->GetPlayerCharacterSheet()->ApplyPotionEffects(UsableTemplate);
				Orb->GetPlayerCharacterSheet()->DecreaseItemCountOnSlot(ESoItemSlot::EIS_Item0);
				Orb->GetPlayerCharacterSheet()->OnItemUsed.Broadcast();
			}
			break;
		case ESoUsableItemType::EUIT_Hammer:
			if (!Orb->SoMovement->IsMovingOnGround())
				return;

		case ESoUsableItemType::EUIT_Throwable:
			if (CanBeInterrupted(EActivity::EA_ItemUsage))
			{
				const FSoItem& Item = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Item0);
				const USoUsableItemTemplate* UsableTemplate = Cast<const USoUsableItemTemplate>(Item.Template);
				if (UsableTemplate == nullptr)
					return;

				if (Item.Amount <= 0)
				{
					USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXFCanNot, Orb->GetActorTransform());
					return;
				}

				SwitchActivity(Orb->SoAItemUsage);
				Orb->GetPlayerCharacterSheet()->OnItemUsed.Broadcast();
			}
			break;

		case ESoUsableItemType::EUIT_Spell:
			if (CanBeInterrupted(EActivity::EA_ItemUsage))
			{
				Orb->SoPlayerCharacterSheet->UseActiveRuneStoneIfCastable();
				Orb->GetPlayerCharacterSheet()->OnItemUsed.Broadcast();
			}
			break;

		default:
			break;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::InteractKeyPressed(bool bPrimary)
{
	if (!CanInteract())
		return;

	if (Orb->Interactables.Num() == 0)
		return;

	const bool bFirstExclusive = ISoInteractable::Execute_IsExclusive(Orb->Interactables[0]);

	if (!bFirstExclusive && Orb->Interactables.Num() > 2)
	{
		if (bPrimary)
			OnInteract(Orb->Interactables[Orb->ActiveInteractable]);
		else
			Orb->SwitchActiveInteractable();
	}
	else
	{
		const bool bFirstPrefersSecondKey = ISoInteractable::Execute_IsSecondKeyPrefered(Orb->Interactables[0]);

		if (bFirstExclusive || Orb->Interactables.Num() == 1)
		{
			if (bPrimary != bFirstPrefersSecondKey)
			{
				OnInteract(Orb->Interactables[0]);
				Orb->ActiveInteractable = 0;
			}
		}
		else
		{
			Orb->ActiveInteractable = (bPrimary != bFirstPrefersSecondKey) ? 0 : 1;
			OnInteract(Orb->Interactables[Orb->ActiveInteractable]);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::OnInteract(AActor* Interactable)
{
	// are we sure it's something? Are we sure we can act?
	if (Interactable != nullptr && CanBeInterrupted())
		ISoInteractable::Execute_Interact(Interactable, Orb);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::CanInteract() const
{
	return Orb->CouldSwing() ||
		(Orb->Interactables.Num() > 0 &&
		(Orb->SoMovement->IsMovingOnGround() || ISoInteractable::Execute_CanBeInteractedWithFromAir(Orb->Interactables[0])));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::StartLeverPush(const FSoLeverData& LeverData, const FSoSplinePoint& SplinePoint, float ZValue)
{
	if (CanBeInterrupted(EActivity::EA_LeverPush))
	{
		Orb->SoALeverPush->SetLeverData(LeverData);
		SwitchActivity(Orb->SoALeverPush);
		Orb->SetPositionOnSplineSP(SplinePoint, ZValue, true, false, LeverData.PushDirection < 0);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::DebugFeature()
{
	// overridden in cam edit!!!

	Orb->OnDebugButtonPressed();

#if WITH_EDITOR
	if (GUnrealEd->PlayWorld)
	{
		const bool bWasPaused = GUnrealEd->PlayWorld->bDebugPauseExecution;
		GUnrealEd->PlayWorld->bDebugPauseExecution = !bWasPaused;
		if (bWasPaused)
			GUnrealEd->PlaySessionResumed();
		else
			GUnrealEd->PlaySessionPaused();
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::OnDmgTaken(const FSoDmg& Dmg, const FSoHitReactDesc& HitReactDesc)
{
	// Modify incoming DMG based on difficulty
	FSoDmg CorrectedDmg = Dmg;
	switch (FSoWorldState::Get().GetGameDifficulty())
	{
		case ESoDifficulty::Sane:
			CorrectedDmg *= 0.5f;
			break;

		// it is called Insane for a reason
		case ESoDifficulty::Insane:
			CorrectedDmg *= 2.0f;
			break;

		default:
			break;
	}

	if ((Orb->DamageBlockCounter > 0.0f || Orb->bGodMode) &&
		HitReactDesc.HitReact != ESoHitReactType::EHR_BreakIntoPieces &&
		HitReactDesc.HitReact != ESoHitReactType::EHR_FallToDeath)
		return true;

	Orb->OnHitSuffered.Broadcast(CorrectedDmg, HitReactDesc);

	if (!Orb->SoAHitReact->Activate(HitReactDesc, (Orb->DamageBlockCounter > 0.0f || Orb->bGodMode) ? true : DecreaseHealth(CorrectedDmg)))
		return false;

	Orb->DamageBlockCounter = HitReactDesc.HitReact != ESoHitReactType::EHR_JumpAwayLight ? Orb->DamageBlockTime : Orb->DamageBlockTimeMelee;

	// Shake something
	if (HitReactDesc.HitReact != ESoHitReactType::EHR_FallToDeath && HitReactDesc.HitReact != ESoHitReactType::EHR_BreakIntoPieces)
	{
		USoPlatformHelper::PlayCameraShake(Orb, Orb->HitReactCamShake);
		USoPlatformHelper::PlayForceFeedback(Orb, Orb->HitReactControllerVibration);
		Orb->PlayHitReactLightEffectBP();
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoActivity::OnMeleeHit(const FSoMeleeHitParam& HitParam)
{
	Orb->OnMeleeHitSuffered.Broadcast(Orb, HitParam);

	const bool bSurvived = OnDmgTaken(HitParam.Dmg, HitParam.HitReactDesc);
	if (bSurvived)
		Orb->DamageBlockCounter = Orb->DamageBlockTimeMelee;
	USoAudioManager::PlaySoundAtLocation(Orb, bSurvived ? Orb->SFXFOnMeleeHit : Orb->SFXFOnMeleeHitDeath, FTransform(FVector(HitParam.Hit.ImpactPoint)));

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::OnPushed(const FVector& DeltaMovement, float DeltaSeconds, bool bStuck, AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage)
{
	if (bStuck)
		Orb->SoMovement->SetPushedAndStuck(RematerializeLocation, DamageAmountIfStuck, bForceDamage);

	Orb->SoMovement->ForceUpdateFloor();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::SwitchToRelevantState(bool bArmedAllowed)
{
	/** only if we are the active activity */
	if (Orb->SoActivity != this)
		return;

	if (LastActivity == Orb->SoALillian || Orb->SoPlayerCharacterSheet->GetBoolValue(USoStaticHelper::GetLillianFormName()))
	{
		SwitchActivity(Orb->SoALillian);
		return;
	}

	if (USoGameInstance* GameInstance = USoGameInstance::GetInstance(Orb))
	{
		// if (GameInstance->CanPlayEnterSequenceForMap(USoLevelHelper::GetMapNameFromObject(Orb)))
		if (GameInstance->ShouldStartLevelEnterSequence())
		{
			// Wait is used to block character movement input
			// input value (wait duration) is irrelevant as this action will be interrupted later, but it has to be longer
			// than the wait time before cutscene starts (which is usually around 4 sec iirc)
			Orb->SoAWait->Enter(10000.0f);
			return;
		}
	}

	if (bArmedAllowed &&
		(LastActivity == Orb->SoAWeaponInArm || LastActivity == Orb->SoAStrike) &&
		Orb->SoPlayerCharacterSheet->HasWeaponReady())
		SwitchActivity(Orb->SoAWeaponInArm);
	else
	{
		const bool bMovingOnGround = Orb->SoMovement->IsMovingOnGround();
		const bool bCollisionDecreased = Orb->IsCollisionDecreased();
		if (bCollisionDecreased && bMovingOnGround && !Orb->CanIncreaseCollision())
		{
			SwitchActivity(Orb->SoARoll);
			Orb->SoARoll->LastActivity = Orb->SoADefault;
		}
		else
			SwitchActivity(Orb->SoADefault);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::OnJumped()
{
	Orb->SetRollJumpLevel(Orb->LegJumpLevelIndex);
	Orb->OnInteractionLabelChanged.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::OnSuperModeChange(bool bEnter)
{
	if (bEnter)
	{
		Orb->SoMovement->SetMovementMode(EMovementMode::MOVE_None);
		Orb->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		Orb->SoMovement->SetMovementMode(EMovementMode::MOVE_Falling);
		Orb->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::SuperModeTick(float DeltaSeconds)
{
	const float FlyVelocity = FMath::Max(600.0f, MovementSpeed);
	const float DeltaZ = DeltaSeconds * Orb->GetMovementYAxisValue() * FlyVelocity;
	const float DeltaX = DeltaSeconds * Orb->GetMovementXAxisValue() * FlyVelocity * Orb->SavedCamMovModifier;

	const float OrbZ = Orb->GetActorLocation().Z + DeltaZ;
	const int32 SavedCamModifier = Orb->SavedCamMovModifier;
	Orb->SetPositionOnSplineSP(Orb->SoMovement->GetSplineLocation() + DeltaX, OrbZ, false, false);
	Orb->LastLandZ = OrbZ - Orb->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Orb->SavedCamMovModifier = SavedCamModifier;
	Orb->bResetSavedCamMovModifierAfterTick = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::SetEnabledState()
{
	Orb->GetCapsuleComponent()->SetCollisionProfileName(EnabledCollisionProfileName);

	Orb->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Orb->ShowCharacterBP(false);
	Orb->GetMesh()->bPauseAnims = false;

	Orb->SoMovement->SetMovementMode(MOVE_Walking);

	Orb->bJumpPressedRecently = false;
	Orb->bBanJumpCauseSpamming = false;

	Orb->bRollPressed = false;

	Orb->gTimerManager->ClearTimer(Orb->JumpPressedRecentlyTimer);
	Orb->gTimerManager->ClearTimer(Orb->LegLandRecentlyTimer);

	// Orb->bDamageImmunity = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::SetDisabledState()
{
	// Orb->gTimerManager->ClearTimer(Orb->HitReactTimer);
	// Orb->bDamageImmunity = true;

	Orb->SoMovement->Velocity = FVector(0, 0, 0);
	Orb->SoMovement->StopActiveMovement();
	Orb->ActiveRollJumpLevel = 0;
	Orb->SoMovement->SetMovementMode(MOVE_None);

	Orb->GetCapsuleComponent()->SetCollisionProfileName(DisabledCollisionProfileName);

	Orb->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Orb->HideCharacterBP(false);
	Orb->GetMesh()->bPauseAnims = true;

	Orb->Interactables.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::StartFloating()
{
	Orb->bFloatingActive = true;
	Orb->SoFloatVFXNew->Activate(true);
	Orb->FloatStartSFX->Activate(true);
	Orb->SoMovement->SetFallVelocityOverride(true, Orb->FloatingVelocity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::StopFloating()
{
	Orb->bFloatingActive = false;
	Orb->SoFloatVFXNew->Deactivate();
	Orb->FloatStopSFX->Activate(true);
	Orb->SoMovement->SetFallVelocityOverride(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::OverrideAllMovementSpeed(float Value)
{
	Orb->SoADefault->MovementSpeed = Value;
	Orb->SoARoll->MovementSpeed = Value;
	Orb->SoAWeaponInArm->MovementSpeed = Value;
	Orb->SoAStrike->MovementSpeed = Value;
	Orb->SoASlide->MovementSpeed = Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::ClearCooldownInAirFlags()
{
	for (FSoCooldownCounter& Cooldown : Orb->Cooldowns)
		Cooldown.bAllowInAir = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoActivity::UpdateRollAnimDynamicValue(float SpeedMultiplier)
{
	const FVector2D Vel = FVector2D(Orb->SoMovement->Velocity);
	const FVector2D Look = FVector2D(Orb->GetActorForwardVector());
	const int32 Sign = FMath::Sign(Vel | Look);
	Orb->DynamicAnimValue = Sign * 2.f * (Orb->SoMovement->Velocity.Size2D() / Orb->SoMovement->MaxWalkSpeed) * SpeedMultiplier;
}
