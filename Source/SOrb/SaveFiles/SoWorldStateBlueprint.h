// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Basic/SoDifficulty.h"

#include "SoWorldStateBlueprint.generated.h"

struct FSoStateMetadataSaveSlotEntry;
class AActor;

/**
 *   Interface to load/write data to the storage
 *
 *  There are different ways to store information here, more specific ones will be created when it becomes more clear what we need to store (for player, npc-s, shops, etc.)
 *
 *  - Entries:
 *		Each actor can store any int/float/vector values with a given ValueName
 *		Values are stored based on LevelName and ActorName, so ValueName has to be unique only within the same actor for the same variable types
 *		It is something like this:
 *		Map:
 *			Key: LevelName
 *			Value: Map:
 *						Key: ActorName
 *						Value: Struct
 *									Map<ValueName, int32>
 *									Map<ValueName, float>
 *									Map<ValueName, fvector>
 *
 *		In reality it has some extra Struct wrapper so it works with UPROPERTY() macro (nested containers are not supported directly)
 *
 *	- NameSets:
 *		Each level has a set of names
 *		Each actor can add/remove/check names from his level
 *
 *	- ItemMap:
 *		ItemList-s can be saved/loaded
 *		unique FName is used for identification
 *
 * Typical usage:
 *		read values in BeginPlay()
 *		write values when they are changed / in EndPlay()
 *
 *  Despite Yoda's wisdom it is absolutely ok to try to read not yet written values (out reference is ignored)
 */
UCLASS()
class SORB_API USoWorldState : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	// functions to store value in the global entry storage
	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static bool ReadIntValue(const AActor* Actor, FName ValueName, UPARAM(Ref)int32& Value);

	/** returns with int value or 0 if it did not exist */
	UFUNCTION(BlueprintPure, Category = "SoWorldState", meta = (WorldContext = "WorldContextObject"))
	static int32 GetIntValue(const UObject* WorldContextObject, FName ValueName);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static bool ReadBoolValue(const AActor* Actor, FName ValueName, UPARAM(Ref)bool& bValue);

	UFUNCTION(BlueprintPure, Category = "SoWorldState", meta = (WorldContext = "WorldContextObject"))
	static bool GetBoolValue(const UObject* WorldContextObject, FName ValueName);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static void WriteIntValue(const AActor* Actor, FName ValueName, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static void WriteBoolValue(const AActor* Actor, FName ValueName, bool bValue);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static bool ReadFloatValue(const AActor* Actor, FName ValueName, UPARAM(Ref)float& Value);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static void WriteFloatValue(const AActor* Actor, FName ValueName, float Value);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static bool ReadVectorValue(const AActor* Actor, FName ValueName, UPARAM(Ref)FVector& Value);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static void WriteVectorValue(const AActor* Actor, FName ValueName, const FVector& Value);

	// each level has one StringSet
	// actors can add, remove or check strings there
	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static void AddNameToSet(const AActor* Actor, FName Name);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static void AddActorNameToSet(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static void RemoveActorNameFromSet(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static void RemoveNameFromSet(const AActor* Actor, FName Name);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static bool IsNameInSet(const AActor* Actor, FName Name);

	UFUNCTION(BlueprintPure, Category = "SoWorldState")
	static bool IsActorNameInSet(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState", meta = (WorldContext = "WorldContextObject"))
	static void AddMyNameToSet(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SoWorldState", meta = (WorldContext = "WorldContextObject"))
	static void RemoveMyNameFromSet(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "SoWorldState", meta = (WorldContext = "WorldContextObject"))
	static bool IsMyNameInSet(const UObject* WorldContextObject);


	UFUNCTION(BlueprintCallable, Category = "SoWorldState")
	static void SetDisplayedProgressName(FName DisplayedProgressName);


	UFUNCTION(BlueprintPure, Category = "SoWorldState", meta = (WorldContext = "WorldContextObject"))
	static ESoDifficulty GetGameDifficulty();

private:
	static bool CheckAndValidateFName(FName Name);
};
