// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoCharacter.h"

#include "EngineGlobals.h"
#include "EngineMinimal.h" // ANY_PACKAGE enum search
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Curves/CurveFloat.h"
#if WITH_EDITOR
#include "Editor/UnrealEdEngine.h"
#endif // WITH_EDITOR
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/PostProcessVolume.h"

#include "DestructibleActor.h"
#include "DestructibleComponent.h"

#include "FMODEvent.h"
#include "Levels/SoLevelManager.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameInstance.h"
#include "Basic/SoAudioManager.h"
#include "Settings/SoGameSettings.h"
#include "Settings/Input/SoInputNames.h"
#include "SaveFiles/SoWorldState.h"

#include "Online/Achievements/SoAchievementManager.h"

#include "Online/Analytics/SoAnalyticsComponent.h"
#include "SoPlayerCharacterSheet.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "SoPlayerProgress.h"
#include "Items/SoInventoryComponent.h"

#include "SoCharStates/SoActivity.h"
#include "SoCharStates/SoADead.h"
// #include "SoCharStates/SoAFallToDeath.h"
#include "SoCharStates/SoAHitReact.h"
#include "SoCharStates/SoAAiming.h"
#include "SoCharStates/SoASlide.h"
#include "SoCharStates/SoASwing.h"
#include "SoCharStates/SoADefault.h"
#include "SoCharStates/SoALillian.h"
#include "SoCharStates/SoARoll.h"
#include "SoCharStates/SoALeverPush.h"
#include "SoCharStates/SoACarry.h"
#include "SoCharStates/SoAWeaponInArm.h"
#include "SoCharStates/SoAStrike.h"
#include "SoCharStates/SoAItemUsage.h"
#include "SoCharStates/SoAInteractWithEnvironment.h"
#include "SoCharStates/SoAWait.h"
#include "SoCharStates/SoAWaitForActivitySwitch.h"
#include "SoCharStates/SoAInUI.h"
#include "SoCharStates/SoATeleport.h"
#include "SoCharStates/SoACameraEdit.h"
#include "SoCharStates/SoASkyControlEdit.h"
#include "SoCharStates/SoACharShadowEdit.h"

#include "SplineLogic/SoPlayerSpline.h"
#include "SplineLogic/SoSplineHelper.h"
#include "SplineLogic/SoSplinePointPtr.h"
#include "SplineLogic/SoMarker.h"
#include "SplineLogic/SoLocationRegistry.h"

#include "Interactables/SoInteractable.h"
#include "Interactables/SoCarryable.h"
#include "Objects/SoSwingCenter.h"
#include "SoSpringArmComponent.h"
#include "Basic/SoGameMode.h"
#include "SoWizard.h"
#include "SoBounceSFXOverrideBox.h"
#include "Projectiles/SoProjectileSpawnerComponent.h"

#include "UI/General/SoUITypes.h"
#include "UI/SoUISystem.h"
#include "UI/Menu/SoUIMenuMain.h"

#include "SoPlayerController.h"
#include "Basic/SoGameSingleton.h"
#include "Levels/SoLevelHelper.h"
#include "Items/ItemTemplates/SoWeaponTemplate.h"

#include "SoLocalization.h"
#include "Basic/Helpers/SoDateTimeHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoChar, All, All);

bool ASoCharacter::bEnableQuickSaveLoad = false;
const FName ASoCharacter::SpellcasterCapacityName = TEXT("SpellcasterCapacity");


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoCharacter::ASoCharacter(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<USoPlayerCharacterSheet>(CharacterSheetName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	LillianMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SoLillianMesh"));
	LillianMesh->SetupAttachment(GetRootComponent());

	WeatherVFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoWeatherVFX"));
	WeatherVFX->bAbsoluteRotation = true;
	WeatherVFX->SetupAttachment(GetRootComponent());

	CapsuleBottomVFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoCapsuleBottomVFX"));
	CapsuleBottomVFX->SetupAttachment(GetRootComponent());

	ProjectileSpawner->SetSpawnProjectileOnSpline(true);
	ProjectileSpawner->SetupAttachment(GetMesh());


	SoSword = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoSword"));
	SoSword->SetVisibility(false);
	SoSword->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SoOffHandWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoOffHandWeapon"));
	SoOffHandWeapon->SetVisibility(false);
	SoOffHandWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SoSwordFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoSwordFX"));
	SoSwordFX->SetVisibility(false);
	SoSwordFX->SetupAttachment(SoSword);

	SoOffHandWeaponFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoOffHandWeaponFX"));
	SoOffHandWeaponFX->SetVisibility(false);
	SoOffHandWeaponFX->SetupAttachment(SoOffHandWeapon);


	SoItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoItemMesh"));
	SoItemMesh->SetVisibility(false);

	SoInventory = CreateDefaultSubobject<USoInventoryComponent>(TEXT("SoInventory"));

	SoPlayerCharacterSheet = Cast<USoPlayerCharacterSheet>(SoCharacterSheet);
	SoPlayerProgress = CreateDefaultSubobject<USoPlayerProgress>(TEXT("SoPlayerProgress"));
	SoAnalytics = CreateDefaultSubobject<USoAnalyticsComponent>(TEXT("SoAnalytics"));

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USoSpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = 500.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 75.f);
	// CameraBoom->RelativeRotation = FRotator(0.f, 180.f, 0.f);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bEnableCameraLag = true;

	// Create a camera and attach to boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	SideViewCameraComponent->bUsePawnControlRotation = false;

	SoMovement->RotationRate = FRotator(0.0f, 9720.0f, 0.0f); // ...at this rotation rate
	SoMovement->GravityScale = 2.f;
	SoMovement->AirControl = 0.80f;
	SoMovement->GroundFriction = 3.f;

	SoMovement->MaxFlySpeed = 600.f;

	// Don't rotate when the controller rotates.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	bCollisionAlreadyDecreased = false;

#define CREATE_ACTIVITY(Var) Var = CreateDefaultSubobject<U##Var>(#Var);
	CREATE_ACTIVITY(SoARoll)
	CREATE_ACTIVITY(SoADead)
	CREATE_ACTIVITY(SoAHitReact)
	CREATE_ACTIVITY(SoADefault)
	CREATE_ACTIVITY(SoALillian)
	CREATE_ACTIVITY(SoASlide)
	CREATE_ACTIVITY(SoASwing)
	CREATE_ACTIVITY(SoALeverPush)
	CREATE_ACTIVITY(SoACarry)
	// CREATE_ACTIVITY(SoACarryPickUp)
	CREATE_ACTIVITY(SoACarryDrop)
	CREATE_ACTIVITY(SoAAiming)
	CREATE_ACTIVITY(SoAWeaponInArm)
	CREATE_ACTIVITY(SoAStrike)
	CREATE_ACTIVITY(SoAItemUsage)
	CREATE_ACTIVITY(SoAInteractWithEnvironment);
	CREATE_ACTIVITY(SoASoAWaitForActivitySwitch);
	CREATE_ACTIVITY(SoAWait);
	CREATE_ACTIVITY(SoAInUI)
	CREATE_ACTIVITY(SoATeleport)
	CREATE_ACTIVITY(SoACameraEdit)
	CREATE_ACTIVITY(SoASkyControlEdit)
	CREATE_ACTIVITY(SoACharShadowEdit)
#undef CREATE_ACTIVITY


	SoCenterNew = CreateDefaultSubobject<USceneComponent>(TEXT("SoCenterNew"));
	SoCenterNew->bAbsoluteRotation = true;
	SoCenterNew->bAbsoluteScale = true;
	SoCenterNew->SetupAttachment(GetMesh(), FName("Body"));

	SoFloatVFXNew = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoFloatVFXNew"));
	SoFloatVFXNew->bAbsoluteScale = true;
	SoFloatVFXNew->SetupAttachment(GetMesh(), FName("Body"));

	SoQuickTeleportPre = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoQuickTeleportPre"));
	SoQuickTeleportPre->bAbsoluteScale = true;
	SoQuickTeleportPre->SetupAttachment(GetMesh(), FName("Body"));
	SoQuickTeleportPre->bAutoActivate = false;

	SoBreak = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoBreak"));
	SoBreak->bAbsoluteScale = true;
	SoBreak->SetupAttachment(GetMesh(), FName("Body"));
	SoBreak->bAutoActivate = false;

	SoQuickTeleportPost = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoQuickTeleportPost"));
	SoQuickTeleportPost->bAbsoluteScale = true;
	SoQuickTeleportPost->SetupAttachment(GetMesh(), FName("Body"));
	SoQuickTeleportPost->bAutoActivate = false;

	SoResVFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoResVFX"));
	SoResVFX->SetupAttachment(SoCenterNew);
	SoResVFX->bAutoActivate = false;

	SoResCPVFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoResCPVFX"));
	SoResCPVFX->SetupAttachment(SoCenterNew);
	SoResCPVFX->bAutoActivate = false;

	SoNewSmoke = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoNewSmoke"));
	SoNewSmoke->SetupAttachment(SoCenterNew);

	SoNewLights = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SoNewLights"));
	SoNewLights->SetupAttachment(GetRootComponent());

	SoGlowSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoGlowSphere"));
	SoGlowSphere->SetupAttachment(SoCenterNew);
	SoGlowSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	SoResSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("SoResSFX"));
	SoResSFX->SetupAttachment(SoCenterNew);
	SoResSFX->bAutoActivate = false;

	SoResCPSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("SoResCPSFX"));
	SoResCPSFX->SetupAttachment(SoCenterNew);
	SoResCPSFX->bAutoActivate = false;

	SlideSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("SlideSFX"));
	SlideSFX->SetupAttachment(GetRootComponent());
	SlideSFX->bAutoActivate = false;

	JumpSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("JumpSFX"));
	JumpSFX->SetupAttachment(SoCenterNew);
	JumpSFX->bAutoActivate = false;


	RollSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("RollSFX"));
	RollSFX->SetupAttachment(GetRootComponent());
	RollSFX->bAutoActivate = false;

	FloatStartSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("FloatStartSFX"));
	FloatStartSFX->SetupAttachment(GetRootComponent());
	FloatStartSFX->bAutoActivate = false;

	FloatStopSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("FloatStopSFX"));
	FloatStopSFX->SetupAttachment(GetRootComponent());
	FloatStopSFX->bAutoActivate = false;


	DialogueData.PositionIndex = 1;
	DialogueData.ParticipantName = TEXT("Warriorb");
	DialogueData.ParticipantDisplayName = FROM_STRING_TABLE_DIALOGUE("char_name_warriorb");
	DialogueData.TextBoxY = -524;
	DialogueData.FloorPosition = FVector2D(220.0f, -500.0f);
	DialogueData.FadeInAnimName = TEXT("Char_Arrive");
	DialogueData.FadeOutAnimName = TEXT("Char_Leave");

//#if WARRIORB_USE_CHARACTER_SHADOW
	SoScreenCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SoScreenCapture"));
	SoScreenCapture->SetupAttachment(GetRootComponent());
//#endif // WARRIORB_USE_CHARACTER_SHADOW

	WeaponProjectileSpawner = CreateDefaultSubobject<USoProjectileSpawnerComponent>(TEXT("SoWeaponProjectileSpawner"));
	WeaponProjectileSpawner->SetupAttachment(RootComponent);

	SFXBounceVariants.SetNum(5);
	RollJumpPitches.SetNum(5);

#if WITH_EDITOR
	bEnableQuickSaveLoad = true;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::PreInitializeComponents()
{
	SoActivity = SoADefault;
	SoMovement->bOrientRotationToMovement = true;
	SoMovement->JumpZVelocity = SoADefault->JumpZVelocity;
	SoMovement->MaxWalkSpeed = SoADefault->MovementSpeed;

	Super::PreInitializeComponents();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	LillianMesh->SetVisibility(false);
	LillianMesh->SetComponentTickEnabled(false);
	LillianMesh->bNoSkeletonUpdate = true;

	SoSword->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("Weapon_R"));
	SoSword->SetRelativeRotation(WeaponLocalRot);

	SoOffHandWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("Weapon_L"));
	SoOffHandWeapon->SetRelativeRotation(WeaponLocalRot);

	SelectWeapon(SoPlayerCharacterSheet->GetEquippedItem(ESoItemSlot::EIS_Weapon0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts or when spawned
void ASoCharacter::BeginPlay()
{
	GameInstance = USoGameInstance::GetInstance(this);
	const bool bIsInMenuLevel = GameInstance->IsMenu();

	CurrentGlowSphereColor = GlowSphereColorDefault;
	CurrentSmokeColor = SmokeColorDefault;

#if WARRIORB_USE_CHARACTER_SHADOW
	// Setup fake shadow
	if (!bIsInMenuLevel && ShadowMaterialPostProcessDynamic == nullptr)
	{
		ensure(ShadowMaterialPostProcess);
		ShadowMaterialPostProcessDynamic = UMaterialInstanceDynamic::Create(ShadowMaterialPostProcess, this);

		TArray<AActor*> ActorArray;
		UGameplayStatics::GetAllActorsOfClass(this, APostProcessVolume::StaticClass(), ActorArray);

		AActor* PostProcessActor = ActorArray.Num() > 0 ? ActorArray[0] : nullptr;
		if (PostProcessActor == nullptr)
		{
			UE_LOG(LogSoChar, Error, TEXT("Failed to setup shadow post process material: the level should contain exactly one post process volume!"))
		}
		else
		{
			// Manually start begin play on actor
			if (!PostProcessActor->HasActorBegunPlay())
			{
				PostProcessActor->DispatchBeginPlay();
			}
			LevelPostProcessVolume = Cast<APostProcessVolume>(ActorArray[0]);
		}
	}
	SoScreenCapture->ShowOnlyActors = { this };
#endif // WARRIORB_USE_CHARACTER_SHADOW
	SoScreenCapture->Deactivate();

	SoWizard = NewObject<USoWizard>(this, WizardClass);
	SoWizard->Initialize(this);

	GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, Mass);

	Super::BeginPlay();

	gTimerManager = &GetWorld()->GetTimerManager();
	// for idle detection
	SavedPosition = GetActorLocation();

	AddTickPrerequisiteComponent(SoMovement);
	CameraBoom->AddTickPrerequisiteActor(this);
	SideViewCameraComponent->AddTickPrerequisiteComponent(CameraBoom);


	GetMesh()->AddTickPrerequisiteActor(this);
	GetMesh()->AddTickPrerequisiteComponent(SideViewCameraComponent);


	SoPlayerCharacterSheet->RecalculateStats();
	SoPlayerCharacterSheet->RestoreHealth();

	// created in blueprint's begin play
	if (ResRuneVisualActor)
	{
		ResRuneVisualActor->Deactivate();
		ResRuneVisualActor->SetActorHiddenInGame(true);
	}

#if WARRIORB_WITH_EDITOR
	// Location is the closest spline in editor
	if (SoMovement->GetSplineLocation().GetSpline() == nullptr)
	{
		FSoSplinePointPtr LocationPtr;
		USoSplineHelper::UpdateSplineLocationRef(this, LocationPtr, true, false, true);
		FSoSplinePoint Location = LocationPtr.Extract();
		Location.SetReferenceActor(this);
		SoMovement->SetSplineLocation(Location);
		ActiveCheckPointLocation = Location;
		ActiveCheckpointZLocation = GetActorLocation().Z;
		OnSplineChanged({}, Location);
	}

	if (SoMovement->GetSplineLocation().GetSpline() == nullptr)
		UE_LOG(LogSoChar, Error, TEXT("Failed to place Character to the closest spline in the Editor"));

	// In editor game is already started
	GameInstance->SetGameStarted(true);

	SoMovement->Velocity.Z = 1000;
	SoMovement->SetMovementMode(EMovementMode::MOVE_Falling);
#else

	// Load first so that the menu has the right state
	// FSoWorldState::Get().LoadGame();

	// Pause the game and open the menu in non editor builds by default
	// EDIT: menu is not paused anymore
	//bool bPausedGame = false;
	//if (GameInstance)
	//	bPausedGame = GameInstance->PauseGame(true, true);
	//
	//if (!bPausedGame)
	//	UE_LOG(LogSoChar, Error, TEXT("Failed to pause game. Improper UI setup?"));
#endif // WARRIORB_WITH_EDITOR

	// Handle main menu
	if (bIsInMenuLevel)
	{
		GameInstance->OpenMenuInstant();
		GameInstance->SetGameStarted(false);
	}

	// Teleport to the active checkpoint name, initial value is from
	if (!WARRIORB_WITH_EDITOR || bIsInMenuLevel)
	{
		TeleportToActiveCheckpointName();
	}

	// Start music and VFX on Spline
	if (const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SoMovement->GetSplineLocation().GetSpline()))
	{
		UpdateMusic(true);

		UParticleSystem* NewTemplate = nullptr;
		if (PlayerSpline->GetLevelClaims().Num() > 0)
			NewTemplate = USoLevelHelper::GetWeatherVFXForLevel(PlayerSpline->GetLevelClaims()[0].LevelName);

		WeatherVFX->SetTemplate(NewTemplate);
		OnSplineChanged({}, SoMovement->GetSplineLocation());
	}

	SoADefault->UpdateCamera(0.1f);
	CameraBoom->ForceUpdate();

	// Subscribe to device changed
	if (ASoPlayerController* SoController = GetSoPlayerController())
	{
		SoController->OnDeviceTypeChanged().AddDynamic(this, &Self::HandleDeviceTypeChanged);
		DeviceType = SoController->GetCurrentDeviceType();
	}

	CapsuleBottomVFX->SetRelativeLocation(FVector(0.0f, 0.0f, -NormalHeight));
	CapsuleBottomVFX->Deactivate();

	LastAutoSaveTime = 0.0f;

	if (MusicOverride == nullptr)
		UpdateMusic(true);

	if (USoGameSettings* GameSettings = USoGameSettings::GetInstance())
		USoGameSettings::Get().OnCharacterSkinChanged.AddDynamic(this, &ASoCharacter::UpdateCharacterSkin);

	UpdateCharacterSkin();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ASoPlayerController* SoController = GetSoPlayerController())
		SoController->OnDeviceTypeChanged().RemoveDynamic(this, &Self::HandleDeviceTypeChanged);

	if (USoGameSettings* GameSettings = USoGameSettings::GetInstance())
		GameSettings->OnCharacterSkinChanged.RemoveDynamic(this, &ASoCharacter::UpdateCharacterSkin);

	Super::EndPlay(EndPlayReason);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::UpdateCharacterSkin()
{
	UpdateCharacterSkinBP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called every frame
void ASoCharacter::Tick(float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick"), STAT_Tick, STATGROUP_SoCharacter);

	const float OldDamageBlockCounter = DamageBlockCounter;
	DamageBlockCounter = FMath::Max(DamageBlockCounter - DeltaSeconds, -1.0f);
	if (OldDamageBlockCounter > 0.0f && DamageBlockCounter <= 0.0f)
		OnDamageImmunityOver.Broadcast();

	bSavedCameraBoomZUpdateBlock = false;

	if (!bUseSavedCameraBoomZ && CameraUnfreezeCounter >= 0.0f)
		CameraUnfreezeCounter -= DeltaSeconds;

	ForcedMovementCounter = FMath::Max(ForcedMovementCounter - DeltaSeconds, -1.0f);

	// NOTE: Must be fore activity
	// Open spell cast switch
	static constexpr float DeltaSimilar = 0.001;
	if (IsUsingRightThumbStick())
	{
		if (FMath::IsNearlyEqual(PreviousGamepadRightX, GamepadRightX, DeltaSimilar)
			&& FMath::IsNearlyEqual(PreviousGamepadRightY, GamepadRightY, DeltaSimilar))
		{
			RightThumbstickSameValueFrames++;
		}
		else
		{
			RightThumbstickSameValueFrames = 0;
		}

		// Apply some delay, so that we don't open the menu spell menu accidentally
		if (SoAInUI->IsSpellCastSwitchOpen())
		{
			TickQuickSelectionSum = 0.f;
		}
		else
		{
			TickQuickSelectionSum += DeltaSeconds;
			if (TickQuickSelectionSum > ThresholdOpenQuickSelection)
			{
				TickQuickSelectionSum = 0.f;
				ToggleSpells(true);
			}
		}

		PreviousGamepadRightX = GamepadRightX;
		PreviousGamepadRightY = GamepadRightY;

		// UE_LOG(LogTemp, Warning, TEXT("Right: X = %f, Y = %f, Angle = %f, RightThumbstickSameValueFrames = %d"),
			// GamepadRightX, GamepadRightY, GetGamepadRightDirectionDegrees(), RightThumbstickSameValueFrames);
	}
	else
	{
		TickQuickSelectionSum = 0.f;
		RightThumbstickSameValueFrames = 0;
	}

	if (IsUsingLeftThumbStick())
	{
		if (FMath::IsNearlyEqual(PreviousGamepadLeftX, GamepadLeftX, DeltaSimilar)
			&& FMath::IsNearlyEqual(PreviousGamepadLeftY, GamepadLeftY, DeltaSimilar))
		{
			LeftThumbstickSameValueFrames++;
		}
		else
		{
			LeftThumbstickSameValueFrames = 0;
		}

		PreviousGamepadLeftX = GamepadLeftX;
		PreviousGamepadLeftY = GamepadLeftY;

		// UE_LOG(LogTemp, Warning, TEXT("Left: X = %f, Y = %f, Angle = %f, LeftThumbstickSameValueFrames = %d"),
			// GamepadLeftX, GamepadLeftY, GetGamepadLeftDirectionDegrees(), LeftThumbstickSameValueFrames);
	}
	else
	{
		LeftThumbstickSameValueFrames = 0;
	}

	if (bFlyCheatOn)
		SoActivity->SuperModeTick(DeltaSeconds);

	// Update activity
	SoActivity->Tick(DeltaSeconds);

	// update camera
	SoActivity->UpdateCamera(DeltaSeconds);

	// update SoulKeeper usage
	const FSoSplinePoint& SplineLoc = SoMovement->GetSplineLocation();
	const ASoPlayerSpline* Spline = Cast<ASoPlayerSpline>(SplineLoc.GetSpline());
	if (bCanUseSoulkeeper != (Spline != nullptr &&
							  Spline->IsSoulkeeperUsageAllowed(SplineLoc.GetDistance(), GetActorLocation().Z) &&
							  Cast<ASoCarryable>(GetMovementBaseActor(this)) == nullptr))
	{
		bCanUseSoulkeeper = !bCanUseSoulkeeper;
		CanUseSoulkeeperChanged.Broadcast();
	}

	// update smoke vfx
	static const FName SmokeParamName = FName("SpawnRate");
	static const FName SmokeColorParamName = FName("SmokeColor");
	static const FName GlowColorParamName = FName("GlowTint");
	static const FVector2D InRange = FVector2D(0.0f, 750.0f);
	static const FVector2D OutRange = FVector2D(5.0f, 100.0f);
	const float VelocitySize = SoMovement->Velocity.Size();
	SoNewSmoke->SetFloatParameter(SmokeParamName, FMath::GetMappedRangeValueClamped(InRange, OutRange, VelocitySize));

	SoNewSmoke->SetVectorParameter(SmokeColorParamName, CurrentSmokeColor);
	SoGlowSphere->SetVectorParameterValueOnMaterials(GlowColorParamName, CurrentGlowSphereColor);


	// update bounce params


	// update super
	Super::Tick(DeltaSeconds);

	// update mesh fade
	UpdateMeshFade(DeltaSeconds);

	// update strike trail fade
	for (int32 i = WeaponTrailMeshFadeList.Num() - 1; i >= 0; i--)
	{
		WeaponTrailMeshFadeList[i].Counter -= DeltaSeconds;
		const float Percent = 1.0f - WeaponTrailMeshFadeList[i].Counter / WeaponTrailMeshFadeList[i].MaxValue;
		const float ParamValue = TrailCurve != nullptr ? TrailCurve->GetFloatValue(Percent) : Percent;
		static const FName SlideValueName = FName("Slide");
		WeaponTrailMeshFadeList[i].Mesh->GetStaticMeshComponent()->SetScalarParameterValueOnMaterials(SlideValueName, ParamValue);

		if (WeaponTrailMeshFadeList[i].Counter <= 0.0f)
		{
			WeaponTrailMeshFadeList[i].Mesh->GetStaticMeshComponent()->SetVisibility(false);
			WeaponTrailMeshFadeList[i].Mesh->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			WeaponTrailMeshes.Add(WeaponTrailMeshFadeList[i].Mesh);
			WeaponTrailMeshFadeList.RemoveAtSwap(i);
		}
	}

	// update strike Cooldown
	const bool bOnGround = SoMovement->IsMovingOnGround();
	for (int32 i = Cooldowns.Num() - 1; i >= 0; i--)
		if (bOnGround || Cooldowns[i].bAllowInAir)
		{
			Cooldowns[i].bAllowInAir = true;
			Cooldowns[i].Counter -= DeltaSeconds * SoPlayerCharacterSheet->GetCooldownCounterMultiplier();
			if (Cooldowns[i].Counter <= 0.0f)
			{
				CooldownEnded.Broadcast(i, -1.0f, Cooldowns[i].Object);
				Cooldowns.RemoveAt(i); // not swap cause order matters for UI!
			}
		}

	// update cam thingy
	if (bResetSavedCamMovModifierAfterTick)
	{
		bResetSavedCamMovModifierAfterTick = false;
		SavedCamMovModifier = 0;
	}

	//
	// update orientations:
	//
	const float YawCenter = SideViewCameraComponent->GetComponentRotation().Yaw;

	{
		const FRotator OriginalRotation = SoGlowSphere->GetComponentRotation();
		SoGlowSphere->SetWorldRotation(FRotator(OriginalRotation.Pitch, YawCenter + 90.0f, OriginalRotation.Roll));
	}


	// update soulkeeper orientation
	if (ResRuneVisualActor != nullptr && ResRuneVisualActor->IsActive())
	{
		const FRotator OriginalRotation = ResRuneVisualActor->GetActorRotation();
		ResRuneVisualActor->SetActorRotation(FRotator(OriginalRotation.Pitch, USoMathHelper::ClampYaw(YawCenter, 15.0f, OriginalRotation.Yaw), OriginalRotation.Roll));
	}

	OrientTowardsCameraList.RemoveAllSwap([](const USceneComponent* Ptr) { return Ptr == nullptr; });
	for (auto* ComponentToRotate : OrientTowardsCameraList)
	{
		const FRotator OriginalRotation = ComponentToRotate->GetComponentRotation();
		ComponentToRotate->SetWorldRotation(FRotator(OriginalRotation.Pitch, USoMathHelper::ClampYaw(YawCenter, 15.0f, OriginalRotation.Yaw), OriginalRotation.Roll));
	}

	if (EnemyHPWidgetList.Num() > 0.0f)
	{
		EnemyHPWidgetList.RemoveAllSwap([](const USceneComponent* Ptr) { return Ptr == nullptr; });
		const FRotator Rotation = (-SideViewCameraComponent->GetForwardVector()).Rotation();
		for (auto* ComponentToRotate : EnemyHPWidgetList)
			ComponentToRotate->SetWorldRotation(Rotation);
	}

	// update shadow post process
	SoActivity->UpdateCharMaterials(DeltaSeconds);

	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	if (Velocity.SizeSquared2D() > 3.0f)
		LastVelocityDirection = Velocity.GetSafeNormal();

	bLandedRecently = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called to bind functionality to input
void ASoCharacter::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	// NOTE if you modify any of the names here also modify the name in the input settings
	InInputComponent->BindAxis("MouseX");
	InInputComponent->BindAxis("MouseY");

	InInputComponent->BindAxis(FSoInputAxisName::GamepadRightX, this, &Self::MoveGamepadRightX);
	InInputComponent->BindAxis(FSoInputAxisName::GamepadRightY, this, &Self::MoveGamepadRightY);
	InInputComponent->BindAxis(FSoInputAxisName::GamepadLeftX, this, &Self::MoveGamepadLeftX);
	InInputComponent->BindAxis(FSoInputAxisName::GamepadLeftY, this, &Self::MoveGamepadLeftY);

	InInputComponent->BindAxis(FSoInputAxisName::Move, this, &Self::Move);
	InInputComponent->BindAxis(FSoInputAxisName::MoveY);

	// Track what was pressed, released
	TrackInputPressReleased(InInputComponent, FSoInputActionName::ToggleSpell, false);

	InInputComponent->BindAction(FSoInputActionName::Strike0, IE_Pressed, this, &Self::Strike0Pressed);
	InInputComponent->BindAction(FSoInputActionName::Strike1, IE_Pressed, this, &Self::Strike1Pressed);

	InInputComponent->BindAction(FSoInputActionName::Roll, IE_Pressed, this, &Self::RollPressed);
	InInputComponent->BindAction(FSoInputActionName::Roll, IE_Released, this, &Self::RollReleased);

	InInputComponent->BindAction(FSoInputActionName::LockFaceDirection, IE_Pressed, this, &Self::LockFaceDirectionPressed);
	InInputComponent->BindAction(FSoInputActionName::LockFaceDirection, IE_Released, this, &Self::LockFaceDirectionReleased);

	InInputComponent->BindAction(FSoInputActionName::Umbrella, IE_Pressed, this, &Self::UmbrellaPressed);
	InInputComponent->BindAction(FSoInputActionName::Umbrella, IE_Released, this, &Self::UmbrellaReleased);

	InInputComponent->BindAction(FSoInputActionName::TakeWeaponAway, IE_Pressed, this, &Self::TakeWeaponAway);

	InInputComponent->BindAction(FSoInputActionName::Jump, IE_Pressed, this, &Self::JumpPressed);

	InInputComponent->BindAction(FSoInputActionName::Interact0, IE_Pressed, this, &Self::Interact0);
	InInputComponent->BindAction(FSoInputActionName::Interact1, IE_Pressed, this, &Self::Interact1);

	InInputComponent->BindAction("LMB", IE_Pressed, this, &Self::LeftClickPressed);
	InInputComponent->BindAction("LMB", IE_Released, this, &Self::LeftClickReleased);

	InInputComponent->BindAction("RMB", IE_Pressed, this, &Self::RightClickPressed);
	InInputComponent->BindAction("RMB", IE_Released, this, &Self::RightClickReleased);

	InInputComponent->BindAction(FSoInputActionName::ToggleWeapon, IE_Pressed, this, &Self::ToggleWeapons);
	InInputComponent->BindAction(FSoInputActionName::ToggleItem, IE_Pressed, this, &Self::ToggleItems);
	InInputComponent->BindAction<FSoDelegateBoolOneParam>(FSoInputActionName::ToggleSpell, IE_Pressed, this, &Self::ToggleSpells, false);

	InInputComponent->BindAction(FSoInputActionName::UseItemFromSlot0, IE_Pressed, this, &Self::UseItemFromSlot0);

	InInputComponent->BindAction(FSoInputActionName::QuickSaveLoad0, IE_Pressed, this, &Self::QuickSaveLoad0);
	InInputComponent->BindAction(FSoInputActionName::QuickSaveLoad1, IE_Pressed, this, &Self::QuickSaveLoad1);

	InInputComponent->BindAction(FSoInputActionName::CharacterPanels, IE_Pressed, this, &Self::ToggleCharacterPanels).bExecuteWhenPaused = true;

	// Video DEMO
#if WARRIORB_WITH_VIDEO_DEMO
	InInputComponent->BindAction(FSoInputActionName::StartVideoLoopPlayback, IE_Pressed, this, &Self::StartVideoLoopPlayback).bExecuteWhenPaused = true;
	InInputComponent->BindAction(FSoInputActionName::StartVideoLoopPlaybackGamepad, IE_Pressed, this, &Self::StartVideoLoopPlaybackGamepad).bExecuteWhenPaused = true;
	InInputComponent->BindAction(FSoInputActionName::StopVideoLoopPlayback, IE_Pressed, this, &Self::StopVideoLoopPlayback).bExecuteWhenPaused = true;
	InInputComponent->BindAction(FSoInputActionName::RestartDemo, IE_Pressed, this, &Self::RestartDemo).bExecuteWhenPaused = true;
#endif // WARRIORB_WITH_VIDEO_DEMO

	// Register UI commands
	const int32 UICmdNum = static_cast<int32>(ESoUICommand::EUC_ReleasedMax);
	const int32 UICmdPressedNum = static_cast<int32>(ESoUICommand::EUC_PressedMax);
	const UEnum* EnumPtr = FSoInputActionName::GetUICommandEnum();
	if (EnumPtr != nullptr)
	{
		for (int32 Index = 0; Index < UICmdNum; ++Index)
		{
			// Ignore Num command EUC_PressedMax
			if (Index == UICmdPressedNum)
				continue;

			// Remove EUC_ or EUC_R
			const ESoUICommand Command = static_cast<ESoUICommand>(Index);
			const bool bPressed = Index < UICmdPressedNum;
			const FName EnumName = FName(*EnumPtr->GetNameStringByIndex(Index).Mid(bPressed ? 4 : 5));

			// NOTE: order matters here
			// Track base press event
			{
				FInputActionBinding UIEvent_Pressed(EnumName, IE_Pressed);
				UIEvent_Pressed.bExecuteWhenPaused = true;
				UIEvent_Pressed.ActionDelegate.GetDelegateWithKeyForManualSet().BindUObject(this, &Self::UIInputPressed, Command);
				InInputComponent->AddActionBinding(UIEvent_Pressed);
			}

			// Track base released event
			{
				FInputActionBinding UIEvent_Released(EnumName, IE_Released);
				UIEvent_Released.bExecuteWhenPaused = true;
				UIEvent_Released.ActionDelegate.GetDelegateWithKeyForManualSet().BindUObject(this, &Self::UIInputReleased, Command);
				InInputComponent->AddActionBinding(UIEvent_Released);
			}

			// Add the event that the user experiences
			{
				FInputActionBinding UIEventActionBinding(EnumName, bPressed ? IE_Pressed : IE_Released);
				UIEventActionBinding.bExecuteWhenPaused = true;
				UIEventActionBinding.ActionDelegate.GetDelegateWithKeyForManualSet().BindUObject(this, &Self::HandleUICommand, Command);
				InInputComponent->AddActionBinding(UIEventActionBinding);
			}
		}
	}

#if WITH_EDITOR
	InInputComponent->BindAction(FSoInputActionName::EditorCameraEditMode, IE_Pressed, this, &Self::SwitchCameraEditMode);
	InInputComponent->BindAction(FSoInputActionName::EditorSkyEditMode, IE_Pressed, this, &Self::SwitchSkyEditMode);
	InInputComponent->BindAction(FSoInputActionName::EditorCharShadowEditMode, IE_Pressed, this, &Self::SwitchCharShadowEditMode);

	InInputComponent->BindAction(FSoInputActionName::EditorCreateKey, IE_Pressed, this, &Self::CreateKey);
	InInputComponent->BindAction(FSoInputActionName::EditorReloadKeys, IE_Pressed, this, &Self::LoadEditedData);
	InInputComponent->BindAction(FSoInputActionName::EditorSaveKeys, IE_Pressed, this, &Self::SaveEditedData);
	InInputComponent->BindAction(FSoInputActionName::EditorDeleteKey, IE_Pressed, this, &Self::DeleteActiveKeyNode);
	InInputComponent->BindAction(FSoInputActionName::EditorCopyFromKey, IE_Pressed, this, &Self::CopyActiveKeyData);
	InInputComponent->BindAction(FSoInputActionName::EditorPasteToKey, IE_Pressed, this, &Self::PasteToActiveKeyData);
	InInputComponent->BindAction(FSoInputActionName::EditorMoveClosestKeyNodeHere, IE_Pressed, this, &Self::MoveClosestKeyHere);
	InInputComponent->BindAction(FSoInputActionName::EditorJumpToNextKey, IE_Pressed, this, &Self::JumpToNextKey);
	InInputComponent->BindAction(FSoInputActionName::EditorJumpToPrevKey, IE_Pressed, this, &Self::JumpToPrevKey);
	InInputComponent->BindAction(FSoInputActionName::EditorSpecialEditButtonPressed0, IE_Pressed, this, &Self::SpecialEditButtonPressed0);
	InInputComponent->BindAction(FSoInputActionName::EditorSpecialEditButtonPressed1, IE_Pressed, this, &Self::SpecialEditButtonPressed1);
	InInputComponent->BindAction(FSoInputActionName::EditorCtrl, IE_Pressed, this, &Self::CtrlPressed);
	InInputComponent->BindAction(FSoInputActionName::EditorCtrl, IE_Released, this, &Self::CtrlReleased);
	InInputComponent->BindAction(FSoInputActionName::EditorMiddleMouseButton, IE_Pressed, this, &Self::MiddleMouseBtnPressed);
	InInputComponent->BindAction(FSoInputActionName::EditorMiddleMouseButton, IE_Released, this, &Self::MiddleMouseBtnReleased);
#endif
	InInputComponent->BindAction(FSoInputActionName::EditorSuperEditMode, IE_Pressed, this, &Self::SuperEditModePressed);
	InInputComponent->BindAction(FSoInputActionName::EditorDebugFeature, IE_Pressed, this, &Self::DebugFeature).bExecuteWhenPaused = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::TrackInputPressReleased(UInputComponent* InInputComponent, FName ActionName, bool bTrackWhenPaused)
{
	// Track base press event
	{
		FInputActionBinding Event_Pressed(ActionName, IE_Pressed);
		Event_Pressed.bExecuteWhenPaused = bTrackWhenPaused;
		Event_Pressed.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &Self::InputPressed, ActionName);
		InInputComponent->AddActionBinding(Event_Pressed);
	}

	// Track base released event
	{
		FInputActionBinding Event_Released(ActionName, IE_Released);
		Event_Released.bExecuteWhenPaused = bTrackWhenPaused;
		Event_Released.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &Self::InputReleased, ActionName);
		InInputComponent->AddActionBinding(Event_Released);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::InputPressed(FName ActionName)
{
	InputPressedActionNames.Add(ActionName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::InputReleased(FName ActionName)
{
	InputPressedActionNames.Remove(ActionName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::UIInputPressed(FKey Key, ESoUICommand Command)
{
	UIInputPressedCommands.Add(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::UIInputReleased(FKey Key, ESoUICommand Command)
{
	UIInputPressedCommands.Remove(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType)
{
	DeviceType = InDeviceType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::Move(float Value)
{
	if (bUseMovementOverride)
		Value = MovementOverrideValue;

	if (FMath::IsNearlyZero(Value, KINDA_SMALL_NUMBER))
		SavedCamMovModifier = 0;

	SoActivity->Move(Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::LockFaceDirectionPressed()
{
	bLockForwardVecPressed = true;
	SoMovement->bOrientRotationToMovement = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::LockFaceDirectionReleased()
{
	bLockForwardVecPressed = false;
	SoMovement->bOrientRotationToMovement = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OverrideFloatingVelocity(bool bOverride, float OverrideValue)
{
	FloatingVelocity = OverrideValue;
	if (bFloatingActive)
	{
		SoMovement->SetFallVelocityOverride(bOverride, OverrideValue);
		if (!bOverride)
		{
			UmbrellaReleased();
			SoActivity->StopFloating();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::SetPositionOnSplineSP(const FSoSplinePoint& InSplinePoint,
										 float ZValue,
										 bool bCollisionChecksAndPutToGround,
										 bool bStoreAsCheckpoint,
										 bool bLookBackwards)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SetPositionOnSplineSP"), STAT_SetPositionOnSplineSP, STATGROUP_SoCharacter);

	if (!InSplinePoint.IsValid())
	{
		UE_LOG(LogSoChar, Error, TEXT("SetPositionOnSplineSP() called with invalid InSplinePoint"));
		return false;
	}

	const FSoSplinePoint OldSplineLocation = SoMovement->GetSplineLocation();

	// First try is the input point
	FSoSplinePoint SpawnSplineLoc = InSplinePoint;

	// Find save location along Z axis
	bool bFound = true;
	FVector Location = bCollisionChecksAndPutToGround ? CalcSafeSpawnLocationAlongZ(SpawnSplineLoc, ZValue, bFound)
													  : SpawnSplineLoc.GetWorldLocation(ZValue);
	if (!bFound)
	{
		const FVector SavedFirstLocation = Location;
		const float SplineDistanceDelta = GetCapsuleComponent()->GetScaledCapsuleRadius() * 2.0f;

		// Try to the right of the spline point
		SpawnSplineLoc += SplineDistanceDelta;
		Location = CalcSafeSpawnLocationAlongZ(SpawnSplineLoc, ZValue, bFound);
		if (!bFound)
		{
			// Try to the left
			SpawnSplineLoc = InSplinePoint - SplineDistanceDelta;
			Location = CalcSafeSpawnLocationAlongZ(SpawnSplineLoc, ZValue, bFound);
			if (!bFound)
			{
				// no reason to move anywhere if nothing helps :(
				Location = SavedFirstLocation;
				SpawnSplineLoc = InSplinePoint;
			}
		}
	}

	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	SpawnSplineLoc.SetReferenceActor(this);
	SoMovement->SetSplineLocation(SpawnSplineLoc);

	if (OldSplineLocation.GetSpline() != SpawnSplineLoc.GetSpline())
		OnSplineChanged(OldSplineLocation, SpawnSplineLoc);

	if (bStoreAsCheckpoint)
	{
		ActiveCheckPointLocation = SpawnSplineLoc;
		ActiveCheckpointZLocation = ZValue;
	}

	const FVector FaceDir = SpawnSplineLoc.GetDirection() * ((bLookBackwards) ? -1.0f : 1.0f);
	SetActorRotation(FaceDir.Rotation());
	StoredForwardVector = FaceDir;
	LastInputDirection = StoredForwardVector;
	SavedCamMovModifier = 0;
	bResetSavedCamMovModifierAfterTick = true;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoCharacter::CalcSafeSpawnLocationAlongZ(const FSoSplinePoint& SplinePoint, float ZValue, bool& bOutSafeFound)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("CalcSafeSpawnLocationAlongZ"), STAT_CalcSafeSpawnLocationAlongZ, STATGROUP_SoCharacter);

	bOutSafeFound = true;

	const float ScaledCapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const FVector Start = SplinePoint.ToVector(ZValue);
	const FVector ToEnd = -FVector(0.f, 0.f, 500);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	static const FName PawnName = FName("Pawn");

	auto DoSweep = [this, &ToEnd, &TraceParams](const FVector& StartLocation, FHitResult& HitResult) -> bool
	{
		return GetWorld()->SweepSingleByProfile(HitResult,
												StartLocation,
												StartLocation + ToEnd,
												GetCapsuleComponent()->GetComponentQuat(),
												PawnName,
												GetCapsuleComponent()->GetCollisionShape(),
												TraceParams);
	};

	FHitResult HitData(ForceInit);
	if (DoSweep(Start, HitData))
	{
		if (HitData.bStartPenetrating)
		{
			const FVector Z[2] = { FVector(0.0f, 0.0f, 1.0f), FVector(0.0f, 0.0f, -1.0f) };

			for (int32 i = 0; i < 2; ++i)
			{
				const bool bUsePenetrationDepth = (HitData.PenetrationDepth > KINDA_SMALL_NUMBER) && ((HitData.Normal - Z[i]).SizeSquared() < KINDA_SMALL_NUMBER);
				// ScaledCapsuleHalfHeight * 2 is the greatest we can normally try, because this way the new capsule touches the original one
				// -> we did not teleport through ceiling / floor
				// TODO: improve this?!
				const float MoveDistance = bUsePenetrationDepth ? HitData.PenetrationDepth + ScaledCapsuleHalfHeight : ScaledCapsuleHalfHeight * 2;
				const FVector FixedStart = Start + Z[i] * MoveDistance;
				FHitResult HitDataSecondary(ForceInit);
				if (DoSweep(FixedStart, HitDataSecondary))
				{
					if (!HitDataSecondary.bStartPenetrating)
						return HitDataSecondary.TraceStart + (HitDataSecondary.TraceEnd - HitDataSecondary.TraceStart) * HitDataSecondary.Time;
				}
				else
					return FixedStart;
			}

			bOutSafeFound = false;
		}
		else
			return HitData.TraceStart + (HitData.TraceEnd - HitData.TraceStart) * HitData.Time;
	}

	return Start;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::IsFacingVelocity() const
{
	return (GetActorForwardVector().GetSafeNormal2D() | GetVelocity().GetSafeNormal2D()) > 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::DecreaseCollisionSize()
{
	if (bCollisionAlreadyDecreased) // it is a valid case now
		return;

	bCollisionAlreadyDecreased = true;
	const FVector MeshOffset = FVector(0, 0, NormalHeight - DecreasedHeight);
	GetCapsuleComponent()->SetCapsuleHalfHeight(DecreasedHeight);
	GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, Mass);
	GetMesh()->AddLocalTransform(FTransform(MeshOffset));
	SoNewLights->AddLocalTransform(FTransform(MeshOffset));

	CapsuleBottomVFX->SetRelativeLocation(FVector(0.0f, 0.0f, -DecreasedHeight));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::CanIncreaseCollision() const
{
	if (!bCollisionAlreadyDecreased)
		return false;

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	const float FullHeight = CapsuleComp->GetScaledCapsuleHalfHeight() * 2;

	const FVector MeshOffset = FVector(0, 0, FullHeight);
	FHitResult Hit;

	const FTransform RelativeTransform = CapsuleComp->GetRelativeTransform();
	CapsuleComp->AddLocalTransform(FTransform(MeshOffset), true, &Hit);			// move up
	CapsuleComp->SetRelativeTransform(RelativeTransform, true);					// restore

	if (Hit.bBlockingHit)
	{
		const ECollisionChannel ColChannel = Hit.Component->GetCollisionObjectType();
		if (ColChannel == ECollisionChannel::ECC_Destructible)
			return true;
	}

	// we may have died during this process :/
	if (SoActivity == SoADead || SoActivity == SoAHitReact)
		return false;

	return !(Hit.bBlockingHit);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OnBounce(bool bWallJump, float NewStoredRollValue, const FVector& HitPoint, const FVector& HitNormal)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnBounce"), STAT_OnBounce, STATGROUP_SoCharacter);

	SoActivity->OnBounce(bWallJump, NewStoredRollValue, HitPoint, HitNormal);
	OnRollHitBP(HitPoint, HitNormal, true);

	// Achievement for WallJump, in Prologue it's instant achievement, in main game you have to do that and heighest bounce before tut 3 to get the achievement
	if (USoPlatformHelper::IsDemo())
	{
		static const FName WallJumperAchievementName = FName("A_WallJumper");
		if (bWallJump && !SoPlayerCharacterSheet->GetBoolValue(WallJumperAchievementName))
		{
			USoAchievementManager::Get(this).UnlockAchievement(this, WallJumperAchievementName);
		}
	}
	else
	{
		// Normal Game
		if (bWallJump && !SoPlayerCharacterSheet->GetBoolValue(WallJumpDoneName))
		{
			SoPlayerCharacterSheet->SetBoolValue(WallJumpDoneName, true);
			CheckSelfTrainedAchievement();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::CanPerformWallJump(const FHitResult& HitResult)
{
	AActor* Actor = HitResult.GetActor();
	if (Actor != nullptr && Actor->ActorHasTag(USoActivity::NoWallJumpSurface))
		return false;

	return DidPlayerJump();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::Landed(const FHitResult& Hit)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Landed"), STAT_Landed, STATGROUP_SoCharacter);

	Super::Landed(Hit);
	// it is needed cause of the slippery surfaces
	BaseChange();

	SoActivity->OnLanded();

	bForceGroundAnims = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	OnInteractionLabelChanged.Broadcast();
	OnMovementModeChangedNotify.Broadcast();

	if (PrevMovementMode == EMovementMode::MOVE_Walking && SoMovement->MovementMode == EMovementMode::MOVE_Falling)
	{
		LastSwitchToAirTime = GetWorld()->GetTimeSeconds();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::CanJumpInternal_Implementation() const
{
	return (SoMovement && SoMovement->IsJumpAllowed() && SoMovement->IsMovingOnGround());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::BaseChange()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("BaseChange"), STAT_BaseChange, STATGROUP_SoCharacter);

	Super::BaseChange();
	AActor* ActualMovementBase = GetMovementBaseActor(this);
	if (ActualMovementBase)
	{
		SoActivity->OnBaseChanged(ActualMovementBase);
		// UE_LOG(LogSoChar, Error, TEXT("New movement base: %s"), *ActualMovementBase->GetName());
	}
	// else
	// 	UE_LOG(LogSoChar, Error, TEXT("Movement base cleared"));



	// TEMP disabled, maybe we won't need this?!
	//if (ActualMovementBase != nullptr)
	//	SoMovement->SetSlowDownToMaxWalkSpeedInFall(!ActualMovementBase->IsRootComponentStatic());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::JumpPressedRecentlyTimeOver()
{
	bJumpPressedRecently = false;
	bBanJumpCauseSpamming = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::IsAlive_Implementation() const
{
	return SoActivity != SoADead;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::Kill_Implementation(bool bPhysical)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Kill_Implementation"), STAT_Kill_Implementation, STATGROUP_SoCharacter);

	FSoDmg Dmg;
	if (bPhysical)
		Dmg.Physical = (SoPlayerCharacterSheet->GetMaxHealth() + 1.0f) / (1.0f - SoPlayerCharacterSheet->GetPhysicalResistance());
	else
		Dmg.Magical = (SoPlayerCharacterSheet->GetMaxHealth() + 1.0f) / (1.0f - SoPlayerCharacterSheet->GetMagicResistance());

	SoActivity->OnDmgTaken({ Dmg }, {});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::CauseDmg_Implementation(const FSoDmg& Dmg, const FSoHitReactDesc& HitReactDesc)
{
	SoActivity->OnDmgTaken(Dmg, HitReactDesc);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::MeleeHit_Implementation(const FSoMeleeHitParam& HitParam)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("MeleeHit_Implementation"), STAT_MeleeHit_Implementation, STATGROUP_SoCharacter);

	if (DamageBlockCounter > 0.0f)
		return true;

	return SoActivity->OnMeleeHit(HitParam);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::SubscribeOnWalkThroughMortal_Implementation(const FSoNotifyActorSingle& OnWalkThrough, bool bSubscribe)
{
	if (bSubscribe)
		OnWalkThroughMortal.Add(OnWalkThrough);
	else
		OnWalkThroughMortal.Remove(OnWalkThrough);

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::AutoSave(float MinTimeSinceLastSave)
{
#if PLATFORM_SWITCH
	MinTimeSinceLastSave = FMath::Max(MinTimeSinceLastSave, 120.0f);
#endif
	// Save game every 5 minutes
	const float ThresholdSeconds = LastAutoSaveTime + MinTimeSinceLastSave;
	if (GetWorld()->GetTimeSeconds() > ThresholdSeconds)
	{
		LastAutoSaveTime = GetWorld()->GetTimeSeconds();
		if (GameInstance)
			GameInstance->SaveGameForCurrentState();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::ModifySpellsCapacity(int32 ModifyAmount)
{
	IDlgDialogueParticipant::Execute_ModifyIntValue(this, SpellcasterCapacityName, true, ModifyAmount);
	if (GetSpellsCapacity() < 0)
		SetSpellsCapacity(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SetSpellsCapacity(int32 NewCapacity)
{
	IDlgDialogueParticipant::Execute_ModifyIntValue(this, SpellcasterCapacityName, false, FMath::Abs(NewCapacity));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 ASoCharacter::GetSpellsCapacity() const
{
	return IDlgDialogueParticipant::Execute_GetIntValue(this, SpellcasterCapacityName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::EnsureSpellsCapacityLimits(int32 SpellsCapacity/*= 2*/)
{
	if (GetSpellsCapacity() < SpellsCapacity)
		SetSpellsCapacity(SpellsCapacity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::AddInteractable(AActor* Interactable)
{
	if (Interactable == nullptr || !Interactable->GetClass()->ImplementsInterface(USoInteractable::StaticClass()))
	{
		UE_LOG(LogSoChar, Warning, TEXT("ASoCharacter::AddInteractable called, but the input actor does not implement the ISoInteractable interface!"));
		return;
	}

	if (Interactables.Num() > 0 && ISoInteractable::Execute_IsExclusive(Interactable) && !ISoInteractable::Execute_IsExclusive(Interactables[0]))
		Interactables.Insert(Interactable, 0);
	else
		Interactables.AddUnique(Interactable);

	if (Interactables.Num() == 3)
		ActiveInteractable = 0;

	// if there is 2, one for R and one for T T should be always above
	if (Interactables.Num() == 2)
	{
		const bool b0PrefersSecond = ISoInteractable::Execute_IsSecondKeyPrefered(Interactables[0]);
		const bool b1PrefersSecond = ISoInteractable::Execute_IsSecondKeyPrefered(Interactables[1]);
		if (b0PrefersSecond && !b1PrefersSecond)
			Interactables.Swap(0, 1);
	}

	OnInteractionLabelChanged.Broadcast();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RemoveInteractable(AActor* Interactable)
{
	const bool bWasExclusive = ISoInteractable::Execute_IsExclusive(Interactable);
	Interactables.Remove(Interactable);

	if (bWasExclusive)
	{
		for (int32 i = 1; i < Interactables.Num(); ++i)
			if (ISoInteractable::Execute_IsExclusive(Interactables[i]))
			{
				Interactables.Swap(0, i);
				break;
			}
	}

	// if there is 2, one for R and one for T T should be always above
	if (Interactables.Num() == 2)
	{
		const bool b0PrefersSecond = ISoInteractable::Execute_IsSecondKeyPrefered(Interactables[0]);
		const bool b1PrefersSecond = ISoInteractable::Execute_IsSecondKeyPrefered(Interactables[1]);
		if (b0PrefersSecond && !b1PrefersSecond)
			Interactables.Swap(0, 1);
	}

	SwitchActiveInteractable();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RemoveAllInteractable()
{
	Interactables.Empty();
	SwitchActiveInteractable();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SwitchActiveInteractable()
{
	ActiveInteractable += 1;
	if (ActiveInteractable >= Interactables.Num())
		ActiveInteractable = 0;

	OnInteractionLabelChanged.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AActor* ASoCharacter::GetInteractionTipRelevantData(int32 Index, UPARAM(Ref)bool& bVisible, UPARAM(Ref)bool& bPrimaryKey)
{
	switch (Index)
	{
		case 0:
		{
			if (Interactables.Num() < 1)
			{
				bVisible = false;
				return nullptr;
			}

			const int32 AssociatedInteractableIndex = Interactables.Num() > 2 ? ActiveInteractable : 0;
			bVisible = true;
			bPrimaryKey = (Interactables.Num() > 2) ? true : !ISoInteractable::Execute_IsSecondKeyPrefered(Interactables[AssociatedInteractableIndex]);
			return Interactables[AssociatedInteractableIndex];
		}

		case 1:
			if (Interactables.Num() < 2 || ISoInteractable::Execute_IsExclusive(Interactables[0]))
			{
				bVisible = false;
				return nullptr;
			}
			bVisible = true;
			bPrimaryKey = (Interactables.Num() > 2) ? false : ISoInteractable::Execute_IsSecondKeyPrefered(Interactables[0]);
			return (Interactables.Num() == 2) ? Interactables[1] : nullptr;

		default:
			bVisible = false;
			return nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::StartCarryStuff(ASoCarryable* Stuff)
{
	if (Stuff != nullptr)
	{
		CarriedStuff = Stuff;
		SoActivity->SwitchActivity(SoACarry);
	}
	else
		UE_LOG(LogSoChar, Error, TEXT("ASoCharacter::StartCarryStuff called with nullptr!"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::StartLeverPush(const struct FSoLeverData& LeverData, const FSoSplinePoint& SplinePoint, float ZValue)
{
	SoActivity->StartLeverPush(LeverData, SplinePoint, ZValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::DebugFeature()
{
	// overridden in cam edit!!!
	SoActivity->DebugFeature();

	//if (USoGameSettings* GameSettings = USoGameSettings::GetInstance())
		//GameSettings->AutosetGamepadUIFromConnectedGamepad();

	// USoGameInstance::GetInstance(this)->TeleportToChapter(ESoChapterType::CT_Act2);
	// SoAInUI->StartSpellCastSwitch();
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%f"), GetGamepadRightDirectionDegress()));
	// GetPlayerController()->ConsoleCommand("HighResShot 1920x1080");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::QuickSaveLoad(int32 Index)
{
	if (!bEnableQuickSaveLoad)
		return;

	if (GameInstance)
	{
		const bool bEpisode = GameInstance->IsEpisode();

		const auto& WorldState = FSoWorldState::Get();
		if (bCtrlPressed)
		{
			if (bEpisode)
			{
				if (GameInstance->LoadGameForCurrentEpisode(false))
					UE_LOG(LogSoChar, Warning, TEXT("Episode reloaded!"));
			}
			else
			{
				if (GameInstance->LoadGameForChapter(Index, false))
					UE_LOG(LogSoChar, Warning, TEXT("Loaded from %s!"), *WorldState.GetSaveFilePath());
			}
		}
		else
		{
			if (bEpisode)
			{
				if (GameInstance->SaveGameForEpisode(WorldState.GetEpisodeName()))
					UE_LOG(LogSoChar, Warning, TEXT("Episode saved!"));
			}
			else
			{
				if (GameInstance->SaveGameForChapter(Index))
					UE_LOG(LogSoChar, Warning, TEXT("Saved to %s - old file is overriden!"), *WorldState.GetSaveFilePath());
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::SpawnResRune()
{
	FSoSplinePoint SplineLocation = SoMovement->GetSplineLocation();
	ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());

	if (!bCanUseSoulkeeper)
		return false;

	ResLocation = SplineLocation;
	ResLocationZValue = GetActorLocation().Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	if (ResRuneVisualActor != nullptr && PlayerSpline != nullptr)
	{
		const FVector CharLocation = GetActorLocation();
		const FVector CamLocation = SideViewCameraComponent->GetComponentLocation();
		FVector DirVector = (CharLocation - CamLocation) * (PlayerSpline->ShouldSpawnSKBetweenCharacterAndCamera() ? -1.0f : 1.0f);
		DirVector.Z = 0;
		DirVector.Normalize();

		const FVector TotemLocation = CharLocation + DirVector * 100;
		ResRuneVisualActor->SetActorLocation(TotemLocation);
		ResRuneVisualActor->SetActorRotation((CharLocation - TotemLocation).Rotation());
		ResRuneVisualActor->SetActorHiddenInGame(false);
		ResRuneVisualActor->Activate();

		OnSoulkeeperPlaced.Broadcast();
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::Revive(bool bSoulKeeper, bool bCanUseSplineOverrideLocation, bool bWantToLeaveSpline)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Revive"), STAT_Revive, STATGROUP_SoCharacter);

	if (SoActivity->BlocksTeleportRequest())
		return;

	SoActivity->OnTeleportRequest();

	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SoMovement->GetSplineLocation().GetSpline());
	const bool bSoulKeeperNotUsed = !bSoulKeeper ||
									ResRuneVisualActor == nullptr ||
									!ResRuneVisualActor->IsActive() ||
									(bCanUseSplineOverrideLocation && (PlayerSpline != nullptr && PlayerSpline->GetResPointOverride(bWantToLeaveSpline) != nullptr));

	if (bSoulKeeperNotUsed)
		PickupResRune();

	bLastRespawnWasSK = !bSoulKeeperNotUsed;
	bLastRespawnWasRetreat = bWantToLeaveSpline;

	// has to be called first so the collision of the dead parts are disabled before respawn
	OnRespawn(!bSoulKeeperNotUsed);
	OnPlayerRematerialized.Broadcast();
	OnPlayerRespawn.Broadcast();

	bool bTeleported = false;
	if (PlayerSpline != nullptr)
	{
		if (bCanUseSplineOverrideLocation)
		{
			if (ASoMarker* Marker = PlayerSpline->GetResPointOverride(bWantToLeaveSpline))
			{
				bTeleported = true;
				SoATeleport->SetupTeleport(Marker->GetSplineLocation(), Marker->GetActorLocation().Z, false, false);
			}
		}
	}

	SoPlayerCharacterSheet->OnRevive(bSoulKeeperNotUsed);
	if (!bTeleported)
	{
		if (bSoulKeeperNotUsed)
			SoATeleport->SetupTeleport(ActiveCheckPointLocation, ActiveCheckpointZLocation, false, false);
		else
			SoATeleport->SetupTeleport(ResLocation, ResLocationZValue, false, false);
	}

	if (MusicOverride == nullptr && PlayerSpline != nullptr)
		USoAudioManager::Get(this).SetMusic(PlayerSpline->GetMusic(), bSoulKeeperNotUsed);

	UpdateMaterialBP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::FireEventsAsIfRespawned()
{
	OnPlayerRematerialized.Broadcast();
	OnPlayerRespawn.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::PickupResRune()
{
	if (ResRuneVisualActor != nullptr && ResRuneVisualActor->IsActive())
	{
		ResRuneVisualActor->Deactivate();
		ResRuneVisualActor->SetActorHiddenInGame(true);
		SoPlayerCharacterSheet->SetSpellUsageCount(SoulKeeperSpell, 1, false);
		OnSoulkeeperPickedUp.Broadcast();
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SetActiveCheckpointLocationName(FName LocationName)
{
	//const bool bIsLoading = GameInstance->IsSavingOrLoading();
	//const bool bAnyLevelLoading = USoLevelHelper::IsAnyLevelLoading(this);
	FSoWorldState::Get().SetCheckpointLocation(LocationName);
}

FName ASoCharacter::GetActiveCheckpointLocationName() const
{
	return FSoWorldState::Get().GetCheckpointLocation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SetCheckpointLocation(const FSoSplinePoint& InSplinePoint, const float ZValue)
{
	ActiveCheckPointLocation = InSplinePoint;
	ActiveCheckpointZLocation = ZValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::TeleportToCheckpointName(FName CheckPointName, bool bPerformTeleportation)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("TeleportToCheckpointName"), STAT_TeleportToCheckpointName, STATGROUP_SoCharacter);

	ASoLocationRegistry* LocationRegistry = ASoLocationRegistry::GetInstance(this);
	if (LocationRegistry == nullptr)
	{
		USoPlatformHelper::PrintErrorToAll(this, TEXT("Failed to update checkpoint location: location registry not found!"));
		return;
	}
	UE_LOG(LogSoChar, Verbose, TEXT("TeleportToCheckpointName = `%s`"), *CheckPointName.ToString());

	if (!LocationRegistry->HasMarker(CheckPointName))
	{
		USoPlatformHelper::PrintErrorToAll(this, FString::Printf(TEXT("ASoLocationRegistry does not have CheckpointName with name = `%s`"), *CheckPointName.ToString()));
	}

	ActiveCheckPointLocation = LocationRegistry->GetSplineLocation(CheckPointName);
	ActiveCheckpointZLocation = LocationRegistry->GetZLocation(CheckPointName);
	LastTeleportedCheckpointName = CheckPointName;
	if (bPerformTeleportation)
	{
		// if this "if statement" ever changes check on SoPlayerCharacterState::OnReload() regarding level starter cutscenes (or just do not change it)
		if (SoMovement != nullptr && SoMovement->GetSplineLocation().GetSpline() != nullptr)
		{
			// Teleport
			SoATeleport->SetupTeleport(ActiveCheckPointLocation,
				ActiveCheckpointZLocation,
				true,
				LocationRegistry->ShouldMarkerLookBackwards(CheckPointName));
		}
		else
		{
			// Instant
			SetPositionOnSplineSP(ActiveCheckPointLocation,
				ActiveCheckpointZLocation,
				false,
				true,
				LocationRegistry->ShouldMarkerLookBackwards(CheckPointName));
		}
	}

	if (SoMovement->GetSplineLocation().GetSpline() == nullptr)
	{
		UE_LOG(LogSoChar, Error, TEXT("TeleportToCheckpointName: Could not find spline from CheckPointName = `%s`"), *CheckPointName.ToString());
	}
	else if (bPerformTeleportation)
	{
		// Claim here because the game might be paused
		bool bAllLevelLoaded;
		FSoLevelManager::Get().ClaimSplineLocation(SoMovement->GetSplineLocation(), bAllLevelLoaded);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::TeleportToActiveCheckpointName()
{
	// Location is saved location name
	TeleportToCheckpointName(GetActiveCheckpointLocationName());

	// Ups, no spline, something is wrong with the CheckPointName
	// Try default location
	if (SoMovement->GetSplineLocation().GetSpline() == nullptr)
	{
		const FString Message = FString::Printf(TEXT("Spline was not found for CheckPointName = `%s`. Retrying default checkpoint"), *GetActiveCheckpointLocationName().ToString());

		// This is an error in everything besides the menu level
		if (!USoLevelHelper::IsInMenuLevel(this))
			USoPlatformHelper::PrintErrorToAll(this, Message);

		TeleportToCheckpointName(ASoLocationRegistry::GetDefaultCheckpointLocationName());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SufferOverlapDamage(AActor* SourceActor, const FSoDmg& Damage, const FSoHitReactDesc& HitReactDesc)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SufferOverlapDamage"), STAT_SufferOverlapDamage, STATGROUP_SoCharacter);

	if (DamageBlockCounter >= 0.0f)
		return;

	CauseDmg_Implementation(Damage, HitReactDesc);
	OnOverlapDamageTaken.Broadcast(SourceActor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::PreOverlapDamage(AActor* SourceActor)
{
	if (bBounceAndDamageOverlapSpellUsed && SoActivity == SoARoll && bJumpPressedRecently)
	{
		if (ISoMortal::Execute_IsBounceAble(SourceActor))
		{
			FSoHitReactDesc EnemyHitReactDesc;
			EnemyHitReactDesc.HitReact = ESoHitReactType::EHR_Nothing;
			EnemyHitReactDesc.AssociatedActor = this;
			EnemyHitReactDesc.Irresistibility = 10;
			ISoMortal::Execute_CauseDmg(SourceActor, FSoDmg{ 0.0f, BounceAndDamageOverlapSpellDamage }, EnemyHitReactDesc);
			SoMovement->Velocity.Z = RollJumpLevels.Last(1);
			SoMovement->SetMovementMode(EMovementMode::MOVE_Falling);
			ISoMortal::Execute_DisplayVisualEffect(this, ESoVisualEffect::EVE_ElectrifiedYellow);
			DamageBlockCounter = DamageBlockTime;
			OnBounceSpellUsed.Broadcast(SourceActor);
			USoAudioManager::PlaySoundAtLocation(this, SFXBounceOnEnemy, GetActorTransform(), true);

			return false;
		}

		ASoGameMode::Get(this).DisplayText(GetActorLocation(), ESoDisplayText::EDT_Immune);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OnWalkThroughEnemy(AActor* Enemy)
{
	OnWalkThroughMortal.Broadcast(Enemy);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCharacter::ModifyRollJumpLevel(int32 DeltaLevel)
{
	ActiveRollJumpLevel = FMath::Clamp(ActiveRollJumpLevel + DeltaLevel, 0, RollJumpLevels.Num() - 1);
	return RollJumpLevels[ActiveRollJumpLevel];
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCharacter::SetRollJumpLevel(int32 Level)
{
	ActiveRollJumpLevel = FMath::Clamp(Level, 0, RollJumpLevels.Num() - 1);
	return RollJumpLevels[ActiveRollJumpLevel];
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCharacter::GetActiveRollJumpHeight() const
{
	check(ActiveRollJumpLevel < RollJumpLevels.Num() && ActiveRollJumpLevel >= 0);
	return RollJumpLevels[ActiveRollJumpLevel];
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCharacter::GetRollJumpHeight(int32 JumpLevel) const
{
	check(JumpLevel < RollJumpLevels.Num() && JumpLevel >= 0);
	return RollJumpLevels[JumpLevel];
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::FreezeCameraZ(bool bFreeze)
{
	bUseSavedCameraBoomZ = bFreeze;
	if (bFreeze && !bSavedCameraBoomZUpdateBlock)
	{
		SavedCameraBoomZ = CameraBoom->GetComponentLocation().Z;
		SavedCameraBoomZSource = SavedCameraBoomZ;
		SavedCameraBoomZTarget = SavedCameraBoomZ;
	}

	CameraUnfreezeCounter = CameraUnfreezeTime;

	bSavedCameraBoomZUpdateBlock = true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::FreezeCameraZToValue(float ZToFreeze)
{
	bUseSavedCameraBoomZ = true;
	SavedCameraBoomZTarget = ZToFreeze;
	SavedCameraBoomZSource = SavedCameraBoomZ;
	CameraUnfreezeCounter = CameraUnfreezeTime;
	bSavedCameraBoomZUpdateBlock = true;
	SavedCameraBoomZCounter = SavedCameraBoomZBlendTime;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SetCameraXDelta(float NewDelta, bool bInstantIn)
{
	CameraXDeltaSource = bInstantIn ? NewDelta : CameraXDeltaCurrent;
	CameraXDeltaTarget = NewDelta;
	CameraXDeltaCounter = CameraXDeltaBlendTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SetMinCameraZ(bool bUseMin, float MinValue)
{
	bUseCamMinZValue = bUseMin;
	MinCamZValue = MinValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SetMaxCameraZ(bool bUseMax, float MaxValue)
{
	bUseCamMaxZValue = bUseMax;
	MaxCamZValue = MaxValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::AddCooldown(const UObject* CooldownObject, float Duration, bool bCanCountInAir)
{
	if (CooldownObject == nullptr)
		return;

	if (Duration > 0.0f)
	{
		int32 i = 0;
		for (; i < Cooldowns.Num() && Cooldowns[i].Counter > Duration; ++i);

		Cooldowns.Insert({ CooldownObject, Duration, bCanCountInAir }, i);
		CooldownStarted.Broadcast(i, Duration, CooldownObject);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RemoveCooldown(const UObject* CooldownObject)
{
	for (int32 i = 0; i < Cooldowns.Num(); ++i)
		if (Cooldowns[i].Object == CooldownObject)
			Cooldowns[i].Counter = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RemoveAllCooldown()
{
	// update strike Cooldown
	for (int32 i = Cooldowns.Num() - 1; i >= 0; i--)
	{
		Cooldowns[i].Counter -= -1.0f;
		CooldownEnded.Broadcast(i, -1.0f, Cooldowns[i].Object);
		Cooldowns.RemoveAt(i); // not swap cause order matters for UI!
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::AddWeaponBoost(const FSoWeaponBoost& BoostToAdd)
{
	for (const FSoWeaponBoost& WeaponBoost : WeaponBoosts)
		if (BoostToAdd.OwnerEffect == WeaponBoost.OwnerEffect)
			return;

	WeaponBoosts.Add(BoostToAdd);
	if (SoSword->IsVisible())
		SelectWeapon(SoPlayerCharacterSheet->GetEquippedItem(ESoItemSlot::EIS_Weapon0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RemoveWeaponBoost(USoEffectBase* AssociatedEffect)
{
	for (int32 i = WeaponBoosts.Num() - 1; i>=0; i--)
		if (WeaponBoosts[i].OwnerEffect == AssociatedEffect)
		{
			if (WeaponBoosts[i].WeaponTemplate == SoPlayerCharacterSheet->GetEquippedItem(ESoItemSlot::EIS_Weapon0).Template)
			{
				SoSwordFX->SetVisibility(false);
				SoOffHandWeaponFX->SetVisibility(false);
			}

			WeaponBoosts.RemoveAtSwap(i);
			return;
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoCamKeyNode ASoCharacter::GetInterpolatedCamData() const
{
	const FSoSplinePoint& SplineLocation = SoMovement->GetSplineLocation();
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());

	if (PlayerSpline == nullptr)
		return FSoCamKeyNode();

	const float Distance = SplineLocation.GetDistance();
	const FSoCamNodeList& SplineCamData = PlayerSpline->GetCamNodeList();
	int32 CamKey = ActiveCamKey;
	return SplineCamData.GetInterpolatedCamData(Distance, CamKey);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RightClickPressed()
{
	bRightMouseBtnPressed = true;
	SoActivity->RightBtnPressed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OnUmbrellaOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (OtherActor == this)
		return;

	SoActivity->OnUmbrellaOverlap();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCharacter::GetSoundTimeDelayStopSpam() const
{
	return USoDateTimeHelper::NormalizeTime(SoundTimeDelayStopSpam);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCharacter::GetLateBounceThreshold() const
{
	return USoDateTimeHelper::NormalizeTime(LateBounceThreshold);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCharacter::GetForcedToMoveTimeAfterWallJump() const
{
	return ForcedToMoveTimeAfterWallJump;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCharacter::GetForcedToMoveTimeAfterHit() const
{
	return ForcedToMoveTimeAfterHit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SpawnBounceEffects(bool bWallJump, const FVector& HitPoint)
{
	// bubble stops this
	if (bFloatingActive)
		return;

	// Play sound but also prevents spam
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - BounceTimeSinceLastSoundPlayed > GetSoundTimeDelayStopSpam())
	{
		auto* BounceVariant = GetBounceSFX(bWallJump ? 1 : RollJumpPitches.Num() - 2);
		USoAudioManager::PlaySoundAtLocation(this, BounceVariant, FTransform(GetActorTransform()));
		BounceTimeSinceLastSoundPlayed = CurrentTime;
	}
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), bWallJump ? VFXBounceLarge : VFXBounceSmall, HitPoint, FRotator::ZeroRotator, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::FadeAndKillDestructible(ADestructibleActor* Destructible)
{
	if (Destructible != nullptr)
	{
		FadeAndKillList.Add({ Destructible, 2.0f });
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::FadeOutMeshes(TArray<AStaticMeshActor*> InMeshesToFade, float Speed, bool bCanHide)
{
	for (AStaticMeshActor* Actor : InMeshesToFade)
		if (Actor != nullptr)
			FadeOutMesh(Actor, Speed, bCanHide);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::FadeInMeshes(TArray<AStaticMeshActor*> InMeshesToFade, float Speed)
{
	for (AStaticMeshActor* Actor : InMeshesToFade)
		if (Actor != nullptr)
			FadeInMesh(Actor, Speed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::FadeOutMesh(AStaticMeshActor* MeshToFade, float Speed, bool bCanHide)
{
	if (!IsValid(MeshToFade))
		return;

	TArray<FSoFadeEntry>* FadeLists[] = { &MeshesToFade, &FadedMeshes };
	for (int32 i = 0; i < 2; ++i)
		for (FSoFadeEntry& FadeEntry : (*FadeLists[i]))
			if (FadeEntry.Mesh == MeshToFade)
			{
				FadeEntry.InstanceCount += 1;
				FadeEntry.FadeSpeed = (FadeEntry.FadeSpeed + Speed) / 2.0f;
				return;
			}

	MeshesToFade.Add({ MeshToFade, 1, 1.0f, Speed, bCanHide });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::FadeInMesh(AStaticMeshActor* MeshToFade, float Speed)
{
	if (!IsValid(MeshToFade))
		return;

	// check FadedMeshes first, these are invisible assets atm
	for (int32 i = 0; i < FadedMeshes.Num(); ++i)
		if (FadedMeshes[i].Mesh == MeshToFade)
		{
			FadedMeshes[i].InstanceCount = FMath::Max(FadedMeshes[i].InstanceCount - 1, 0);
			FadedMeshes[i].FadeSpeed = (FadedMeshes[i].FadeSpeed + Speed) / 2.0f;

			if (FadedMeshes[i].InstanceCount == 0)
			{
				// no more box wants this one faded out, move from one array to the other so it will fade back
				if (FadedMeshes[i].Mesh != nullptr)
				{
					FadedMeshes[i].Mesh->GetStaticMeshComponent()->SetVisibility(true, true);
					MeshesToFade.Add({ FadedMeshes[i]});
				}
				FadedMeshes.RemoveAt(i);
			}

			return;
		}
	// not found in outfaded array, check the active list
	for (FSoFadeEntry& FadeEntry : MeshesToFade)
		if (FadeEntry.Mesh == MeshToFade)
		{
			FadeEntry.InstanceCount = FMath::Max(FadeEntry.InstanceCount - 1, 0);
			FadeEntry.FadeSpeed = (FadeEntry.FadeSpeed + Speed) / 2.0f;
			return;
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::UpdateMeshFade(float DeltaSeconds)
{
	static const FName OPParamName = ("OP_Add");

	// 1. fade and kill list:

	for (int32 i = FadeAndKillList.Num() - 1; i >= 0; --i)
	{
		if (FadeAndKillList[i].Destructible == nullptr)
		{
			FadeAndKillList.RemoveAtSwap(i);
		}
		else
		{
			FadeAndKillList[i].FadeCounter -= DeltaSeconds;
			if (FadeAndKillList[i].FadeCounter < KINDA_SMALL_NUMBER)
			{
				FadeAndKillList[i].Destructible->GetDestructibleComponent()->SetVisibility(false);
				FadeAndKillList[i].Destructible->SetLifeSpan(0.1f);
				FadeAndKillList.RemoveAtSwap(i);
			}
			else
			{
				FadeAndKillList[i].Destructible->GetDestructibleComponent()->SetScalarParameterValueOnMaterials(OPParamName, FadeAndKillList[i].FadeCounter);
			}
		}
	}

	const bool bOnlyOutFade = (SoActivity == SoADead || SoActivity == SoACameraEdit);

	// 2. standard fade
	for (int32 i = MeshesToFade.Num() - 1; i >= 0; --i)
	{
		if (!IsValid(MeshesToFade[i].Mesh))
		{
			MeshesToFade.RemoveAt(i);
		}
		else
		{
			const bool bFadeOut = MeshesToFade[i].InstanceCount > 0;

			if (!bFadeOut && bOnlyOutFade)
				continue;

			const float FadeTarget = bFadeOut ? 0.0f : 1.0f;
			MeshesToFade[i].FadeValue = FMath::FInterpTo(MeshesToFade[i].FadeValue, FadeTarget, DeltaSeconds, MeshesToFade[i].FadeSpeed);

			if (MeshesToFade[i].Mesh != nullptr)
				MeshesToFade[i].Mesh->GetStaticMeshComponent()->SetScalarParameterValueOnMaterials(OPParamName, MeshesToFade[i].FadeValue);

			if (FMath::Abs(MeshesToFade[i].FadeValue - FadeTarget) < KINDA_SMALL_NUMBER)
			{
				if (bFadeOut && MeshesToFade[i].Mesh != nullptr)
				{
					if (!MeshesToFade[i].bCanHideIfOutFaded)
						continue;

					MeshesToFade[i].Mesh->GetStaticMeshComponent()->SetVisibility(false, true);
					FadedMeshes.Add(MeshesToFade[i]);
				}
				MeshesToFade.RemoveAt(i);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SelectWeapon(const FSoItem& Item)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SelectWeapon"), STAT_SelectWeapon, STATGROUP_SoCharacter);

	// clear old
	const USoWeaponTemplate* WeaponTemplate = Cast<const USoWeaponTemplate>(Item.Template);
	if (WeaponTemplate == nullptr)
		return;

	SoSword->SetVisibility(!bForceHideRightWeapon);
	SoSword->SetStaticMesh(WeaponTemplate->GetStaticMeshNew());

	SoSword->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponTemplate->GetBoneName());
	SoSword->SetRelativeRotation(WeaponLocalRot);

	UParticleSystem* WeaponFX = nullptr;
	for (const FSoWeaponBoost& WeaponBoost : WeaponBoosts)
		if (WeaponTemplate == WeaponBoost.WeaponTemplate)
			WeaponFX = WeaponBoost.ParticleSystem;

	SoSwordFX->SetTemplate(WeaponFX);
	SoSwordFX->SetVisibility(WeaponFX != nullptr && !bForceHideRightWeapon);

	const bool bDualWield = WeaponTemplate->GetWeaponType() == ESoWeaponType::EWT_DualWield;
	SoOffHandWeapon->SetVisibility(bDualWield && !bForceHideLeftWeapon);
	// SoOffHandWeapon->SetVisibility(bDualWield ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (bDualWield)
	{
		SoOffHandWeapon->SetStaticMesh(WeaponTemplate->GetStaticMeshNew());
		SoOffHandWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponTemplate->GetBoneNameSecondary());
		SoOffHandWeapon->SetRelativeRotation(WeaponLocalRot);
	}

	SoOffHandWeaponFX->SetTemplate(WeaponFX);
	SoOffHandWeaponFX->SetVisibility(bDualWield && WeaponFX != nullptr && !bForceHideLeftWeapon);

	ArmedAnimationSetNew = WeaponTemplate->GetNewAnimations();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OnArmedStateLeft()
{
	SoMovement->ClearRootMotionDesc();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::IsSoulkeeperActive() const
{
	return ResRuneVisualActor != nullptr && ResRuneVisualActor->IsActive();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OnSplineChanged(const FSoSplinePoint& OldLocation, const FSoSplinePoint& NewLocation)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnSplineChanged"), STAT_OnSplineChanged, STATGROUP_SoCharacter);

	// update camera dir correction
	if (OldLocation.IsValid(false) && NewLocation.IsValid(false))
	{
		SavedCamMovModifier *= ((FVector2D(OldLocation.GetDirection()) | FVector2D(NewLocation.GetDirection())) > 0) ? 1 : -1;
		AutoSave(300.0f);
	}

	UParticleSystem* NewTemplate = nullptr;
	const ASoPlayerSpline* NewSpline = Cast<ASoPlayerSpline>(NewLocation.GetSpline());
	if (NewSpline != nullptr)
		if (NewSpline->GetLevelClaims().Num() > 0)
			NewTemplate = USoLevelHelper::GetWeatherVFXForLevel(NewSpline->GetLevelClaims()[0].LevelName);

	if (NewTemplate != WeatherVFX->Template)
		WeatherVFX->SetTemplate(NewTemplate);

	OnPlayerSplineChanged.Broadcast(OldLocation.GetSpline(), NewSpline);

	if (MusicOverride == nullptr)
		UpdateMusic(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SetMusicOverride(UFMODEvent* Music, bool bInstantOverride, UObject* Requester)
{
 	MusicOverride = Music;
	MusicOverrideRequester = Requester;
	UpdateMusic(bInstantOverride);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::ClearMusicOverrideFromRequester(UObject* Requester)
{
	if (MusicOverrideRequester == Requester)
	{
		MusicOverride = nullptr;
		UpdateMusic(true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::AddBounceSFXOverride(ASoBounceSFXOverrideBox* SFXOverride)
{
	if (SFXOverride != nullptr)
	{
		BounceSFXOverrideStack.Add(SFXOverride);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RemoveBounceSFXOverride(ASoBounceSFXOverrideBox* SFXOverride)
{
	BounceSFXOverrideStack.Remove(SFXOverride);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UFMODEvent* ASoCharacter::GetBounceSFX(int32 Level) const
{
	if (BounceSFXOverrideStack.Num() > 0 && BounceSFXOverrideStack.Last(0) != nullptr)
		return BounceSFXOverrideStack.Last(0)->GetBounceSFX(Level);

	return SFXBounceVariants.IsValidIndex(Level) ? SFXBounceVariants[Level] : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::UpdateMusic(bool bSkipRequestDelay)
{
	UFMODEvent* MusicToSet = MusicOverride;

	if (MusicToSet == nullptr)
		if (const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SoMovement->GetSplineLocation().GetSpline()))
		{
			if (!SoPlayerCharacterSheet->GetBoolValue(USoStaticHelper::GetForceNoEnvMusicName()))
				MusicToSet = PlayerSpline->GetMusic();
		}

	if (MusicToSet != nullptr)
		USoAudioManager::Get(this).SetMusic(MusicToSet, bSkipRequestDelay);
	else
		USoAudioManager::Get(this).FadeOutActiveMusic();

	if (MusicToSet != nullptr)
		SoPlayerCharacterSheet->SetBoolValue(MusicToSet->GetFName(), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EActivity ASoCharacter::GetActivity() const
{
	return SoActivity ? SoActivity->GetID() : EActivity::EA_Default;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIVideoPlayer* ASoCharacter::GetUIVideoPlayer() const
{
	return UISystem ? UISystem->GetVideoPlayer() : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIMenuMain* ASoCharacter::GetUIMainMenu() const
{
	return UISystem ? UISystem->GetMainMenu() : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::IsMainMenuOpened() const
{
	if (USoUIMenuMain* MainMenu = GetUIMainMenu())
		return ISoUIEventHandler::Execute_IsOpened(MainMenu);

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::InitUISystem()
{
	// Already set
	if (UISystem != nullptr)
		return;

	if (*UISystemClass == nullptr)
	{
		UE_LOG(LogSoChar, Error, TEXT("Did not set UISystemClass!"))
		return;
	}

	UISystem = CreateWidget<USoUISystem>(GetPlayerController(), UISystemClass);
	if (UISystem)
		UISystem->AddToViewport();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::OnPreLanded(const FHitResult& Hit)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnPreLanded"), STAT_OnPreLanded, STATGROUP_SoCharacter);

	bLandedRecently = true;

	bAirJumpedSinceLastLand = false;
	OnPreLand.Broadcast();
	return SoActivity->OnPreLanded(Hit);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::AddItem(const FSoItem& Item, bool bPrintInDlgLog)
{
	const int32 SlotIndex = SoInventory->AddItem(Item);
	SoPlayerCharacterSheet->OnItemPickedUp(Item, SlotIndex);

	if (bPrintInDlgLog)
	{
		OnDlgItemChange.Broadcast(Item, true);
		USoAudioManager::PlaySoundAtLocation(this, SFXFItemGained, GetActorTransform());
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::RemoveItem(const USoItemTemplate* Template, bool bPrintInDlgLog)
{
	const int32 ItemIndex = SoInventory->GetIndexFromTemplate(Template);

	if (ItemIndex < 0)
	{
		if (Template == nullptr)
			UE_LOG(LogSoChar, Warning, TEXT("Remove item called with invalid item tempalte (nullptr)!"))
		else
			UE_LOG(LogSoChar, Warning, TEXT("Remove item called with a template the character does not have (%s!!"), *Template->GetName());

		return false;
	}

	const FSoItem Item = SoInventory->GetItem(ItemIndex);
	SoPlayerCharacterSheet->OnItemRemoved(Item);
	SoInventory->RemoveItem(ItemIndex, Item.Amount);

	if (bPrintInDlgLog)
		OnDlgItemChange.Broadcast(Item, false);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::CanEquipToSlot(ESoItemSlot ItemSlot) const
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::ForceHideWeapon(float Duration, bool bPrimary)
{
	if (bPrimary)
	{
		bForceHideRightWeapon = true;
		SoSword->SetVisibility(false, true);
	}
	else
	{
		bForceHideLeftWeapon = true;
		SoOffHandWeapon->SetVisibility(false, true);
	}

	FTimerDelegate TimerCallback;
	TimerCallback.BindLambda([bPrimary, this]
	{
		if (bPrimary)
			bForceHideRightWeapon = false;
		else
			bForceHideLeftWeapon = false;

		if (SoActivity->IsArmedState())
			SoAWeaponInArm->RefreshWeapon();
	});
	GetWorld()->GetTimerManager().SetTimer(bPrimary ? ForceHideRightWeaponOver : ForceHideLeftWeaponOver, TimerCallback, Duration, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::ClearForceHideWeapon()
{
	bForceHideRightWeapon = false;
	gTimerManager->ClearTimer(ForceHideRightWeaponOver);
	bForceHideLeftWeapon = false;
	gTimerManager->ClearTimer(ForceHideLeftWeaponOver);

	if (SoActivity->IsArmedState())
		SoAWeaponInArm->RefreshWeapon();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SetAnimInstanceClassFixed(UClass* NewClass)
{
	// For details: https://gist.github.com/underscorediscovery/46ecaf5cc1ba97f1a7e922f6a77a5705

	// We may be doing parallel evaluation on the current anim instance
	// Calling this here with true will block this init till that thread completes
	// and it is safe to continue

	const bool bBlockOnTask = true;					// wait on evaluation task so it is safe to swap the buffers
	const bool bPerformPostAnimEvaluation = true;	// Do PostEvaluation so we make sure to swap the buffers back.
	GetMesh()->HandleExistingParallelEvaluationTask(bBlockOnTask, bPerformPostAnimEvaluation);
	GetMesh()->SetAnimInstanceClass(NewClass);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::GetBoolValue_Implementation(FName ValueName) const	{ return SoPlayerCharacterSheet->GetBoolValue(ValueName); }
float ASoCharacter::GetFloatValue_Implementation(FName ValueName) const	{ return SoPlayerCharacterSheet->GetFloatValue(ValueName); }
int32 ASoCharacter::GetIntValue_Implementation(FName ValueName) const	{ return SoPlayerCharacterSheet->GetIntValue(ValueName); }
FName ASoCharacter::GetNameValue_Implementation(FName ValueName) const	{ return SoPlayerCharacterSheet->GetNameValue(ValueName); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::ModifyBoolValue_Implementation(FName ValueName, bool bValue)
{
	SoPlayerCharacterSheet->SetBoolValue(ValueName, bValue);

	/** maybe item is attached to this bool value, remove it if that is the case */
	if (bValue == false)
	{
		USoItemTemplate* QuestItem = SoInventory->FindQuestItem(ValueName);
		if (QuestItem != nullptr)
			RemoveItem(QuestItem, true);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RemoveBoolsWithPrefix(const FString& PreFix)
{
	SoPlayerCharacterSheet->RemoveBoolsWithPrefix(PreFix);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::ModifyIntValue_Implementation(FName ValueName, bool bDelta, int32 Value)
{
	OnDlgIntModified.Broadcast(ValueName, Value);
	SoPlayerCharacterSheet->SetIntValue(ValueName, Value, bDelta);

	if (ValueName == USoStaticHelper::NameGold)
		LastModifiedGoldAmount = Value;

	OnIntModified.Broadcast(ValueName, SoPlayerCharacterSheet->GetIntValue(ValueName));

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::ModifyFloatValue_Implementation(FName ValueName, bool bDelta, float Value)
{
	SoPlayerCharacterSheet->SetFloatValue(ValueName, Value, bDelta);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::ModifyNameValue_Implementation(FName ValueName, FName Value)
{
	SoPlayerCharacterSheet->SetNameValue(ValueName, Value);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OnPushed_Implementation(const FVector& DeltaMovement, float DeltaSeconds, bool bStuck, AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage)
{
	SoActivity->OnPushed(DeltaMovement, DeltaSeconds, bStuck, RematerializeLocation, DamageAmountIfStuck, bForceDamage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult) const
{
	return SoActivity->ShouldBounceOnHit(OutStepUpHitVelocity, OutBounceDamping, HitResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OnChapterLoaded()
{
	//#if WARRIORB_WITH_EDITOR
	//#else
	//	if (USoGameInstance::GetInstance(this)->IsChapter())
	//	{
	//		const FName ChapterName = USoLevelHelper::GetChapterFNameFromObject(this);
	//
	//		if (!SoPlayerCharacterSheet->GetBoolValue(ChapterName))
	//		{
	//			SoPlayerCharacterSheet->SetBoolValue(ChapterName, true);
	//			FSoChapterMapParams ChapterParams;
	//			USoGameSingleton::GetInstance()->GetChapterData(ChapterName, ChapterParams);
	//			if (ChapterParams.LevelEnterAnimation != nullptr)
	//			{
	//				SoAInteractWithEnvironment->Enter(ChapterParams.LevelEnterAnimation, nullptr, nullptr);
	//			}
	//		}
	//	}
	//#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::CheckSelfTrainedAchievement()
{
	if (SoPlayerCharacterSheet->GetBoolValue(WallJumpDoneName) &&
		SoPlayerCharacterSheet->GetBoolValue(MaxBounceDoneName))
		USoAchievementManager::Get(this).UnlockAchievementIfSatisfied(this, AchievementSelfTrained);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SetCameraLagEnabled(bool bEnabled)
{
	CameraBoom->bEnableCameraLag = bEnabled;
	CameraBoom->bEnableCameraRotationLag = bEnabled;

	//if (bEnabled)
	//	UE_LOG(LogTemp, Warning, TEXT("Cam lag enabled"))
	//else
	//	UE_LOG(LogTemp, Warning, TEXT("Cam lag disabled"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::Strike0Pressed()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Strike0Pressed"), STAT_Strike0Pressed, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI || DamageBlockCounter > 0.0f)
		return;

	bSpecialStrikeWasRequestedLast = false;
	SoActivity->StrikePressed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::Strike1Pressed()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Strike1Pressed"), STAT_Strike1Pressed, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI || DamageBlockCounter > 0.0f)
		return;

	bSpecialStrikeWasRequestedLast = true;
	SoActivity->StrikePressed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::CouldSwing() const
{
	if (!SoMovement->IsFalling() || !SoActivity->CanBeInterrupted(EActivity::EA_Swinging))
		return false;

	const float WorldLocZ = GetActorLocation().Z;
	for (int32 i = 0; i < SwingCenters.Num(); ++i)
		if (SwingCenters[i]->GetActorLocation().Z > WorldLocZ)
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RemoveSkyDiveField(AActor* SkyDiveField)
{
	SkyDiveFields.Remove(SkyDiveField);
	SkyDiveFields.Remove(nullptr);

	if (SkyDiveFields.Num() == 0)
	{
		OverrideFloatingVelocity(false);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::StartVideoLoopPlaybackGamepad()
{
	const ASoPlayerController* SoController = GetSoPlayerController();
	if (!GameInstance || !SoController)
		return;

	if (SoController->IsInputKeyDown(EKeys::Gamepad_LeftTrigger) && SoController->IsInputKeyDown(EKeys::Gamepad_RightTrigger))
	{
		StartVideoLoopPlayback();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::StartVideoLoopPlayback()
{
	GameInstance->StartVideoLoopPlayback();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::StopVideoLoopPlayback()
{
	GameInstance->StopVideoLoopPlayback();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RestartDemo()
{
	GameInstance->LoadGameForDemo(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OverrideAllMovementSpeed(float NewSpeed)
{
	SoARoll->OverrideMovementSpeed(NewSpeed);
	SoADead->OverrideMovementSpeed(NewSpeed);
	SoAHitReact->OverrideMovementSpeed(NewSpeed);
	SoADefault->OverrideMovementSpeed(NewSpeed);
	SoALillian->OverrideMovementSpeed(NewSpeed);
	SoASlide->OverrideMovementSpeed(NewSpeed);
	SoASwing->OverrideMovementSpeed(NewSpeed);
	SoALeverPush->OverrideMovementSpeed(NewSpeed);
	SoACarry->OverrideMovementSpeed(NewSpeed);
	// SoACarryPickUp->OverrideMovementSpeed(NewSpeed);
	SoACarryDrop->OverrideMovementSpeed(NewSpeed);
	SoAAiming->OverrideMovementSpeed(NewSpeed);
	SoAWeaponInArm->OverrideMovementSpeed(NewSpeed);
	SoAStrike->OverrideMovementSpeed(NewSpeed);
	SoAItemUsage->OverrideMovementSpeed(NewSpeed);
	SoAInteractWithEnvironment->OverrideMovementSpeed(NewSpeed);
	SoASoAWaitForActivitySwitch->OverrideMovementSpeed(NewSpeed);
	SoAWait->OverrideMovementSpeed(NewSpeed);
	SoAInUI->OverrideMovementSpeed(NewSpeed);
	SoATeleport->OverrideMovementSpeed(NewSpeed);
	SoACameraEdit->OverrideMovementSpeed(NewSpeed);
	SoASkyControlEdit->OverrideMovementSpeed(NewSpeed);
	SoACharShadowEdit->OverrideMovementSpeed(NewSpeed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::UseMovementOverride(bool bUse, float OverrideValue)
{
	bUseMovementOverride = bUse;
	MovementOverrideValue = OverrideValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCameraComponent* ASoCharacter::GetSideViewCameraComponent() const { return SideViewCameraComponent; }
USoSpringArmComponent* ASoCharacter::GetCameraBoom() const { return CameraBoom; }
int32 ASoCharacter::GetClosestCamIndex() const { return SoACameraEdit->GetClosestCamIndex(); }

APlayerController* ASoCharacter::GetPlayerController() const { return Cast<APlayerController>(GetController()); }
ASoPlayerController* ASoCharacter::GetSoPlayerController() const { return Cast<ASoPlayerController>(GetController()); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::IsSaveAllowed() const
{
	return SoActivity != nullptr && SoActivity->IsSaveAllowed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::Interact0()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Interact0"), STAT_Interact0, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI)
		return;

	SoActivity->InteractKeyPressed(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::Interact1()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Interact1"), STAT_Interact1, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI)
		return;

	SoActivity->InteractKeyPressed(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::TakeWeaponAway()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("TakeWeaponAway"), STAT_TakeWeaponAway, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI)
		return;

	SoActivity->TakeWeaponAway();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::ToggleItems()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ToggleItems"), STAT_ToggleItems, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI)
		return;

	SoActivity->ToggleItems();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::ToggleSpells(bool bQuickSelectionMode)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ToggleSpells"), STAT_ToggleSpells, STATGROUP_SoCharacter);

	SoAInUI->ToggleSpells(bQuickSelectionMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::UseItemFromSlot0()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UseItemFromSlot0"), STAT_UseItemFromSlot0, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI)
		return;

	SoActivity->UseItemFromSlot0();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::JumpPressed()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("JumpPressed"), STAT_JumpPressed, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI)
		return;

	SoActivity->JumpPressed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::RollPressed()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RollPressed"), STAT_RollPressed, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI)
		return;

	bRollPressed = true;
	SoActivity->RollPressed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::ToggleWeapons()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ToggleWeapons"), STAT_ToggleWeapons, STATGROUP_SoCharacter);

	if (bGameInputBlockedByUI)
		return;

	SoActivity->ToggleWeapons();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ASoCharacter::IncreaseCollisionSize()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("IncreaseCollisionSize"), STAT_IncreaseCollisionSize, STATGROUP_SoCharacter);

	SoActivity->IncreaseCollisionSize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::OnBlocked()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnBlocked"), STAT_OnBlocked, STATGROUP_SoCharacter);

	SoActivity->OnBlocked();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SuperEditModePressed()
{
	SoActivity->SuperEditModePressed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::LeftClickPressed()
{
	bLeftMouseBtnPressed = true; SoActivity->LeftBtnPressed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::ToggleCharacterPanels()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ToggleCharacterPanels"), STAT_ToggleCharacterPanels, STATGROUP_SoCharacter);

	SoAInUI->ToggleCharacterPanels();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::HandleUICommand(FKey Key, ESoUICommand Command)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("HandleUICommand"), STAT_HandleUICommand, STATGROUP_SoCharacter);

	if (!GameInstance->IsVideoLooping())
	{
 		SoActivity->HandleUICommand(Key, Command);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacter::SwitchFromUI()
{
	if (SoActivity == SoAInUI)
		SoAInUI->SwitchToRelevantState(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacter::IsInDialogue() const
{
	return SoActivity == SoAInUI && SoAInUI->IsInDialogue();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// <Input bindings>
bool ASoCharacter::IsToggleSpellsPressed() const { return InputPressedActionNames.Contains(FSoInputActionName::ToggleSpell); }

float ASoCharacter::GetXAxisValue() const { return InputComponent ? InputComponent->GetAxisValue("MouseX") : 0.f; }
float ASoCharacter::GetYAxisValue() const { return InputComponent ? InputComponent->GetAxisValue("MouseY") : 0.f; }

float ASoCharacter::GetMovementXAxisValue() const { return InputComponent ? InputComponent->GetAxisValue(FSoInputAxisName::Move) : 0.f; }
float ASoCharacter::GetMovementYAxisValue() const { return InputComponent ? InputComponent->GetAxisValue(FSoInputAxisName::MoveY) : 0.f; }

// </Input bindings>
///////////////////////////////////////////////////////////////////////////////////////////////////////////


bool ASoCharacter::CanPauseOnIdle() const { return SoActivity ? SoActivity->CanPauseOnIdle() : true; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// <camera edit functions>

#if WITH_EDITOR
void ASoCharacter::SaveEditedData() { SoActivity->SaveEditedData(); }
void ASoCharacter::LoadEditedData() { SoActivity->LoadEditedData(); }

void ASoCharacter::SwitchCameraEditMode()		{ SoActivity->SwitchActivity(SoActivity == SoACameraEdit	 ? Cast<USoActivity>(SoADefault) : SoACameraEdit);	   }
void ASoCharacter::SwitchSkyEditMode()			{ SoActivity->SwitchActivity(SoActivity == SoASkyControlEdit ? Cast<USoActivity>(SoADefault) : SoASkyControlEdit); }
void ASoCharacter::SwitchCharShadowEditMode()	{ SoActivity->SwitchActivity(SoActivity == SoACharShadowEdit ? Cast<USoActivity>(SoADefault) : SoACharShadowEdit); }

void ASoCharacter::CreateKey() { SoActivity->CreateKey(); }
void ASoCharacter::MoveClosestKeyHere() { SoActivity->MoveClosestKeyHere(); }
void ASoCharacter::DeleteActiveKeyNode() { SoActivity->DeleteActiveKeyNode(); }

void ASoCharacter::CopyActiveKeyData() { SoActivity->CopyActiveKeyData(); }
void ASoCharacter::PasteToActiveKeyData() { SoActivity->PasteToActiveKeyData(); }

void ASoCharacter::JumpToNextKey() { SoActivity->JumpToNextKey(); }
void ASoCharacter::JumpToPrevKey() { SoActivity->JumpToPrevKey(); }

void ASoCharacter::SpecialEditButtonPressed0() { SoActivity->SpecialEditButtonPressed(0); }
void ASoCharacter::SpecialEditButtonPressed1() { SoActivity->SpecialEditButtonPressed(1); }

#endif


// </camera edit functions>
///////////////////////////////////////////////////////////////////////////////////////////////////////////
