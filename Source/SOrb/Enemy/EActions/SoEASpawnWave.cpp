// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEASpawnWave.h"

#include "Enemy/SoEnemy.h"
#include "Enemy/SoEnemySpawner.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEASpawnWave::OnEnter(ASoEnemy* Owner)
{
	ASoEnemySpawner* Spawner = Owner->GetTargetSpawner(Index);
	if (Spawner != nullptr)
	{
		Spawner->DeactivateAndReset();
		Spawner->Activate();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEASpawnWave::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	ASoEnemySpawner* Spawner = Owner->GetTargetSpawner(Index);
	return Spawner != nullptr && Spawner->IsActiveAndNotDone();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEASpawnWave::OnLeave(ASoEnemy* Owner)
{
}
