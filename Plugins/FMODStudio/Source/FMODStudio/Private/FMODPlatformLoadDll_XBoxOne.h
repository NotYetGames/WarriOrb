// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.
#pragma once

#include "fmoddurango.h"

void *FMODPlatformLoadDll(const TCHAR *LibToLoad)
{
    return LoadLibrary(LibToLoad);
}

FMOD_RESULT FMODPlatformSystemSetup()
{
    FMOD_DURANGO_THREADAFFINITY Affinity = { 0 };
    Affinity.mixer = FMOD_THREAD_CORE4;
    Affinity.studioUpdate = FMOD_THREAD_CORE5;
    Affinity.studioLoadBank = FMOD_THREAD_CORE5;
    Affinity.studioLoadSample = FMOD_THREAD_CORE5;
    return FMOD_Durango_SetThreadAffinity(&Affinity);
}
