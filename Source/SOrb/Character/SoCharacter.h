// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CharacterBase/SoCharacterBase.h"

#include "DlgDialogueParticipant.h"
#include "Basic/SoDialogueParticipant.h"

// helper structs
#include "SoCharacterDataTypes.h"
#include "Settings/Input/SoInputSettingsTypes.h"
#include "UI/General/SoUITypes.h"
#include "Online/Achievements/SoAchievementManager.h"

// enum types: EActivity, EAnimEvents
#include "SoCharStates/SoActivityTypes.h"

// FSoCamKeyNode
#include "SplineLogic/SoCameraData.h"

// FSoSkyControlValue
#include "SplineLogic/SoSkyControlData.h"

// SoItemType
#include "Items/SoItemTypes.h"

// FSoItem
#include "Items/SoItem.h"

#include "SoCharacter.generated.h"

// Components
class USoPlayerProgress;
class USoPlayerCharacterSheet;
class USoInventoryComponent;
class USoAnalyticsComponent;
class USoSpringArmComponent;
class USoProjectileSpawnerComponent;
class UParticleSystemComponent;
class USceneCaptureComponent2D;
class UCameraComponent;
class USoSpringArmComponent;
class UInputComponent;
class ASoBounceSFXOverrideBox;

// Activities
class USoActivity;
class USoADead;
class USoAHitReact;
class USoADefault;
class USoALillian;
class USoARoll;
class USoASlide;
class USoASwing;
class USoAAiming;
class USoAWeaponInArm;
class USoAStrike;
class USoALeverPush;
class USoACarry;
// class USoACarryPickUp;
class USoACarryDrop;
class USoAInUI;
class USoAItemUsage;
class USoAInteractWithEnvironment;
class USoAWait;
class USoASoAWait;
class USoASoAWaitForActivitySwitch;
class USoATeleport;
class USoACameraEdit;
class USoASkyControlEdit;
class USoACharShadowEdit;

// Misc
class ASoInteractableActor;
class ASoCarryable;
class ASoSwingCenter;
class ISoInteractable;
class USoWizard;
class ASoCharacter;
class USoUISystem;
class USoUIMenuMain;
class USoUIVideoPlayer;
class AActor;
class UDlgContext;
class USoGameInstance;
class USoItemTemplateRuneStone;
class UFMODEvent;
class UFMODAudioComponent;
class UParticleSystem;
class UCurveFloat;
class APostProcessVolume;
class ASoPlayerSpline;
class ASoPlayerController;
class UCameraShake;
class UForceFeedbackEffect;
struct FSoAnimationSet;
class UMaterial;
class USoEffectBase;
class ADestructibleActor;

//
// Delegates
//

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoNotify);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoWaitForLevelShow, bool, bNotEvenLoaded);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoSplineChangeNotify, const ASoSpline*, OldSpline, const ASoSpline*, NewSpline);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoPlayerSplineChangeNotify, const ASoPlayerSpline*, OldSpline, const ASoPlayerSpline*, NewSpline);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoForceAreaEnteredDisplay, const FText&, AreaName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoDlgIntModified, FName, ValueName, int32, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoDlgItemChange, const FSoItem&, Item, bool, bGained);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSoCooldownChanged, int32, Index, float, RemainingTime, const UObject*, ObjectWithCooldown);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoChangeUIVisibilityNotify, bool, bShow);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoNotifyIntModified, FName, IntName, int32, NewValue);



UCLASS()
class SORB_API ASoCharacter : public ASoCharacterBase, public ISoDialogueParticipant, public IDlgDialogueParticipant
{
	GENERATED_BODY()
	typedef ASoCharacter Self;

	DECLARE_DELEGATE_OneParam(FSoDelegateBoolOneParam, bool);
public:

	ASoCharacter(const FObjectInitializer& ObjectInitializer);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <ACharacter interface>

	void PreInitializeComponents() override;
	void PostInitializeComponents() override;
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason);
	void Tick(float DeltaSeconds) override;
	void SetupPlayerInputComponent(UInputComponent* InInputComponent) override;
	void Landed(const FHitResult& Hit) override;
	void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
protected:
	bool CanJumpInternal_Implementation() const override;
	void BaseChange() override;
	// </ACharacter interface>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

public:
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Interfaces>

	// ISoDialogueParticipant
	virtual const FSoDialogueParticipantData& GetParticipantData() const override { return DialogueData; }

	// DlgDialogueParticipant interface
	FName GetParticipantName_Implementation() const { return DialogueData.ParticipantName; }
	FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const { return DialogueData.ParticipantDisplayName; }
	ETextGender GetParticipantGender_Implementation() const { return ETextGender::Neuter; }
	UTexture2D* GetParticipantIcon_Implementation(FName ActiveSpeaker, FName ActiveSpeakerState) const { return DialogueData.ParticipantIcon; }

	bool ModifyIntValue_Implementation(FName ValueName, bool bDelta, int32 Value);
	bool ModifyFloatValue_Implementation(FName ValueName, bool bDelta, float Value);
	float GetFloatValue_Implementation(FName ValueName) const;
	int32 GetIntValue_Implementation(FName ValueName) const;
	bool ModifyNameValue_Implementation(FName ValueName, FName Value);
	FName GetNameValue_Implementation(FName ValueName) const;

	bool OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName) { return false; }
	bool CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const { return false; }

	/** check/add progress data */
	bool ModifyBoolValue_Implementation(FName ValueName, bool bValue);
	bool GetBoolValue_Implementation(FName ValueName) const;

	void RemoveBoolsWithPrefix(const FString& PreFix);

	// ISoSplineWalker
	void OnPushed_Implementation(const FVector& DeltaMovement, float DeltaSeconds, bool bStuck, AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage) override;

	// ISoMortal
	virtual bool IsAlive_Implementation() const override;
	virtual bool HasInvulnerability_Implementation() const override { return DamageBlockCounter > 0.0f; }
	virtual void Kill_Implementation(bool bPhysical) override;
	virtual void CauseDmg_Implementation(const FSoDmg& Dmg, const FSoHitReactDesc& HitReactDesc) override;
	virtual bool MeleeHit_Implementation(const FSoMeleeHitParam& HitParam) override;

	virtual bool SubscribeOnWalkThroughMortal_Implementation(const FSoNotifyActorSingle& OnWalkThrough, bool bSubscribe = true) override;


	void AutoSave(float MinTimeSinceLastSave);

	//
	// Spells
	//

	UFUNCTION(BlueprintCallable, Category = "Spells")
	void ModifySpellsCapacity(int32 ModifyAmount);

	UFUNCTION(BlueprintCallable, Category = "Spells")
	void SetSpellsCapacity(int32 NewCapacity);

	UFUNCTION(BlueprintPure, Category = "Spells")
	int32 GetSpellsCapacity() const;

	void EnsureSpellsCapacityLimits(int32 SpellsCapacity = 2);

	// </Interfaces>
	////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////
	// <ASoCharacterBase>
	virtual void OnSplineChanged(const FSoSplinePoint& OldLocation, const FSoSplinePoint& NewLocation) override;
	virtual bool ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult) const override;
	virtual bool OnPreLanded(const FHitResult& Hit) override;
	virtual void OnBounce(bool bWallJump, float NewStoredMoveValue, const FVector& HitPoint, const FVector& HitNormal) override;
	virtual bool CanPerformWallJump(const FHitResult& HitResult) override;
	virtual void OnBlocked() override;
	// </ASoCharacterBase>
	////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Input bindings>

	// mouse/gamepad-right-stick axis
	float GetXAxisValue() const;
	float GetYAxisValue() const;

	// WASD/gamepad left stick value
	UFUNCTION(BlueprintCallable)
	float GetMovementXAxisValue() const;
	float GetMovementYAxisValue() const;

	// Get Right gamepad direction
	// https://en.wikipedia.org/wiki/Atan2#/media/File:Atan2_60.svg
	// Top circle values are negatives from [-180, 0]
	// Bottom circle values are positive from [0, 180]
	// Y at top = -1
	// Y at bottom = 1
	FORCEINLINE float GetGamepadRightDirectionDegrees() const
	{
		return FMath::RadiansToDegrees(FMath::Atan2(GamepadRightY, GamepadRightX));
	}

	// Get Left gamepad direction
	// https://en.wikipedia.org/wiki/Atan2#/media/File:Atan2_60.svg
	// Top circle values are positive from [0, 180]
	// Bottom values are negative from [-180, 0]
	// Y at top = 1
	// Y at bottom = -1
	FORCEINLINE float GetGamepadLeftDirectionDegrees() const
	{
		return FMath::RadiansToDegrees(FMath::Atan2(GamepadLeftY, GamepadLeftX));
	}

	float GetGamepadRightXAxisValue() const { return GamepadRightX; }
	float GetGamepadRightYAxisValue() const { return GamepadRightY; }
	float GetGamepadLeftXAxisValue() const { return GamepadLeftX; }
	float GetGamepadLeftYAxisValue() const { return GamepadLeftY; }

	FORCEINLINE bool IsUsingRightThumbStick() const
	{
		return !FMath::IsNearlyZero(GamepadRightX) || !FMath::IsNearlyZero(GamepadRightY);
	}
	FORCEINLINE bool IsUsingLeftThumbStick() const
	{
		return !FMath::IsNearlyZero(GamepadLeftX) || !FMath::IsNearlyZero(GamepadLeftY);
	}

	FORCEINLINE bool IsUsingRightThumbStickWithSameValueForAtLeastFrames(int32 Frames) const
	{
		return RightThumbstickSameValueFrames >= Frames;
	}
	FORCEINLINE bool IsUsingLeftThumbStickWithSameValueForAtLeastFrames(int32 Frames) const
	{
		return LeftThumbstickSameValueFrames >= Frames;
	}

	// Tells us if UI is pressed or not
	void SetGameInputBlockedByUI(bool bValue) { bGameInputBlockedByUI = bValue; }
	void UIInputPressed(FKey Key, const ESoUICommand Command);
	void UIInputReleased(FKey Key, const ESoUICommand Command);
	void ClearInputStates()
	{
		bGameInputBlockedByUI = false;
		UIInputPressedCommands.Empty();
		InputPressedActionNames.Empty();
	}

	FORCEINLINE bool IsAnyUIInputPressed() const { return UIInputPressedCommands.Num() > 0;  }
	FORCEINLINE int32 GetNumUIInputsPressed() const { return UIInputPressedCommands.Num(); }
	FORCEINLINE bool IsUIInputPressed(ESoUICommand Command) { return UIInputPressedCommands.Contains(Command); }

	// AND, all must be pressed
	FORCEINLINE bool AreUIInputsPressed(const TSet<ESoUICommand>& Commands) const
	{
		return UIInputPressedCommands.Includes(Commands);
	}

	// OR, one of the following must be present
	FORCEINLINE bool AreAnyUIInputsPressed(const TSet<ESoUICommand>& Commands) const
	{
		return UIInputPressedCommands.Intersect(Commands).Num() > 0;
	}

	// Tracks the input press released for the action name
	void TrackInputPressReleased(UInputComponent* InInputComponent, FName ActionName, bool bTrackWhenPaused = false);

	// Tells us which commands are pressed released
	void InputPressed(FName ActionName);
	void InputReleased(FName ActionName);

	UFUNCTION()
	void HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType);

	// called each frame with the movement axis (a-d or gamepad left stick)
	void Move(float Value);

	// called on gamepad right stick
	void MoveGamepadRightX(float Value) { GamepadRightX = Value; }
	void MoveGamepadRightY(float Value) { GamepadRightY = Value; }

	// called on gamepad left stick
	void MoveGamepadLeftX(float Value) { GamepadLeftX = Value; }
	void MoveGamepadLeftY(float Value) { GamepadLeftY = Value; }

	bool IsToggleSpellsPressed() const;

	// used only for edit, cause the mouse is removed from the game
	void LeftClickPressed();
	void LeftClickReleased() { bLeftMouseBtnPressed = false; }

	void RightClickPressed();
	void RightClickReleased() { bRightMouseBtnPressed = false; }

	// press & hold
	void RollPressed();
	void RollReleased() { bRollPressed = false; }

	UFUNCTION(BlueprintCallable, Category = UI)
	void HandleUICommand(FKey Key, ESoUICommand Command);

	void LockFaceDirectionPressed();
	void LockFaceDirectionReleased();

	void Strike0Pressed();
	void Strike1Pressed();

	void JumpPressed();

	// SkyDiving
	void UmbrellaPressed() { bUmbrellaPressed = true; }

	UFUNCTION(BlueprintCallable)
	void UmbrellaReleased() { bUmbrellaPressed = false; }

	void ToggleWeapons();
	void ToggleItems();
	void ToggleSpells(bool bQuickSelectionMode);

	void UseItemFromSlot0();
	void SuperEditModePressed();
	void ToggleCharacterPanels();

	void QuickSaveLoad0() { QuickSaveLoad(42); }
	void QuickSaveLoad1() { QuickSaveLoad(43); }
	void QuickSaveLoad(int32 Index);

	UFUNCTION(BlueprintCallable, Category = Combat)
	void TakeWeaponAway();

	void Interact0();
	void Interact1();

	// Video
	void StartVideoLoopPlayback();
	void StartVideoLoopPlaybackGamepad();
	void StopVideoLoopPlayback();

	// Demo
	void RestartDemo();

	// used for quick tests
	UFUNCTION(BlueprintCallable)
	void DebugFeature();

	// used or quick tests in bp, called by DebugFeature()
	UFUNCTION(BlueprintImplementableEvent)
	void OnDebugButtonPressed();

	// used by the mechanism which stops the user from bouncing via space spam
	void JumpPressedRecentlyTimeOver();
	// </Input bindings>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <checkpoint>
	UFUNCTION(BlueprintCallable, Category = Health)
	bool IsSoulkeeperActive() const;

	bool CanUseSoulkeeperAtLocation() const { return bCanUseSoulkeeper; }

	UFUNCTION(BlueprintImplementableEvent)
	void OnDeathBP(bool bWithSoulkeeper);

	UFUNCTION(BlueprintImplementableEvent)
	void OnRespawn(bool bSoulkeeperUsed);

	/** special hit react, BP spawns destructible mesh */
	UFUNCTION(BlueprintImplementableEvent)
	void OnBreakIntoPieces();

	/** called only if it was broken into peaces */
	UFUNCTION(BlueprintImplementableEvent)
	void OnRematerialized();


	/** resets player stats and events are fired to reset the world! */
	UFUNCTION(BlueprintCallable, Category = Health)
	void Revive(bool bSoulKeeper, bool bCanUseSplineOverrideLocation = false, bool bWantToLeaveSpline = false);

	UFUNCTION(BlueprintCallable, Category = Health)
	void FireEventsAsIfRespawned();


	UFUNCTION(BlueprintCallable, Category = ResRune)
	bool SpawnResRune();

	UFUNCTION(BlueprintCallable, Category = ResRune)
	void PickupResRune();

	UFUNCTION(BlueprintCallable, Category = Checkpoint)
	void SetActiveCheckpointLocationName(FName LocationName);

	FName GetActiveCheckpointLocationName() const;

	UFUNCTION(BlueprintCallable, Category = Checkpoint)
	void SetCheckpointLocation(const FSoSplinePoint& InSplinePoint, const float ZValue);

	// Updates the current location of the Character to the CheckPoint indentified by CheckPointName
	UFUNCTION(BlueprintCallable, Category = Checkpoint)
	void TeleportToCheckpointName(FName CheckPointName, bool bPerformTeleportation = true);

	// Helper method for TeleportToCheckpointName that updates the location of the character to the active checkpoint name
	UFUNCTION(BlueprintCallable, Category = Checkpoint)
	void TeleportToActiveCheckpointName();
	// </checkpoint>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <item>
	UFUNCTION(BlueprintCallable, Category = Item)
	bool AddItem(const FSoItem& Item, bool bPrintInDlgLog = false);

	/* removes the first item instance with matching template */
	UFUNCTION(BlueprintCallable, Category = Item)
	bool RemoveItem(const USoItemTemplate* Template, bool bPrintInDlgLog = false);

	UFUNCTION(BlueprintCallable, Category = Item)
	bool CanEquipToSlot(ESoItemSlot ItemSlot) const;
	// </item>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <misc>

	bool CanPauseOnIdle() const;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ShowCharacterBP(bool bInstant);

	UFUNCTION()
	void ShowCharacter() { ShowCharacterBP(true); }

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void HideCharacterBP(bool bInstant);

	void ForceHideWeapon(float Duration, bool bPrimary);
	UFUNCTION(BlueprintCallable)
	void ClearForceHideWeapon();


	UFUNCTION(BlueprintCallable, Category = AnimationIssue)
	void SetAnimInstanceClassFixed(UClass* NewClass);

	void OnChapterLoaded();

	void CheckSelfTrainedAchievement();

	void SetInterruptedWizardDialogue(UDlgContext* Context) { InterruptedWizardDialogue = Context; }
	UDlgContext* GetInterruptedWizardDialogue() { return InterruptedWizardDialogue; }

	UFUNCTION(BlueprintCallable, Category = Health)
	void SufferOverlapDamage(AActor* SourceActor, const FSoDmg& Damage, const FSoHitReactDesc& HitReactDesc);

	/** return value true: overlap dmg can be called */
	UFUNCTION(BlueprintCallable, Category = Health)
	bool PreOverlapDamage(AActor* SourceActor);


	UFUNCTION(BlueprintCallable, Category = Health)
	void OnWalkThroughEnemy(AActor* Enemy);

	UFUNCTION(BlueprintPure)
	bool HasDamageBlock() const { return DamageBlockCounter > 0.0f; }


	bool IsSaveAllowed() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Material)
	void UpdateMaterialBP();

	UFUNCTION(BlueprintImplementableEvent)
	void SetEditorWidgetVisibility(ESoEditorUI Editor, bool bShow);

	UFUNCTION(BlueprintImplementableEvent)
	void OnPostQuickTravelBP(FVector OldLocation);

	UFUNCTION(BlueprintImplementableEvent)
	void OnRandomNewFunction(FVector OldLocation);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void SetCameraLagEnabled(bool bEnabled);


	UFUNCTION(BlueprintCallable)
	void SetMusicOverride(UFMODEvent* Music, bool bInstantOverride = false, UObject* Requester = nullptr);

	UFUNCTION(BlueprintCallable)
	void ClearMusicOverrideFromRequester(UObject* Requester);


	UFUNCTION(BlueprintCallable)
	void AddBounceSFXOverride(ASoBounceSFXOverrideBox* SFXOverride);

	UFUNCTION(BlueprintCallable)
	void RemoveBounceSFXOverride(ASoBounceSFXOverrideBox* SFXOverride);

	UFMODEvent* GetBounceSFX(int32 Level) const;


	UFUNCTION(BlueprintCallable)
	void UpdateMusic(bool bSkipRequestDelay);

	UFUNCTION(BlueprintCallable)
	void FadeAndKillDestructible(ADestructibleActor* Destructible);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void FadeOutMesh(AStaticMeshActor* MeshToFade, float Speed, bool bCanHide = true);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void FadeInMesh(AStaticMeshActor* MeshToFade, float Speed);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void FadeOutMeshes(TArray<AStaticMeshActor*> MeshesToFade, float Speed, bool bCanHide = true);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void FadeInMeshes(TArray<AStaticMeshActor*> MeshesToFade, float Speed);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void AddToOrientTowardsCameraList(USceneComponent* SceneComponentToAdd) { OrientTowardsCameraList.Add(SceneComponentToAdd); }

	UFUNCTION(BlueprintCallable, Category = Camera)
	void RemoveFromOrientTowardsCameraList(USceneComponent* SceneComponentToRemove) { OrientTowardsCameraList.RemoveSwap(SceneComponentToRemove); }

	UFUNCTION(BlueprintCallable, Category = Camera)
	void AddToEnemyHPWidgetList(USceneComponent* SceneComponentToAdd) { EnemyHPWidgetList.AddUnique(SceneComponentToAdd); }

	UFUNCTION(BlueprintCallable, Category = Camera)
	void RemoveFromEnemyHPWidgetList(USceneComponent* SceneComponentToRemove) { EnemyHPWidgetList.RemoveSwap(SceneComponentToRemove); }

	// Switches from ingame ui to default
	void SwitchFromUI();


	UFUNCTION(BlueprintPure)
	bool IsInDialogue() const;

	UFUNCTION(BlueprintCallable, Category = CameraFade)
	void CallFadeFromBlackInstant() { FadeFromBlackInstant.Broadcast(); }


	UFUNCTION(BlueprintCallable, meta=(ScriptName="SetForceHideInteractionTip"))
	void ForceHideInteractionTip(bool bHide)
	{
		bForceHideInteractionTip = bHide;
		OnInteractionLabelChanged.Broadcast();
	}

	// </misc>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Getter functions>
	UFUNCTION(BlueprintPure, Category = State)
	EActivity GetActivity() const;

	USoPlayerCharacterSheet* GetPlayerCharacterSheet() const { return SoPlayerCharacterSheet; }
	USoInventoryComponent* GetInventory() { return SoInventory; }

	UFUNCTION(BlueprintCallable, Category = Inventory)
	const USoInventoryComponent* GetInventory() const { return SoInventory; }

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	USoWizard* GetWizard() const { return SoWizard; }

	USoPlayerProgress* GetPlayerProgress() const { return SoPlayerProgress; }
	USoAnalyticsComponent* GetAnalyticsComponent() const { return SoAnalytics; }
	USoUISystem* GetUISystem() const { return UISystem; }
	USoUIVideoPlayer* GetUIVideoPlayer() const;
	USoUIMenuMain* GetUIMainMenu() const;
	bool IsMainMenuOpened() const;

	UFUNCTION(BlueprintCallable, Category = UI)
	void InitUISystem();

	APlayerController* GetPlayerController() const;
	ASoPlayerController* GetSoPlayerController() const;

	const TArray<FSoCooldownCounter>& GetCooldowns() const { return Cooldowns; }

	FSoCooldownChanged& OnCooldownStarted() { return CooldownStarted; }
	FSoCooldownChanged& OnCooldownEnded() { return CooldownEnded; }
	FSoCooldownChanged& OnCooldownBlocksEvent() { return CooldownBlocksEvent; }

	FSoNotify& OnCanUseSoulKeeperChanged() { return CanUseSoulkeeperChanged; }

	FSoNotify& OnToggleUI() { return ToggleUI; }

	// </Getter functions>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Combat>

	/** setups the visual for the input item, changes the weapon(s) to visible but does not activate collision */
	void SelectWeapon(const FSoItem& Item);

	/** disable && hide weapon, clear rootmotion, maybe increase collision size */
	void OnArmedStateLeft();

	// </Combat>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Interactions>
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void AddInteractable(AActor* Interactable);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RemoveInteractable(AActor* Interactable);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RemoveAllInteractable();

	void SwitchActiveInteractable();

	/**  Returns data for UI */
	UFUNCTION(BlueprintCallable, Category = Interaction)
	AActor* GetInteractionTipRelevantData(int32 Index, bool& bVisible, bool& bPrimaryKey);

	UFUNCTION(BlueprintCallable, Category = Interaction)
	void StartCarryStuff(ASoCarryable* Stuff);

	UFUNCTION(BlueprintCallable, Category = Interaction)
	void StartLeverPush(const struct FSoLeverData& LeverData, const FSoSplinePoint& SplinePoint, float ZValue);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Interaction)
	int32 GetAvailableInteractableNum() const { return Interactables.Num(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Interaction)
	AActor* GetActiveInteractable() const { return ActiveInteractable < Interactables.Num() ? Interactables[ActiveInteractable] : nullptr; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Interaction)
	int32 GetActiveInteractableIndex() const { return ActiveInteractable; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Interaction)
	AActor* GetInteractable(int32 Index) const { return Index < Interactables.Num() ? Interactables[Index] : nullptr; }

	UFUNCTION(BlueprintCallable, Category = Interaction)
	void RequestInteractionMessageDisplay() { OnInteractableMessageRequested.Broadcast(); }

	UFUNCTION(BlueprintCallable, Category = Interaction)
	void RequestInteractionMessageHide() { OnInteractableMessageHideRequest.Broadcast(); }

	UFUNCTION(BlueprintCallable, Category = Interaction)
	void RequestForcedAreaChange(const FText& AreaName) { OnForcedAreaChange.Broadcast(AreaName); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Interaction)
	bool CouldSwing() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	void RemoveSkyDiveField(AActor* SkyDiveField);

	void AddSwingCenter(ASoSwingCenter* SwingCenter) { SwingCenters.AddUnique(SwingCenter); OnInteractionLabelChanged.Broadcast(); }
	void RemoveSwingCenter(ASoSwingCenter* SwingCenter) { SwingCenters.Remove(SwingCenter); OnInteractionLabelChanged.Broadcast(); }
	// </Interactions>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Movement>

	UFUNCTION(BlueprintCallable, Category = Movement)
	void OverrideFloatingVelocity(bool bOverride, float OverrideValue = -200.0f);

	// Teleport character to the specified InSplinePoint
	bool SetPositionOnSplineSP(const FSoSplinePoint& InSplinePoint,
							  float ZValue,
							  bool bCollisionChecksAndPutToGround = true,
							  bool bStoreAsCheckpoint = false,
							  bool bLookBackwards = false);

	void SetMovementStop() { MovementStopMultiplier = 0.0f; }

	// true if the player pressed the jump button (only once) in the given time interval
	FORCEINLINE bool DidPlayerJump() const { return (bJumpPressedRecently && !bBanJumpCauseSpamming); }

	UFUNCTION(BlueprintImplementableEvent)
	void OnRollHitBP(const FVector& HitPoint, const FVector& HitNormal, bool bWallHit);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Movement)
	bool IsFacingVelocity() const;

	float GetActiveRollJumpHeight() const;
	float GetRollJumpHeight(int32 JumpLevel) const;

	UFUNCTION()
	void OnUmbrellaOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	void SpawnBounceEffects(bool bWallJump, const FVector& HitPoint);

	float GetSoundTimeDelayStopSpam() const;
	float GetLateBounceThreshold() const;
	float GetForcedToMoveTimeAfterWallJump() const;
	float GetForcedToMoveTimeAfterHit() const;
	// </Movement>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <CAMERA>
	UCameraComponent* GetSideViewCameraComponent() const;
	USoSpringArmComponent* GetCameraBoom() const;

	int32 GetClosestCamIndex() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Camera)
	FSoCamKeyNode GetInterpolatedCamData() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Camera)
	float GetLastLandZ() const { return LastLandZ; }

	UFUNCTION(BlueprintCallable, Category = Camera)
	void SetCameraFadeOutBlocked(bool bFadeOutBlocked) { bCameraFadeOutBlocked = bFadeOutBlocked; }
	// </CAMERA>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	UFUNCTION()
	void UpdateCharacterSkin();

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateCharacterSkinBP();

protected:

	/**
	 *  Checks if the character capsule can be placed in given location
	 *  if not then it tries above/below but without teleporting through floor / ceiling
	 */
	FVector CalcSafeSpawnLocationAlongZ(const FSoSplinePoint& SplinePoint, float ZValue, bool& bOutSafeFound);

	// ???
	UFUNCTION(BlueprintCallable, Category = Collision)
	void OnLandedAnimFinished() {};

	UFUNCTION(BlueprintCallable, Category = Collision)
	void DecreaseCollisionSize();
	UFUNCTION(BlueprintCallable, Category = Collision)
	void IncreaseCollisionSize();

	bool IsCollisionDecreased() const { return bCollisionAlreadyDecreased; }

	void UpdateMeshFade(float DeltaSeconds);

	// return false if there isn't enough place to stand up
	bool CanIncreaseCollision() const;

	float ModifyRollJumpLevel(int32 DeltaLevel);
	float SetRollJumpLevel(int32 Level);


	UFUNCTION(BlueprintCallable, Category = Camera)
	void FreezeCameraZ(bool bFreeze);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void FreezeCameraZToValue(float ZToFreeze);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void SetCameraXDelta(float NewDelta, bool bInstantIn = false);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void SetMinCameraZ(bool bUseMin, float MinValue = 0.0f);

	UFUNCTION(BlueprintCallable, Category = Camera)
	void SetMaxCameraZ(bool bUseMax, float MaxValue = 0.0f);

	void AddCooldown(const UObject* CooldownObject, float Duration, bool bCanCountInAir);

	UFUNCTION(BlueprintCallable, Category = Cooldown)
	void RemoveCooldown(const UObject* CooldownObject);

	UFUNCTION(BlueprintCallable, Category = Cooldown)
	void RemoveAllCooldown();

	// only applied if it is not yet added
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void AddWeaponBoost(const FSoWeaponBoost& BoostToAdd);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	void RemoveWeaponBoost(USoEffectBase* AssociatedEffect);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	const TArray<FSoWeaponBoost>& GetWeaponBoosts() { return WeaponBoosts; }


	UFUNCTION(BlueprintCallable, Category = MusicLevel)
	void OverrideAllMovementSpeed(float NewSpeed);

	UFUNCTION(BlueprintCallable, Category = MusicLevel)
	void UseMovementOverride(bool bUse, float OverrideValue);


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <camera edit functions>
#if WITH_EDITOR
	void SaveEditedData();
	void LoadEditedData();

	void SwitchCameraEditMode();
	void SwitchSkyEditMode();
	void SwitchCharShadowEditMode();

	void CreateKey();
	void MoveClosestKeyHere();
	void DeleteActiveKeyNode();

	void CopyActiveKeyData();
	void PasteToActiveKeyData();

	void JumpToNextKey();
	void JumpToPrevKey();

	void SpecialEditButtonPressed0();
	void SpecialEditButtonPressed1();

	void CtrlPressed() { bCtrlPressed = true; }
	void CtrlReleased() { bCtrlPressed = false; }

	void MiddleMouseBtnPressed() { bMiddleMousePressed = true; }
	void MiddleMouseBtnReleased() { bMiddleMousePressed = false; }
#endif
	// </camera edit functions>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Key Lights>

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PlayHitReactLightEffectBP();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PlayHealLightEffectBP();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PlayInstantLightEffectBP(FLinearColor Color, float PlayRate = 1.0f);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RefreshDefaultKeyLights();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RefreshDefaultKeyMovementLights();
	// </Key Lights>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// VARIABLES
public:
	const FName ActiveDisplayNameName = FName("DisplayName");
	const FName DoubleJumpName = FName("bDoubleJump");
	const FName UmbrellaName = FName("bUmbrella");

	const FName WallJumpDoneName = FName("bMaxWallJumpDone");
	const FName MaxBounceDoneName = FName("bMaxBounceDone");

public:
	static bool bEnableQuickSaveLoad;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Activities>
	UPROPERTY(BlueprintReadOnly)
	USoActivity* SoActivity;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoADead* SoADead;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoAHitReact* SoAHitReact;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoADefault* SoADefault;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoALillian* SoALillian;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoARoll* SoARoll;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoASlide* SoASlide;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoASwing* SoASwing;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoAAiming* SoAAiming;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoAWeaponInArm* SoAWeaponInArm;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoAStrike* SoAStrike;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoALeverPush* SoALeverPush;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoACarry* SoACarry;

	// UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	// USoACarryPickUp* SoACarryPickUp;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoACarryDrop* SoACarryDrop;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoAInUI* SoAInUI;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoAItemUsage* SoAItemUsage;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoAInteractWithEnvironment* SoAInteractWithEnvironment;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoAWait* SoAWait;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoASoAWaitForActivitySwitch* SoASoAWaitForActivitySwitch;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoATeleport* SoATeleport;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoACameraEdit* SoACameraEdit;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoASkyControlEdit* SoASkyControlEdit;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, NoClear, Instanced, Category = Activity)
	USoACharShadowEdit* SoACharShadowEdit;
	// </Activities>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

protected:

	UPROPERTY(BlueprintAssignable)
	FSoNotifyIntModified OnIntModified;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Hitreact & Checkpoint>

public:

	UPROPERTY(BlueprintAssignable)
	FSoWaitForLevelShow OnWaitForLevelShowStart;

	UPROPERTY(BlueprintAssignable)
	FSoNotify OnWaitForLevelShowEnd;

	UPROPERTY(BlueprintAssignable)
	FSoNotify OnMainLoadingScreenShow;


	UPROPERTY(BlueprintAssignable)
	FSoNotify FadeToBlack;

	UPROPERTY(BlueprintAssignable)
	FSoNotify FadeFromBlack;

	UPROPERTY(BlueprintAssignable)
	FSoNotify FadeToBlackInstant;

	UPROPERTY(BlueprintAssignable)
	FSoNotify FadeFromBlackInstant;


	/** Respawn near Soulkeeper/Checkpoint/Soultrap */
	UPROPERTY(BlueprintAssignable)
	FSoNotify OnPlayerRespawn;

	/** Respawn near Checkpoint, Soulkeeper or RematerializationPoint (after a Death or BreakIntoPeaces/FallToDeath HitReact) */
	UPROPERTY(BlueprintAssignable)
	FSoNotify OnPlayerRematerialized;

	UPROPERTY(BlueprintAssignable)
	FSoNotify OnDamageTaken;

	UPROPERTY(BlueprintAssignable)
	FSoNotifyActor OnOverlapDamageTaken;

	UPROPERTY(BlueprintAssignable)
	FSoNotifyActor OnWalkThroughMortal;


	UPROPERTY(BlueprintAssignable)
	FSoNotify OnDamageImmunityOver;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
	FSoNotify OnWorthyStatusGained;

	UFUNCTION(BlueprintCallable)
	void FireWorthyStatusGained() { OnWorthyStatusGained.Broadcast(); }

	FTimerHandle ShowCharacterTimer;

protected:
	UPROPERTY()
	USoGameInstance* GameInstance = nullptr;

	FSoSplinePoint ActiveCheckPointLocation;
	float ActiveCheckpointZLocation;

	// resurrection location
	FSoSplinePoint ResLocation;
	float ResLocationZValue = 0;

	// spawned in blueprint beginplay, hidden by default
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ResRune)
	ASoInteractableActor* ResRuneVisualActor;


	UPROPERTY(BlueprintReadWrite, Category = Health)
	bool bShieldActive = false;

	UPROPERTY(BlueprintReadOnly, Category = Health)
	float ActualOPValue = 1.0f;

	bool bLastRespawnWasSK = false;

	UPROPERTY(EditAnywhere, Category = Health)
	USoItemTemplateRuneStone* SoulKeeperSpell;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MatData)
	FVector SmokeColorDefault = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MatData)
	FVector SmokeColorRoll = FVector(1.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MatData)
	FVector SmokeColorSwing = FVector(1.0f, 0.0f, 0.0f);


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MatData)
	FVector GlowSphereColorDefault = FVector(0.0f, 0.889458f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MatData)
	FVector GlowSphereColorRoll = FVector(0.5f, 0.9f, 4.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MatData)
	FVector GlowSphereColorSwing = FVector(1.0f, 0.0f, 0.0f);

	FVector CurrentSmokeColor;
	FVector CurrentGlowSphereColor;


	// <Hitreact & Checkpoint>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Components>

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera)
	UCameraComponent* SideViewCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	USoSpringArmComponent* CameraBoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	USkeletalMeshComponent* LillianMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	FRotator WeaponLocalRot;

	//
	// Cam Effects
	//
	UPROPERTY(EditAnywhere, Category = FX)
	TSubclassOf<UCameraShake> HitReactCamShake;

	UPROPERTY(EditAnywhere, Category = FX)
	UForceFeedbackEffect* HitReactControllerVibration;

	//
	// VFX
	//

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystemComponent* WeatherVFX = nullptr;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystemComponent* CapsuleBottomVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VFX)
	UParticleSystemComponent* SoQuickTeleportPre = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VFX)
	UParticleSystemComponent* SoQuickTeleportPost = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VFX)
	UParticleSystemComponent* SoBreak = nullptr;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystemComponent* SoResVFX = nullptr;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystemComponent* SoResCPVFX = nullptr;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystemComponent* SoFloatVFXNew = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VFX)
	UParticleSystemComponent* SoNewSmoke = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VFX)
	UParticleSystemComponent* SoNewLights = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Collision)
	UStaticMeshComponent* SoGlowSphere;

	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODAudioComponent* SlideSFX;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODAudioComponent* RollSFX;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODAudioComponent* FloatStartSFX;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODAudioComponent* FloatStopSFX;

	UPROPERTY(EditAnywhere, Category = VFX)
	UFMODAudioComponent* SoResSFX = nullptr;

	UPROPERTY(EditAnywhere, Category = VFX)
	UFMODAudioComponent* SoResCPSFX = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODAudioComponent* JumpSFX;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterSheet)
	USoInventoryComponent* SoInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterSheet)
	USoPlayerCharacterSheet* SoPlayerCharacterSheet;

	/** Class writing the notes */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Progress)
	USoPlayerProgress* SoPlayerProgress;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Analytics)
	USoAnalyticsComponent* SoAnalytics;

	// weapon in right hand
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	UStaticMeshComponent* SoSword;
	// weapon in left hand
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	UStaticMeshComponent* SoOffHandWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	UParticleSystemComponent* SoSwordFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	UParticleSystemComponent* SoOffHandWeaponFX;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	UStaticMeshComponent* SoItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	USceneComponent* SoCenterNew;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoProjectileSpawnerComponent* WeaponProjectileSpawner;

	// </Components>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Dialogues>
	UPROPERTY(EditAnywhere, Category = "Dialogues")
	FSoDialogueParticipantData DialogueData;

	UPROPERTY(EditAnywhere, Category = "Dialogues")
	TMap<FName, FText> DialogueDisplayNameMap;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FSoDlgIntModified OnDlgIntModified;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FSoDlgItemChange OnDlgItemChange;
	// </Components>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Input>

	// The *Pressed tells us if the that button/key/keys is/are pressed
	UPROPERTY(BlueprintReadOnly)
	bool bRollPressed = false;

	// Some UI is blocking the game input
	bool bGameInputBlockedByUI = false;

	// Tracks all pressed UI command
	TSet<ESoUICommand> UIInputPressedCommands;

	// Tracks all pressed input action names
	TSet<FName> InputPressedActionNames;

	bool bLockForwardVecPressed = false;
	bool bUmbrellaPressed = false;
	bool bMiddleMousePressed = false;
	bool bCtrlPressed = false;
	bool bLeftMouseBtnPressed = false;
	bool bRightMouseBtnPressed = false;

	bool bSpecialStrikeWasRequestedLast = false;

	bool bJumpPressedRecently = false;
	bool bBanJumpCauseSpamming = false;

	FTimerHandle JumpPressedRecentlyTimer;
	FTimerHandle LegLandRecentlyTimer;

	// In real time
	UPROPERTY(EditAnywhere)
	float ThresholdOpenQuickSelection = 0.1f;

	UPROPERTY(BlueprintReadOnly)
	float DamageBlockCounter = -1.0f;

	UPROPERTY(EditAnywhere)
	float DamageBlockTime = 0.5f;

	UPROPERTY(EditAnywhere)
	float DamageBlockTimeMelee = 1.0f;

	UPROPERTY(BlueprintReadWrite)
	bool bBounceAndDamageOverlapSpellUsed = false;

	UPROPERTY(BlueprintReadWrite)
	float BounceAndDamageOverlapSpellDamage = 12.0f;

	// Accumulated in Tick
	float TickQuickSelectionSum = 0.f;

	// </Input>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Movement>
	// Keep track of the last used
	UPROPERTY(BlueprintReadOnly)
	FName LastTeleportedCheckpointName = FName(TEXT("sgdfhgshduifygh87rh7"));

	UPROPERTY(BlueprintReadOnly)
	bool bFloatingActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement)
	float FloatingVelocity = -200.f;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bLandedRecently = false;


	int32 ActiveRollJumpLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement)
	float LateBounceThreshold = 0.2f;

	float LastMissedBounceTime = -1.0f;
	int32 MissedBounceLevel = -1;
	float MissedBounceLocationZ = -1;

	bool bCollisionAlreadyDecreased;

	float ForcedMovementCounter = -1.0f;
	float ForcedMovementValue = 0.0f;

	/** static forward run, used for music levels */
	UPROPERTY(BlueprintReadWrite)
	bool bForceForwardRun = false;

	/** forced weapon mode, character can't take the weapon away, used for music levels */
	UPROPERTY(BlueprintReadWrite)
	bool bForceWeaponOn = false;


	// used when character spline related forward dir is fix
	FVector StoredForwardVector;

	bool bFreezeOrientationUntilLand = false;

	// SavedPostion is used to detect idle
	FVector SavedPosition;
	float IdleTime = 0.0f;

	bool bAirJumpedSinceLastLand = false;

	float LastJumpTime = -1.0f;
	float LastSwitchToAirTime = -1.0f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MovementConstValue)
	TArray<float> RollJumpLevels;

	UPROPERTY(EditFixedSize, EditAnywhere, BlueprintReadOnly, Category = MovementConstValue)
	TArray<float> RollJumpPitches;


	/** if the character jumps in leg mode the RollJumpLevel will be modified to this index. OnLand it is modified back to level 0 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MovementConstValue)
	int32 LegJumpLevelIndex = 1;

	// space & roll helpers
	float StoredMovementInputDirOnJump = -1.0f;
	float StoredTimeOnJump = -1.0f;

	// Sound
	float BounceTimeSinceLastSoundPlayed = 0.f;

	// Used to stop sound spam delay on stairs
	const float SoundTimeDelayStopSpam = 0.2f;

public:
	// used to notify objects before land/bounce
	UPROPERTY(BlueprintAssignable)
	FSoNotify OnPreLand;

	// if the character jumped right after land - true landing wasn't performed but the animation system needs this information
	UPROPERTY(BlueprintAssignable)
	FSoNotify OnLandJump;

	/** Fired if the player moves from one spline to another */
	UPROPERTY(BlueprintAssignable)
	FSoSplineChangeNotify OnPlayerSplineChanged;

	UPROPERTY(BlueprintAssignable)
	FSoPlayerSplineChangeNotify OnAreaChanged;

	UPROPERTY(BlueprintAssignable)
	FSoForceAreaEnteredDisplay OnForcedAreaChange;

	UPROPERTY(BlueprintAssignable)
	FSoNotify OnLevelSwitchStart;

	UPROPERTY(BlueprintAssignable)
	FSoNotify OnMovementModeChangedNotify;

	UPROPERTY(BlueprintAssignable)
	FSoNotifyActor OnBounceSpellUsed;

	UPROPERTY(BlueprintAssignable)
	FSoChangeUIVisibilityNotify ChangeUIVisibility;

	bool bGodMode = false;
	bool bFlyCheatOn = false;

protected:
	static const FName SpellcasterCapacityName;

	// some data for the animations, used by roll, swing
	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float DynamicAnimValue = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bAlmostLanded = false;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	FVector LastInputDirection = FVector{ 0.0f, 0.0f, 0.0f };

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	FVector LastVelocityDirection = FVector{ 0.0f, 0.0f, 0.0f };

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bForceGroundAnims = false;

	bool bUseMovementOverride = false;
	float MovementOverrideValue = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	TArray<AActor*> SkyDiveFields;
	// </Movement>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <misc>
	bool bCameraFadeOutBlocked = false;

	// Track for how long a certain value is for
	float PreviousGamepadRightX = 0.f;
	float PreviousGamepadRightY = 0.f;
	float PreviousGamepadLeftX = 0.f;
	float PreviousGamepadLeftY = 0.f;

	int32 LeftThumbstickSameValueFrames = 0;
	int32 RightThumbstickSameValueFrames = 0;

	float GamepadRightX = 0.f;
	float GamepadRightY = 0.f;
	float GamepadLeftX = 0.f;
	float GamepadLeftY = 0.f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USoWizard> WizardClass;

	UPROPERTY()
	USoWizard* SoWizard;

	UPROPERTY()
	UDlgContext* InterruptedWizardDialogue = nullptr;

	// Used to create the UISystem
	UPROPERTY(EditAnywhere, Category = UI)
	TSubclassOf<USoUISystem> UISystemClass;

	// Root UI system
	UPROPERTY(BlueprintReadOnly)
	USoUISystem* UISystem = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FSoSkyControlValue SkyControlValue;

	UPROPERTY()
	UFMODEvent* MusicOverride = nullptr;

	UPROPERTY()
	UObject* MusicOverrideRequester = nullptr;

	UPROPERTY()
	TArray<ASoBounceSFXOverrideBox*> BounceSFXOverrideStack;

	// Keep track of the last modified gold amount
	UPROPERTY(BlueprintReadOnly)
	int32 LastModifiedGoldAmount = 0;

	// Cached Current device Type
	UPROPERTY(BlueprintReadOnly)
	ESoInputDeviceType DeviceType = ESoInputDeviceType::Keyboard;


	// Keep track of the last modified gold amount
	UPROPERTY(BlueprintReadOnly)
	bool bLastRespawnWasRetreat = false;

public:

	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFClothEquip = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXWeaponEquip = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXUsableEquip = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXPotionEquip = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFItemGained = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFOnMeleeHit = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFOnMeleeHitDeath = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFLegJump = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFLegLand = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXArmedLand = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFFallToDeath = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFBreakIntoPieces = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFCanNot = nullptr;

	UPROPERTY(EditFixedSize, EditAnywhere, Category = SFX)
	TArray<UFMODEvent*> SFXBounceVariants;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXSlideCrouchStart = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXSlideCrouchEnd = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXSlideJump = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXRollEnter = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXRollLeave = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXRollEnterNoMovement = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXRollLeaveNoMovement = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXWeaponSwitch = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXItemSwitch = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXCarryStart = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXCarryStop = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXArmedStart = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXArmedStop = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXSwing = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXSwingEnd = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXBounceOnEnemy = nullptr;

	//
	// VFX
	//

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystem* VFXBounceSmall = nullptr;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystem* VFXBounceLarge = nullptr;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystem* VFXSlide = nullptr;

protected:
	// </misc>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <strikes>

protected:
	UPROPERTY(BlueprintAssignable)
	FSoCooldownChanged CooldownStarted;

	UPROPERTY(BlueprintAssignable)
	FSoCooldownChanged CooldownEnded;

	UPROPERTY(BlueprintAssignable)
	FSoCooldownChanged CooldownBlocksEvent;


	UPROPERTY(BlueprintReadOnly)
	FSoAnimationSet ArmedAnimationSetNew;


	UPROPERTY(EditAnywhere, Category = Strikes)
	UCurveFloat* TrailCurve;

	UPROPERTY()
	TArray<AStaticMeshActor*> WeaponTrailMeshes;

	UPROPERTY()
	TArray<FSoWeaponTrailFade> WeaponTrailMeshFadeList;

	UPROPERTY(BlueprintReadOnly)
	TArray<FSoCooldownCounter> Cooldowns;


	UPROPERTY(BlueprintReadOnly)
	TArray<FSoWeaponBoost> WeaponBoosts;


	bool bForceHideLeftWeapon = false;
	bool bForceHideRightWeapon = false;
	FTimerHandle ForceHideLeftWeaponOver;
	FTimerHandle ForceHideRightWeaponOver;
	// </strikes>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Interactables>
	TArray<AActor*> Interactables;

	int32 ActiveInteractable = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bShowInteractionTip = true;

	UPROPERTY(BlueprintReadOnly)
	bool bForceHideInteractionTip = false;

	UPROPERTY(BlueprintReadWrite, Category = Interaction)
	ASoCarryable* CarriedStuff = nullptr;

	TArray<ASoSwingCenter*> SwingCenters;

	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FSoNotify OnInteractionLabelChanged;

	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FSoNotify OnSoulkeeperPickedUp;

	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FSoNotify OnSoulkeeperPlaced;

	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FSoNotify OnInteractableMessageRequested;

	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FSoNotify OnInteractableMessageHideRequest;


	UPROPERTY(BlueprintReadOnly, Category = Interaction)
	bool bCanUseSoulkeeper;

	UPROPERTY(BlueprintAssignable)
	FSoNotify CanUseSoulkeeperChanged;
	// </Interactables>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <PostProcess shadow>

	/** only reference */
	UPROPERTY()
	APostProcessVolume* LevelPostProcessVolume;

	UPROPERTY(EditAnywhere, Category = ShadowMaterial)
	UMaterial* ShadowMaterialPostProcess;

	/** dynamic post process material used for fake character shadows */
	UPROPERTY()
	UMaterialInstanceDynamic* ShadowMaterialPostProcessDynamic = nullptr;

	bool bShadowWasEnabled = false;

	/** fake shadow map renderer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShadowMaterial)
	USceneCaptureComponent2D* SoScreenCapture;

	UPROPERTY(BlueprintReadOnly)
	int32 ActiveShadowKey = 0;
	// <PostProcess shadow>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Update Lists>

	/** Meshes currently fading in/out based on the FadeIn/FadeOut calls */
	UPROPERTY()
	TArray<FSoFadeEntry> MeshesToFade;

	/** Faded entries - they are simply set to invisible and awiting FadeIn() call */
	UPROPERTY()
	TArray<FSoFadeEntry> FadedMeshes;

	UPROPERTY()
	TArray<FSoFadeAndKillDestructible> FadeAndKillList;


	UPROPERTY()
	TArray<USceneComponent*> OrientTowardsCameraList;

	UPROPERTY()
	TArray<USceneComponent*> EnemyHPWidgetList;

	// </Update Lists>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <Camera>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
	int32 ActiveCamKey = 0;

	float SavedCamRelative = BIG_NUMBER;
	float LastLandZ;

	bool bUseSavedCameraBoomZ = false;
	// when the character capsule changes (switch to/from roll, jump/land) the collision is temporary disabled
	// in this case the collision areas are left and entered again, which can ruin the saved cameraZ, that is blocked with this bool
	bool bSavedCameraBoomZUpdateBlock = false;
	float SavedCameraBoomZ = 0.0f;
	// target used to produce a smooth blend between the original Z and the one which is forced if we want to freeze a fix Z value
	float SavedCameraBoomZSource = 0.0f;
	float SavedCameraBoomZTarget = 0.0f;
	float SavedCameraBoomZCounter = 0.0f;
	const float SavedCameraBoomZBlendTime = 1.3f;

	bool bUseCamMinZValue = false;
	float MinCamZValue = 0.0f;

	bool bUseCamMaxZValue = false;
	float MaxCamZValue = 0.0f;

	float CameraUnfreezeCounter = 0.0f;
	float CameraUnfreezeTime = 1.0f;

	float CameraXDeltaSource = 0.0f;
	float CameraXDeltaTarget = 0.0f;
	float CameraXDeltaCurrent = 0.0f;

	float CameraXDeltaCounter = 0.0f;
	const float CameraXDeltaBlendTime = 1.3f;

	int32 SavedCamMovModifier = 1;
	// used when we can not reset it directly because the camera is not yet updated and it would be overridden with wrong value again
	bool bResetSavedCamMovModifierAfterTick = false;

	// movement is blocked if it is 0, resets if the input is 0
	// can be used to force stop the character until the player releases and presses the button again
	int32 MovementStopMultiplier = 1;

	bool bRotatedCamera = false;

	int32 MovDir = 1;
	float HorizontalOffset = 0;

	// Absolute time in seconds when the game was saved
	float LastAutoSaveTime = 0.0f;

	UPROPERTY(BlueprintAssignable)
	FSoNotify ToggleUI;
	// </Camera>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

public:

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <CONST VALUES>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pause)
	float MaxIdleTime = 15.0f;

	// if any of the heights change (from 42/72) the const values used in SoAActivity with JumpZVelocity should be recalculated
	// the original jump height we want to keep is 285.102
	// h = v0 * v0 / (2*g)
	// when the collision is decreased the value (1057.36) can stay
	// when the collision is not decreased before jump the character capsule is pushed so that the top of the decreased capsule is at the same location
	// as the top of the original capsule, the jump velocity must be corrected so that it reaches the same height (285.102)
	// current difference is 117.82, used in USoActivity::JumpPressed()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Capsule)
	float DecreasedHeight = 42.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Capsule)
	float NormalHeight = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Capsule)
	float Mass = 127.666199f;

	bool bDebugSkipDialogues = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MovementConstValue)
	float ForcedToMoveTimeAfterHit = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MovementConstValue)
	float ForcedToMoveTimeAfterWallJump = 0.30f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MovementConstValue)
	float QuickTeleportPreTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MovementConstValue)
	float QuickTeleportPostTime = 0.2;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Achievements)
	FSoConditionalAchievement AchievementSelfTrained;
	// </CONST VALUES>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

protected:
	FTimerManager* gTimerManager = nullptr;

	//
	// Our friends
	//
	friend class USoActivity;
	friend class USoAAiming;
	friend class USoARoll;
	friend class USoASwing;
	friend class USoASlide;
	friend class USoADefault;
	friend class USoALillian;
	friend class USoALeverPush;
	friend class USoACarry;
	// friend class USoACarryPickUp;
	friend class USoACarryDrop;
	friend class USoAWeaponInArm;
	friend class USoAStrike;
	friend class USoADead;
	friend class USoAHitReact;
	friend class USoATeleport;
	friend class USoACameraEdit;
	friend class USoASkyControlEdit;
	friend class USoACharShadowEdit;
	friend class USoAInUI;
	friend class USoAItemUsage;
	friend class USoAInteractWithEnvironment;
	friend class USoASoAWaitForActivitySwitch;
	friend class USoASoAWait;
	friend class ASoPreviewCharacter;

	friend class USoCharacterMovementComponent;
	friend class USoPlayerCharacterSheet;
};
