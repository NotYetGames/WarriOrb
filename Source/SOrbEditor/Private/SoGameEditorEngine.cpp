// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoGameEditorEngine.h"

#include "SOrbEditorModule.h"
// #include "SOrbGameMode.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameEditorEngine::USoGameEditorEngine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USoGameEditorEngine::PreExit()
{
	UE_LOG(LogSoEditor, Verbose, TEXT("USoGameEditorEngine::PreExit()"));

	// NOTE: This destroys all the worlds so we must call it at the end.
	Super::PreExit();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameEditorEngine::WorldAdded(UWorld* World)
{
	Super::WorldAdded(World);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameEditorEngine::WorldDestroyed(UWorld* InWorld)
{
	Super::WorldDestroyed(InWorld);
}
