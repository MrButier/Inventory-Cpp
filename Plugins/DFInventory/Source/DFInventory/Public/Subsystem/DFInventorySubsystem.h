#pragma once

#include "CoreMinimal.h"
#include "Settings/InventorySaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DFInventorySubsystem.generated.h"

// Subsystem to handle inventory persistence across map transitions.
UCLASS()
class DFINVENTORY_API UDFInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// Stores inventory data in the GameInstance memory (Does NOT write to disk). Useful for map transitions.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void StoreInventoryData(FName Key, const FItemSaveData& Data);

	// Retrieves inventory data from the GameInstance memory. Returns true if found.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RetrieveInventoryData(FName Key, FItemSaveData& OutData);

	// Removes a specific inventory from the memory cache.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveStoredInventoryData(FName Key);

	// Clears all inventory data from memory.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearAllStoredInventoryData();

private:

	UPROPERTY(Transient)
	TMap<FName, FItemSaveData> SavedInventories;
};
