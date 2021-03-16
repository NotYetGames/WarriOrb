// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

#include "fmod.hpp"
#include "GenericPlatform/GenericPlatform.h"

FMOD_RESULT F_CALLBACK FMODLogCallback(FMOD_DEBUG_FLAGS flags, const char *file, int line, const char *func, const char *message);

void AcquireFMODFileSystem();
void ReleaseFMODFileSystem();
void AttachFMODFileSystem(FMOD::System *system, FGenericPlatformTypes::int32 fileBufferSize);
