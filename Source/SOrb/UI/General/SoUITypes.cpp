// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUITypes.h"
#include "Basic/Helpers/SoDateTimeHelper.h"

DEFINE_LOG_CATEGORY(LogSoUI);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIEventHandler::USoUIEventHandler(const FObjectInitializer& PCIP)
	: Super(PCIP)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float FSoUITrackedInputKeyDown::GetKeyDownThresholdSeconds() const
{
	return USoDateTimeHelper::NormalizeTime(KeyDownThresholdSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float FSoUITrackedInputKeyDown::GetKeyDownThresholdDelaySeconds() const
{
	return USoDateTimeHelper::NormalizeTime(KeyDownThresholdDelaySeconds);
}
