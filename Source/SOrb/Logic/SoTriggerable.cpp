// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoTriggerable.h"

#include "GameFramework/Actor.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoTriggerable::USoTriggerable(const FObjectInitializer& PCIP)
	: Super(PCIP)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoTriggerHelper::USoTriggerHelper(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoTriggerHelper::~USoTriggerHelper()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoTriggerHelper::TriggerAllElement(TArray<FSoTriggerableData>& TriggerableData)
{
	for (FSoTriggerableData& Entry : TriggerableData)
		if (Entry.TargetTriggerable != nullptr && Entry.TargetTriggerable->GetClass()->ImplementsInterface(USoTriggerable::StaticClass()))
			ISoTriggerable::Execute_Trigger(Entry.TargetTriggerable, Entry.TriggerData);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoTriggerHelper::TriggerAll(TArray<FSoTriggerableDataArray>& TriggerData)
{
	for (FSoTriggerableDataArray& Entry : TriggerData)
	{
		if (Entry.TargetTriggerable != nullptr && Entry.TargetTriggerable->GetClass()->ImplementsInterface(USoTriggerable::StaticClass()))
		{
			for (const FSoTriggerData& Data : Entry.TriggerData)
				ISoTriggerable::Execute_Trigger(Entry.TargetTriggerable, Data);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoTriggerHelper::TriggerActor(AActor* ActorToTrigger, int32 SourceIdentifier)
{
	if (ActorToTrigger != nullptr)
		ISoTriggerable::Execute_Trigger(ActorToTrigger, { SourceIdentifier, {} });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoTriggerHelper::TriggerActorArray(TArray<AActor*> ActorsToTrigger, int32 SourceIdentifier)
{
	for (AActor* Actor : ActorsToTrigger)
		if (Actor != nullptr)
			ISoTriggerable::Execute_Trigger(Actor, { SourceIdentifier,{} });
}
