// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

 #include "Engine/GameViewportClient.h"
 #include "SoGameViewportClient.generated.h"

class USoGameInstance;



 UCLASS()
class SORB_API USoGameViewportClient : public UGameViewportClient
 {
	 GENERATED_BODY()
 public:

	// UGameViewportClient Interface
	void Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice = true) override;
	void LostFocus(FViewport* InViewport) override;
	void ReceivedFocus(FViewport* InViewport) override;
	FString ConsoleCommand(const FString& Command) override;
	void Tick(float DeltaTime) override;
	void PostRender(UCanvas* Canvas) override;

	void FadeIn(float Duration) { Fade(Duration, true); }
	void FadeOut(float Duration) { Fade(Duration, false); }
	bool IsFading() const { return bIsFading; }

	static USoGameViewportClient* GetInstance();

	/** Clear fading state */
	void ClearFade();

public:
	DECLARE_EVENT_OneParam(USoGameViewportClient, FSoGameViewportFadingDelegate, bool)
	FSoGameViewportFadingDelegate OnFadingChanged;

protected:
	// Own methods
	USoGameInstance* GetSoGameInstance();

	/** Used for Fade to and from black */
	void Fade(float Duration, bool bInToBlack);

	/** Does the actual screen fading */
	void DrawScreenFade(UCanvas* Canvas);

	void SetIsFading(bool bValue);

protected:
	// Values used by our screen fading
	bool bIsFading = false;
	bool bToBlack = false; // Fading to black will be applied even if alpha is 1
	float FadeAlpha = 0.f;
	float FadeStartTime = 0.f;
	float FadeDuration = 0.f;
};
