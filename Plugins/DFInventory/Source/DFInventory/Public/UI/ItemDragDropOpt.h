#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ItemDragDropOpt.generated.h"

class UInventoryComponent;
class UItemData;

/**
 * Drag and drop operation for inventory items.
 * Holds reference to the source inventory and slot, as well as the item being dragged.
 */
UCLASS()
class DFINVENTORY_API UItemDragDropOpt : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** The inventory component where the drag started. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Drag Drop", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UInventoryComponent> SourceInventory;
	
	/** The slot index where the drag started. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Drag Drop", meta = (ExposeOnSpawn = "true"))
	int32 SourceSlotIndex = -1;

	/** The item data being dragged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Drag Drop", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UItemData> DraggedItem;
};
