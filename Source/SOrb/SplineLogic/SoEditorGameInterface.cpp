// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEditorGameInterface.h"

#include "Misc/Paths.h"
#include "EngineUtils.h"

#include "IO/DlgConfigWriter.h"
#include "IO/DlgConfigParser.h"

#include "SplineLogic/SoSplineHelper.h"
#include "SplineLogic/SoSplineGraph.h"
#include "SoCameraData.h"
#include "SoPlayerSpline.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Levels/SoLevelHelper.h"
#include "Basic/SoGameMode.h"
#include "Enemy/SoEnemySpawner.h"
#include "Enemy/SoEnemy.h"

DEFINE_LOG_CATEGORY(LogSoCameraDataIO);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorGameInterface::LoadCameraData(UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogSoCameraDataIO, Error, TEXT("LoadCameraData: Failed to load Camera Data: no world"));
		return false;
	}

	// In MainMenu, Ignore
	if (USoLevelHelper::IsInMenuLevel(World))
	{
		UE_LOG(LogSoCameraDataIO, Log, TEXT("LoadCameraData: In MainMenu, ignoring"));
		return false;
	}

	FSoWorldCamData CameraData;
	const FString FileName =
		TEXT("SO/Data/CameraData/") + USoLevelHelper::GetMapNameFromObject(World).ToString() + TEXT("CamData.txt");
	FDlgConfigParser Parser(FPaths::ProjectContentDir() + FileName, "So");

	Parser.ReadAllProperty(FSoWorldCamData::StaticStruct(), &CameraData, nullptr);
	UE_LOG(LogSoCameraDataIO, Log, TEXT("LoadCameraData: Camera Data Loaded from %s"), *FileName);

	// Setup for all splines
	for (TActorIterator<ASoPlayerSpline> ActorItr(World); ActorItr; ++ActorItr)
	{
		ASoPlayerSpline* SoSpline = *ActorItr;
		if (CameraData.Data.Contains(SoSpline->GetSplineName()))
			SoSpline->SetupCameraData(CameraData.Data[SoSpline->GetSplineName()]);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorGameInterface::SaveCameraData(UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogSoCameraDataIO, Error, TEXT("Failed to save camera data: no world!"));
		return false;
	}
	if (IsRunningCommandlet())
	{
		UE_LOG(LogSoCameraDataIO, Log, TEXT("SaveCameraData: Running commandlet, ignoring"));
		return false;
	}

	UE_LOG(LogSoCameraDataIO, Verbose, TEXT("SaveCameraData = %d"), GIsCookerLoadingPackage);
	const FString OutFileName =
		TEXT("SO/Data/CameraData/") + USoLevelHelper::GetMapNameStringFromObject(World) + TEXT("CamData.txt");

	// Save all spline
	FSoWorldCamData AllCam;
	TArray<FString> SplineNameList;
	for (TActorIterator<ASoPlayerSpline> ActorItr(World); ActorItr; ++ActorItr)
	{
		const ASoPlayerSpline* SoSpline = *ActorItr;
		FString SplineName = SoSpline->GetSplineName();
		
		// sanity check
		int32 Index = 0;
		const bool bAlreadyThere = SplineNameList.Find(SplineName, Index);
		if (bAlreadyThere)
		{
#if WITH_EDITOR
			UE_LOG(LogSoCameraDataIO, Error, TEXT("Two spline with the same name %s"), *SplineName);
#endif
			SplineName.Append("1");
		}

		SplineNameList.Add(SplineName);
		AllCam.Data.Add(SplineName, SoSpline->GetCamNodeList());
	}

	// sort to minimalize difference in output config file
	AllCam.Data.KeySort([](const FString& First, const FString& Second) { return First < Second; });

	FDlgConfigWriter Writer("So");
	Writer.Write(FSoWorldCamData::StaticStruct(), &AllCam);
	Writer.ExportToFile(FPaths::ProjectContentDir() + OutFileName);
	UE_LOG(LogSoCameraDataIO, Log, TEXT("SaveCameraData: Camera data saved to %s"), *OutFileName);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorGameInterface::ToggleSplinePosCorrection()
{
	USoSplineHelper::SetSplinePosCorrection(!USoSplineHelper::IsSplinePosCorrectionEnabled());
	return USoSplineHelper::IsSplinePosCorrectionEnabled();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorGameInterface::IsSplinePosCorrectionEnabled()
{
	return USoSplineHelper::IsSplinePosCorrectionEnabled();
}


// int32 Count = 0;
// for (TActorIterator<ASoSplineGraph> It(World, ASoSplineGraph::StaticClass()); It; ++It)
// {
// 	if (Count != 0)
// 	{
// 		UE_LOG(LogSoSplineGraph, Warning, TEXT("More than one spline graph is placed in the level, that should not happen!"));
// 		return false;
// 	}
// 	(*It)->Rebuild();
// 	++Count;
// }
//
// if (Count == 0)
// {
// 	UE_LOG(LogSoSplineGraph, Warning, TEXT("Failed to rebuild navigation graph: could not locate spline graph instance!"));
// 	return false;
// }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoEditorGameInterface::FixEnemyGroupName(FName& InOutName)
{
	if (InOutName == NAME_None)
		return;

	const FString GroupString = InOutName.ToString();
	static const FString PreFix = FString("EG_");
	if (!GroupString.Mid(0, 3).Equals(PreFix))
		InOutName = FName(*(PreFix + GroupString));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorGameInterface::GetEnemyGroupDataConfigPath(UWorld* World)
{
	if (World == nullptr)
		return "";

	return FPaths::ProjectContentDir() + TEXT("SO/AIConfig/EnemyGroups_") + USoLevelHelper::GetMapNameStringFromObject(World) + TEXT(".txt");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorGameInterface::BuildEnemyGroupDataConfigFile(UWorld* World)
{
	// In MainMenu, Ignore
	if (USoLevelHelper::IsInMenuLevel(World))
	{
		UE_LOG(LogSoCameraDataIO, Log, TEXT("BuildEnemyGroupDataConfigFile: In MainMenu, ignoring"));
		return false;
	}
	if (IsRunningCommandlet())
	{
		UE_LOG(LogSoCameraDataIO, Log, TEXT("BuildEnemyGroupDataConfigFile: Running commandlet, ignoring"));
		return false;
	}

	FSoEnemyGroups EnemyGroups;
	for (TActorIterator<ASoEnemy> It(World, ASoEnemy::StaticClass()); It; ++It)
	{
		const FName GroupName = (*It)->GetSoGroupName();
		int32* CountPtr = EnemyGroups.EnemyGroupMap.Find(GroupName);
		if (CountPtr == nullptr)
			EnemyGroups.EnemyGroupMap.Add(GroupName, 1);
		else
			EnemyGroups.EnemyGroupMap[GroupName] += 1;
	}

	for (TActorIterator<ASoEnemySpawner> It(World, ASoEnemySpawner::StaticClass()); It; ++It)
	{
		const FName GroupName = (*It)->GetSoGroupName();
		int32* CountPtr = EnemyGroups.EnemyGroupMap.Find(GroupName);
		if (CountPtr == nullptr)
			EnemyGroups.EnemyGroupMap.Add(GroupName, (*It)->GetEnemyNum());
		else
			EnemyGroups.EnemyGroupMap[GroupName] += (*It)->GetEnemyNum();
	}

	FDlgConfigWriter Writer("So");
	Writer.Write(FSoEnemyGroups::StaticStruct(), &EnemyGroups);

	return Writer.ExportToFile(GetEnemyGroupDataConfigPath(World));
}
