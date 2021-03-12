// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "Delegates/DelegateCombinations.h"
#include "Items/SoItemTypes.h"
#include "CharacterBase/SoIMortalTypes.h"

#include "SoUITypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSoUI, Log, All);
// DECLARE_LOG_CATEGORY_EXTERN(LogSoUI, All, All);

class UUserWidget;
class USoUIUserWidget;
class USoUIUserWidgetArray;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input for the UI
// to create a new event a new entry has to be added here, and an event has to be created with the proper name
// name is e.g. "Left" from EUC_Left, and also Action2 from both EUC_Action2 and EUC_RAction2
// See ASoCharacter::SetupPlayerInputComponent for setup of these commands
//
//
// EUC_Left/EUC_Right - are used in game but also in the UI
//					  - sync with EUC_MainMenuLeft/EUC_MainMenuRight if modified
// EUC_Up/EUC_Down  - are only used in the ui
//				  - sync with EUC_MainMenuUp/EUC_MainMenuDown if modified
UENUM(BlueprintType)
enum class ESoUICommand : uint8
{
	// first commands supports input binding (action pressed events)

	// navigation in panels (has to stay with value 0..3, because stuff is indexed with it)
	// NOTE: left/right is also used for the in game input to move left or right ;)
	EUC_Left = 0			UMETA(DisplayName = "Left"),
	EUC_Right				UMETA(DisplayName = "Right"),
	EUC_Up					UMETA(DisplayName = "Up"),
	EUC_Down				UMETA(DisplayName = "Down"),

	// top level left/right command, most likely q/e on keyboard
	EUC_TopLeft				UMETA(DisplayName = "TopLeft"),
	EUC_TopRight			UMETA(DisplayName = "TopRight"),

	// j - e.g. equip / change slot item
	EUC_Action0				UMETA(DisplayName = "Action0"),
	// i - e.g. clear slot
	EUC_Action1				UMETA(DisplayName = "Action1"),
	// k - e.g. compare while pressed
	EUC_Action2				UMETA(DisplayName = "Action2"),

	EUC_ActionBack			UMETA(DisplayName = "ActionBack (back/exit)"),

	// Constants that allows us to always control the menu
	// NOTE: these differ by Left/Right/Up/Down by not allowing the user to change them
	EUC_MainMenuLeft		UMETA(DisplayName = "MainMenuLeft"),
	EUC_MainMenuRight		UMETA(DisplayName = "MainMenuRight"),
	EUC_MainMenuUp			UMETA(DisplayName = "MainMenuUp"),
	EUC_MainMenuDown		UMETA(DisplayName = "MainMenuDown"),
	EUC_MainMenuEnter		UMETA(DisplayName = "MainMenuEnter"),
	EUC_MainMenuBack		UMETA(DisplayName = "MainMenuBack"),

	// Spell UI commands
	EUC_SpellLeft			UMETA(DisplayName = "SpellLeft"),
	EUC_SpellRight			UMETA(DisplayName = "SpellRight"),
	EUC_SpellUp				UMETA(DisplayName = "SpellUp"),
	EUC_SpellDown			UMETA(DisplayName = "SpellDown"),
	EUC_SpellSelect			UMETA(DisplayName = "SpellSelect"),
	EUC_SpellSelectAndCast	UMETA(DisplayName = "SpellSelectAndCast"),

	EUC_PressedMax			UMETA(Hidden),

	// all released event has to start with EUC_R!!! - they react to action released events
	EUC_RAction2			UMETA(DisplayName = "Action2Up"),
	EUC_ReleasedMax			UMETA(Hidden),
};


//
// ID for the ingame editor bp widgets
//
UENUM(BlueprintType)
enum class ESoEditorUI : uint8
{
	EEUI_Camera		UMETA(DisplayName = "Camera"),
	EEUI_Sky		UMETA(DisplayName = "Sky"),
	EEUI_CharShadow	UMETA(DisplayName = "CharShadow"),

	EEUI_MAX		UMETA(Hidden),
};


//
// ID for the UI panels
//
UENUM(BlueprintType)
enum class ESoUIPanel : uint8
{
	// main char UI with char sheet, inventory, skills, etc.
	EUP_Character		UMETA(DisplayName = "Character"),
	EUP_ItemContainer	UMETA(DisplayName = "ItemContainer"),
	EUP_Book			UMETA(DisplayName = "Book"),

	EUP_MAX				UMETA(Hidden),
};

//
// ID for Event types, does not have anything to do with ISoUIEventHandler interface
//
UENUM(BlueprintType)
enum class ESoUIEventType : uint8
{
	None = 0,

	MouseEnter,
	MouseLeave,
	MouseButtonDown,
	MouseButtonUp
};

//
// UI Event Handler
//
UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoUIEventHandler : public UInterface
{
	GENERATED_UINTERFACE_BODY()

};

class SORB_API ISoUIEventHandler
{
	GENERATED_IINTERFACE_BODY()

	// Return true if command was handled, false otherwise
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = UI)
	bool OnUICommand(ESoUICommand Command);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = UI)
	void Open(bool bOpen);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = UI)
	bool IsOpened() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = UI)
	bool CanBeInterrupted() const;
};


USTRUCT(BlueprintType)
struct FSoUIWidgetArray
{
	GENERATED_USTRUCT_BODY()
public:

	// mesh to fade out
	UPROPERTY()
	TArray<UUserWidget*> Widgets;
};


USTRUCT(BlueprintType)
struct FSoUITextCommandPair
{
	GENERATED_USTRUCT_BODY()

public:
	FSoUITextCommandPair() {}
	FSoUITextCommandPair(ESoUICommand InCommand, FText InText) :
		Command(InCommand), Text(InText) {}

public:
	UPROPERTY(BlueprintReadWrite)
	ESoUICommand Command = ESoUICommand::EUC_ReleasedMax;

	UPROPERTY(BlueprintReadWrite)
	FText Text;
};


// Track the total time spent on a key down
USTRUCT(BlueprintType)
struct FSoUITrackedInputKeyDown
{
	GENERATED_USTRUCT_BODY()
public:

	// Time in seconds after which we move in one direction or another if the move key is pressed down
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Time")
	float KeyDownThresholdSeconds = 0.02f;

	// Delay in seconds before triggering the moving
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Time")
	float KeyDownThresholdDelaySeconds = 0.5f;

	// Are we tracking?
	// TickDelayTimeSum > KeyDownThresholdDelaySeconds
	UPROPERTY()
	bool bTrack = false;

	// Counter, current Total time spent in Tick()
	UPROPERTY()
	float TickTimeSum = 0.f;

	// Counter, current time open in Tick(), should be larger than KeyDownThresholdDelaySeconds
	// For bTrack to be set to true
	UPROPERTY()
	float TickDelayTimeSum = 0.f;

public:
	FORCEINLINE void Reset()
	{
		bTrack = false;
		TickTimeSum = 0.f;
		TickDelayTimeSum = 0.f;
	}

	FORCEINLINE bool Tick(float InDeltaTime, TFunction<void()> CallbackSuccess)
	{
		if (!InternalPreTick(InDeltaTime))
			return false;

		if (!bTrack)
			return false;

		// Update sum
		TickTimeSum += InDeltaTime;

		// Can Call
		if (TickTimeSum > GetKeyDownThresholdSeconds())
		{
			CallbackSuccess();

			TickTimeSum = 0.f;
			return true;
		}

		return false;
	}

	float GetKeyDownThresholdDelaySeconds() const;
	float GetKeyDownThresholdSeconds() const;


protected:
	// Tick the update to bTrack
	// If it returns false we should abort the NativeTick of the calling function
	FORCEINLINE bool InternalPreTick(float InDeltaTime)
	{
		if (bTrack)
			return true;

		// Update delay sum
		TickDelayTimeSum += InDeltaTime;

		// Can finally start tracking
		if (TickDelayTimeSum > GetKeyDownThresholdDelaySeconds())
		{
			bTrack = true;
			return true;
		}

		return false;
	}
};


//
// Delegates
//

// we need a non-multicast version for the delegates for the subscribe function
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoEquipmentSlotChanged, ESoItemSlot, Slot);
DECLARE_DYNAMIC_DELEGATE_OneParam(FSoEquipmentSlotChangedSingle, ESoItemSlot, Slot);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoUINotify);
DECLARE_DYNAMIC_DELEGATE(FSoUINotifySingle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoUINotifyTwoInt, int32, Current, int32, Max);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FSoUINotifyTwoIntSingle, int32, Current, int32, Max);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoUINotifyTwoFloat, float, Current, float, Max);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FSoUINotifyTwoFloatSingle, float, Current, float, Max);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoUINotifyDmg, const FSoDmg&, UnresistedDmg);
DECLARE_DYNAMIC_DELEGATE_OneParam(FSoUINotifyDmgSingle, const FSoDmg&, UnresistedDmg);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSoUIPanelEvent, ESoUIPanel, Panel, bool, bOpened, AActor*, Initiator);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FSoUIPanelEventSingle, ESoUIPanel, Panel, bool, bOpened, AActor*, Initiator);


DECLARE_MULTICAST_DELEGATE_OneParam(FSoUIEventTypeEvent, ESoUIEventType);
DECLARE_MULTICAST_DELEGATE_OneParam(FSoUIUserWidgetRequestHighlightChangeEvent, bool);

// Event Happened
DECLARE_MULTICAST_DELEGATE(FSoUIUserWidgetEventCPP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoUIUserWidgetEvent);

DECLARE_MULTICAST_DELEGATE_OneParam(FSoUIUserWidgetChangedEventCPP, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoUIUserWidgetChangedEvent, bool, bNewValue);

// Navigate
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoPreNavigateEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoPostNavigateEvent, bool, bHandled);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoNavigateOnPressedHandleChilEvent, int32, ChildIndex, USoUIUserWidget*, Widget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoNavigateOnPressedHandleChildArrayEvent, int32, ChildIndex, USoUIUserWidgetArray*, ArrayWidget);

DECLARE_DELEGATE_TwoParams(FSoNavigateOnPressedHandleChilEventCPP, int32, USoUIUserWidget*);
DECLARE_DELEGATE_TwoParams(FSoNavigateOnPressedHandleChildArrayEventCPP, int32, USoUIUserWidgetArray*);


// Handle child
DECLARE_MULTICAST_DELEGATE_TwoParams(FSoPostSetSelectedIndexEvent, int32, int32);

// Array Child highlight changed
DECLARE_MULTICAST_DELEGATE_TwoParams(FSoArrayChildBoolChangedEventCPP, int32, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoArrayChildBoolChangedEvent, int32, ChildIndex, bool, bHighlight);


// UI opened changed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoUIOpenChangedEvent, bool, bOpened);
