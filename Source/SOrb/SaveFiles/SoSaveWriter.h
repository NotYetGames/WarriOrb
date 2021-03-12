// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "IO/DlgJsonWriter.h"

#include "Items/ItemTemplates/SoItemTemplate.h"


class FSoSaveWriter : public FDlgJsonWriter
{
	typedef FDlgJsonWriter Super;
public:
	FSoSaveWriter() : Super()
	{
		bLogVerbose = false;
	}

	bool CanSaveAsReference(const UProperty* Property, const UObject* Object) override
	{
		if (Super::CanSaveAsReference(Property, Object))
			return true;

		const UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
		const UObjectProperty* ObjProperty = Cast<UObjectProperty>(ArrayProperty == nullptr ? Property : ArrayProperty->Inner);
		if (ObjProperty != nullptr)
		{
			if (ObjProperty->PropertyClass == nullptr)
				return false;

			if (ObjProperty->PropertyClass->IsChildOf(USoItemTemplate::StaticClass()))
				return true;
		}

		return false;
	}
};
