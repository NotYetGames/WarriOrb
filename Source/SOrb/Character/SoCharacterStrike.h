// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Animation/AnimSequenceBase.h"
#include "CharacterBase/SoIMortalTypes.h"
#include "Logic/SoCooldown.h"
#include "SoCharacterStrike.generated.h"

class UTexture2D;
class USoEffectBase;
class ASoProjectile;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoJump
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere)
	float Time;

	UPROPERTY(EditAnywhere)
	float Velocity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoHorizontalMovement
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere)
	float StartTime;

	UPROPERTY(EditAnywhere)
	float EndTime;

	UPROPERTY(EditAnywhere)
	float Velocity;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoStrikeEntry
{
	GENERATED_USTRUCT_BODY()

public:

	/** Time between strike start and trail display start */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float TrailDelay = 0.0f;

	/** Does not effect trail, only the actual time the hit check is executed, has to be greater than or equal to TrailDelay */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float StrikeDelay = 0.1f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float StrikeDmgDuration = 0.1f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	UStaticMesh* TrailMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	bool bAttachToCharacter = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float TrailDisplayTime = 0.6f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	int32 TranslucencySortPriority = 0;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	TArray<TSubclassOf<USoEffectBase>> HitEffects;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoCharacterStrikeData
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	FSoRootMotionDesc RootMotionDesc;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	bool bStopVerticalRootMotionNearApex = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	bool bBlendIn = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	UAnimSequenceBase* AnimSequenceNew;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	UAnimSequenceBase* AnimEndingSequenceLeft;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	UAnimSequenceBase* AnimEndingSequenceRight;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float AnimDuration = 1.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float AnimEndDuration = 0.2f;


	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	TArray<FSoStrikeEntry> StrikeList;

	/** if true and the character isn't in air the strike is delayed and the character executed a jump first */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	bool bRequiresJump = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	TArray<FSoJump> Jumps;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	TArray<FSoHorizontalMovement> HorizontalMovements;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float IgnoreWallHitBellowZThreshold = -5.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	bool bFreezeInAir = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	bool bStartWithLand = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	bool bForceGroundAnimsOnBlendOut = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float RangeAttackDelay = -1.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	TSubclassOf<ASoProjectile> ProjectileClass;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	int32 WeaponToHideIndex = -1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float WeaponHideDuration = 0.2f;
};

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class SORB_API USoCharacterStrike : public UObject, public ISoCooldown
{
	GENERATED_BODY()

public:

	/** save/load helper */
	static USoCharacterStrike* GetTemplateFromPath(const FString& Path);

	USoCharacterStrike();

#if WITH_EDITOR
	void PreSave(const ITargetPlatform* TargetPlatform) override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedChainEvent) override;
#endif

	// ICooldown
	virtual float GetCooldownDuration_Implementation() const override { return GetCooldownTime(); }
	virtual UTexture2D* GetCooldownIcon_Implementation() const override { return CooldownIcon; }
	virtual bool CanCountDownInAir_Implementation() const override { return bCooldownReducedInAir; }

	int32 GetStrikeNum() const { return CharacterStrikes.Num(); }
	const FSoCharacterStrikeData& GetStrikeData(int32 Index) const;

	const TArray<FSoCharacterStrikeData>& GetCharacterStrikes() const { return CharacterStrikes; }

	float GetCooldownTime() const;
	UTexture2D* GetCooldownIcon() const { return CooldownIcon; }

	ESoStatusEffect GetStatusEffect() const { return EffectToApply; }

protected:
	virtual void UpdateLocalizedFields();

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	TArray<FSoCharacterStrikeData> CharacterStrikes;

	/** Only used if > 0.0f, using the strike in chain ignores cooldown */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	float CooldownTime = -1.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	FText StrikeName;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	UTexture2D* CooldownIcon;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	bool bCooldownReducedInAir = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = StrikeData)
	ESoStatusEffect EffectToApply = ESoStatusEffect::ESE_NumOf;

	// Used for all FTexts
	static FString TextNamespace;
};
