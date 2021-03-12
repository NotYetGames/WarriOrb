// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "SoEnemyVoiceManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDisplayVoice, const FVector&, WorldPosition, const FText&, DisplayText);

class AActor;
class UFMODAudioComponent;
class UFMODEvent;

USTRUCT()
struct FSoEnemyVoice
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY()
	AActor* Enemy = nullptr;

	UPROPERTY()
	UFMODAudioComponent* Voice = nullptr;

	UPROPERTY()
	bool bForced = false;
};


UCLASS(Blueprintable, BlueprintType)
class SORB_API USoEnemyVoiceManager : public UObject
{
	GENERATED_BODY()
	typedef USoEnemyVoiceManager Self;

public:
	USoEnemyVoiceManager(const FObjectInitializer& ObjectInitializer);
	~USoEnemyVoiceManager();


	UFUNCTION(BlueprintPure, DisplayName = "Get So Enemy Voice Manager", meta = (WorldContext = "WorldContextObject"))
	static USoEnemyVoiceManager* GetInstance(const UObject* WorldContextObject);
	static USoEnemyVoiceManager& Get(const UObject* WorldContextObject)
	{
		check(IsValid(WorldContextObject));
		auto* Instance = GetInstance(WorldContextObject);
		check(IsValid(Instance));
		return *Instance;
	}

	UFUNCTION(BlueprintCallable, Category = EnemyVO)
	bool PlayEnemyVoice(AActor* Enemy, UFMODEvent* EventToPlay, ESoVoiceType Type);

	UFUNCTION(BlueprintCallable, Category = EnemyVO)
	void StopEnemyVoice(AActor* Enemy);

protected:
	FSoEnemyVoice* GetActiveVoiceFromEnemy(AActor* Enemy);
	FSoEnemyVoice* GetActiveVoiceFromClass(AActor* Enemy);
	FSoEnemyVoice* GetFreeVoiceSlot(UClass* RequesterClass, bool bIgnoreCheck = false);

	bool PlayEnemyVoice_Internal(FSoEnemyVoice& Voice, AActor* Enemy, UFMODEvent* EventToPlay, bool bForceOverride);

	void RemoveVoice(FSoEnemyVoice* VoicePtr);

protected:
	UPROPERTY(BlueprintAssignable)
	FOnDisplayVoice OnDisplayVoice;

	UFUNCTION()
	void CleanupActiveVoiceList();

	UPROPERTY()
	TArray<FSoEnemyVoice> ActiveVoices;

	bool bCleanupWIP = false;
};
