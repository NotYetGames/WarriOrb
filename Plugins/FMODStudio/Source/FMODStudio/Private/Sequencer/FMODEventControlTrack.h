// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MovieSceneNameableTrack.h"
#include "FMODEventControlTrack.generated.h"

/** Handles control of an FMOD Event */
UCLASS(MinimalAPI)
class UFMODEventControlTrack : public UMovieSceneNameableTrack
{
    GENERATED_UCLASS_BODY()

public:
    virtual TArray<UMovieSceneSection *> GetAllControlSections() const { return ControlSections; }

public:
    // Begin UMovieSceneTrack interface
    virtual void RemoveAllAnimationData() override;
    virtual bool HasSection(const UMovieSceneSection &Section) const override;
    virtual void AddSection(UMovieSceneSection &Section) override;
    virtual void RemoveSection(UMovieSceneSection &Section) override;
    virtual bool IsEmpty() const override;
    virtual const TArray<UMovieSceneSection *> &GetAllSections() const override;
    virtual void AddNewSection(FFrameNumber SectionTime);
    virtual UMovieSceneSection *CreateNewSection() override;
// End UMovieSceneTrack interface

#if WITH_EDITORONLY_DATA
    virtual FText GetDefaultDisplayName() const override;
#endif

private:
    /** List of all event control sections. */
    UPROPERTY()
    TArray<UMovieSceneSection *> ControlSections;
};
