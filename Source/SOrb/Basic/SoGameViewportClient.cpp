// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoGameViewportClient.h"

#include "SoGameInstance.h"
#include "Engine/Canvas.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameViewportClient::Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice)
{
	Super::Init(WorldContext, OwningGameInstance, bCreateNewAudioDevice);
	if (USoGameInstance* SoGameInstance = GetSoGameInstance())
		SoGameInstance->SetGameViewportClient(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameViewportClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameViewportClient::LostFocus(FViewport* InViewport)
{
	Super::LostFocus(InViewport);
	if (USoGameInstance* SoGameInstance = GetSoGameInstance())
		SoGameInstance->OnGameViewportFocusLost();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameViewportClient::ReceivedFocus(FViewport* InViewport)
{
	Super::ReceivedFocus(InViewport);
	if (USoGameInstance* SoGameInstance = GetSoGameInstance())
		SoGameInstance->OnGameViewportFocusReceived();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoGameViewportClient::ConsoleCommand(const FString& Command)
{
	return Super::ConsoleCommand(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameViewportClient::PostRender(UCanvas* Canvas)
{
	Super::PostRender(Canvas);

	// Fade if requested, you could use the same DrawScreenFade method from any canvas such as the HUD
	if (bIsFading)
	{
		DrawScreenFade(Canvas);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameInstance* USoGameViewportClient::GetSoGameInstance()
{
	return Cast<USoGameInstance>(GameInstance);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameViewportClient* USoGameViewportClient::GetInstance()
{
	if (!GEngine)
		return nullptr;

	return Cast<USoGameViewportClient>(GEngine->GameViewport);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameViewportClient::SetIsFading(bool bValue)
{
	bIsFading = bValue;
	OnFadingChanged.Broadcast(bIsFading);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameViewportClient::ClearFade()
{
	SetIsFading(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameViewportClient::Fade(float Duration, bool bInToBlack)
{
	if (!World)
		return;

	SetIsFading(true);
	bToBlack = bInToBlack;
	FadeDuration = Duration;
	FadeStartTime = World->GetRealTimeSeconds();
	check(!(FadeStartTime < 0.f));
	check(FadeDuration > 0.f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameViewportClient::DrawScreenFade(UCanvas* Canvas)
{
	if (!bIsFading)
		return;

	if (!World)
		return;

	const float Time = World->GetRealTimeSeconds();
	if (Time < FadeStartTime)
	{
		// Most likely these two are from different worlds, reset timer to now
		// This would cause a bug when transitioning to the main menu level
		FadeStartTime = Time;
	}
	check(Time >= FadeStartTime);

	// Make sure that we stay black in a fade to black
	const float Alpha = FMath::Clamp((Time - FadeStartTime) / FadeDuration, 0.f, 1.f);
	if (FMath::IsNearlyEqual(Alpha, 1.f) && !bToBlack)
	{
		// Finished fading
		SetIsFading(false);
	}
	else
	{
		const FColor OldColor = Canvas->DrawColor;
		FLinearColor FadeColor = FLinearColor::Black;
		FadeColor.A = bToBlack ? Alpha : 1 - Alpha;
		Canvas->DrawColor = FadeColor.ToFColor(true);
		Canvas->DrawTile(Canvas->DefaultTexture, 0, 0, Canvas->ClipX, Canvas->ClipY, 0, 0, Canvas->DefaultTexture->GetSizeX(), Canvas->DefaultTexture->GetSizeY());
		Canvas->DrawColor = OldColor;
	}
}
