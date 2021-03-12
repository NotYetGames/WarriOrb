// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

#include "GameFramework/CheatManager.h"

#include "SoCheatManager.generated.h"

class AActor;
class APawn;

UCLASS()
class USoCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:

	//
	// UCheatManager interface
	//

	void Fly() override
	{
		if (AllowEngineCheats())
			Super::Fly();
	}
	void Walk() override
	{
		if (AllowEngineCheats())
			Super::Walk();
	}
	void Ghost() override
	{
		if (AllowEngineCheats())
			Super::Ghost();
	}
	void God() override
	{
		if (AllowEngineCheats())
			Super::God();
	}
	void DamageTarget(float DamageAmount) override
	{
		if (AllowEngineCheats())
			Super::DamageTarget(DamageAmount);
	}
	void DestroyTarget() override
	{
		if (AllowEngineCheats())
			Super::DestroyTarget();
	}
	void DestroyAll(TSubclassOf<AActor> aClass) override
	{
		if (AllowEngineCheats())
			Super::DestroyAll(aClass);
	}
	void DestroyAllPawnsExceptTarget() override
	{
		if (AllowEngineCheats())
			Super::DestroyAllPawnsExceptTarget();
	}
	void DestroyPawns(TSubclassOf<APawn> aClass) override
	{
		if (AllowEngineCheats())
			Super::DestroyPawns(aClass);
	}
	void Summon(const FString& ClassName) override
	{
		if (AllowEngineCheats())
			Super::Summon(ClassName);
	}
	void PlayersOnly() override
	{
		if (AllowEngineCheats())
			Super::PlayersOnly();
	}
	void Teleport() override
	{
		if (AllowEngineCheats())
			Super::Teleport();
	}


	//
	// Own methods
	//

	bool AllowCheats() const;
	bool AllowEngineCheats() const { return true; }

	// Matches WARRIORB_PASSWORD_ENABLE_CONSOLE_CHEATS?
	static bool IsAllowCheatsPasswordValid(const FString& UserPassword);
};
