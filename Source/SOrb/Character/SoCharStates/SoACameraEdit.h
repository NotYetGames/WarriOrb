// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SplineLogic/SoCameraData.h"
#include "SoACameraEdit.generated.h"

USTRUCT(BlueprintType)
struct FSoCamKeyPreset
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadOnly)
	FString Name;

	UPROPERTY(BlueprintReadOnly)
	TArray<FSoCamKeyNode> Variants;
};

UCLASS()
class SORB_API USoACameraEdit : public USoActivity
{

	GENERATED_BODY()

public:

	USoACameraEdit();

	virtual void Tick(float DeltaSeconds) override;
	virtual void UpdateCamera(float DeltaSeconds) override;

	// show/hide help text
	virtual void InteractKeyPressed(bool bPrimaryKey) override;
	// switch preset subtype
	virtual void DebugFeature() override;

	// save/load from file
	void SaveEditedData() override;
	void LoadEditedData() override;
	// add/move/remove key node
	void CreateKey() override;
	void MoveClosestKeyHere() override;
	void DeleteActiveKeyNode() override;
	// modify the edge property (KeyNode 0 modifies the behavior between KeyNode 0..1, KeyNode N doesn't do anything)
	// if the interpolation is spline based, it calculates with the interpolated spline position, otherwise it interpolates linearly between the two KeyNode
	void SpecialEditButtonPressed(int32 Index) override;

	virtual void CopyActiveKeyData() override;
	virtual void PasteToActiveKeyData() override;

	// teleport the character into the KeyNode position
	void JumpToNextKey() override;
	void JumpToPrevKey() override;

	virtual void ToggleWeapons() override;

	virtual void SuperEditModePressed() override;

	// get data for gui
	UFUNCTION(BlueprintCallable, Category = Camera)
	float GetPrevCamKeyNodeSplineDistance() const;

	UFUNCTION(BlueprintCallable, Category = Camera)
	float GetNextCamKeyNodeSplineDistance() const;

	int32 GetClosestCamIndex() const;

	virtual bool OnDmgTaken(const FSoDmg& Damage, const FSoHitReactDesc& HitReactDesc) override { return true; }

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;
	
	virtual bool ShouldUpdateMeshFade() const override { return false; }

protected:

	UPROPERTY(BlueprintReadOnly, Category = Camera)
	bool bOnCamKeyPosition = false;

	UPROPERTY(BlueprintReadOnly, Category = Camera)
	bool bCamEditHintVisible = false;

	// <Presets>
	UPROPERTY(BlueprintReadOnly, Category = Camera)
	bool bPresetUsed = false;

	UPROPERTY(BlueprintReadOnly, Category = Camera)
	int32 ActivePreset = 0;

	UPROPERTY(BlueprintReadOnly, Category = Camera)
	int32 ActivePresetSubType = 0;

	UPROPERTY(BlueprintReadOnly, Category = Camera)
	TArray<FSoCamKeyPreset> Presets;
	// </Presets>


	// protection from everything + the ability to fly
	bool bSuperMode = true;

	FSoCamKeyNode NodeCopy;
	bool bNodeCopyInitialized = false;
};

