// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#include "FMODEventControlSection.h"
#include "FMODEventControlSectionTemplate.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "UObject/SequencerObjectVersion.h"
#include "UObject/Package.h"

FFMODEventControlChannel::FFMODEventControlChannel()
{
    SetEnum(FindObject<UEnum>(ANY_PACKAGE, TEXT("EFMODEventControlKey")));
}

UFMODEventControlSection::UFMODEventControlSection(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetRange(TRange<FFrameNumber>::All());

    int32 LinkerCustomVersion = GetLinkerCustomVersion(FSequencerObjectVersion::GUID);
    EMovieSceneCompletionMode CompletionMode;

    if (LinkerCustomVersion < FSequencerObjectVersion::WhenFinishedDefaultsToRestoreState)
    {
        CompletionMode = EMovieSceneCompletionMode::KeepState;
    }
    else if (LinkerCustomVersion < FSequencerObjectVersion::WhenFinishedDefaultsToProjectDefault)
    {
        CompletionMode = EMovieSceneCompletionMode::RestoreState;
    }
    else
    {
        CompletionMode = EMovieSceneCompletionMode::ProjectDefault;
    }

#if WITH_EDITOR

    ChannelProxy = MakeShared<FMovieSceneChannelProxy>(ControlKeys, FMovieSceneChannelMetaData(), TMovieSceneExternalValue<uint8>());

#else

    ChannelProxy = MakeShared<FMovieSceneChannelProxy>(ControlKeys);

#endif
}

FMovieSceneEvalTemplatePtr UFMODEventControlSection::GenerateTemplate() const
{
    return FFMODEventControlSectionTemplate(*this);
}
