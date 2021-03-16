// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Sections/MovieSceneParameterSection.h"
#include "MovieSceneNameableTrack.h"
#include "FMODEventParameterTrack.generated.h"

/** Handles manipulation of event parameters in a movie scene. */
UCLASS(MinimalAPI)
class UFMODEventParameterTrack : public UMovieSceneNameableTrack
{
    GENERATED_UCLASS_BODY()

public:
    // Begin UMovieSceneTrack interface
    virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection &InSection) const override;
    virtual UMovieSceneSection *CreateNewSection() override;
    virtual void RemoveAllAnimationData() override;
    virtual bool HasSection(const UMovieSceneSection &Section) const override;
    virtual void AddSection(UMovieSceneSection &Section) override;
    virtual void RemoveSection(UMovieSceneSection &Section) override;
    virtual bool IsEmpty() const override;
    virtual const TArray<UMovieSceneSection *> &GetAllSections() const override;
// End UMovieSceneTrack interface

#if WITH_EDITORONLY_DATA
    virtual FText GetDefaultDisplayName() const override;
#endif

public:
    /** Adds a (scalar) event parameter key to the track. */
    void FMODSTUDIO_API AddParameterKey(FName ParameterName, FFrameNumber Time, float Value);

private:
    /** The sections owned by this track. */
    UPROPERTY()
    TArray<UMovieSceneSection *> Sections;
};
