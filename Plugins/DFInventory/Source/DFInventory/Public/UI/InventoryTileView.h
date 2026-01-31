#pragma once

#include "CoreMinimal.h"
#include "Components/TileView.h"
#include "InventoryTileView.generated.h"

class UItemData;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileViewItemAdded, UItemData*, Item, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileViewItemRemoved, UItemData*, Item, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileViewItemUpdated, int32, Index, UItemData*, Item);

// virtualized grid tailored for Inventory Components. Automatically populates from inventory and handles updates.
UCLASS(BlueprintType, Blueprintable)
class DFINVENTORY_API UInventoryTileView : public UTileView
{
	GENERATED_BODY()

public:
	// The inventory component currently being displayed.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory List")
	TWeakObjectPtr<UInventoryComponent> InventoryComponent;

	// Fired when an item is added to the list.
	UPROPERTY(BlueprintAssignable, Category = "Inventory List")
	FOnTileViewItemAdded OnItemAddedDelegate;

	// Fired when an item is removed from the list.
	UPROPERTY(BlueprintAssignable, Category = "Inventory List")
	FOnTileViewItemRemoved OnItemRemovedDelegate;

	// Fired when an item is updated in the list. 
	UPROPERTY(BlueprintAssignable, Category = "Inventory List")
	FOnTileViewItemUpdated OnItemUpdatedDelegate;

	// Sets the inventory component to display and binds to its update events.
	UFUNCTION(BlueprintCallable, Category = "Inventory List")
	void SetInventoryComponent(UInventoryComponent* NewComponent);

	// Determines if an item should be displayed in the list.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory List")
	bool ItemFilter(UItemData* Item);
	virtual bool ItemFilter_Implementation(UItemData* Item);

	/** Rebuilds the validation list from the component. Call this if your Filter criteria changes. */
	UFUNCTION(BlueprintCallable, Category = "Inventory List")
	void RefreshInventoryList();

protected:
	/** Event Handlers for Inventory Changes */
	UFUNCTION(BlueprintCallable, Category = "Inventory List")
	void OnItemAdded(UItemData* Item, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Inventory List")
	void OnItemRemoved(UItemData* Item, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Inventory List")
	void OnItemUpdated(int32 Index, UItemData* Item);
	
	// Cache of what item is currently in what slot index, used for efficient incremental updates.
	UPROPERTY(Transient)
	TMap<int32, TWeakObjectPtr<UItemData>> CurrentItemMap;

    // Blueprint Hooks
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory List", meta=(DisplayName="On Item Added"))
	void BP_OnItemAdded(UItemData* Item, int32 Amount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory List", meta=(DisplayName="On Item Removed"))
	void BP_OnItemRemoved(UItemData* Item, int32 Amount);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory List", meta=(DisplayName="On Item Updated"))
	void BP_OnItemUpdated(int32 Index, UItemData* Item);
};
