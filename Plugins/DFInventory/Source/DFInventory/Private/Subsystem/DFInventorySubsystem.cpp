#include "Subsystem/DFInventorySubsystem.h"

void UDFInventorySubsystem::StoreInventoryData(FName Key, const FItemSaveData& Data)
{ SavedInventories.Add(Key, Data); }

bool UDFInventorySubsystem::RetrieveInventoryData(FName Key, FItemSaveData& OutData)
{
	if (SavedInventories.Contains(Key))
	{
		OutData = SavedInventories[Key];
		return true;
	}
	return false;
}

void UDFInventorySubsystem::RemoveStoredInventoryData(FName Key)
{ SavedInventories.Remove(Key); }

void UDFInventorySubsystem::ClearAllStoredInventoryData()
{ SavedInventories.Empty(); }
