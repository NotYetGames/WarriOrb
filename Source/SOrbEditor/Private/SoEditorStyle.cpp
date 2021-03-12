// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedPtr<FSlateStyleSet> FSoEditorStyle::StyleSet = nullptr;
FString FSoEditorStyle::EngineContentRoot = FString();


// Initialize static variables
const FName FSoEditorStyle::PROPERTY_BuildIcon(TEXT("SOrbEditor.BuildAll"));

// Const icon sizes
static const FVector2D Icon16x16(16.0f, 16.0f);
static const FVector2D Icon20x20(20.0f, 20.0f);
static const FVector2D Icon40x40(40.0f, 40.0f);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoEditorStyle::Initialize()
{
	// Only register once
	if (StyleSet.IsValid())
		return;

	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	EngineContentRoot = FPaths::EngineContentDir() / TEXT("Editor/Slate");
	// StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate/Icons"));

	// Define the icons for the toolbar buttons

	StyleSet->Set(
		"SOrbEditor.LoadCameraData",
		new FSlateImageBrush(GetEngineContentPath("Icons/Profiler/Profiler_Load_Profiler_40x.png"), Icon40x40)
	);
	StyleSet->Set(
		"SOrbEditor.LoadCameraData.Small",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_Downloads_16x.png"), Icon16x16)
	);
	StyleSet->Set(
		"SOrbEditor.SaveCameraData",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_levels_SaveModified_40x.png"), Icon40x40)
	);
	StyleSet->Set(
		"SOrbEditor.SaveCameraData.Small",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_levels_SaveModified_16px.png"), Icon16x16)
	);
	StyleSet->Set(
		"SOrbEditor.ToggleSplinePositionCorrection",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_Landscape_Tool_Splines_40x.png"), Icon40x40)
	);
	StyleSet->Set(
		"SOrbEditor.ToggleSplinePositionCorrection.Small",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_Landscape_Tool_Splines_20x.png"), Icon20x20)
	);
	StyleSet->Set(
		"SOrbEditor.ToggleDisplayCameraKeys",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_StaticMeshEd_CameraLocked_40x.png"), Icon40x40)
	);
	StyleSet->Set(
		"SOrbEditor.ToggleDisplayCameraKeys.Small",
		new FSlateImageBrush(GetEngineContentPath("Icons/AssetIcons/CameraAnim_16x.png"), Icon16x16)
	);

	StyleSet->Set(
		"SOrbEditor.RebuildEnemyGroups",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_Blueprint_AddMacro_40px.png"), Icon40x40)
	);
	StyleSet->Set(
		"SOrbEditor.RebuildEnemyGroups.Small",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_Blueprint_AddMacro_16px.png"), Icon16x16)
	);

	StyleSet->Set(
		PROPERTY_BuildIcon,
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_build_40x.png"), Icon40x40)
	);
	StyleSet->Set(
		GetSmallProperty(PROPERTY_BuildIcon),
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_build_40x.png"), Icon20x20)
	);

	StyleSet->Set(
		"SOrbEditor.CopyActorsToClipboard",
		new FSlateImageBrush(GetEngineContentPath("Icons/Edit/icon_Edit_Copy_40x.png"), Icon40x40)
	);
	StyleSet->Set(
		"SOrbEditor.CopyActorsToClipboard.Small",
		new FSlateImageBrush(GetEngineContentPath("Icons/Edit/icon_Edit_Copy_40x.png"), Icon20x20)
	);

	// Level Editor Save All
	StyleSet->Set(
		"SOrbEditor.SaveAllItems",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_file_saveall_40x.png"), Icon40x40)
	);
	StyleSet->Set(
		"SOrbEditor.SaveAllEffectInstances",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_file_saveall_40x.png"), Icon40x40)
	);
	StyleSet->Set(
		"SOrbEditor.SaveAllCharacterStrikes",
		new FSlateImageBrush(GetEngineContentPath("Icons/icon_file_saveall_40x.png"), Icon40x40)
	);

	// Local
	// const FString ProjectContentDir = FPaths::ProjectContentDir();
	// StyleSet->Set("Name", new FSlateImageBrush(ProjectContentDir / "image.png", Icon40x40));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoEditorStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}
