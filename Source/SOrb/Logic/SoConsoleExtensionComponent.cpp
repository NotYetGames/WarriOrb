// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoConsoleExtensionComponent.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"


USoConsoleExtensionComponent::USoConsoleExtensionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void USoConsoleExtensionComponent::BeginPlay()
{
	Super::BeginPlay();
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	for (const UField* Field = OwnerActor->GetClass()->Children; Field != nullptr; Field = Field->Next)
	{
		if (const UFunction* Function = Cast<UFunction>(Field))
		{
			const FString FunctionName = Field->GetName();
			if (PreFix.Compare(FunctionName.Left(PreFix.Len())) == 0)
			{
				ConsoleObjects.Add(ConsoleManager.RegisterConsoleCommand(
					*FunctionName,
					*FunctionName,
					FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([this, FunctionName](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
					{
						if (AActor* Actor = GetOwner())
						{
							FString Command = FunctionName;
							for (const FString& Param : Args)
								Command += FString(" ") + Param;

							Actor->CallFunctionByNameWithArguments(*Command, Ar, UGameplayStatics::GetPlayerCharacter(Actor, 0), true);
						}
					}), ECVF_Default)
				);
			}
		}
	}
}


void USoConsoleExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (IConsoleObject* Object : ConsoleObjects)
		IConsoleManager::Get().UnregisterConsoleObject(Object);

	ConsoleObjects.Empty();

	Super::EndPlay(EndPlayReason);
}
