#include "UI/InventoryTileView.h"
#include "Component/InventoryComponent.h"
#include "Data/ItemData.h"

void UInventoryTileView::SetInventoryComponent(UInventoryComponent* NewComponent)
{
	if (InventoryComponent.Get() == NewComponent) return;

	UInventoryComponent* OldComp = InventoryComponent.Get();
	if (OldComp && OldComp->OnItemUpdated.IsAlreadyBound(this, &UInventoryTileView::OnItemUpdated))
	{
		OldComp->OnItemUpdated.RemoveDynamic(this, &UInventoryTileView::OnItemUpdated);
		OldComp->OnInventoryRefresh.RemoveDynamic(this, &UInventoryTileView::RefreshInventoryList);
	}
	
	InventoryComponent = NewComponent;
	ClearListItems();
	
	UInventoryComponent* NewComp = InventoryComponent.Get();
	if (NewComp)
	{
		if (!NewComp->OnItemUpdated.IsAlreadyBound(this, &UInventoryTileView::OnItemUpdated))
		{ NewComp->OnItemUpdated.AddDynamic(this, &UInventoryTileView::OnItemUpdated); }
		
		if (!NewComp->OnInventoryRefresh.IsAlreadyBound(this, &UInventoryTileView::RefreshInventoryList))
		{ NewComp->OnInventoryRefresh.AddDynamic(this, &UInventoryTileView::RefreshInventoryList); }

		RefreshInventoryList();
	}
}

void UInventoryTileView::OnItemAdded(UItemData* Item, int32 Amount)
{
	BP_OnItemAdded(Item, Amount);
}

void UInventoryTileView::OnItemRemoved(UItemData* Item, int32 Amount)
{ BP_OnItemRemoved(Item, Amount); }

void UInventoryTileView::OnItemUpdated(int32 Index, UItemData* Item)
{
	if (Index < 0) return; 

	TWeakObjectPtr<UItemData>* OldItemPtr = CurrentItemMap.Find(Index);
	UItemData* OldItem = OldItemPtr ? OldItemPtr->Get() : nullptr;
	
	if (OldItem)
	{
		if (OldItem != Item || !ItemFilter(OldItem))
		{
			RemoveItem(OldItem);
		}
	}

	if (Item && ItemFilter(Item))
	{
		if (OldItem != Item)
		{
			AddItem(Item);
		}
		else 
		{
			// If already contained, we don't need to add.
			// However, check just in case it was missed or removed by external means.
			if (!GetListItems().Contains(Item))
			{
				AddItem(Item);
			}
		}
	}

	if (Item) { CurrentItemMap.Add(Index, Item); }
	else { CurrentItemMap.Remove(Index); }

	BP_OnItemUpdated(Index, Item);
}

void UInventoryTileView::RefreshInventoryList()
{
	ClearListItems();
	CurrentItemMap.Empty();
	
	if (!InventoryComponent.IsValid()) return;
	
	const TArray<TObjectPtr<UItemData>>& Items = InventoryComponent->InventoryItems;
	
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		UItemData* Item = Items[i];
		if (Item)
		{
			if (ItemFilter(Item)) AddItem(Item);
			CurrentItemMap.Add(i, Item);
		}
	}
}

bool UInventoryTileView::ItemFilter_Implementation(UItemData* Item)
{
	return Item != nullptr;
}

