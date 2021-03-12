// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "RenderCore/Public/RendererInterface.h"

class SoCustomVisibilityQuery: public ICustomVisibilityQuery
{
public:
	/** prepares the query for visibility tests */
	virtual bool Prepare() override { return true; }

	/** test primitive visiblity */
	virtual bool IsVisible(int32 VisibilityId, const FBoxSphereBounds& Bounds) override { return false; }

	/** return true if we can call IsVisible from a ParallelFor */
	virtual bool IsThreadsafe() override
	{
		return true;
	}

	virtual ~SoCustomVisibilityQuery() {};


	virtual uint32 AddRef() const override { return ++NumRefs; }
	virtual uint32 Release() const override { return 0; }
	virtual uint32 GetRefCount() const override { return 0; }

private:

	mutable int32 NumRefs;
};

class SoCustomCulling : public ICustomCulling
{
public:

	virtual ~SoCustomCulling() {};

	virtual ICustomVisibilityQuery* CreateQuery(const FSceneView& View) override
	{
		return &CustomVisibilityQuery;
	}

protected:

	SoCustomVisibilityQuery CustomVisibilityQuery;
};

