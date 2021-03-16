// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "fmod_studio_common.h"

namespace FMOD
{
namespace Studio
{
class EventDescription;
class EventInstance;
}
}

class FFMODEventEditor : public FAssetEditorToolkit
{
public:
    virtual void RegisterTabSpawners(const TSharedRef<class FTabManager> &NewTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager> &NewTabManager) override;

    /**
	* Edits the specified event
	*
	* @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	* @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	* @param	Event					The event to edit
	*/
    void InitFMODEventEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost> &InitToolkitHost, class UFMODEvent *Event);

    /** Constructor */
    FFMODEventEditor();

    /** Destructor */
    virtual ~FFMODEventEditor();

    UFMODEvent *GetEditedEvent() const;
    FMOD::Studio::EventDescription *GetEventDescription() const;
    void PlayEvent();
    void PauseEvent();
    void StopEvent();
    void SetParameterValue(int32 ParameterIdx, float Value);
    TArray<float> &GetParameterValues();

    /** IToolkit interface */
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;

    TArray<float> ParameterValues;

private:
    FMOD::Studio::EventInstance *CurrentPreviewEventInstance;

    void HandlePreBanksReloaded();
    void HandleBanksReloaded();
    void HandleBeginPIE(bool bSimulating);

    /** Creates all internal widgets for the tabs to point at */
    void CreateInternalWidgets();

    /**	Spawns the tab with the FMOD event inside */
    TSharedRef<SDockTab> SpawnTab_EventEditor(const FSpawnTabArgs &Args);

    TSharedPtr<class SFMODEventEditorPanel> FMODEventEditorPanel;
    TSharedPtr<SDockTab> OwnerTab;

    /**	The tab id for the event editor tab */
    static const FName EventEditorTabId;

    /** FMOD event editor app identifier string */
    static const FName FMODEventEditorAppIdentifier;

    class UFMODEvent *EditedEvent;

    FDelegateHandle BeginPIEDelegateHandle;
};
