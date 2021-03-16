// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#include "FMODEventControlSectionTemplate.h"
#include "FMODAmbientSound.h"
#include "FMODAudioComponent.h"
#include "Evaluation/MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"

struct FPlayingToken : IMovieScenePreAnimatedToken
{
    FPlayingToken(UObject &InObject)
    {
        bPlaying = false;

        if (UFMODAudioComponent *AudioComponent = Cast<UFMODAudioComponent>(&InObject))
        {
            if (IsValid(AudioComponent))
            {
                bPlaying = AudioComponent->IsPlaying();
            }
        }
    }

    virtual void RestoreState(UObject &Object, IMovieScenePlayer &Player) override
    {
        UFMODAudioComponent *AudioComponent = CastChecked<UFMODAudioComponent>(&Object);

        if (AudioComponent)
        {
            if (bPlaying)
            {
                AudioComponent->Play();
            }
            else
            {
                AudioComponent->Stop();
            }
        }
    }

private:
    bool bPlaying;
};

struct FPlayingTokenProducer : IMovieScenePreAnimatedTokenProducer
{
    static FMovieSceneAnimTypeID GetAnimTypeID() { return TMovieSceneAnimTypeID<FPlayingTokenProducer>(); }

private:
    virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject &Object) const override { return FPlayingToken(Object); }
};

struct FFMODEventKeyState : IPersistentEvaluationData
{
    FKeyHandle LastKeyHandle;
    FKeyHandle InvalidKeyHandle;
};

struct FFMODEventControlExecutionToken : IMovieSceneExecutionToken
{
    FFMODEventControlExecutionToken(EFMODEventControlKey InEventControlKey, FFrameTime InKeyTime)
        : EventControlKey(InEventControlKey)
        , KeyTime(InKeyTime)
    {
    }

    /** Execute this token, operating on all objects referenced by 'Operand' */
    virtual void Execute(const FMovieSceneContext &Context, const FMovieSceneEvaluationOperand &Operand, FPersistentEvaluationData &PersistentData,
        IMovieScenePlayer &Player)
    {
        for (TWeakObjectPtr<> &WeakObject : Player.FindBoundObjects(Operand))
        {
            UFMODAudioComponent *AudioComponent = Cast<UFMODAudioComponent>(WeakObject.Get());

            if (!AudioComponent)
            {
                AFMODAmbientSound *AmbientSound = Cast<AFMODAmbientSound>(WeakObject.Get());
                AudioComponent = AmbientSound ? AmbientSound->AudioComponent : nullptr;
            }

            if (IsValid(AudioComponent))
            {
                Player.SavePreAnimatedState(*AudioComponent, FPlayingTokenProducer::GetAnimTypeID(), FPlayingTokenProducer());

                if (EventControlKey == EFMODEventControlKey::Play)
                {
                    if (AudioComponent->IsPlaying())
                    {
                        AudioComponent->Stop();
                    }

                    EFMODSystemContext::Type SystemContext =
                        (GWorld && GWorld->WorldType == EWorldType::Editor) ? EFMODSystemContext::Editor : EFMODSystemContext::Runtime;
                    AudioComponent->PlayInternal(SystemContext);
                }
                else if (EventControlKey == EFMODEventControlKey::Stop)
                {
                    AudioComponent->Stop();
                }
            }
        }
    }

    EFMODEventControlKey EventControlKey;
    FFrameTime KeyTime;
};

FFMODEventControlSectionTemplate::FFMODEventControlSectionTemplate(const UFMODEventControlSection &Section)
    : ControlKeys(Section.ControlKeys)
{
}

void FFMODEventControlSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand &Operand, const FMovieSceneContext &Context,
    const FPersistentEvaluationData &PersistentData, FMovieSceneExecutionTokens &ExecutionTokens) const
{
    const bool bPlaying = Context.IsSilent() == false && Context.GetDirection() == EPlayDirection::Forwards &&
                          Context.GetRange().Size<FFrameTime>() >= FFrameTime(0) && Context.GetStatus() == EMovieScenePlayerStatus::Playing;

    if (!bPlaying)
    {
        ExecutionTokens.Add(FFMODEventControlExecutionToken(EFMODEventControlKey::Stop, FFrameTime(0)));
    }
    else
    {
        TRange<FFrameNumber> PlaybackRange = Context.GetFrameNumberRange();
        TMovieSceneChannelData<const uint8> ChannelData = ControlKeys.GetData();

        // Find the index of the key handle that exists before this time
        TArrayView<const FFrameNumber> Times = ChannelData.GetTimes();
        TArrayView<const uint8> Values = ChannelData.GetValues();

        const int32 LastKeyIndex = Algo::UpperBound(Times, PlaybackRange.GetUpperBoundValue()) - 1;
        if (LastKeyIndex >= 0 && PlaybackRange.Contains(Times[LastKeyIndex]))
        {
            FFMODEventControlExecutionToken NewToken((EFMODEventControlKey)Values[LastKeyIndex], Times[LastKeyIndex]);
            ExecutionTokens.Add(MoveTemp(NewToken));
        }
    }
}
