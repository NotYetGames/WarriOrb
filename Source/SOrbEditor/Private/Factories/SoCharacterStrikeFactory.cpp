// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "Factories/SoCharacterStrikeFactory.h"
#include "Character/SoCharacterStrike.h"


///////////////////////////////////////////////////////
//// USoCharacterStrikeFactory
USoCharacterStrikeFactory::USoCharacterStrikeFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USoCharacterStrike::StaticClass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UObject* USoCharacterStrikeFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
													UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<USoCharacterStrike>(InParent, Class, Name, Flags | RF_Transactional);
}
