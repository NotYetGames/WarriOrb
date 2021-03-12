// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SoALeverPush.generated.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoALeverPush : public USoActivity
{
	GENERATED_BODY()
public:

	USoALeverPush();

	virtual void Tick(float DeltaSeconds) override;

	virtual bool DecreaseHealth(const FSoDmg& Damage) override;

	virtual void InteractKeyPressed(bool bPrimary) override;

	virtual void JumpPressed() override {};
	virtual void Move(float Value) override {};
	
	void SetLeverData(FSoLeverData InLeverData);

	const FSoLeverData& GetLeverData() const;

	bool IsFinished() const;

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

protected:

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	FSoLeverData LeverData;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float WaitAfterLeverPull = -1;

	bool bIsFinished = false;
};
inline void USoALeverPush::SetLeverData(FSoLeverData InLeverData) { LeverData = InLeverData; }
inline const FSoLeverData& USoALeverPush::GetLeverData() const { return LeverData; }
inline bool USoALeverPush::IsFinished() const { return bIsFinished; }
