// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#include "FMODStudioStyle.h"
#include "EditorStyle/Public/Interfaces/IEditorStyleModule.h"
//#include "EditorStyle/Public/EditorStyleSet.h"
#include "Modules/ModuleManager.h"

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(Style.RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_BRUSH(RelativePath, ...) FSlateBoxBrush(Style.RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////
// FFMODStudioStyle

TSharedPtr<FSlateStyleSet> FFMODStudioStyle::StyleInstance = NULL;

void FFMODStudioStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
    }

    SetStyle(StyleInstance.ToSharedRef());
}

void FFMODStudioStyle::Shutdown()
{
    ResetToDefault();
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

TSharedRef<FSlateStyleSet> FFMODStudioStyle::Create()
{
    IEditorStyleModule &EditorStyle = FModuleManager::LoadModuleChecked<IEditorStyleModule>(TEXT("EditorStyle"));
    TSharedRef<FSlateStyleSet> StyleRef = EditorStyle.CreateEditorStyleInstance();
    FSlateStyleSet &Style = StyleRef.Get();

    const FVector2D Icon20x20(20.0f, 20.0f);
    const FVector2D Icon40x40(40.0f, 40.0f);

    Style.Set("ClassIcon.FMODAmbientSound", new IMAGE_BRUSH("Icons/AssetIcons/AmbientSound_16x", FVector2D(16.0f, 16.0f)));
    Style.Set("ClassThumbnail.FMODAmbientSound", new IMAGE_BRUSH("Icons/AssetIcons/AmbientSound_64x", FVector2D(64.0f, 64.0f)));

    Style.Set("ClassIcon.FMODAudioComponent", new IMAGE_BRUSH("Icons/ActorIcons/SoundActor_16x", FVector2D(16.0f, 16.0f)));
    //Style.Set( "ClassThumbnail.FMODAudioComponent", new IMAGE_BRUSH( "Icons/ActorIcons/SoundActor_64x",  FVector2D(64.0f, 64.0f) ) );

    Style.Set("ClassIcon.FMODAsset", new IMAGE_BRUSH("Icons/ActorIcons/SoundActor_16x", FVector2D(16.0f, 16.0f)));
    //Style.Set( "ClassThumbnail.FMODAsset", new IMAGE_BRUSH( "Icons/ActorIcons/SoundActor_64x", FVector2D(64.0f, 64.0f)  ) );

    return StyleRef;
}

//////////////////////////////////////////////////////////////////////////

#undef IMAGE_BRUSH
