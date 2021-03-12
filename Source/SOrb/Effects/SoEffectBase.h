// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

#include "CharacterBase/SoIMortalTypes.h"

#include "SoEffectBase.generated.h"

class ASoCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogSoEffectSystem, Log, All);

DECLARE_STATS_GROUP(TEXT("SoEffect"), STATGROUP_SoEffect, STATCAT_Advanced);

////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoEffectType : uint8
{
	/** effect dies after Duration sec */
	EET_TimeBased			UMETA(DisplayName = "TimeBased"),
	/** one time effect */
	EET_Instant				UMETA(DisplayName = "Instant"),
	/** effect is on as long as it is not removed */
	EET_Item				UMETA(DisplayName = "Item"),
	/** effect is used when *something* happens, child has to update the *something* counter */
	EET_Counted				UMETA(DisplayName = "Counted"),

	EET_NumOf				UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ESoDisplayText : uint8
{
	EDT_Immune			UMETA(DisplayName = "Immune"),
	EDT_Resisted		UMETA(DisplayName = "Resisted"),
	EDT_SecondChance	UMETA(DisplayName = "Second Chance"),
	EDT_Blocked			UMETA(DisplayName = "Blocked"),
	EDT_Infected		UMETA(DisplayName = "Infected"),

	EDT_NoNeadForHeal	UMETA(DisplayName = "NoNeedForHeal"),
	EDT_MissingTarget	UMETA(DisplayName = "MissingTarget"),

	EDT_CantUseInSKFreeZone		UMETA(DisplayName = "CantUseSKFreeZone"),
	EDT_CantUseInAir			UMETA(DisplayName = "CantUseInAir"),

	EDT_SpellRegained			UMETA(DisplayName = "SpellRegained"),
	EDT_InstantRecharge			UMETA(DisplayName = "InstantRecharge"),

	EDT_HPGained				UMETA(DisplayName = "HPGained"),
	EDT_SpellCapacityGained		UMETA(DisplayName = "SpellCapacityGained"),

	EDT_Max				UMETA(DisplayName = "Invalid") // Merch actually uses it now rofl
};


/**
 *
 */
UCLASS(Blueprintable, BlueprintType, Abstract)
class SORB_API USoEffectBase : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	void PreSave(const ITargetPlatform* TargetPlatform) override;
#endif // WITH_EDITOR

	UFUNCTION(BlueprintNativeEvent, Category = General)
	bool CanBeApplied(AActor* TargetOwner);

	ESoDisplayText GetCantBeAppliedReason() const { return CantBeAppliedReason; }

	/** Return false: remove */
	bool Apply(AActor* InOwner);

	/** called on timebased/counted effects if another is applied from the same type */
	UFUNCTION(BlueprintNativeEvent, Category = General)
	bool Reapply();

	/** Called when the effect is forced stopped - e.g. some has to be if the owner ends up dead, item based if item is removed, etc. */
	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void Interrupt();

	// functions called after construction to decide what functions will be called on this instance
	bool ShouldReceiveTick() const { return bKidWantsOnTick || EffectType == ESoEffectType::EET_TimeBased || EffectType == ESoEffectType::EET_Counted; }

	bool ShouldReceiveNotifyOnDeath() const { return bGetNotifyOnDeath; }
	bool ShouldBeDestroyedOnSpellReset() const { return bBeDestroyedOnSpellsReset; }
	bool ShouldBeDestroyedOnDeath() const { return bBeDestroyedOnDeath; }

	bool ShouldReceiveNotifyOnMeleeHit() const { return bGetNotifyOnMeleeHit; }
	bool ShouldReceiveNotifyOnMeleeHitTaken() const { return bGetNotifyOnMeleeHitTaken; }
	bool ShouldReceiveNotifyOnHitTaken() const { return bGetNotifyOnHitTaken; }
	bool ShouldReceiveNotifyOnWalkThroughEnemies() const { return bGetNotifyOnWalkThroughEnemies; }

	ESoEffectType GetEffectType() const { return EffectType; }

	/** optional functions */

	/** Return false: has to be removed */
	bool Tick(float DeltaSeconds);


	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnOwnerDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnOwnerMeleeHit(AActor* Target, const FSoMeleeHitParam& Param);

	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnOwnerMeleeHitTaken(const FSoMeleeHitParam& Param);

	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnOwnerHitTaken(const FSoDmg& Dmg, const FSoHitReactDesc& HitReact);

	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnWalkThroughMortal(AActor* Mortal);

	UFUNCTION(BlueprintPure, Category = "General")
	UTexture2D* GetEffectIcon() { return DisplayIcon; }

	/** works with time based effects, returns 1.0f at start and 0.0f at the end */
	UFUNCTION(BlueprintPure, Category = TimeBased)
	float GetPercent();

	const TArray<TSubclassOf<USoEffectBase>>& GetEffectsToRemoveOnApply() const { return EffectsToRemoveOnApply; }

protected:

	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnTick(float DeltaSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnApplied();

	/** called when a timebased or counted effect is finished */
	UFUNCTION(BlueprintImplementableEvent, Category = "TimeBased")
	void OnDurationOver();

	UFUNCTION(BlueprintCallable, Category = "Counted")
	void IncreaseCount();

	UFUNCTION(BlueprintCallable, Category = "Counted")
	void PrintMessage(const FString& TextToPrint);

	UFUNCTION(BlueprintCallable, Category = "Counted")
	void PrintError(const FString& TextToPrint, bool bJustWarning = false);

	UFUNCTION(BlueprintPure, Category = "Helper")
	ASoCharacter* GetSoChar();

	UFUNCTION(BlueprintCallable, Category = "Helper")
	void UnlockAchievement(AActor* WorldContext, FName Name);

	virtual void UpdateLocalizedFields();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	ESoEffectType EffectType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bKidWantsOnTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bBeDestroyedOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bGetNotifyOnDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bBeDestroyedOnSpellsReset = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bGetNotifyOnMeleeHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bGetNotifyOnMeleeHitTaken = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bGetNotifyOnHitTaken = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bGetNotifyOnWalkThroughEnemies = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	TArray<TSubclassOf<USoEffectBase>> EffectsToRemoveOnApply;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeBased")
	float Duration = 0.0f;

	float Counter = 0.0f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Counted")
	int32 MaxCount = 1;

	/** how much time happened the counted effect already */
	int32 CountNum = 0;

	/** Owner, has to implement USoMortal interface with a valid GetCharacterSheet() override */
	UPROPERTY(BlueprintReadOnly, Category = "General")
	AActor* Owner = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General|UI")
	UTexture2D* DisplayIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General|UI")
	FLinearColor DisplayIconColor = FLinearColor{ 1.0f, 1.0f, 1.0f, 1.0f };

	/** tooltip line */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General|UI")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General|UI")
	ESoDisplayText CantBeAppliedReason = ESoDisplayText::EDT_Max;

	// Used for all FTexts
	static FString TextNamespace;
};
