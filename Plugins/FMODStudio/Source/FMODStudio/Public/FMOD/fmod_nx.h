#ifndef _FMOD_NX_H
#define _FMOD_NX_H

#include "fmod.h"

/*
[ENUM] 
[
    [DESCRIPTION]
    Cores available for thread assignment.

    [REMARKS]

    [SEE_ALSO]
    FMOD_NX_THREADAFFINITY
]
*/
typedef enum FMOD_THREAD
{
    FMOD_THREAD_DEFAULT = 0,        /* Use default thread assignment. */
    FMOD_THREAD_CORE0   = 1 << 0,
    FMOD_THREAD_CORE1   = 1 << 1,
    FMOD_THREAD_CORE2   = 1 << 2,   /* Default for all threads. */
} FMOD_THREAD;


/*
[STRUCTURE]
[
    [DESCRIPTION]
    Mapping of cores to threads.

    [REMARKS]

    [SEE_ALSO]
    FMOD_THREAD
    FMOD_NX_SetThreadAffinity
]
*/
typedef struct FMOD_NX_THREADAFFINITY
{
    unsigned int mixer;              /* Software mixer thread. */
    unsigned int stream;             /* Stream thread. */
    unsigned int nonblocking;        /* Asynchronous sound loading thread. */
    unsigned int file;               /* File thread. */
    unsigned int geometry;           /* Geometry processing thread. */
    unsigned int profiler;           /* Profiler thread. */
    unsigned int studioUpdate;       /* Studio update thread. */
    unsigned int studioLoadBank;     /* Studio bank loading thread. */
    unsigned int studioLoadSample;   /* Studio sample loading thread. */
} FMOD_NX_THREADAFFINITY;


/*
[API]
[
    [DESCRIPTION]
    Control which core particular FMOD threads are created on.

    [PARAMETERS]
    'affinity'    Pointer to a structure that describes the affinity for each thread.

    [REMARKS]
    Call before System::init or affinity values will not apply.

    [SEE_ALSO]
    FMOD_NX_THREADAFFINITY
]
*/
extern "C" FMOD_RESULT F_API FMOD_NX_SetThreadAffinity(FMOD_NX_THREADAFFINITY *affinity);


/*
[API]
[
    [DESCRIPTION]
    Select between nn::htcs (host-target communications) and nn::socket for socket implementation.

    Both nn:socket and nn:htcs support communication with FMOD Profiler and FMOD Studio.

    Net streaming audio is only supported by nn::socket.

    [PARAMETERS]
    'enabled'   Use HTCS for socket communication. Default = FALSE.

    [REMARKS]
    Call before System::init or selection will not apply.

    HTCS is only available in the logging version of FMOD.

    [SEE_ALSO]
]
*/
extern "C" FMOD_RESULT F_API FMOD_NX_SetHTCSEnabled(FMOD_BOOL enabled);

#endif  /* _FMOD_NX_H */
