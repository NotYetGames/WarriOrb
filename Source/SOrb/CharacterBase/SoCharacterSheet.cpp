// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoCharacterSheet.h"
#include "SoIMortalTypes.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "SaveFiles/SoWorldState.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
USoCharacterSheet::USoCharacterSheet()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	// bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = false;

	bWantsInitializeComponent = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterSheet::InitializeComponent()
{
	Super::InitializeComponent();
	CurrentHealthPoints = GetMaxHealth();
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterSheet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	CurrentHealthPoints = MaxHealthPoints;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterSheet::RestoreHealth()
{
	CurrentHealthPoints = GetMaxHealth();

	OnHealthChanged.Broadcast(CurrentHealthPoints, CurrentHealthPoints);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterSheet::SetMaxHealth(float NewValue, bool bModifyCurrent)
{
	MaxHealthPoints = NewValue;
	const float Max = GetMaxHealth();
	if (bModifyCurrent)
		CurrentHealthPoints = Max;
	OnHealthChanged.Broadcast(CurrentHealthPoints, Max);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoCharacterSheet::SetHealth(float NewValue)
{
	const float Max = GetMaxHealth();
	CurrentHealthPoints = FMath::Clamp(NewValue, 0.0f, Max);
	OnHealthChanged.Broadcast(CurrentHealthPoints, Max);
	return CurrentHealthPoints > 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterSheet::IncreaseHealth(float Delta)
{
	const float Max = GetMaxHealth();
	CurrentHealthPoints = FMath::Clamp(CurrentHealthPoints + Delta, 0.0f, Max);
	OnHealthChanged.Broadcast(CurrentHealthPoints, Max);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterSheet::AddBonusHealth(const FSoDmg& BonusHealthAmount)
{
	BonusHealth += BonusHealthAmount;
	OnBonusHealthChanged.Broadcast(BonusHealth);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterSheet::ClearBonusHealth()
{
	BonusHealth.SetToZero();
	OnBonusHealthChanged.Broadcast(BonusHealth);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoDmg USoCharacterSheet::AddRes(const FSoDmg& Res)
{
	FSoDmg Applied = Res;

	if (Resistance.Physical + Res.Physical > 1.0f)
		Applied.Physical = 1.0f - Resistance.Physical;

	if (Resistance.Magical + Res.Magical > 1.0f)
		Applied.Magical = 1.0f - Resistance.Magical;

	Resistance.Physical = FMath::Clamp(Resistance.Physical + Res.Physical, 0.0f, 1.0f);
	Resistance.Magical = FMath::Clamp(Resistance.Magical + Res.Magical, 0.0f, 1.0f);

	return Applied;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterSheet::RemoveRes(const FSoDmg& Res)
{
	Resistance.Physical = FMath::Clamp(Resistance.Physical - Res.Physical, 0.0f, 1.0f);
	Resistance.Magical = FMath::Clamp(Resistance.Magical - Res.Magical, 0.0f, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoCharacterSheet::ApplyDmg(const FSoDmg& Damage)
{
	FSoDmg AppliedDmg = Damage * (Resistance.OneMinus());
	const float Sum = AppliedDmg.Sum();
	if (Sum > KINDA_SMALL_NUMBER)
	{
		OnDmg.Broadcast(AppliedDmg);

		bool bBonusBlocked = false;
		if (AppliedDmg.HasPhysical() && BonusHealth.HasPhysical())
		{
			const float PreBonusHealth = BonusHealth.Physical;
			BonusHealth.Physical = FMath::Max(BonusHealth.Physical - AppliedDmg.Physical, 0.0f);
			AppliedDmg.Physical = FMath::Max(AppliedDmg.Physical - PreBonusHealth, 0.0f);
			bBonusBlocked = true;
		}
		if (AppliedDmg.HasMagical() && BonusHealth.HasMagical())
		{
			const float PreBonusHealth = BonusHealth.Magical;
			BonusHealth.Magical = FMath::Max(BonusHealth.Magical - AppliedDmg.Magical, 0.0f);
			AppliedDmg.Magical = FMath::Max(AppliedDmg.Magical - PreBonusHealth, 0.0f);
			bBonusBlocked = true;
		}
		if (bBonusBlocked)
			OnBonusHealthChanged.Broadcast(BonusHealth);

		OnDmgApplied.Broadcast(AppliedDmg);

		const float PostBonusSum = AppliedDmg.Sum();
		if (PostBonusSum > KINDA_SMALL_NUMBER)
		{
			const bool bAlive = CurrentHealthPoints > PostBonusSum;
			CurrentHealthPoints = FMath::Clamp(CurrentHealthPoints - PostBonusSum, 0.0f, GetMaxHealth());

			if (!bAlive)
			{
				OnPreDeath.Broadcast();
				return CurrentHealthPoints > KINDA_SMALL_NUMBER;
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoDmg USoCharacterSheet::GetReducedDmg(const FSoDmg& Damage)
{
	return Damage * (Resistance.OneMinus());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoCharacterSheet::GetMaxHealth() const
{
	switch (FSoWorldState::Get().GetGameDifficulty())
	{
		case ESoDifficulty::Sane:
			return MaxHealthPoints * SaneHPMultiplier;

		case ESoDifficulty::Insane:
			return MaxHealthPoints * InsaneHPMultiplier;

		case ESoDifficulty::Intended:
		default:
			return MaxHealthPoints;
	}
}