#include "Component/InventoryComponent.h"
#include "Subsystem/DFInventorySubsystem.h"
#include "Rules/InventorySaveRules.h"
#include "Data/ItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/InventorySaveGame.h"
#include "Settings/DFInventorySettings.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 1. Persistence Strategy: Delegate to SaveRules
	if (SaveRules)
	{
		if (SaveRules->HandleBeginPlay(this))
		{
			// Loaded successfully
			return;
		}
	}

	// 3. Initialize Inventory
	// If we have default items (from Editor), we preserve them.
	if (InventoryItems.Num() > 0)
	{
		if (InventoryItems.Num() != MaxItemSlots)
		{
			InventoryItems.SetNum(MaxItemSlots);
		}
		OnInventoryRefresh.Broadcast();
	}
	else
	{
		// Only start fresh if no default items exist
		CreateNewInventory();
	}
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, InventoryItems);
	DOREPLIFETIME(UInventoryComponent, SaveRules);
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (SaveRules)
	{
		SaveRules->HandleEndPlay_Explicit(this, EndPlayReason);
	}
}

void UInventoryComponent::CreateNewInventory()
{
	InventoryItems.Empty();
	InventoryItems.SetNum(MaxItemSlots);
	OnInventoryRefresh.Broadcast();
}

void UInventoryComponent::SetMaxItemSlots(int32 NewMaxSlots)
{
	if (NewMaxSlots < 1) return;
	MaxItemSlots = NewMaxSlots;
	
	if (InventoryItems.Num() != MaxItemSlots)
	{
		InventoryItems.SetNum(MaxItemSlots);
		OnInventoryRefresh.Broadcast();
	}
}

bool UInventoryComponent::FindStackableItem(UItemData* NewItem, UItemData*& FoundItem)
{
	FoundItem = nullptr;
	if (!NewItem) return false;
	
	if (TObjectPtr<UItemData>* FoundPtr = InventoryItems.FindByPredicate([&](TObjectPtr<UItemData>& Existing)
		{ return CanItemsStack(NewItem, Existing); }))
	{
		FoundItem = FoundPtr->Get();
		return true;
	}
	return false;
}

bool UInventoryComponent::CanItemsStack(UItemData* NewItem, UItemData* ExistingItem)
{
	if (!NewItem || !ExistingItem || !ExistingItem->IsValidLowLevel()) return false;
	
	return ExistingItem->GetParentItem() == NewItem->GetParentItem()
		&& ExistingItem->ItemSlotsAvailable()
		&& ItemStackCondition(NewItem, ExistingItem);
}

int32 UInventoryComponent::FindEmptySlot()
{ return InventoryItems.IndexOfByPredicate([](const TObjectPtr<UItemData>& Item){ return Item == nullptr; }); }

int32 UInventoryComponent::FindItemIndex(UItemData* Item)
{
	if (!Item) return -1;
	return InventoryItems.IndexOfByPredicate([Item](const TObjectPtr<UItemData>& SlotItem)
	{ return SlotItem.Get() == Item; });
}

bool UInventoryComponent::IsInventoryFull()
{
	bool bIsAtMaxCount = InventoryItems.Num() >= MaxItemSlots;
	bool bHasNullSlot = FindEmptySlot() != -1;
	return bIsAtMaxCount && !bHasNullSlot;
}

bool UInventoryComponent::AddItemToInventory(UItemData* NewItem)
{
	UItemData* FoundItem = nullptr;
	if (!NewItem) return false;
	
	while (NewItem->GetItemAmount() > 0)
	{
		int ItemIndex = -1;
		if (FindStackableItem(NewItem, FoundItem))
		{
			StackItem(FoundItem, NewItem, ItemIndex);
			OnItemUpdated.Broadcast(ItemIndex, FoundItem);
		}
		else if (IsInventoryFull())
		{
			HandleInventoryFull(NewItem);
			OnInventoryFull.Broadcast();
			return true;
		}
		else
		{
			AddItem(NewItem, ItemIndex);
			OnItemUpdated.Broadcast(ItemIndex, NewItem);
			return false;
		}
	}
	return false;
}

bool UInventoryComponent::StackItem(UItemData* FoundItem, UItemData* NewItem, int32& ItemIndex)
{
	int32 IncomingAmount = NewItem->GetItemAmount();
	int32 Remainder = FoundItem->AddItemAmount(IncomingAmount);
	NewItem->SetItemAmount(Remainder);
	
	int32 AddedAmount = IncomingAmount - Remainder;
	if (AddedAmount > 0)
	{ 
		ItemIndex = FindItemIndex(FoundItem);
		return true;
	}
	return false;
}

void UInventoryComponent::RemoveItemsByIndex(const TArray<int32>& ItemIndexes)
{
	for (int32 Index : ItemIndexes)
	{ RemoveItemFromInventory(Index); }
}

void UInventoryComponent::RemoveItemsByObject(const TArray<UItemData*>& ItemObjects)
{
	for (UItemData* ItemData : ItemObjects)
	{
		int32 Index = FindItemIndex(ItemData);
		RemoveItemFromInventory(Index);
	}
}

void UInventoryComponent::RemoveItemFromInventory(int32 ItemIndex)
{
	if (InventoryItems.IsValidIndex(ItemIndex) && InventoryItems[ItemIndex])
	{
		InventoryItems[ItemIndex] = nullptr;
		OnItemUpdated.Broadcast(ItemIndex, nullptr);
	}
}

void UInventoryComponent::AddItem(UItemData* NewItem, int32& ItemIndex)
{
	ItemIndex = FindEmptySlot();
	if (ItemIndex == INDEX_NONE) return;
	InventoryItems[ItemIndex] = NewItem;
}

bool UInventoryComponent::AddItemAtIndex(UItemData* NewItem, int32 Index)
{
	if (!NewItem || !InventoryItems.IsValidIndex(Index)) return false;
	
	UItemData* FoundItem = InventoryItems[Index];
	int32 UnusedIndex = -1;
	
	if (FoundItem)
	{
		if (CanItemsStack(NewItem, FoundItem))
		{ return StackItem(FoundItem, NewItem, UnusedIndex); }
		
		return false;
	}
	
	InventoryItems[Index] = NewItem;
	OnItemUpdated.Broadcast(Index, NewItem);
	return true;
}

bool UInventoryComponent::SwapItemSlots(int32 SourceIndex, int32 TargetIndex)
{
	if (!InventoryItems.IsValidIndex(SourceIndex)
		|| !InventoryItems.IsValidIndex(TargetIndex)
		|| SourceIndex == TargetIndex)
	{ return false; }
	
	UItemData* SourceItem = InventoryItems[SourceIndex];
	UItemData* TargetItem = InventoryItems[TargetIndex];
	if (!SourceItem) return false;
	
	if (TargetItem && SourceItem->GetParentItem() == TargetItem->GetParentItem())
	{
		int32 TempIndex = -1; 
		StackItem(TargetItem, SourceItem, TempIndex);
		if (SourceItem->GetItemAmount() <= 0) InventoryItems[SourceIndex] = nullptr;
	}
	else
	{
		InventoryItems[TargetIndex] = SourceItem;
		InventoryItems[SourceIndex] = TargetItem;
	}
	
	OnItemUpdated.Broadcast(SourceIndex, InventoryItems[SourceIndex]);
	OnItemUpdated.Broadcast(TargetIndex, InventoryItems[TargetIndex]);
	return true;
}

bool UInventoryComponent::SplitStack(int32 SourceIndex, int32 TargetIndex, int32 Amount)
{
	if (!InventoryItems.IsValidIndex(SourceIndex) || !InventoryItems.IsValidIndex(TargetIndex)
		|| !InventoryItems[SourceIndex] 
		|| SourceIndex == TargetIndex || Amount <= 0)
	{ return false; }
	
	UItemData* SourceItem = InventoryItems[SourceIndex];
	UItemData* TargetItem = InventoryItems[TargetIndex];
	if (SourceItem->GetItemAmount() < Amount)
	{ return false; }
	
	if (TargetItem && TargetItem->GetParentItem() != SourceItem->GetParentItem())
	{ return false; }
	
	int32 ActuallyMoved;
	if (!TargetItem)
	{
		FItemStruct Info = SourceItem->GetItemInfo();
		TargetItem = NewObject<UItemData>(this, SourceItem->GetClass());
		TargetItem->SetInfo(Info);
		TargetItem->SetItemAmount(Amount);
		
		InventoryItems[TargetIndex] = TargetItem;
		ActuallyMoved = Amount;
	}
	else
	{
		int32 Remainder = TargetItem->AddItemAmount(Amount);
		ActuallyMoved = Amount - Remainder;
	}

	if (ActuallyMoved <= 0) return false;

	SourceItem->AddItemAmount(-ActuallyMoved);
	
	if (SourceItem->GetItemAmount() <= 0) InventoryItems[SourceIndex] = nullptr;
	OnItemUpdated.Broadcast(SourceIndex, InventoryItems[SourceIndex]);
	OnItemUpdated.Broadcast(TargetIndex, TargetItem);

	return true;
}

bool UInventoryComponent::GenericTransferItem(UInventoryComponent* TargetComponent, int32 SourceIndex, int32 TargetIndex)
{
	if (!TargetComponent || !InventoryItems.IsValidIndex(SourceIndex) || !InventoryItems[SourceIndex])
	{ return false; }
	
	UItemData* SourceItem = InventoryItems[SourceIndex];
	FItemStruct SourceInfo = SourceItem->GetItemInfo();
	
	UItemData* CopyItem = NewObject<UItemData>(TargetComponent, SourceItem->GetClass());
	CopyItem->SetInfo(SourceInfo);
	
	if (TargetIndex >= 0)
	{ if (!TargetComponent->AddItemAtIndex(CopyItem, TargetIndex)) return false; }
	else
	{ TargetComponent->AddItemToInventory(CopyItem); }
	
	bool bInTarget = TargetComponent->FindItemIndex(CopyItem) != -1;
	bool bFullyConsumed = CopyItem->GetItemAmount() <= 0;
	
	if (bInTarget || bFullyConsumed)
	{
		RemoveItemFromInventory(SourceIndex);
		return true;
	}
	
	SourceItem->SetItemAmount(CopyItem->GetItemAmount());
	OnItemUpdated.Broadcast(SourceIndex, SourceItem);
	return false;
}

TArray<int32> UInventoryComponent::TransferItemsByIndex(UInventoryComponent* TargetComponent, const TArray<int32>& ItemIndexes)
{
	TArray<int32> FailedIndexes;
	if (!TargetComponent) return ItemIndexes;
	
	for (int32 Index : ItemIndexes)
	{
		if (!GenericTransferItem(TargetComponent, Index)) FailedIndexes.Add(Index);
	}
	return FailedIndexes;
}

TArray<UItemData*> UInventoryComponent::TransferItemsByObject(UInventoryComponent* TargetComponent, const TArray<UItemData*>& ItemObjects)
{
	TArray<UItemData*> FailedItems;
	if (!TargetComponent) return ItemObjects;
	
	for (UItemData* Item : ItemObjects)
	{
		int32 Index = FindItemIndex(Item);
		if (Index == -1) continue;
		
		if (!GenericTransferItem(TargetComponent, Index)) FailedItems.Add(Item);
	}
	return FailedItems;
}

FString UInventoryComponent::GetSaveID() const
{
	return SaveID;
}
