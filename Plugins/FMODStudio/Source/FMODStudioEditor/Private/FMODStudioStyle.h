// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once
#include "SlateCore/Public/Styling/SlateStyle.h"
#include "EditorStyle/Public/EditorStyleSet.h"

class FFMODStudioStyle : public FEditorStyle
{
public:
    static void Initialize();

    static void Shutdown();

private:
    static TSharedRef<class FSlateStyleSet> Create();

private:
    static TSharedPtr<class FSlateStyleSet> StyleInstance;

private:
    FFMODStudioStyle() {}
};
