// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"
#include "SoItemThumbnailRenderer.generated.h"

UCLASS(config = Editor)
class USoItemThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_BODY()

public:
	// Begin UThumbnailRenderer Object
	/**
	 * Returns true if the renderer is capable of producing a thumbnail for the specified asset.
	 *
	 * @param Object the asset to attempt to render
	 */
	bool CanVisualizeAsset(UObject* Object) override;

	/**
	 * Draws a thumbnail for the object that was specified.
	 *
	 * @param Object the object to draw the thumbnail for
	 * @param X the X coordinate to start drawing at
	 * @param Y the Y coordinate to start drawing at
	 * @param Width the width of the thumbnail to draw
	 * @param Height the height of the thumbnail to draw
	 * @param Viewport the viewport being drawn in
	 * @param Canvas the render interface to draw with
	 */
	void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas) override;
};
