// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SplineLogic/SoSkyControlData.h"
#include "SoASkyControlEdit.generated.h"


UCLASS()
class SORB_API USoASkyControlEdit : public USoActivity
{

	GENERATED_BODY()

public:

	USoASkyControlEdit();

	void InitializePresets(const TArray<FName>& InPresetNames) { PresetNames = InPresetNames; }
	
	virtual void Tick(float DeltaSeconds) override;	
	// show/hide help text
	virtual void InteractKeyPressed(bool bPrimaryKey) override;

	// save/load from file
	void SaveEditedData() override;
	void LoadEditedData() override;
	// add/move/remove key node
	void CreateKey() override;
	void MoveClosestKeyHere() override;
	void DeleteActiveKeyNode() override;

	virtual void CopyActiveKeyData() override;
	virtual void PasteToActiveKeyData() override;

	// teleport the character into the KeyNode position
	void JumpToNextKey() override;
	void JumpToPrevKey() override;

	virtual void ToggleWeapons() override {};

	// switches preset
	virtual void SpecialEditButtonPressed(int32 Index) override;

	virtual void SuperEditModePressed() override;

	// get data for gui
	UFUNCTION(BlueprintCallable, Category = UIInfo)
	void IsOnKey(bool& bOnKey, int32& NodeIndex, FName& Preset) const;

	UFUNCTION(BlueprintCallable, Category = UIInfo)
	void GetPrevKeyNodeData(FName& Preset, float& Percent, float& Distance) const;

	UFUNCTION(BlueprintCallable, Category = UIInfo)
	void GetNextKeyNodeData(FName& Preset, float& Percent, float& Distance) const;

	UFUNCTION(BlueprintCallable, Category = UIInfo)
	int32 GetNodeCount() const;

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

protected:

	UPROPERTY(BlueprintReadOnly, Category = Sky)
	bool bOnKeyPosition = false;

	UPROPERTY(BlueprintReadOnly, Category = Sky)
	bool bEditHintVisible = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Sky)
	TArray<FName> PresetNames;

	// protection from everything + the ability to fly
	bool bSuperMode = false;

	FSoSkyControlPoint NodeCopy;
	bool bNodeCopyInitialized = false;
};

