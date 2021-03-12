// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoConsoleExtensionComponent.generated.h"

/**
 *
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SORB_API USoConsoleExtensionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USoConsoleExtensionComponent();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

	UPROPERTY(EditAnywhere)
	FString PreFix = "SO_";

	TArray<IConsoleObject*> ConsoleObjects;
};
