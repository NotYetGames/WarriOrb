// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Engine/GameEngine.h"
#include "SoGameEngine.generated.h"


/**
 * Custom Game Engine functionality.
 * For Editor engine functionality see USoGameEditorEngine
 */
UCLASS()
class USoGameEngine : public UGameEngine
{
	GENERATED_BODY()
public:
	USoGameEngine(const FObjectInitializer& ObjectInitializer);

	// UEngine interface

	void Init(IEngineLoop* InEngineLoop) override;
	void Start() override;
	void PreExit() override;
	void WorldAdded(UWorld* World) override;
	void WorldDestroyed(UWorld* InWorld) override;

	// NOTE: seems to be not used by anybody
	void OnLostFocusPause(bool EnablePause) override;
};
