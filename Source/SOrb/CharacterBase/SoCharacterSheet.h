// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Components/ActorComponent.h"
#include "SoIMortalTypes.h"
#include "UI/General/SoUITypes.h"

#include "SoCharacterSheet.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SORB_API USoCharacterSheet : public UActorComponent
{
GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USoCharacterSheet();

	virtual void InitializeComponent() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = Health)
	void RestoreHealth();

	UFUNCTION(BlueprintCallable, Category = Health)
	void SetMaxHealth(float NewValue, bool bModifyCurrent = true);

	/** value is clamped in [0, MaxHealth], returns true if we are still alive */
	UFUNCTION(BlueprintCallable, Category = Health)
	bool SetHealth(float NewValue);

	UFUNCTION(BlueprintCallable, Category = Health)
	void IncreaseHealth(float Delta);


	UFUNCTION(BlueprintCallable, Category = Health)
	void AddBonusHealth(const FSoDmg& BonusHealthAmount);

	UFUNCTION(BlueprintCallable, Category = Health)
	void ClearBonusHealth();


	UFUNCTION(BlueprintCallable, Category = Health)
	FSoDmg AddRes(const FSoDmg& Res);

	UFUNCTION(BlueprintCallable, Category = Health)
	void RemoveRes(const FSoDmg& Res);


	// return: true if we are still alive, damage resistances reduce the damage
	virtual bool ApplyDmg(const FSoDmg& Damage);

	FSoDmg GetReducedDmg(const FSoDmg& Damage);

	UFUNCTION(BlueprintPure, Category = Health)
	bool IsAlive() const { return CurrentHealthPoints > KINDA_SMALL_NUMBER; }


	UFUNCTION(BlueprintPure, Category = Health)
	float GetHealth() const { return CurrentHealthPoints; }

	UFUNCTION(BlueprintPure, Category = Health)
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = Health)
	float GetHealthPercent() const { return CurrentHealthPoints / GetMaxHealth(); }

	float GetPhysicalResistance() const { return Resistance.Physical; }
	float GetMagicResistance() const { return Resistance.Magical; }

	UFUNCTION(BlueprintPure)
	const FSoDmg& GetBonushHealth() const { return BonusHealth; }

	UFUNCTION(BlueprintPure)
	float GetBonushHealthSum() const { return BonusHealth.Sum(); }

public:

	UPROPERTY(BlueprintAssignable, Category = UI)
	FSoUINotifyTwoFloat OnHealthChanged;

	/** Param is the actual bonus health */
	UPROPERTY(BlueprintAssignable, Category = UI)
	FSoUINotifyDmg OnBonusHealthChanged;

	/** applied dmg after bonus health might drained some dmg */
	UPROPERTY(BlueprintAssignable, Category = UI)
	FSoUINotifyDmg OnDmgApplied;

	UPROPERTY(BlueprintAssignable, Category = UI)
	FSoUINotifyDmg OnDmg;

	/** health is reduced to 0 */
	UPROPERTY(BlueprintAssignable, Category = Health)
	FSoUINotify OnPreDeath;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealthPoints = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CurrentHealthPoints = 30.0f;

	UPROPERTY(EditAnywhere)
	FSoDmg Resistance;

	UPROPERTY(EditAnywhere)
	FSoDmg BonusHealth;


	UPROPERTY(EditAnywhere)
	float SaneHPMultiplier = 0.6f;

	UPROPERTY(EditAnywhere)
	float InsaneHPMultiplier = 1.5f;
};
