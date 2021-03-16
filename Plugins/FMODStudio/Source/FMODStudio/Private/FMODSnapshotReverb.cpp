// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#include "FMODSnapshotReverb.h"

UFMODSnapshotReverb::UFMODSnapshotReverb(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
}

#if WITH_EDITORONLY_DATA
void UFMODSnapshotReverb::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
}
#endif // EDITORONLY_DATA