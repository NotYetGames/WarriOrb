// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once
#include "UObject/Object.h"
#include "Logging/LogMacros.h"
#include "CharacterBase/SoIMortalTypes.h"
#include "SoActivityTypes.h"
#include "Items/SoItemTypes.h"
#include "UI/General/SoUITypes.h"

#include "SoActivity.generated.h"

struct FHitResult;
struct FKey;
class UAnimSequenceBase;

enum class ESoLookDirection : uint8
{
	/** Orient rotation to movement, but can be locked via holding a button (shift?) */
	ELD_Movement,
	/** Look direction is determined before the activity and it stays like that */
	ELD_Frozen,
	/** Character always look to the direction the player want the character to move */
	ELD_Input
};

enum class ESoJumpType : uint8
{
	EJT_Normal,
	EJT_Bounce,
	EJT_BounceIfPressed
};

struct FSoSplinePoint;
class ASoCharacter;

UCLASS(BlueprintType, EditInlineNew, Abstract)
class SORB_API USoActivity : public UObject
{
	GENERATED_BODY()

public:
	USoActivity() {};
	USoActivity(EActivity ActivityID) : ID(ActivityID) {};

	void SetOwner(ASoCharacter* Owner) { Orb = Owner; }

public:

	virtual void PostInitProperties() override;

	// leaves the current activity (*this) and activates the new one
	// this is a forced thing, if the switch is cause by User input CanInterruptActivity() should be called before to check if this function should be called or not
	void SwitchActivity(USoActivity* NewActivity);

	virtual void Tick(float DeltaSeconds);

	virtual void UpdateCamera(float DeltaSeconds);
	virtual void UpdateCharMaterials(float DeltaSeconds);
	virtual void UpdateFloating();

	// events
	UFUNCTION(BlueprintCallable, Category = Activity)
	virtual void OnAnimEvent(EAnimEvents Event);

	UFUNCTION(BlueprintCallable, Category = Collision)
	virtual void IncreaseCollisionSize();

	// called with true if we are being pushed
	// called with false in normal frame update
	void UpdateOrientation(bool bPushedOnSpline = false);

	virtual void OnBaseChanged(AActor* ActualMovementBase);

	// return value: true if you are still alive
	virtual bool DecreaseHealth(const FSoDmg& Damage);

	// so it was false after all...
	virtual void OnDeath();
	// who cares???
	virtual void OnUmbrellaOverlap() {};
	// Nope!
	virtual bool ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult) const { return false; }
	// return false: don't land
	virtual bool OnPreLanded(const FHitResult& Hit);

	// what did I just told you?
	virtual void OnBounce(bool bWallJump, float NewStoredMoveValue, const FVector& HitPoint, const FVector& HitNormal) { check(false); }

	virtual void OnLanded() {};

	// inputs
	// axis updates
	virtual void Move(float Value);
	// Attack! Or? How knows...
	virtual void StrikePressed();

	// if you say left u shall say right too
	virtual void RightBtnPressed();
	// button pressed/released
	// Jump!
	virtual void JumpPressed();

	virtual void SuperEditModePressed() {};

	virtual void TakeWeaponAway();
	virtual void RollPressed();


	virtual void ToggleWeapons();
	virtual void ToggleItems();
	virtual void ToggleSpells(bool bQuickSelectionMode);
	virtual void UseItemFromSlot0();

	// let's do something... with something :/
	virtual void InteractKeyPressed(bool bPrimary);
	virtual void OnInteract(AActor* Interactable);

	UFUNCTION(BlueprintCallable, Category = Collision)
	virtual bool CanInteract() const;

	virtual void StartLeverPush(const FSoLeverData& LeverData, const FSoSplinePoint& SplinePoint, float ZValue);


	virtual void DebugFeature();

	// save/load from file
	virtual void SaveEditedData() {};
	virtual void LoadEditedData() {};
	// add/move/remove key node
	virtual void CreateKey() {};
	virtual void MoveClosestKeyHere() {};
	virtual void DeleteActiveKeyNode() {};
	// modifies stuff in edit modes
	virtual void SpecialEditButtonPressed(int32 Index) {};

	virtual void CopyActiveKeyData() {};
	virtual void PasteToActiveKeyData() {};
	// teleport the character into the KeyNode position
	virtual void JumpToNextKey() {};
	virtual void JumpToPrevKey() {};

	virtual void LeftBtnPressed() {};

	virtual bool CanPauseOnIdle() const { return true; }


	virtual bool OnDmgTaken(const FSoDmg& Dmg, const FSoHitReactDesc& HitReactDesc);
	virtual bool OnMeleeHit(const FSoMeleeHitParam& HitParam);
	virtual void OnBlocked() {};

	virtual void OnPushed(const FVector& DeltaMovement,
						  float DeltaSeconds,
						  bool bStuck,
						  AActor* RematerializeLocation,
						  int32 DamageAmountIfStuck,
						  bool bForceDamage);

	FORCEINLINE EActivity GetID() const { return ID; }

	FORCEINLINE bool IsArmedState() const { return bArmed; }
	FORCEINLINE virtual bool CanBeArmedState() const { return IsArmedState(); }

	UFUNCTION(BlueprintCallable, Category = Umbrella)
	FORCEINLINE float GetUmbrellaPlayRate() const { return UmbrellaPlayRate; }


	// called every frame, increases/decreases collision
	// for simple case override ShouldDecreaseCollision() if necessary
	// for complicated stuff override the HandleCollision function
	virtual void HandleCollision();

	// if an activity requires smaller collision this function should be overriden and return true
	// if the characer is in the air he has smaller collision anyway
	virtual bool ShouldDecreaseCollision() const;

	virtual bool ShouldUpdateMeshFade() const { return true; }

	// returns false if the current activity isn't interruptible (by user input)
	// kinda weak try, used when the user wants to switch action
	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const { return false; }

	virtual bool CanOpenCharacterPanels() const { return true; }

	// revive/teleport
	virtual bool BlocksTeleportRequest() const { return false; }
	virtual void OnTeleportRequest() {};
	virtual bool IsSaveAllowed() const { return true; }

	bool IsInMultipleGamepadCommandsWhitelist(ESoUICommand Command) const;
	bool CanEscapeToMenu(FKey Key, ESoUICommand Command) const;
	bool CanHandleUICommand(ESoUICommand Command) const;
	void ForwardCommandToMainMenu(ESoUICommand Command);
	virtual void HandleUICommand(FKey Key, ESoUICommand Command);

	// switch to default/armed/roll mode based bArmedAllowed, last activity, collision size and available space
	void SwitchToRelevantState(bool bArmedAllowed);

	virtual void StartFloating();
	virtual void StopFloating();

	void OverrideMovementSpeed(float Value) { MovementSpeed = Value; }

	UFUNCTION(BlueprintCallable)
	void OverrideAllMovementSpeed(float Value);

	// used in editor activities
	void OnSuperModeChange(bool bEnter);
	void SuperModeTick(float DeltaSeconds);

	float GetMovementSpeed() const { return MovementSpeed; }

protected:

	virtual void OnEnter(USoActivity* OldActivity);
	virtual void OnExit(USoActivity* NewActivity);
	// called on old activity once the new activity became active, can be used to force another activity switch
	virtual void OnPostExit(USoActivity* NewActivity) {};

	// sometimes we have to set the roll jump level if we jump
	virtual void OnJumped();

	virtual bool CanJumpFromAir() const { return false; }


	bool OnPreLandedBounce(const FHitResult& Hit);


	void SetEnabledState();
	void SetDisabledState();



	void ClearCooldownInAirFlags();

	void UpdateRollAnimDynamicValue(float SpeedMultiplier = 1.0f);

public:
	static const FName TrampolineTag;
	static const FName SlipperyPosTag;
	static const FName SlipperyNegTag;
	static const FName RollOnlySurface;
	static const FName NoWallJumpSurface;
	static const FName NoBounceSurface; // wall bounce only atm

	static const FName TriggerOnStand; // character triggers it if it becomes base

	static const FName EnabledCollisionProfileName;
	static const FName DisabledCollisionProfileName;

protected:

	ASoCharacter* Orb = nullptr;
	const EActivity ID = EActivity::EA_Max;

	ESoLookDirection LookDirection = ESoLookDirection::ELD_Movement;
	ESoJumpType JumpType = ESoJumpType::EJT_Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementSpeed = 450;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanHaveDefaultKeyLights = true;

	/** in non-roll mode we have to calculate with 939.36 (-117.82) if the capsule is not yet decreased because of the movement */
	UPROPERTY(EditAnywhere)
	float JumpZVelocity = 1057.18;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DefaultBounceDampening = 0.6;

	bool bStaminaReg = true;

	// used to decide if we have to return to WeaponInArm or to Default after an interruption (e.g. HitReact)
	// or if we have to put away our weapons or not in OnExit()
	UPROPERTY(BlueprintReadOnly)
	bool bArmed = false;

	// used to temporary disable orientrotationtomovement
	bool bWasPushedThisFrame = false;

	// if umbrella is used this value has to be modified to 1.0f in child state for the "animation"
	float UmbrellaPlayRate = -1.0f;

	/** */
	UPROPERTY(BlueprintReadOnly)
	USoActivity* LastActivity = nullptr;

	// Sound
	float PreLandedBounceGroundTimeSinceLastSoundPlayed = 0.f;
};
