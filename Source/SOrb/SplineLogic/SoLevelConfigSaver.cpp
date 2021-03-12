// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoLevelConfigSaver.h"
#include "SoEditorGameInterface.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoLevelConfigSaver::PreSave(const ITargetPlatform* TargetPlatform)
{
	Super::PreSave(TargetPlatform);

	if (GetWorld() != nullptr)
		FSoEditorGameInterface::SaveCameraData(GetWorld());
}
