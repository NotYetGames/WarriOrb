// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoCharacterStrikeThumbnailRenderer.h"

#include "SceneTypes.h"
#include "CanvasItem.h"
#include "Engine/Texture2D.h"

#include "Character/SoCharacterStrike.h"


bool USoCharacterStrikeThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	USoCharacterStrike* CharStrike = Cast<USoCharacterStrike>(Object);
	return CharStrike != nullptr && CharStrike->GetCooldownIcon() != nullptr;
}

void USoCharacterStrikeThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas)
{
	// If any of these are null it will simply show the default text
	USoCharacterStrike* CharStrike = CastChecked<USoCharacterStrike>(Object);
	UTexture2D* Texture = CharStrike->GetCooldownIcon();
	if (Texture == nullptr)
		return;

	// Draw the Image
	FCanvasTileItem CanvasTile(FVector2D(X, Y), Texture->Resource, FVector2D(Width, Height), FLinearColor::White);
	CanvasTile.BlendMode = SE_BLEND_Opaque;
	CanvasTile.Draw(Canvas);
}
