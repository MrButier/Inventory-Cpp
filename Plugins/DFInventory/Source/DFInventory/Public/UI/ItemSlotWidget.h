#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "UI/InventoryWidgetBase.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ItemSlotWidget.generated.h"

class UItemData;
class UInventoryComponent;
class UDragDropOperation;

/**
 * Widget representing a single slot in an inventory.
 * Automatically updates when the underlying inventory changes.
 * Can also be used as a list entry in a TileView.
 */
UCLASS()
class DFINVENTORY_API UItemSlotWidget : public UInventoryWidgetBase, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	
	/** The index of this slot in the inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Slot", meta = (ExposeOnSpawn = "true"))
	int32 SlotIndex = -1;

	/** The inventory component this slot belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Slot")
	TWeakObjectPtr<UInventoryComponent> InventoryComponent;

	/** The current item in this slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Slot")
	TWeakObjectPtr<UItemData> ItemData;

public:
	
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
	void InitSlot(UInventoryComponent* Component, int32 Index);

	/** Called when the slot needs to resolve its data and update visualization. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory Slot")
	void RefreshSlot();
	virtual void RefreshSlot_Implementation();

	/** 
	 * Implemented in Blueprint to update the visual widgets (Icon, Text).
	 * Called automatically after logic is resolved.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Slot", meta=(DisplayName="On Update Item"))
	void BP_OnUpdateItem(UItemData* Item);

protected:
	
	/** Pivot for the drag visual generally CenterCenter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Slot")
	EDragPivot DragPivot = EDragPivot::CenterCenter;
	
	/**
	 * Override this to provide a custom widget for the drag operation.
	 * If nullptr is returned, no visual will be shown (or default behavior).
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Inventory Slot")
	UUserWidget* GetDragVisual();
	virtual UUserWidget* GetDragVisual_Implementation();

	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** Event Handlers */
	UFUNCTION()
	void OnItemUpdated(int32 Index, UItemData* Item);

	UFUNCTION()
	void OnItemDataChanged();

	/** List Entry Interface specific implementation */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
};
