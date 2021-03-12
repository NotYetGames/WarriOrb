// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once
#include "Templates/SharedPointer.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"

class FSoEditorStyle
{
public:
	static void Initialize();
	static void Shutdown();

	// Get singleton
	static TSharedPtr<ISlateStyle> Get() { return StyleSet; }

	// Editor style name
	static FName GetStyleSetName() { return TEXT("SOrbEditorStyle"); }

	/** Gets the small property name variant */
	static FName GetSmallProperty(const FName& PropertyName)
	{
		return FName(*(PropertyName.ToString() + TEXT(".Small")));
	}

	/** Get the RelativePath to the Engine Content Dir */
	static FString GetEngineContentPath(const FString& RelativePath)
	{
		return EngineContentRoot / RelativePath;
	}

public:
	static const FName PROPERTY_BuildIcon;

private:
	static TSharedPtr<FSlateStyleSet> StyleSet;

	/** Engine content root. */
	static FString EngineContentRoot;
};
