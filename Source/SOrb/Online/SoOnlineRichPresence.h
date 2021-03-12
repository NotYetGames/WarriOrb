// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "SoOnlineRichPresence.generated.h"

class USoPlayerProgress;


// Controls what appears in online applications status
UCLASS()
class SORB_API USoOnlineRichPresence : public UObject
{
	GENERATED_BODY()
public:
	USoOnlineRichPresence();

	void SetOwner(USoPlayerProgress* InOwner);
	void OnSplineChanged();
	void OnChapterChanged();

protected:
	void UpdateFromCurrentState();

protected:
	static const FName TokenMenu;
	static const FName TokenChapter;
	static const FName TokenEpisode;

	UPROPERTY()
	USoPlayerProgress* SoOwner;
};
