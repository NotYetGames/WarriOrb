// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.
#pragma once

#include "fmod_nx.h"

void *FMODPlatformLoadDll(const TCHAR *LibToLoad)
{
    return FPlatformProcess::GetDllHandle(LibToLoad);
}

FMOD_RESULT FMODPlatformSystemSetup()
{
    FMOD_NX_THREADAFFINITY Affinity = { 0 };
    Affinity.mixer = FMOD_THREAD_CORE1;
    Affinity.studioUpdate = FMOD_THREAD_CORE0;
    Affinity.studioLoadBank = FMOD_THREAD_CORE0;
    Affinity.studioLoadSample = FMOD_THREAD_CORE0;
    return FMOD_NX_SetThreadAffinity(&Affinity);
}
