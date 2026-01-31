#include "UI/ItemSlotWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Component/InventoryComponent.h"
#include "Data/ItemData.h"
#include "UI/ItemDragDropOpt.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UItemSlotWidget::InitSlot(UInventoryComponent* Component, int32 Index)
{
	if (InventoryComponent.Get() != Component)
	{
		if (InventoryComponent.IsValid() && InventoryComponent->OnItemUpdated.IsAlreadyBound(this, &UItemSlotWidget::OnItemUpdated))
		{ InventoryComponent->OnItemUpdated.RemoveDynamic(this, &UItemSlotWidget::OnItemUpdated); }

		InventoryComponent = Component;
		SlotIndex = Index;

		if (InventoryComponent.IsValid())
		{
			InventoryComponent->OnItemUpdated.AddDynamic(this, &UItemSlotWidget::OnItemUpdated);
		}
		if (InventoryComponent.IsValid() && !InventoryComponent->OnItemUpdated.IsAlreadyBound(this, &UItemSlotWidget::OnItemUpdated))
		{ InventoryComponent->OnItemUpdated.AddDynamic(this, &UItemSlotWidget::OnItemUpdated); }
	}
	RefreshSlot();
}

void UItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (!InventoryComponent.IsValid()) return;
	
	if (!InventoryComponent->OnItemUpdated.IsAlreadyBound(this, &UItemSlotWidget::OnItemUpdated))
	{ InventoryComponent->OnItemUpdated.AddDynamic(this, &UItemSlotWidget::OnItemUpdated); }
	
	if (ItemData.IsValid() && !ItemData->OnDataChanged.IsAlreadyBound(this, &UItemSlotWidget::OnItemDataChanged))
	{ ItemData->OnDataChanged.AddUniqueDynamic(this, &UItemSlotWidget::OnItemDataChanged); }
	
	RefreshSlot();
}

void UItemSlotWidget::NativeDestruct()
{
	if (InventoryComponent.IsValid() && InventoryComponent->OnItemUpdated.IsAlreadyBound(this, &UItemSlotWidget::OnItemUpdated))
	{ InventoryComponent->OnItemUpdated.RemoveDynamic(this, &UItemSlotWidget::OnItemUpdated); }
	
	if (ItemData.IsValid() && ItemData->OnDataChanged.IsAlreadyBound(this, &UItemSlotWidget::OnItemDataChanged))
	{ ItemData->OnDataChanged.RemoveDynamic(this, &UItemSlotWidget::OnItemDataChanged); }
	
	Super::NativeDestruct();
}

void UItemSlotWidget::OnItemUpdated(int32 Index, UItemData* Item)
{
	if (Index == SlotIndex)
	{ RefreshSlot(); }
}

void UItemSlotWidget::OnItemDataChanged()
{ RefreshSlot(); }

void UItemSlotWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	UItemData* NewItem = Cast<UItemData>(ListItemObject);
	if (NewItem)
	{
		ItemData = NewItem;
		RefreshSlot();
	}
}

void UItemSlotWidget::RefreshSlot_Implementation()
{
	// If we are bound to a component, the slot index is the authority
	if (InventoryComponent.IsValid())
	{
		if (InventoryComponent->GetInventoryItems().IsValidIndex(SlotIndex))
		{ 
			ItemData = InventoryComponent->GetInventoryItems()[SlotIndex];
		}
	}

	if (ItemData.IsValid())
	{
		ItemData->OnDataChanged.AddUniqueDynamic(this, &UItemSlotWidget::OnItemDataChanged);
	}
	
	// Trigger the Blueprint Event to update UI
	BP_OnUpdateItem(ItemData.Get());
}

UUserWidget* UItemSlotWidget::GetDragVisual_Implementation()
{ return nullptr; }

void UItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	if (!ItemData.IsValid()) return;
	
	UItemDragDropOpt* DragOp = NewObject<UItemDragDropOpt>();
	DragOp->SourceInventory = InventoryComponent.Get();
	DragOp->SourceSlotIndex = SlotIndex;
	DragOp->DraggedItem = ItemData.Get();
	
	// Set Default Drag Visual
	// In a real implementation, you might want to create a specific visual widget here
	DragOp->DefaultDragVisual = this; 
	DragOp->Pivot = EDragPivot::CenterCenter;

	OutOperation = DragOp;
}

bool UItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UItemDragDropOpt* InvDragOp = Cast<UItemDragDropOpt>(InOperation);
	if (InvDragOp && InventoryComponent.IsValid())
	{
		if (InvDragOp->SourceInventory == InventoryComponent.Get())
		{
			if (InvDragOp->SourceSlotIndex == SlotIndex) return true;
			
			if (InvDragOp->DraggedItem)
			{
				InventoryComponent->SwapItemSlots(InvDragOp->SourceSlotIndex, SlotIndex);
			}
		}
		else
		{
			// Cross Inventory Drop
			if (InvDragOp->DraggedItem)
			{
			    // Attempt to add item to this specific slot or inventory
			    // InventoryBase::HandleStackItem or AddItem implementation
			}
		}
		return true;
	}
	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}
