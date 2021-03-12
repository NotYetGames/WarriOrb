// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once
#include "Animation/AnimSequenceBase.h"
#include "SoActivity.h"
#include "SoAStrike.generated.h"

class USoWeaponTemplate;
class AStaticMeshActor;

/**
 * Dynamic data for animation blueprint
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoStrikeAnimData
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequenceBase* Anim = nullptr;

	/** may be nullptr, animation state is skipped then, but we have a 0.2 sec blend time */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequenceBase* AnimEndBlend = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequenceBase* AnimEndBlendLeft = nullptr;

	/** play rates are calculated from strike duration so the animation is aligned to the logic */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AnimPlayRate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AnimEndBlendPlayRate;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoAStrike : public USoActivity
{
	GENERATED_BODY()
public:
	USoAStrike();

	virtual void Tick(float DeltaSeconds) override;

	virtual void StrikePressed() override;

	virtual void OnBlocked() override;

	virtual void ToggleWeapons() override {};

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override;

	virtual void OnLanded() override;

	virtual void TakeWeaponAway() override {};

	// checks if there is an equipped weapon with a valid strike which isn't waiting for cooldown
	// if strile fails cause of cooldown cannot sound is played
	bool CanStrike(bool bSpecial, bool bSpawnSFXIfNot = true);

	// checks if the given strike has cooldown at all (not just an active one)
	bool HasStrikeCooldown(bool bSpecial) const;

	void ToggleDmgCheat() { bDmgBoostCheat = !bDmgBoostCheat; }

	float GetStrikePressedRecentlyTimeOffset() const;

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

	virtual void Move(float Value) override;

	void Strike();

	void ForceLand();

	void OnStrikeChainFinished(float BlendTimeToSubtractFromCounter);

	void ApplyTrail(const struct FSoStrikeEntry& StrikeData, bool bAllowTurn);
	void ApplyDamage(const struct FSoStrikeEntry& StrikeData, UStaticMeshComponent* TrailComponent, float WallHitThreshold, int32 StrikeIndex);

	// used by the mechanism which stops the user from using combos via spamming lmb
	void StrikePressedRecentlyTimeOver();

	void UnhideWeapon();

	bool AutoAim();

	bool WouldHitMortals();


protected:

	bool bStrikePressedRecently = false;
	bool bBanStrikeCauseSpamming = false;
	float StrikePressedRecentlyTimeOffset = 0.5;
	FTimerHandle StrikePressedRecentlyTimer;

	/** damn it's too late to write sane comments */
	int32 StrikeComboChainIndex = -1;


	/** used for double buffering and stuff for animation system, 0th, 2th, 4th etc. anims are StrikeA, others are not */
	UPROPERTY(BlueprintReadOnly)
	bool bStrikeA = true;

	UPROPERTY(BlueprintReadOnly)
	bool bStrikeBlendIn = true;

	/** strike in progress if true, blending out or state not active otherwise  */
	UPROPERTY(BlueprintReadOnly)
	bool bIsInStrike = false;

	/** two different data used in ping-pong style for the animation system to work correctly */
	UPROPERTY(BlueprintReadOnly)
	FSoStrikeAnimData AnimDataA;

	UPROPERTY(BlueprintReadOnly)
	FSoStrikeAnimData AnimDataB;

	bool bSpecialStrikeWasExecutedLast = false;

	float StrikeCounter = -1.0f;
	float StrikeCounterStart;
	float ActiveBlendOutTime;

	float AttackSpeedMultiplier = 1.0f;

	const struct FSoCharacterStrikeData* ActiveStrikeData = nullptr;

	UPROPERTY()
	TArray<UStaticMeshComponent*> TrailMeshes;

	UPROPERTY()
	const USoWeaponTemplate* ActiveWeaponTemplate = nullptr;

	float RangeAttackCounter = 0.0f;
	bool bWaitBeforeStrike = false;

	float MovementOverride = 0.0f;

	friend class USoAWeaponInArm;

	bool bFreezeInAir = false;
	ESoStatusEffect AppliedEffect = ESoStatusEffect::ESE_NumOf;
	float LandingAmount = -1.0f;

	bool bSpawnedCanNotSFX = false;
	float ActiveAutoAimDistance = 400.0f;

	UPROPERTY()
	AStaticMeshActor* ActiveTrailMesh = nullptr;


	// only max 2 strike is used atm, this should be array if there would be more
	UPROPERTY()
	TArray<UPrimitiveComponent*> ComponentsAlreadyHitA;

	UPROPERTY()
	TArray<UPrimitiveComponent*> ComponentsAlreadyHitB;

	bool bDmgBoostCheat = false;
};
