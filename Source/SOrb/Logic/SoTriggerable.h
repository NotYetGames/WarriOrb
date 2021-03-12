// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"

#include "SoTriggerable.generated.h"

/**
*  Struct used by a weird communication channel between objects
*  SourceIdentifier is typically the ID of the triggering object, but can be more to give extra information (e.g. triggered movement direction)
*  TargetIdentifiers: can be empty or can contain any integer which can mean pretty much anything based on the object
*  Interpretation of the data depends on the receiver, the sender should respect that
*/
USTRUCT(BlueprintType)
struct FSoTriggerData
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TriggerData)
	int32 SourceIdentifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TriggerData)
	TArray<int32> TargetIdentifiers;
};

/**
*  Helper struct for triggers
*/
USTRUCT(BlueprintType)
struct FSoTriggerableData
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TriggerData)
	AActor* TargetTriggerable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TriggerData)
	FSoTriggerData TriggerData;
};
/**
*  More struct -> more fun
*/
USTRUCT(BlueprintType)
struct FSoTriggerableDataArray
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TriggerData)
	AActor* TargetTriggerable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TriggerData)
	TArray<FSoTriggerData> TriggerData;
};


/** Triggerable interface helper functions */
UCLASS()
class SORB_API USoTriggerHelper : public UObject
{
	GENERATED_UCLASS_BODY()
public:

	virtual ~USoTriggerHelper();

	UFUNCTION(BlueprintCallable, Category = Trigger)
	static void TriggerAllElement(UPARAM(ref)TArray<FSoTriggerableData>& TriggerableData);

	UFUNCTION(BlueprintCallable, Category = Trigger)
	static void TriggerAll(UPARAM(ref)TArray<FSoTriggerableDataArray>& TriggerData);

	UFUNCTION(BlueprintCallable, Category = Trigger)
	static void TriggerActor(AActor* ActorToTrigger, int32 SourceIdentifier);

	UFUNCTION(BlueprintCallable, Category = Trigger)
	static void TriggerActorArray(TArray<AActor*> ActorsToTrigger, int32 SourceIdentifier);
};


/**
 *  Interface to provide a stupid way of communication between different objects
 *  (e.g. a lever can send message to the three-story elevator with the 12 doors to open/close a few)
 *  both C++ and Blueprint classes can implement and use the interface
 */
UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoTriggerable : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
*  Interface to provide a stupid way of communication between different objects
*  (e.g. a lever can send message to the three-story elevator with the 12 doors to open/close a few)
*  both C++ and Blueprint classes can implement and use the interface
*/
class SORB_API ISoTriggerable
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Triggerable)
	void Trigger(const FSoTriggerData& TriggerData);
};
