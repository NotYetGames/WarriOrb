#pragma once

#include "CoreTypes.h"
#include "Templates/SharedPointer.h"
#include "Containers/Array.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"

#include "MovieSceneKeyStruct.h"
#include "SequencerChannelTraits.h"
#include "Channels/MovieSceneChannelHandle.h"
#include "Channels/MovieSceneByteChannel.h"
#include "Sequencer/FMODEventControlSection.h"

#include "FMODChannelEditors.generated.h"

/** Key editor overrides */
bool CanCreateKeyEditor(const FMovieSceneByteChannel *Channel);
TSharedRef<SWidget> CreateKeyEditor(const TMovieSceneChannelHandle<FMovieSceneByteChannel> &Channel, UMovieSceneSection *Section,
    const FGuid &InObjectBindingID, TWeakPtr<FTrackInstancePropertyBindings> PropertyBindings, TWeakPtr<ISequencer> InSequencer);

/** KeyStruct overrides */
TSharedPtr<FStructOnScope> GetKeyStruct(const TMovieSceneChannelHandle<FFMODEventControlChannel> &Channel, FKeyHandle InHandle);

/** Key drawing overrides */
void DrawKeys(FFMODEventControlChannel *Channel, TArrayView<const FKeyHandle> InKeyHandles, TArrayView<FKeyDrawParams> OutKeyDrawParams);

USTRUCT()
struct FFMODEventControlKeyStruct : public FMovieSceneKeyTimeStruct
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, Category = "Key")
    EFMODEventControlKey Value;
};
template <>
struct TStructOpsTypeTraits<FFMODEventControlKeyStruct> : public TStructOpsTypeTraitsBase2<FFMODEventControlKeyStruct>
{
    enum
    {
        WithCopy = false
    };
};
