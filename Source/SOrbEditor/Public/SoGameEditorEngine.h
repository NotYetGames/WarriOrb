// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Editor/UnrealEdEngine.h"
#include "SoGameEditorEngine.generated.h"


/**
 * Custom Game Editor Engine functionality.
 * For runtime engine functionality see USoGameEngine.
 */
UCLASS()
class USoGameEditorEngine : public UUnrealEdEngine
{
	GENERATED_BODY()
public:
	USoGameEditorEngine(const FObjectInitializer& ObjectInitializer);

	// UEngine interface

	// Only executed when the Editor is closed.
	void PreExit() override;
	void WorldAdded(UWorld* World) override;
	void WorldDestroyed(UWorld* InWorld) override;
};
