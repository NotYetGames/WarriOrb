// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoItemThumbnailRenderer.h"
#include "Items/ItemTemplates/SoItemTemplate.h"
#include "SceneTypes.h"
#include "CanvasItem.h"
#include "Engine/Texture2D.h"

bool USoItemThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	USoItemTemplate* ItemTemplate = Cast<USoItemTemplate>(Object);
	return ItemTemplate != nullptr && ItemTemplate->GetIcon() != nullptr;
}

void USoItemThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas)
{
	// If any of these are null it will simply show the default text
	USoItemTemplate* ItemTemplate = CastChecked<USoItemTemplate>(Object);
	UTexture2D* Texture = ItemTemplate->GetIcon();
	if (Texture == nullptr)
		return;

	// Draw the Image
	FCanvasTileItem CanvasTile(FVector2D(X, Y), Texture->Resource, FVector2D(Width, Height), FLinearColor::White);
	CanvasTile.BlendMode = SE_BLEND_Opaque;
	CanvasTile.Draw(Canvas);
}
