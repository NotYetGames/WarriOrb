// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#include "Sequencer/FMODChannelEditors.h"
#include "ISequencerChannelInterface.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "MovieSceneTimeHelpers.h"
#include "MovieSceneToolHelpers.h"
#include "ScopedTransaction.h"
#include "EditorStyleSet.h"

class SFMODEventControlKeyEditor : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFMODEventControlKeyEditor) {}
    SLATE_END_ARGS();

    void Construct(const FArguments &InArgs, TMovieSceneChannelHandle<FMovieSceneByteChannel> InChannelHandle,
        TWeakObjectPtr<UMovieSceneSection> InWeakSection, TWeakPtr<ISequencer> InWeakSequencer, UEnum *InEnum)
    {
        ChannelHandle = InChannelHandle;
        WeakSection = InWeakSection;
        WeakSequencer = InWeakSequencer;

        ChildSlot[MovieSceneToolHelpers::MakeEnumComboBox(InEnum,
            TAttribute<int32>::Create(TAttribute<int32>::FGetter::CreateSP(this, &SFMODEventControlKeyEditor::OnGetCurrentValueAsInt)),
            FOnEnumSelectionChanged::CreateSP(this, &SFMODEventControlKeyEditor::OnChangeKey))];
    }

private:
    int32 OnGetCurrentValueAsInt() const
    {
        using namespace MovieScene;

        FMovieSceneByteChannel *Channel = ChannelHandle.Get();
        ISequencer *Sequencer = WeakSequencer.Pin().Get();
        UMovieSceneSection *OwningSection = WeakSection.Get();
        uint8 Result = 0;

        if (Channel && Sequencer && OwningSection)
        {
            const FFrameTime CurrentTime = MovieScene::ClampToDiscreteRange(Sequencer->GetLocalTime().Time, OwningSection->GetRange());
            EvaluateChannel(Channel, CurrentTime, Result);
        }

        return Result;
    }

    void SetValue(uint8 InValue)
    {
        using namespace MovieScene;
        using namespace Sequencer;

        UMovieSceneSection *OwningSection = WeakSection.Get();
        if (!OwningSection)
        {
            return;
        }

        OwningSection->SetFlags(RF_Transactional);

        FMovieSceneByteChannel *Channel = ChannelHandle.Get();
        ISequencer *Sequencer = WeakSequencer.Pin().Get();

        if (!OwningSection->TryModify() || !Channel || !Sequencer)
        {
            return;
        }

        const FFrameNumber CurrentTime = Sequencer->GetLocalTime().Time.FloorToFrame();
        const bool bAutoSetTrackDefaults = Sequencer->GetAutoSetTrackDefaults();

        EMovieSceneKeyInterpolation Interpolation = Sequencer->GetKeyInterpolation();

        TArray<FKeyHandle> KeysAtCurrentTime;
        Channel->GetKeys(TRange<FFrameNumber>(CurrentTime), nullptr, &KeysAtCurrentTime);

        if (KeysAtCurrentTime.Num() > 0)
        {
            AssignValue(Channel, KeysAtCurrentTime[0], InValue);
        }
        else
        {
            const bool bHasAnyKeys = Channel->GetNumKeys() != 0;

            if (bHasAnyKeys || bAutoSetTrackDefaults == false)
            {
                // When auto setting track defaults are disabled, add a key even when it's empty so that the changed
                // value is saved and is propagated to the property.
                AddKeyToChannel(Channel, CurrentTime, InValue, Interpolation);
            }

            if (bHasAnyKeys)
            {
                TRange<FFrameNumber> KeyRange = TRange<FFrameNumber>(CurrentTime);
                TRange<FFrameNumber> SectionRange = OwningSection->GetRange();

                if (!SectionRange.Contains(KeyRange))
                {
                    OwningSection->SetRange(TRange<FFrameNumber>::Hull(KeyRange, SectionRange));
                }
            }
        }

        // Always update the default value when auto-set default values is enabled so that the last changes
        // are always saved to the track.
        if (bAutoSetTrackDefaults)
        {
            SetChannelDefault(Channel, InValue);
        }
    }

    void OnChangeKey(int32 Selection, ESelectInfo::Type SelectionType)
    {
        FScopedTransaction Transaction(FText::FromString("Set FMOD Event Control Key Value"));
        SetValue(Selection);
        if (ISequencer *Sequencer = WeakSequencer.Pin().Get())
        {
            Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChangedRefreshImmediately);
        }
    }

    TMovieSceneChannelHandle<FMovieSceneByteChannel> ChannelHandle;
    TWeakObjectPtr<UMovieSceneSection> WeakSection;
    TWeakPtr<ISequencer> WeakSequencer;
};

bool CanCreateKeyEditor(const FMovieSceneByteChannel *Channel)
{
    return true;
}

TSharedRef<SWidget> CreateKeyEditor(const TMovieSceneChannelHandle<FMovieSceneByteChannel> &Channel, UMovieSceneSection *Section,
    const FGuid &InObjectBindingID, TWeakPtr<FTrackInstancePropertyBindings> PropertyBindings, TWeakPtr<ISequencer> InSequencer)
{
    const FMovieSceneByteChannel *RawChannel = Channel.Get();

    if (!RawChannel)
    {
        return SNullWidget::NullWidget;
    }

    UEnum *Enum = RawChannel->GetEnum();
    return SNew(SFMODEventControlKeyEditor, Channel, Section, InSequencer, Enum);
}

TSharedPtr<FStructOnScope> GetKeyStruct(const TMovieSceneChannelHandle<FFMODEventControlChannel> &ChannelHandle, FKeyHandle InHandle)
{
    FFMODEventControlChannel *Channel = ChannelHandle.Get();
    if (!Channel)
    {
        return nullptr;
    }

    TMovieSceneChannelData<uint8> ChannelData = Channel->GetData();
    const int32 KeyIndex = ChannelData.GetIndex(InHandle);

    if (KeyIndex == INDEX_NONE)
    {
        return nullptr;
    }

    TSharedPtr<FStructOnScope> KeyStruct = MakeShared<FStructOnScope>(FFMODEventControlKeyStruct::StaticStruct());
    FFMODEventControlKeyStruct *Struct = reinterpret_cast<FFMODEventControlKeyStruct *>(KeyStruct->GetStructMemory());

    Struct->Time = ChannelData.GetTimes()[KeyIndex];
    Struct->Value = (EFMODEventControlKey)ChannelData.GetValues()[KeyIndex];

    Struct->KeyStructInterop.Add(FMovieSceneChannelValueHelper(ChannelHandle, &Struct->Value, MakeTuple(InHandle, Struct->Time)));
    return KeyStruct;
}

void DrawKeys(FFMODEventControlChannel *Channel, TArrayView<const FKeyHandle> InKeyHandles, TArrayView<FKeyDrawParams> OutKeyDrawParams)
{
    static const FName KeyLeftBrushName("Sequencer.KeyLeft");
    static const FName KeyRightBrushName("Sequencer.KeyRight");
    static const FName KeyDiamondBrushName("Sequencer.KeyDiamond");

    const FSlateBrush *LeftKeyBrush = FEditorStyle::GetBrush(KeyLeftBrushName);
    const FSlateBrush *RightKeyBrush = FEditorStyle::GetBrush(KeyRightBrushName);
    const FSlateBrush *DiamondBrush = FEditorStyle::GetBrush(KeyDiamondBrushName);

    TMovieSceneChannelData<uint8> ChannelData = Channel->GetData();

    for (int32 Index = 0; Index < InKeyHandles.Num(); ++Index)
    {
        FKeyHandle Handle = InKeyHandles[Index];

        FKeyDrawParams Params;
        Params.BorderBrush = Params.FillBrush = DiamondBrush;

        const int32 KeyIndex = ChannelData.GetIndex(Handle);
        if (KeyIndex != INDEX_NONE)
        {
            const EFMODEventControlKey Value = (EFMODEventControlKey)ChannelData.GetValues()[KeyIndex];
            if (Value == EFMODEventControlKey::Play)
            {
                Params.BorderBrush = Params.FillBrush = LeftKeyBrush;
                Params.FillOffset = FVector2D(-1.0f, 1.0f);
            }
            else if (Value == EFMODEventControlKey::Stop)
            {
                Params.BorderBrush = Params.FillBrush = RightKeyBrush;
                Params.FillOffset = FVector2D(1.0f, 1.0f);
            }
        }

        OutKeyDrawParams[Index] = Params;
    }
}
