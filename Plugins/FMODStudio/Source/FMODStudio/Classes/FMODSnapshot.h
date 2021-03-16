// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

#include "FMODEvent.h"
#include "FMODSnapshot.generated.h"

/**
 * FMOD Snapshot Asset.
 */
UCLASS()
class FMODSTUDIO_API UFMODSnapshot : public UFMODEvent
{
    GENERATED_UCLASS_BODY()

    /** Descriptive name */
    virtual FString GetDesc() override;
};
