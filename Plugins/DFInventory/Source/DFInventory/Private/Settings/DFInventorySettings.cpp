#include "Settings/DFInventorySettings.h"

void UDFInventorySettings::EnsureExtraInfoValid()
{
	if (ExtraInfo.IsNull()) return;
	
	const UScriptStruct* SoftStruct = ExtraInfo.LoadSynchronous();
	if (!SoftStruct) return;
	
	if (!InstancedExtraInfo.IsValid() || InstancedExtraInfo.GetScriptStruct() != SoftStruct)
	{ InstancedExtraInfo.InitializeAs(SoftStruct); }
}
