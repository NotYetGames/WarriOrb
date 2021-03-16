// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

#include "Modules/ModuleManager.h"

namespace FMOD
{
namespace Studio
{
class System;
class EventDescription;
class EventInstance;
}
}

class UFMODAsset;
class UFMODEvent;
class UWorld;
class AAudioVolume;
struct FInteriorSettings;
struct FFMODListener; // Currently only for private use, we don't export this type

// Which FMOD Studio system to use
namespace EFMODSystemContext
{
enum Type
{
    // For use auditioning sounds within the editor
    Auditioning,

    // For use in PIE and in-game
    Runtime,

    // For use when playing events in an editor world (e.g. previewing a level sequence)
    Editor,

    // Max number of types
    Max
};
}

/**
 * The public interface to this module
 */
class IFMODStudioModule : public IModuleInterface
{
public:
    /**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
    static inline IFMODStudioModule &Get() { return FModuleManager::LoadModuleChecked<IFMODStudioModule>("FMODStudio"); }

    /**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
    static inline bool IsAvailable() { return FModuleManager::Get().IsModuleLoaded("FMODStudio"); }

    /**
	 * Get a pointer to the runtime studio system (only valid in-game or in PIE)
	 */
    virtual FMOD::Studio::System *GetStudioSystem(EFMODSystemContext::Type Context) = 0;

    /**
	 * Set system paused (for PIE pause)
	 */
    virtual void SetSystemPaused(bool paused) = 0;

    /**
	 * Called when user changes any studio settings
	 */
    virtual void RefreshSettings() = 0;

    /**
	 * Called when we enter of leave PIE mode
	 */
    virtual void SetInPIE(bool bInPIE, bool bSimulating) = 0;

    /**
	 * Look up an asset given its name
	 */
    virtual UFMODAsset *FindAssetByName(const FString &Name) = 0;

    /**
	 * Look up an event given its name
	 */
    virtual UFMODEvent *FindEventByName(const FString &Name) = 0;

    /**
	 * Get an event description.
	 * The system type can control which Studio system to use, or leave it as System_Max for it to choose automatically.
	 */
    virtual FMOD::Studio::EventDescription *GetEventDescription(
        const UFMODEvent *Event, EFMODSystemContext::Type Context = EFMODSystemContext::Max) = 0;

    /**
	 * Create a single auditioning instance using the auditioning system
	 */
    virtual FMOD::Studio::EventInstance *CreateAuditioningInstance(const UFMODEvent *Event) = 0;

    /**
	 * Stop any auditioning instance
	 */
    virtual void StopAuditioningInstance() = 0;

    /**
	 * Return whether the listener(s) have moved
	 */
    virtual bool HasListenerMoved() = 0;

    /**
	 * Called to change the listener position for editor mode
	 */
    virtual void SetListenerPosition(int ListenerIndex, UWorld *World, const FTransform &ListenerTransform, float DeltaSeconds) = 0;

    /**
	 * Called to change the listener position for editor mode
	 */
    virtual void FinishSetListenerPosition(int NumListeners, float DeltaSeconds) = 0;

    /**
	 * Return the audio settings for the listener nearest the given location
	 */
    virtual const FFMODListener &GetNearestListener(const FVector &Location) = 0;

    /** This event is fired after all banks were reloaded */
    virtual FSimpleMulticastDelegate &BanksReloadedEvent() = 0;

    /** Return a list of banks that failed to load due to an error */
    virtual TArray<FString> GetFailedBankLoads(EFMODSystemContext::Type Context) = 0;

    /** Return a list of plugins that appear to be needed  */
    virtual TArray<FString> GetRequiredPlugins() = 0;

    /** Register a plugin that is required */
    virtual void AddRequiredPlugin(const FString &Plugin) = 0;

    /** Returns whether sound is enabled for the game */
    virtual bool UseSound() = 0;

    /** Attempts to load a plugin by name */
    virtual bool LoadPlugin(EFMODSystemContext::Type Context, const TCHAR *ShortName) = 0;

    /** Log a FMOD error */
    virtual void LogError(int result, const char *function) = 0;

    /** Returns if the banks have been loaded */
    virtual bool AreBanksLoaded() = 0;
};
