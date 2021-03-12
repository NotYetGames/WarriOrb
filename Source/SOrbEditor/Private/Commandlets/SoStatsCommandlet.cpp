// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoStatsCommandlet.h"


DEFINE_LOG_CATEGORY(LogSoStatsCommandlet);


USoStatsCommandlet::USoStatsCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
	ShowErrorCount = true;
}


int32 USoStatsCommandlet::Main(const FString& Params)
{
	UE_LOG(LogSoStatsCommandlet, Display, TEXT("Starting"));

	// Parse command line - we're interested in the param vals
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamVals;
	UCommandlet::ParseCommandLine(*Params, Tokens, Switches, ParamVals);

	return 0;
}
