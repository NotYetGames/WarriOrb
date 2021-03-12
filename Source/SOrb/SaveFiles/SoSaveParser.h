// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "IO/DlgJsonParser.h"

class FSoSaveParser : public FDlgJsonParser
{
	typedef FDlgJsonParser Super;
public:
	FSoSaveParser() : Super()
	{
		bLogVerbose = false;
	}
	FSoSaveParser(const FString& FilePath) : Super(FilePath)
	{
		bLogVerbose = false;
	}
};
