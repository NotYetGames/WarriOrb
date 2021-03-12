// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEAStunned.h"

#include "Enemy/SoEnemy.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAStunned::OnEnter(ASoEnemy* Owner)
{
	bInterruptible = false;

	ISoMortal::Execute_OnStatusEffectChanged(Owner, StatusEffect, true);

	if (StatusEffect != ESoStatusEffect::ESE_BootTransmutation)
		if (FSoEActions* HitReacts = Owner->GetActionList(ESoActionList::EAL_HitReact))
		{
			HitReact = Owner->GetFirstSatisfiedAction(HitReacts->Array);
			if (HitReact != nullptr)
				HitReact->Start(Owner);
		}

	Owner->OnStunned();

	RestTime = Duration;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAStunned::OnLeave(ASoEnemy* Owner)
{
 	ISoMortal::Execute_OnStatusEffectChanged(Owner, StatusEffect, false);
	Owner->OnStunOver();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAStunned::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	if (HitReact != nullptr)
		if (!HitReact->Tick(DeltaSeconds, Owner))
			HitReact = nullptr;

	RestTime -= DeltaSeconds;

	return HitReact != nullptr || (RestTime > 0.0f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAStunned::Interrupt(ASoEnemy* Owner)
{
	if (HitReact != nullptr)
		HitReact->Interrupt(Owner);

	HitReact = nullptr;
	Super::Interrupt(Owner);
}
