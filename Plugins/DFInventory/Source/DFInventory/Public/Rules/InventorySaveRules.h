#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventorySaveRules.generated.h"

class UInventoryComponent;
enum class EEndPlayReason : uint8;

/**
 * Defines how and when an Inventory Component saves its data.
 * Assign a specific rule set (Disk, Memory, Hybrid) to different objects (Player, Chest, NPC).
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class DFINVENTORY_API UInventorySaveRules : public UObject
{
	GENERATED_BODY()

public:

	/** Unique ID for this inventory instance across map transitions. If empty, auto-generated based on owner name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules|Persistence")
	FString SaveID;	
	// Called when the Component Begins Play. returning True indicates successful load.
	virtual bool HandleBeginPlay(UInventoryComponent* Inventory);

	// Called when the Component Ends Play.
	virtual void HandleEndPlay(UInventoryComponent* Inventory, const ELinkTo* EndPlayReason_Obsolete = nullptr); 

	// Helper to reduce boilerplate in subclasses. Note: Component must implement SaveInventory/LoadInventory.
	virtual void HandleEndPlay_Explicit(UInventoryComponent* Inventory, EEndPlayReason::Type Reason);

	/**
	 * Generates a unique Save ID for the inventory.
	 * Default implementation tries to use PlayerController ID (if owner is pawn) or Owner Name.
	 * Override in Blueprint to provide custom naming logic (e.g. "Chest_1", "Player_Bob").
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Rules")
	FString GetSaveID(UInventoryComponent* Inventory);
	virtual FString GetSaveID_Implementation(UInventoryComponent* Inventory);

public:
	// Class to use for Disk Saving. Defaults to UInventorySaveGame.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules|Disk")
	TSubclassOf<class USaveGame> SaveGameClass;

protected:
	// Helpers for Subclasses
	bool SaveToDisk(UInventoryComponent* Inventory);
	bool LoadFromDisk(UInventoryComponent* Inventory);
	
	bool SaveToMemory(UInventoryComponent* Inventory);
	bool LoadFromMemory(UInventoryComponent* Inventory);

	// Internal Data Helpers (Moved from Component)
	struct FItemSaveData CreateSaveData(UInventoryComponent* Inventory);
	void ApplySaveData(UInventoryComponent* Inventory, const struct FItemSaveData& Data);
};

// =================================================================================================

/**
 * HYBRID (Default):
 * - Saves to MEMORY on Map Transition (Fast).
 * - Saves to DISK on Quit / Game End (Permanent).
 * - Loads from MEMORY first, then DISK.
 */
UCLASS(DisplayName = "Hybrid (Disk & Memory)")
class DFINVENTORY_API UInventorySaveRules_Hybrid : public UInventorySaveRules
{
	GENERATED_BODY()
public:
	virtual bool HandleBeginPlay(UInventoryComponent* Inventory) override;
	virtual void HandleEndPlay_Explicit(UInventoryComponent* Inventory, EEndPlayReason::Type Reason) override;
};

/**
 * DISK ONLY:
 * - Always saves to Disk (.sav file).
 * - Useful for persistent world chests or objects that don't need map-transition buffers.
 */
UCLASS(DisplayName = "Disk Only")
class DFINVENTORY_API UInventorySaveRules_Disk : public UInventorySaveRules
{
	GENERATED_BODY()
public:
	virtual bool HandleBeginPlay(UInventoryComponent* Inventory) override;
	virtual void HandleEndPlay_Explicit(UInventoryComponent* Inventory, EEndPlayReason::Type Reason) override;
};

/**
 * MEMORY ONLY:
 * - Saves to GameInstance Subsystem only.
 * - Data is LOST when Game stops.
 * - Useful for short-term persistence across map changes.
 */
UCLASS(DisplayName = "Memory Only")
class DFINVENTORY_API UInventorySaveRules_Memory : public UInventorySaveRules
{
	GENERATED_BODY()
public:
	virtual bool HandleBeginPlay(UInventoryComponent* Inventory) override;
	virtual void HandleEndPlay_Explicit(UInventoryComponent* Inventory, EEndPlayReason::Type Reason) override;
};
